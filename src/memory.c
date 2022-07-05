#include "memory.h"

MemArena create_arena(u32 init_size, u32 grow_size)
{
    assert(init_size != 0 && init_size != PAGE_SIZE && init_size % PAGE_SIZE == 0, "Invalid init_size");
    assert(grow_size != 0 && grow_size % PAGE_SIZE == 0, "Invalid grow_size");
    
    void *new_chunk = allocate_new_chunk(0, init_size);

    MemArena arena = {
        .arena = (byte*) new_chunk,
        .grow_size = grow_size,
        .size = init_size,
        .used = __REGION_ARR_INIT_SIZE,
        .regions_arr = (MemRegion*) new_chunk,
        .regions_arr_region = (MemRegion*) new_chunk,
        .regions_count = 1,
        .regions_capacity = PAGE_SIZE / sizeof(MemRegion),
    };

    if (!new_chunk)
    {
        log(ERROR, "mmap failed");
        abort();
    }

    MemRegion *first_region = arena.regions_arr;
    first_region->start = (byte*) first_region;
    first_region->size = __REGION_ARR_INIT_SIZE;
    first_region->is_alive = true;

    MemRegion *second_region = arena.regions_arr + 1;
    second_region->start = (byte*) second_region + __REGION_ARR_INIT_SIZE;
    second_region->size = init_size - __REGION_ARR_INIT_SIZE;
    second_region->is_alive = false;

    return arena;
}

static MemRegion add_region(MemArena *arena, byte *start, u32 size, bool is_alive)
{
    MemRegion *ptr_new_region = arena->regions_arr + arena->regions_count;
        
    if (arena->regions_count == arena->regions_capacity)
    {
        u32 prev_regions_capacity = arena->regions_capacity;
                            
        arena->regions_capacity *= 2;
        u32 new_regions_arr_capacity = arena->regions_capacity;
        u32 new_regions_arr_capacity_bytes = arena->regions_capacity * sizeof(MemRegion);

        u32 grow_size = arena->grow_size;
        while (grow_size <= new_regions_arr_capacity_bytes)
            grow_size += arena->grow_size;

        void *new_chunk = allocate_new_chunk(arena->arena + arena->size, grow_size);

        if (!new_chunk)
        {
            log(ERROR, "mmap failed");
            abort();
        }
        
        arena->size += grow_size;
        arena->used += prev_regions_capacity; // The regions vec capacity was doubled

        size_t regions_arr_region_offset = arena->regions_arr_region - arena->regions_arr;

        memcpy(new_chunk, arena->regions_arr, prev_regions_capacity * sizeof(MemRegion));
        arena->regions_arr = new_chunk;
        arena->regions_arr_region = new_chunk + regions_arr_region_offset;

        // Region for new regions array
        arena->regions_arr_region->is_alive = false;
        ptr_new_region = (MemRegion*) new_chunk + prev_regions_capacity;
        ptr_new_region->start = (byte*) new_chunk;
        ptr_new_region->size = new_regions_arr_capacity_bytes;
        ptr_new_region->is_alive = true;
        arena->regions_arr_region = ptr_new_region;

        // Region for new open memory
        ++ptr_new_region;
        ptr_new_region->start = (byte*) new_chunk + new_regions_arr_capacity_bytes;
        ptr_new_region->size = grow_size - new_regions_arr_capacity_bytes;
        ptr_new_region->is_alive = false;

        // Region being added
        ++ptr_new_region;
        
        arena->regions_count += 2;
    }

    ptr_new_region->start = start;
    ptr_new_region->size = size;
    ptr_new_region->is_alive = false;

    ++arena->regions_count;

    return *ptr_new_region;
}

void *_alloc_data(MemArena *restrict arena, u32 size)
{
    arena->used += size;

    for (MemRegion *curr_region = arena->regions_arr;
         curr_region != arena->regions_arr + arena->regions_count;
         ++curr_region)
    {
        if (curr_region->is_alive || curr_region->size < size)
            continue;

        byte *destination = curr_region->start;

        if (curr_region->size == size)
            curr_region->is_alive = true;
        else
        {            
            // Add new live region, taking space from curr_region
            add_region(arena, curr_region->start, size, true);
            
            // Resize dead region that live region was taken from
            curr_region->start += size;
            curr_region->size -= size;
        }

        return destination;
    }

    u32 grow_size = arena->grow_size;
    while (grow_size <= size)
        grow_size += arena->grow_size;

    void *new_chunk = allocate_new_chunk(arena->arena + arena->size, grow_size);
    
    if (!new_chunk)
    {
        log(ERROR, "mmap failed");
        abort();
    }
    
    arena->size += grow_size;
    
    // TODO: Create region for empty space in addition to alloced space
    add_region(arena, (byte*) new_chunk + size, grow_size - size, false);
    add_region(arena, (byte*) new_chunk, size, true);

    return (byte*) new_chunk;
}

