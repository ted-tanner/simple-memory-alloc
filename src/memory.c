#include "memory.h"

MemArena create_arena(u32 init_size, u32 grow_size)
{
    assert(init_size != 0 && init_size >= 2 * __REGION_ARR_INIT_SIZE && init_size % PAGE_SIZE == 0, "Invalid init_size");
    assert(grow_size != 0 && grow_size % PAGE_SIZE == 0, "Invalid grow_size");
    
    void *new_chunk = map_new_memory_chunk(init_size);

    MemArena arena = {
        .arena = (byte*) new_chunk,
        .grow_size = grow_size,
        .size = init_size,
        .used = __REGION_ARR_INIT_SIZE,
        .regions_arr = (MemRegion*) new_chunk,
        .regions_arr_region = (MemRegion*) new_chunk,
        .regions_count = 2,
        .regions_capacity = __REGION_ARR_INIT_SIZE / sizeof(MemRegion),
    };

    if (!new_chunk)
    {
        log(ERROR, "Mapping memory from system failed");
        abort();
    }

    MemRegion *first_region = arena.regions_arr;
    first_region->start = (byte*) arena.regions_arr;
    first_region->size = __REGION_ARR_INIT_SIZE;
    first_region->is_alive = true;

    MemRegion *second_region = arena.regions_arr + 1;
    second_region->start = (byte*) arena.regions_arr + __REGION_ARR_INIT_SIZE;
    second_region->size = init_size - __REGION_ARR_INIT_SIZE;
    second_region->is_alive = false;

    return arena;
}

// WARNING: This function overwrites part of the memory arena as a temporary buffer. This will need to be 
//          changed if this function is used outside of the context of destroying the arena.
static byte *merge_sort_regions_by_start(MemArena *restrict arena, u32 begin, u32 end)
{
    u32 delta = end - begin;
    
    if (delta == 2)
    {
        if (arena->regions_arr[end].start < arena->regions_arr[begin].start)
        {
            MemRegion temp = arena->regions_arr[end];
            arena->regions_arr[end] = arena->regions_arr[begin];
            arena->regions_arr[begin] = temp;
        }
    }
    else if (delta != 1)
    {
        u32 mid = begin + delta / 2;
        byte *partition1 = merge_sort_regions_by_start(arena, begin, mid);
        byte *partition2 = merge_sort_regions_by_start(arena, mid + 1, end);

        if (partition2 < partition1)
        {
            // Max size of half of regions_arr  will be less than the space allocated
            // in the arena that does NOT contain the regions_arr, so it can be used as a temp
            // buffer (it is getting destroyed anyway). The regions array is at the beginning of
            // the arena at first, but if it has been realloced, it will be at the end. Check
            // which is the case to determine where the temp buffer will go.
            byte *temp_buffer = (byte*) arena->regions_arr_region == arena->arena
                ? (byte*) (arena->regions_arr + arena->regions_count)
                : (byte*) arena->regions_arr;

            memcpy(temp_buffer, arena->regions_arr + begin, (mid - begin) * sizeof(MemRegion));
            memcpy(arena->regions_arr + begin,
                   arena->regions_arr + (mid + 1),
                   (end - (mid + 1)) * sizeof(MemRegion));
            memcpy(arena->regions_arr + (mid + 1), temp_buffer, (mid - begin) * sizeof(MemRegion));
        }
    }

    return arena->regions_arr[begin].start;
}

void destroy_arena(MemArena *restrict arena)
{
    merge_sort_regions_by_start(arena, 0, arena->regions_count - 1);

    // Appropriate is_alive to mark region as beginning of memory page
    arena->regions_arr->is_alive = true;

    for (MemRegion *prev = arena->regions_arr, *curr = arena->regions_arr + 1;
         curr != arena->regions_arr + arena->regions_count;
         ++prev, ++curr)
    {
        if (curr->start == prev->start + prev->size)
        {
            curr->is_alive = false;
            prev->size += curr->size;
        }
        else
            curr->is_alive = true;
    }

    for (MemRegion *curr = arena->regions_arr;
         curr != arena->regions_arr + arena->regions_count;
         ++curr)
    {
        // Don't free regions array quite yet
        if (curr >= arena->regions_arr && curr < arena->regions_arr + arena->regions_count)
            continue;
        
        if (curr->is_alive)
            unmap_memory_chunk(curr->start, curr->size);
    }

    unmap_memory_chunk(arena->regions_arr, arena->regions_capacity * sizeof(MemRegion));

    arena->arena = 0;
    arena->size = 0;
    arena->used = 0;
    arena->regions_arr = 0;
    arena->regions_arr_region = 0;
    arena->regions_count = 0;
    arena->regions_capacity = 0;
}

