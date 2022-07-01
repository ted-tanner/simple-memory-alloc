#include "memory.h"

static inline void *allocate_new_chunk(void *start, u32 size)
{
    return mmap(start, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

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
        .regions_count = 1,
        .regions_capacity = PAGE_SIZE / sizeof(MemRegion),
    };

    if (!new_chunk)
    {
        log(ERROR, "mmap failed");
        abort();
    }

    MemRegion *first_region = arena.regions_arr;
    first_region->region = (byte*) first_region + __REGION_ARR_INIT_SIZE;
    first_region->size = init_size - __REGION_ARR_INIT_SIZE;

    return arena;
}

void *_alloc_data(MemArena *restrict arena, u32 size)
{
    arena->used += size;
    assert(arena->used <= arena->size, "MemArena overflow");

    byte *destination;
    MemRegion *curr_region = arena->regions_arr;

    for (; curr_region != arena->regions_arr + arena->regions_count; ++curr_region)
    {
        if (curr_region->size < size)
            continue;

        destination = curr_region->region;

        if (curr_region->size == size)
        {
            // Shift other regions down one index in the array, overwriting current index
            for (MemRegion *prev_region = curr_region++;
                 curr_region != arena->regions_arr + arena->regions_count;
                 prev_region = curr_region++)
            {
                *prev_region = *curr_region;
            }

            --arena->regions_count;
        }
        else
        {
            curr_region->region += size;
            curr_region->size -= size;
        }

        return destination;
    }

    u32 grow_size = arena->grow_size;
    while (grow_size <= size)
        grow_size += arena->grow_size;

    if (arena->regions_count == arena->regions_capacity)
    {
        u32 prev_regions_size = arena->regions_capacity * sizeof(MemRegion);
        
        arena->regions_capacity *= 2;
        u32 new_regions_vec_size = arena->regions_capacity * sizeof(MemRegion);
        
        void *new_chunk = allocate_new_chunk(arena->arena + arena->size, grow_size + new_regions_vec_size);

        if (!new_chunk)
        {
            log(ERROR, "mmap failed");
            abort();
        }
        
        arena->size += grow_size + new_regions_vec_size;
        arena->used += new_regions_vec_size - arena->regions_capacity * sizeof(MemRegion);

        memcpy(new_chunk, arena->regions_arr, prev_regions_size);
        arena->regions_arr = new_chunk;
        
        MemRegion *ptr_next_region = new_chunk + prev_regions_size;
        ptr_next_region->region = (byte*) new_chunk;
        ptr_next_region->size = new_regions_vec_size;
        ++arena->regions_count;

        destination = (byte*) new_chunk + new_regions_vec_size;
    }
    else
    {
        void *new_chunk = allocate_new_chunk(arena->arena + arena->size, grow_size);

        if (!new_chunk)
        {
            log(ERROR, "mmap failed");
            abort();
        }
        
        arena->size += grow_size;
        destination = (byte*) new_chunk;
    }

    MemRegion *next_region = arena->regions_arr + arena->regions_count;
    next_region->region = destination + size;
    next_region->size = grow_size - size;
    ++arena->regions_count;
    
    return destination;
}

void arena_free(MemArena *restrict arena, void *restrict ptr, u32 size)
{
    // TODO: Consider making regions_arr into a hash table to do this in constant time
    for (MemRegion *curr_i = arena->regions_arr; curr_i != arena->regions_arr + arena->regions_count; ++curr_i)
    {
        if (curr_i->region == (byte*) ptr)
        {
            MemRegion *region_before = 0;
            MemRegion *region_after = 0;
            
            for (MemRegion *curr_j = arena->regions_arr; curr_j != arena->regions_arr + arena->regions_count; ++curr_j)
            {
                if (curr_j->region + curr_j->size == ptr)
                    region_before = curr_j;
                else if (curr_j->region == ptr + size)
                    region_after = curr_j;

                if (region_before && region_after)
                    break;
            }

            if (region_before && region_after)
            {
                region_before->size += (size + region_after->size);
                
                for (MemRegion *prev = region_after, *curr_k = region_after + 1;
                     curr_k != arena->regions_arr + arena->regions_count;
                     ++prev, ++curr_k)
                {
                    *prev = *curr_k;
                }

                --arena->regions_count;
            }
            else if (region_before)
                region_before->size += size;
            else if (region_after)
                region_after->region -= size;
            else
            {
                MemRegion *next_region = arena->regions_arr + arena->regions_count;
                next_region->region = (byte*) ptr;
                next_region->size = size;
                ++arena->regions_count;
            }

            break;
        }
    }
}

#ifdef TEST_MODE

ModuleTestSet memory_h_register_tests()
{
    ModuleTestSet set = {
        .module_name = __FILE__,
        .tests = {0},
        .count = 0,
    };

    register_test(&set, "Memory allocation works correctly", test_alloc);

    return set;
}

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
    // alloc_array(&mem, PAGE_SIZE, byte);

    return TEST_PASS;
}

#endif