void arena_free(MemArena *restrict arena, void *restrict ptr)
{
    // TODO: Consider making regions_arr into a hash table to do this in constant time
    MemRegion *curr = arena->regions_arr;
    for (; curr->start != ptr; ++curr);

    MemRegion *region_before = 0;
    MemRegion *region_after = 0;

    if (curr != arena->regions_arr
        && !(curr - 1)->is_alive
        && (curr - 1)->start + (curr - 1)->size == ptr)
    {
        region_before = curr - 1;
    }

    if (curr != arena->regions_arr + arena->regions_count - 1
        && !(curr + 1)->is_alive
        && (curr + 1)->start == ptr + curr->size)
    {
        region_after = curr + 1;
    }

    if (!region_before && !region_after)
        curr->is_alive = false;
    else if (region_before && region_after)
    {
        region_before->size += (curr->size + region_after->size);

        if (curr + 2 < arena->regions_arr + arena->regions_count)
        {
            memmove(curr,
                    curr + 2,
                    (byte*) (arena->regions_arr + arena->regions_count) - (byte*) (curr + 2));
        }
        
        arena->regions_count -= 2;
    }
    else if (region_before)
    {
        region_before->size += curr->size;
                
        memmove(curr,
                region_after,
                (byte*) (arena->regions_arr + arena->regions_count) - (byte*) region_after);
        --arena->regions_count;
    }
    else // region_after
    {
        curr->is_alive = false;
        curr->size += region_after->size;
        
        memmove(region_after,
                region_after + 1,
                (byte*) (arena->regions_arr + arena->regions_count) - (byte*) (region_after + 1));
        --arena->regions_count;
    }
}

#ifdef TEST_MODE

static TEST_RESULT test_alloc()
{
    MemArena mem = create_arena(PAGE_SIZE * 4, PAGE_SIZE * 2);

    u32 usage_before_alloc = mem.used;
    u32 starting_capacity = mem.size;

    byte *array1 = (byte*) alloc_array(&mem, PAGE_SIZE - 19, byte);
    assert(mem.used == usage_before_alloc + PAGE_SIZE - 19, "Incorrect arena usage after allocation");
    assert(mem.size == starting_capacity, "Arena size changed when it should not have");
    assert(mem.regions_count == 1, "Unexpected number of regions");

    byte *array2 = (byte*) alloc_array(&mem, 9, byte);
    assert(array2 == array1 + PAGE_SIZE - 19, "Alloc'ed memory not aligned as expected");
    assert(mem.used == usage_before_alloc + PAGE_SIZE - 10, "Incorrect arena usage after allocation");
    assert(mem.size == starting_capacity, "Arena size changed when it should not have");
    assert(mem.regions_count == 1, "Unexpected number of regions");
    
    byte *array3 = (byte*) alloc_array(&mem, PAGE_SIZE + 10, byte);
    assert(array3 == array1 + PAGE_SIZE - 10, "Alloc'ed memory not aligned as expected");
    assert(mem.used == usage_before_alloc + 2 * PAGE_SIZE, "Incorrect arena usage after allocation");
    assert(mem.size == starting_capacity, "Arena size changed when it should not have");
    assert(mem.regions_count == 1, "Unexpected number of regions");

    // Make sure no seg fault occurs
    array3[PAGE_SIZE + 9] = 42;

    alloc_array(&mem, PAGE_SIZE, byte);
    assert(array3[PAGE_SIZE + 9] == 42, "Memory was incorrectly overwritten");

    return TEST_PASS;
}

// static TEST_RESULT test_alloc_memory_regions()
// {
//     MemArena mem = create_arena(PAGE_SIZE * 4, PAGE_SIZE * 2);

//     for (
    
//     return TEST_PASS;    
// }

static TEST_RESULT test_free()
{
    MemArena mem = create_arena(PAGE_SIZE * 4, PAGE_SIZE * 2);

    void *arr1 = alloc_array(&mem, 10, byte);
    void *arr2 = alloc_array(&mem, 10, byte);
    void *arr3 = alloc_array(&mem, 10, byte);

    assert(mem.used == PAGE_SIZE + 30, "");
    assert(mem.regions_count == 1, "");
    assert(mem.regions_arr[0].start == (byte*) mem.regions_arr, "");
    assert(mem.regions_arr[0].size == PAGE_SIZE, "");
    assert(mem.regions_arr[1].start == arr3 + 10, "");
    assert(mem.regions_arr[1].size == mem.size - mem.used, "");
    
    arena_free(&mem, arr2);

    return TEST_PASS;
}

static TEST_RESULT test_free_memory_regions()
{
    return TEST_PASS;
}

ModuleTestSet memory_h_register_tests()
{
    ModuleTestSet set = {
        .module_name = __FILE__,
        .tests = {0},
        .count = 0,
    };

    register_test(&set, "Memory allocation", test_alloc);
    register_test(&set, "Freeing memory", test_free);
    
    return set;
}

#endif
