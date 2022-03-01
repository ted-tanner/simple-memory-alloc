#include "arena.h"

MemArena new_arena(uint32_t capacity, bool claim_immediately)
{
    char* block = malloc(capacity);
	
    if (!block)
    {
	fprintf(stderr, "MALLOC FAILURE (%s:%d)", __FILE__, __LINE__);
	abort();
    }

    if (claim_immediately)
    {
#if defined(__APPLE__) || defined(__unix__) || defined(linux)

	uint32_t page_size_bytes = (uint32_t) sysconf(_SC_PAGESIZE);
	
#elif defined(WIN32) || defined (_WIN32)
	
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);

	uint32_t page_size_bytes = (uint32_t) system_info.dwPageSize;
	    
#else
	    
	const uint32_t page_size_bytes = 4000;
	
#endif

	// Touch all the allocated pages to claim them immediately
	for (uint32_t i = 0; i < capacity - page_size_bytes; i += page_size_bytes)
	{
	    block[i] = 0;
	}
    }

    MemArena arena = {
	.capacity = capacity,
	.size = 0,
	.regions = (_MemRegion*) block,
	.regions_arr_capacity = capacity < 500000 ? 100 : 500,
	.regions_arr_size = 1,
	.start = block,
	.end = block + capacity,
    };

    _MemRegion first_region = {
	.alive = true,
	.start = block,
	.end = block + arena.regions_arr_capacity * sizeof(_MemRegion),
    };

    assert(arena.regions_arr_capacity * sizeof(_MemRegion) < arena.capacity * 0.1,
	   "Inital regions array takes more than 10% of the arena's capacity.");

    if (arena.regions_arr_capacity * sizeof(_MemRegion) > arena.capacity)
    {
	fprintf(stderr, "ARENA CREATION FAILURE: (%s:%d) | Arena is too small to hold regions array.",
		__FILE__,
		__LINE__);
	abort();
    }
    
    memcpy(block, &first_region, sizeof(first_region));

    arena.size = first_region.end - first_region.start;

    return arena;
}

void free_arena(MemArena* arena)
{
    free(arena->start);
}

void free_from_arena(MemArena* arena, void* pointer)
{
    if (!arena->regions_arr_size)
    {
	return;
    }

    size_t i = 0;
    for (_MemRegion* curr = arena->regions, *prev = {0};
	 i < arena->regions_arr_size;
	 prev = curr++, ++i)
    {
	if ((void*) curr->start == pointer)
	{
	    curr->alive = false;

	    if (curr->end == arena->end)
	    {
		if (prev->end)
		{
		    arena->size = prev->end - arena->start;
		}
		else
		{
		    arena->size = 0;
		}
	    }

   	    return;
	}
    }

    assert(false, "Tried to free region that wasn't in the arena");
}

void* _push_to_arena(MemArena* arena, uint32_t size)
{
    bool resurrect_region = false;
    uint32_t resurrect_region_pos = 0;

    char* prev_end = arena->start;
    for (uint32_t i = 0; i < arena->regions_arr_size; ++i)
    {
	_MemRegion curr = arena->regions[i];

 	if (!curr.alive)
	{
	    if (curr.end - curr.start >= size)
	    {
		resurrect_region_pos = i;
		resurrect_region = true;
		break;
	    }
	    else
	    {
		if (i == arena->regions_arr_size - 1)
		{
		    if (arena->end - curr.start >= size)
		    {
			resurrect_region_pos = i;
			resurrect_region = true;
			break;
		    }
		    else
		    {
			assert(false, "Tried to allocate chunk larger than arena could hold");
			return 0;
		    }
		}
		else
		{
		    _MemRegion next = arena->regions[i + 1];
		    if (next.start - curr.start >= size)
		    {
			resurrect_region_pos = i;
			resurrect_region = true;
			break;
		    }
		    else
		    {
			continue;
		    }
		}
	    }
	}
	
	if (curr.start - prev_end >= size)
	{
	    break;
	}

	resurrect_region = false;
	prev_end = curr.end;
    }

    _MemRegion region;
    if (resurrect_region)
    {
	_MemRegion* region_ptr = arena->regions + resurrect_region_pos;
	region_ptr->alive = true;
	region_ptr->end = region_ptr->start + size;

	if (region_ptr->end - arena->start > arena->size)
	{
	    arena->size = region_ptr->end - arena->start;
	}

	region = *region_ptr;
    }
    else
    {
	if (prev_end + size > arena->end)
	{
	    assert(false, "Tried to allocate chunk larger than arena could hold");
	    return 0;
	}

	region.alive = true;
	region.start = prev_end;
	region.end = prev_end + size;

	if (arena->regions_arr_size == arena->regions_arr_capacity - 1)
	{
	    arena->regions_arr_capacity *= 2;

	    _MemRegion* new_regions_arr = arena_resize_arr(arena,
							   arena->regions,
							   arena->regions_arr_capacity,
							   _MemRegion);
	    arena->regions = new_regions_arr;

	    assert(arena->regions_arr_capacity * sizeof(_MemRegion) / arena->size < 0.1,
		   "Arena regions array grew to more than 10% of the size of the arena"); 
	}

	arena->regions[arena->regions_arr_size] = region;
	++arena->regions_arr_size;
		
	arena->size += size;
    }

    return (void*) region.start;
}

// NOTE: There is a danger here. If the new array size is too big to allow for expanding the array in place,
// the array will be moved. Any previously created pointers into the array will be invalid.
//
// Users of this function CAN ASSUME that shrinking an array will not result in the array being moved
void* _arena_resize_arr(MemArena* arena, void* arr_start, size_t new_size)
{
    char* new_end = ((char*) arr_start) + new_size;

    for (uint32_t i = 0; i < arena->regions_arr_size; ++i)
    {
	_MemRegion* current_region = arena->regions + i;

	if (!current_region->alive)
	{
	    continue;
	}

	if (current_region->start == (char*) arr_start)
	{
	    if (new_end < current_region->end)
	    {
		current_region->end = new_end;
		return arr_start;
	    }
	   	    
	    if (i + 1 < arena->regions_arr_size)
	    {
		_MemRegion* next_region = arena->regions + i + 1;
		char* next_region_start = next_region->start;

		for (uint32_t j = i + 1; !next_region->alive; ++j)
		{
		    if (j > arena->regions_arr_size)
		    {
			next_region_start = arena->end;
			break;
		    }
		    
		    next_region = arena->regions + j;
		    next_region_start = next_region->start;
		}

		if (next_region_start > new_end)
		{
		    current_region->end = new_end;
		    return arr_start;
		}
	    }

	    char* previous_begin = current_region->start;
	    size_t previous_size = current_region->end - current_region->start; 
	    
	    free_from_arena(arena, current_region->start);
	    void* new_location = _push_to_arena(arena, new_size);

	    memmove(new_location, previous_begin, previous_size);
	    
	    return new_location;
	}
    }

    fprintf(stderr, "RESIZE ARRAY FAILURE: (%s:%d) | Arena does not contain array", __FILE__, __LINE__);
    abort();

    return 0;
}