// TODO: Use arena_realloc rather than map_new_chunk for reallocating regions array
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

        void *new_chunk = map_new_memory_chunk(grow_size);

        if (!new_chunk)
        {
            log(ERROR, "Mapping memory from system failed");
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
    ptr_new_region->is_alive = is_alive;

    ++arena->regions_count;

    return *ptr_new_region;
}

static inline void remove_region(MemArena *arena, MemRegion *region)
{
    memmove(region, region + 1, (byte*) (arena->regions_arr + arena->regions_count) - (byte*) region);
    --arena->regions_count;
}

void *arena_alloc(MemArena *restrict arena, u32 size)
{
    arena->used += size;

    for (MemRegion *curr_region = arena->regions_arr + arena->regions_count - 1;
         curr_region >= arena->regions_arr;
         --curr_region)
    {
        if (curr_region->is_alive || curr_region->size < size)
            continue;

        byte *destination = curr_region->start;

        if (curr_region->size == size)
            curr_region->is_alive = true;
        else
        {
            // Add new live region, taking space from curr_region
            add_region(arena, destination, size, true);
            
            // Resize dead region that live region was taken from
            curr_region->start += size;
            curr_region->size -= size;
        }

        return destination;
    }

    u32 grow_size = arena->grow_size;
    while (grow_size <= size)
        grow_size += arena->grow_size;

    void *new_chunk = map_new_memory_chunk(grow_size);
    
    if (!new_chunk)
    {
        log(ERROR, "Mapping memory from system failed");
        abort();
    }

    arena->size += grow_size;
    
    // Create region for empty space in addition to alloced space
    add_region(arena, (byte*) new_chunk + size, grow_size - size, false);
    add_region(arena, (byte*) new_chunk, size, true);

    return (byte*) new_chunk;
}

