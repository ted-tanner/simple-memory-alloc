#include <stdbool.h>
#include <stdio.h>

#include "assert.h"
#include "arena.h"
#include "dynarray.h"

MemArena arena;
DynArray all_arenas;

int main()
{
    arena = new_arena(36860000, true);

    all_arenas = new_dynarr(5, MemArena*);

    MemArena* arena_ptr = &arena;
    dynarr_push(&all_arenas, arena_ptr);
        
    for (uint32_t i = 0; i < 60000; ++i)
    {
	if (!(i % 1))
	{
	    printf("Arena region size/capacity: %d/%d\n", arena.regions_arr_size, arena.regions_arr_capacity);
	}
	
	uint8_t* arr = arena_alloc_array(&arena, 15, uint8_t);
	for (uint32_t j = 0; j < 15; ++j)
	{
	    arr[j] = j;
	}
    }

    for (uint32_t i = 0; i < arena.regions_arr_size; ++i)
    {
	if (!arena.regions[i].end)
	{
	    printf("FAILED!\n");
	    abort();
	}
    }

    // TODO Test arena_resize_arr
    // TODO Use an arena array for regions dyn array

    // free_from_arena(&arena, arena.start + 30);
    // free_from_arena(&arena, arena.start + 45);
    // free_from_arena(&arena, arena.start + 60);
    // free_from_arena(&arena, arena.start + 75);
    // free_from_arena(&arena, arena.start + 90);
    
    // uint32_t prev_size = arena.size;
    
    // uint8_t* new_loc = (uint8_t*) arena_resize_arr(&arena, arena.start + 15, 301, uint8_t);

    // assert((char*) new_loc == arena.start + prev_size, "Not right spot");
    // assert(arena.size == prev_size + 301, "Incorrect size");
    
    free_arena(&arena);
    
    return 0;
}
