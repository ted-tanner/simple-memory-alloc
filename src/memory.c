#include "memory.h"

static inline void *allocate_new_chunk(void *start, u32 size);

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

void *_alloc_data(MemArena *restrict arena, void *restrict data_ptr, size_t size)
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

        memcpy(destination, data_ptr, size);
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
        
        MemRegion region_from_regions_arr = {
            .region = (byte*) new_chunk,
            .size = new_regions_vec_size,
        };

        MemRegion *ptr_next_region = new_chunk + prev_regions_size;
        *ptr_next_region = region_from_regions_arr;
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

    MemRegion new_region = {
        .region = destination + size,
        .size = grow_size - size,
    };

    MemRegion *next_region = arena->regions_arr + arena->regions_count;
    *next_region = new_region;
    ++arena->regions_count;
    
    memcpy(destination, data_ptr, size);
    return destination;
}

static inline void *allocate_new_chunk(void *start, u32 size)
{
    return mmap(start, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}