void *arena_realloc(MemArena *restrict arena, void *restrict ptr, u32 new_size)
{
    MemRegion *restrict ptr_region = 0;

    for (MemRegion *curr = arena->regions_arr + arena->regions_count - 1;
         curr >= arena->regions_arr;
         --curr)
    {
        if (curr->start == ptr)
        {
            ptr_region = curr->is_alive ? curr : 0;
            break;
        }
    }

    if (!ptr_region)
    {
        assert(false, "Attempted to realloc a pointer that wasn't allocated");
        return 0;
    }

    if (new_size == ptr_region->size)
        return ptr;

    i64 delta = (i64) new_size - (i64) ptr_region->size;
    arena->used += delta;

    MemRegion *restrict region_after = 0;
    MemRegion *restrict region_before = 0;

    // The pointers will remain at 0 if regions are alive so these are needed to track
    // whether regions have been found
    bool found_region_after = false;
    bool found_region_before = false;

    for (MemRegion *curr = arena->regions_arr + arena->regions_count - 1;
         curr >= arena->regions_arr;
         --curr)
    {
        if (!found_region_after && curr->start == ptr_region->start + ptr_region->size)
        {
            found_region_after = true;
            region_after = curr->is_alive ? 0 : curr;

            if (found_region_before)
                break;
        }
        else if (!found_region_before && curr->start + curr->size == ptr_region->start)
        {
            found_region_before = true;
            region_before = curr->is_alive ? 0 : curr;

            if (found_region_after)
                break;
        }
    }

    if (new_size < ptr_region->size)
    {        
        ptr_region->size = new_size;

        if (region_after)
        {
            // delta will be negative because new_size < ptr_region->size
            region_after->start += delta;
            region_after->size -= delta;
        }
        else if (region_before)
        {
            region_before->size -= delta;
            ptr_region->start -= delta;
        }
        else // TODO: Test 
            add_region(arena, ptr_region->start + new_size, -delta, false);

        return ptr_region->start;
    }
    else
    {
        u32 combined_regions_size;
       
        if (region_after && (combined_regions_size = ptr_region->size + region_after->size) >= new_size)
        {
            if (combined_regions_size == new_size)
            {
                // TODO: Test
                ptr_region->size = new_size;
                remove_region(arena, region_after);
            }
            else
            {
                ptr_region->size = new_size;
                region_after->start += delta;
                region_after->size -= delta;
            }

            return ptr;
        }
        else if (region_before && (combined_regions_size = ptr_region->size + region_before->size) >= new_size)
        {                       
            if (combined_regions_size == new_size)
            {
                printf("\n HERE!!!!! \n\n");
                // TODO: Test
                ptr_region->start = region_before->start;
                ptr_region->size = new_size;
                
                remove_region(arena, region_before);
            }
            else
            {
                ptr_region->start -= delta;
                region_before->size -= delta;
                ptr_region->size = new_size;
            }

            return ptr_region->start;
        }
        else if (region_before && region_after &&
                 (combined_regions_size = ptr_region->size + region_before->size + region_after->size) >= new_size)
        {
            if (combined_regions_size == new_size)
            {
                // TODO: Test
                ptr_region->start = region_before->start;
                ptr_region->size = new_size;
                
                remove_region(arena, region_before);
                remove_region(arena, region_after);
            }
            else
            {
                // TODO: Test
                ptr_region->start -= region_before->size;
                i64 delta_less_region_before_size = delta - region_before->size;
                
                region_after->start += delta_less_region_before_size;
                region_after->size -= delta_less_region_before_size;
                    
                ptr_region->size = new_size;

                remove_region(arena, region_before);
            }

        }
        else
        {
            // TODO: Test
            void *new_mem_block = arena_alloc(arena, new_size);

            // alloc_arena will increase arena->used, but the delta has already been added so we need
            // to subtract new_size here
            arena->used -= new_size;
            
            memcpy(new_mem_block, ptr_region->start, ptr_region->size);
            ptr_region->is_alive = false;

            return new_mem_block;
        }
    }

    assert(false, "Unreachable");
    return 0;
}

void arena_free(MemArena *restrict arena, void *restrict ptr)
{
    MemRegion *restrict ptr_region = 0;
    
    MemRegion *restrict dead_region_before = 0;
    MemRegion *restrict dead_region_after = 0;

    for (MemRegion *curr = arena->regions_arr + arena->regions_count - 1;
         curr >= arena->regions_arr;
         --curr)
    {

        if (curr->start == ptr)
            ptr_region = curr;
        else if (!curr->is_alive)
        {
            if(curr->start + curr->size == ptr)
            {
                dead_region_before = curr;

                if (dead_region_after)
                    break;
            }
            else if (ptr_region && curr->start - ptr_region->size == ptr)
            {
                dead_region_after = curr;

                if (dead_region_before)
                    break;

            }
        }
    }

    if (!ptr_region)
    {
        assert(false, "Attempted to free a pointer that wasn't allocated");
        return;
    }

    arena->used -= ptr_region->size;

    if (!dead_region_before && !dead_region_after)
        ptr_region->is_alive = false;
    else if (dead_region_before && dead_region_after)
    {
        dead_region_before->size += (ptr_region->size + dead_region_after->size);

        remove_region(arena, ptr_region);
        remove_region(arena, dead_region_after);
    }
    else if (dead_region_before)
    {
        dead_region_before->size += ptr_region->size;
        remove_region(arena, ptr_region);
    }
    else // dead_region_after
    {
        dead_region_after->start -= ptr_region->size;
        dead_region_after->size += ptr_region->size;

        remove_region(arena, ptr_region);
    }
}

#ifdef TEST_MODE

static void print_arena_details(MemArena *arena)
{
    printf("-----------------------------------------\n\n");
    
    for (u32 i = 0; i < arena->regions_count; ++i)
    {
        MemRegion curr_region = arena->regions_arr[i];
        printf("Start: %p\n", curr_region.start);
        printf("Size: %u\n", curr_region.size);
        printf("Alive: %s\n\n", curr_region.is_alive ? "Yes" : "No");
    }

    printf("-----------------------------------------\n\n\n");
}

static TEST_RESULT test_alloc_and_free()
{
    MemArena mem = create_arena(PAGE_SIZE * 5, PAGE_SIZE * 2);

    u32 usage_before_alloc = mem.used;
    u32 regions_count_before_alloc = mem.regions_count;
    u32 starting_capacity = mem.size;

    MemRegion *empty_region = mem.regions_arr + 1;
    byte *empty_region_start_before_alloc = empty_region->start;
    u32 empty_region_size_before_alloc = empty_region->size;

    assert(empty_region->start == mem.arena + mem.regions_capacity * sizeof(MemRegion),
           "Incorrect starting dead region position");
    assert(empty_region->size == mem.size - mem.regions_capacity * sizeof(MemRegion),
           "Incorrect starting dead region size");
    assert(!empty_region->is_alive,"Incorrect starting dead region liveness");

    const u32 array1_size = PAGE_SIZE + 19;
    byte *array1 = (byte*) arena_alloc(&mem, array1_size);
    assert(mem.used == usage_before_alloc + array1_size, "Incorrect arena usage after allocation");
    assert(mem.size == starting_capacity, "Arena size changed when it should not have");
    assert(mem.regions_count == regions_count_before_alloc + 1, "Unexpected number of regions");

    assert(empty_region_start_before_alloc == array1, "Incorrect region position");
    assert(mem.regions_arr[mem.regions_count - 1].start == array1, "Incorrect region position");
    assert(mem.regions_arr[mem.regions_count - 1].size == array1_size, "Incorrect region size");
    assert(mem.regions_arr[mem.regions_count - 1].is_alive, "Incorrect region liveness");

    assert(empty_region->start == empty_region_start_before_alloc + array1_size, "Incorrect region position");
    assert(empty_region->size == empty_region_size_before_alloc - array1_size, "Incorrect region size");
    assert(!empty_region->is_alive, "Incorrect region liveness");
    
    const u32 array2_size = 9;
    byte *array2 = (byte*) arena_alloc(&mem, array2_size);
    assert(array2 == array1 + array1_size, "Alloc'ed memory not aligned as expected");
    assert(mem.used == usage_before_alloc + array1_size + array2_size, "Incorrect arena usage after allocation");
    assert(mem.size == starting_capacity, "Arena size changed when it should not have");
    assert(mem.regions_count == regions_count_before_alloc + 2, "Unexpected number of regions");

    assert(mem.regions_arr[mem.regions_count - 1].start == array2, "Incorrect region position");
    assert(mem.regions_arr[mem.regions_count - 1].size == array2_size, "Incorrect region size");
    assert(mem.regions_arr[mem.regions_count - 1].is_alive, "Incorrect region liveness");

    assert(empty_region->start == array1 + array1_size + array2_size, "Incorrect region position");
    assert(empty_region->size == empty_region_size_before_alloc - array1_size - array2_size, "Incorrect region size");
    assert(!empty_region->is_alive, "Incorrect region liveness");

    const u32 array3_size = 12;
    byte *array3 = (byte*) arena_alloc(&mem, array3_size);
    assert(mem.used == usage_before_alloc + array1_size + array2_size + array3_size,
           "Incorrect arena usage after allocation");
    assert(mem.regions_count == regions_count_before_alloc + 3, "Unexpected number of regions");
    
    assert(mem.regions_arr[mem.regions_count - 1].start == array3, "Incorrect region position");
    assert(mem.regions_arr[mem.regions_count - 1].size == array3_size, "Incorrect region size");
    assert(mem.regions_arr[mem.regions_count - 1].is_alive, "Incorrect region liveness");

    arena_free(&mem, array2);
    assert(mem.used == usage_before_alloc + array1_size + array3_size,
           "Incorrect arena usage after free");
    assert(mem.regions_count == regions_count_before_alloc + 3, "Unexpected number of regions");

    assert(mem.regions_arr[mem.regions_count - 1].start == array3, "Incorrect region position");
    assert(mem.regions_arr[mem.regions_count - 1].size == array3_size, "Incorrect region size");
    assert(mem.regions_arr[mem.regions_count - 1].is_alive, "Incorrect region liveness");

    assert(mem.regions_arr[mem.regions_count - 2].start == array2, "Incorrect region position");
    assert(mem.regions_arr[mem.regions_count - 2].size == array2_size, "Incorrect region size");
    assert(!mem.regions_arr[mem.regions_count - 2].is_alive, "Incorrect region liveness");

    const u32 array4_size = array2_size;
    byte *array4 = (byte*) arena_alloc(&mem, array4_size);
    assert(mem.used == usage_before_alloc + array1_size + array3_size + array4_size,
           "Incorrect arena usage after allocation");
    assert(mem.regions_count == regions_count_before_alloc + 3, "Unexpected number of regions");
    assert(array2 == array4, "array2 and array4 should point to the same location");

    assert(mem.regions_arr[mem.regions_count - 2].start == array4, "Incorrect region position");
    assert(mem.regions_arr[mem.regions_count - 2].size == array4_size, "Incorrect region size");
    assert(mem.regions_arr[mem.regions_count - 2].is_alive, "Incorrect region liveness");

    // Region after
    u32 region_after_size_before_free = mem.regions_arr[1].size;
    arena_free(&mem, array3);
    assert(mem.used == usage_before_alloc + array1_size + array4_size,
           "Incorrect arena usage after free");
    assert(mem.regions_count == regions_count_before_alloc + 2, "Unexpected number of regions");

    assert(mem.regions_arr[1].start == array3, "Incorrect region position");
    assert(mem.regions_arr[1].size == region_after_size_before_free + array3_size, "Incorrect region size");
    assert(!mem.regions_arr[1].is_alive, "Incorrect region liveness");

    // Region before
    const u32 array5_size = 50;
    byte *array5 = (byte*) arena_alloc(&mem, array5_size);

    const u32 array6_size = 60;
    byte *array6 = (byte*) arena_alloc(&mem, array6_size);

    const u32 array7_size = 70;
    byte *array7 = (byte*) arena_alloc(&mem, array7_size);

    assert(mem.used == usage_before_alloc + array1_size + array4_size + array5_size + array6_size
           + array7_size, "Incorrect arena usage after allocation");

    arena_free(&mem, array5);
    assert(mem.used == usage_before_alloc + array1_size + array4_size + array6_size + array7_size,
           "Incorrect arena usage after free");
    assert(mem.regions_count == regions_count_before_alloc + 5, "Unexpected number of regions");
    
    arena_free(&mem, array6);
    assert(mem.used == usage_before_alloc + array1_size + array4_size + array7_size,
           "Incorrect arena usage after free");
    assert(mem.regions_count == regions_count_before_alloc + 4, "Unexpected number of regions");

    assert(mem.regions_arr[mem.regions_count - 1].start == array7, "Incorrect region position");
    assert(mem.regions_arr[mem.regions_count - 1].size == array7_size, "Incorrect region size");
    assert(mem.regions_arr[mem.regions_count - 1].is_alive, "Incorrect region liveness");

    assert(mem.regions_arr[mem.regions_count - 2].start == array5, "Incorrect region position");
    assert(mem.regions_arr[mem.regions_count - 2].size == array5_size + array6_size, "Incorrect region size");
    assert(!mem.regions_arr[mem.regions_count - 2].is_alive, "Incorrect region liveness");

    // Regions before and after
    u32 before_size_before_free = mem.regions_arr[mem.regions_count - 2].size;
    u32 after_size_before_free = mem.regions_arr[1].size;
    
    arena_free(&mem, array7);
    assert(mem.used == usage_before_alloc + array1_size + array4_size, "Incorrect arena usage after free");
    assert(mem.regions_count == regions_count_before_alloc + 2, "Unexpected number of regions");
        
    assert(mem.regions_arr[mem.regions_count - 1].start == array5, "Incorrect region position");
    assert(mem.regions_arr[mem.regions_count - 1].size ==
           before_size_before_free + array7_size + after_size_before_free, "Incorrect region size");
    assert(!mem.regions_arr[mem.regions_count - 1].is_alive, "Incorrect region liveness");

    destroy_arena(&mem);

    return TEST_PASS;
}

static TEST_RESULT test_alloc_fill_arena()
{
    const u32 init_arena_size = PAGE_SIZE * 6;
    const u32 arena_grow_size = PAGE_SIZE * 2;
    MemArena mem = create_arena(init_arena_size, arena_grow_size);

    for (; mem.used != init_arena_size; arena_alloc(&mem, PAGE_SIZE));
    assert(mem.size == init_arena_size, "");

    arena_alloc(&mem, 1);
    assert(mem.size == init_arena_size + arena_grow_size, "");
    assert(mem.used == init_arena_size + 1, "");
    
    for (; mem.size == init_arena_size + arena_grow_size; arena_alloc(&mem, PAGE_SIZE));

    assert(mem.size == init_arena_size + 2 * arena_grow_size, "");
    
    destroy_arena(&mem);

    return TEST_PASS;
}


static TEST_RESULT test_add_region()
{
    MemArena mem = create_arena(PAGE_SIZE * 10, PAGE_SIZE * 2);

    u32 starting_regions_capacity = mem.regions_capacity;
    u32 starting_regions_count = mem.regions_count;

    assert((byte*) mem.regions_arr == mem.arena,
           "Regions array at unexpected location");
    assert(mem.regions_capacity == __REGION_ARR_INIT_SIZE / sizeof(MemRegion),
           "Unexpected regions array capacity");
        
    for (u32 i = 0; i < starting_regions_capacity - starting_regions_count; ++i)
        arena_alloc(&mem, 1);

    assert((byte*) mem.regions_arr == mem.arena,
           "Regions array at unexpected location");
    assert(mem.regions_capacity == __REGION_ARR_INIT_SIZE / sizeof(MemRegion),
           "Unexpected regions array capacity");

    arena_alloc(&mem, 1);

    assert((byte*) mem.regions_arr != mem.arena,
           "Regions array at unexpected location");
    assert(mem.regions_capacity == starting_regions_capacity * 2,
           "Unexpected regions array capacity");

    destroy_arena(&mem);
    
    return TEST_PASS;
}

static TEST_RESULT test_realloc()
{
    MemArena mem = create_arena(PAGE_SIZE * 5, PAGE_SIZE * 2);

    u32 arena_used_before_alloc = mem.used;

    const u32 array1_size = 100;
    void *array1 = arena_alloc(&mem, array1_size);

    byte *array1_pos = mem.regions_arr[mem.regions_count - 1].start;
    assert(array1_pos == array1, "");
    
    u32 arena_regions_before_realloc = mem.regions_count;

    assert(mem.regions_arr[mem.regions_count - 1].size == array1_size, "");
    assert(mem.regions_arr[mem.regions_count - 1].start == array1_pos, "");

    // if, if
    u32 realloc_size = 100;
    arena_realloc(&mem, array1, realloc_size);

    assert(mem.used == arena_used_before_alloc + realloc_size, "");
    assert(mem.regions_count == arena_regions_before_realloc, "");
    assert(mem.regions_arr[mem.regions_count - 1].size == array1_size, "");
    assert(mem.regions_arr[mem.regions_count - 1].start == array1_pos, "");

    // else, if, else
    realloc_size = 200;
    byte *new_pos = arena_realloc(&mem, array1, realloc_size);
    assert(new_pos == array1_pos, "");

    assert(mem.used == arena_used_before_alloc + realloc_size, "");
    assert(mem.regions_count == arena_regions_before_realloc, "");
    assert(mem.regions_arr[mem.regions_count - 1].size == realloc_size, "");
    assert(mem.regions_arr[mem.regions_count - 1].start == array1_pos, "");

    // if, if
    realloc_size = 150;
    new_pos = arena_realloc(&mem, array1, realloc_size);
    assert(new_pos == array1_pos, "");

    assert(mem.used == arena_used_before_alloc + realloc_size, "");
    assert(mem.regions_count == arena_regions_before_realloc, "");
    assert(mem.regions_arr[mem.regions_count - 1].size == realloc_size, "");
    assert(mem.regions_arr[mem.regions_count - 1].start == array1_pos, "");

    // else, else if 1, else
    const u32 array2_size = 50;
    byte *array2 = arena_alloc(&mem, array2_size);
    arena_alloc(&mem, 60);

    arena_free(&mem, array1_pos);

    u32 used_before_realloc = mem.used;
    u32 regions_before_realloc = mem.regions_count;

    u32 region_after_size_before_realloc = mem.regions_arr[mem.regions_count - 3].size;

    realloc_size = 70;
    void *new_array2 = arena_realloc(&mem, array2, realloc_size);

    assert(mem.used == used_before_realloc + realloc_size - array2_size, "");
    assert(mem.regions_count == regions_before_realloc, "");
    assert(mem.regions_arr[mem.regions_count - 2].size == realloc_size, "");
    assert(mem.regions_arr[mem.regions_count - 2].start == array2 - (realloc_size - array2_size), "");
    assert(mem.regions_arr[mem.regions_count - 2].start == new_array2, "");

    assert(mem.regions_arr[mem.regions_count - 3].size ==
           region_after_size_before_realloc - (realloc_size - array2_size), "");
    assert(mem.regions_arr[mem.regions_count - 3].start == array1, "");
    
    // if, else if
    used_before_realloc = mem.used;
    regions_before_realloc = mem.regions_count;

    u32 new_array2_size = realloc_size;
    u32 region_before_size_before_realloc = mem.regions_arr[mem.regions_count - 3].size;

    realloc_size = 40;
    void *new_array2_after_realloc = arena_realloc(&mem, new_array2, realloc_size);

    assert(mem.used == used_before_realloc + realloc_size - new_array2_size, "");
    assert(mem.regions_count == regions_before_realloc, "");
    assert(mem.regions_arr[mem.regions_count - 2].size == realloc_size, "");
    assert(mem.regions_arr[mem.regions_count - 2].start == new_array2 + (new_array2_size - realloc_size), "");
    assert(mem.regions_arr[mem.regions_count - 2].start == new_array2_after_realloc, "");

    assert(mem.regions_arr[mem.regions_count - 3].size ==
           region_before_size_before_realloc - (realloc_size - new_array2_size), "");
    assert(mem.regions_arr[mem.regions_count - 3].start == array1, "");

    // else, else if 1, if

    // Manually set up conditions for test
    u32 temp_buffer_size = 1000;
    byte *temp_buffer = arena_alloc(&mem, temp_buffer_size);

    MemRegion *temp_buffer_region = 0;
    for (u32 i = 0; i < mem.regions_count; ++i)
    {
        if (mem.regions_arr[i].start == temp_buffer)
        {
            temp_buffer_region = mem.regions_arr + i;
            break;
        }
    }

    assert(temp_buffer_region, "");
    
    u32 region_before_size = 185;
    MemRegion region_before = add_region(&mem, temp_buffer_region->start, region_before_size, false);

    u32 array3_size = 250;
    add_region(&mem, temp_buffer_region->start + region_before_size, array3_size, false);
        
    u32 region_after_size = temp_buffer_size - array3_size - region_before_size;
    MemRegion region_after = add_region(&mem, temp_buffer_region->start, region_after_size, true);

    remove_region(&mem, temp_buffer_region);

    used_before_realloc = mem.used;
    regions_before_realloc = mem.regions_count;

    region_after_size_before_realloc = mem.regions_arr[mem.regions_count - 3].size;

    print_arena_details(&mem);

    realloc_size = array3_size + region_before_size;
    void *new_array3 = arena_realloc(&mem, array3, realloc_size);

    assert(mem.used == used_before_realloc + realloc_size - array3_size, "");
    assert(mem.regions_count == regions_before_realloc - 1, "");
    assert(mem.regions_arr[mem.regions_count - 2].size == realloc_size, "");
    assert(mem.regions_arr[mem.regions_count - 2].start == array3 - (realloc_size - array3_size), "");
    assert(mem.regions_arr[mem.regions_count - 2].start == new_array3, "");

    // TODO: Finish testing all the cases (there are 11)
    
    destroy_arena(&mem);
    
    return TEST_PASS;
}

ModuleTestSet memory_h_register_tests()
{
    ModuleTestSet set = {
        .module_name = __FILE__,
        .tests = {0},
        .count = 0,
    };

    register_test(&set, test_alloc_and_free);
    register_test(&set, test_alloc_fill_arena);
    register_test(&set, test_add_region);
    register_test(&set, test_realloc);
    
    return set;
}

#endif
