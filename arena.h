#ifndef __ARENA_H
#define __ARENA_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if defined(__APPLE__) || defined(__unix__) || defined(linux)
#include <unistd.h>
#elif defined(WIN32) || defined (_WIN32)
#include <windows.h>
#endif

#include "assert.h"
#include "dynarray.h"

typedef struct _region {
    bool alive;
    char* start;
    char* end;
} _MemRegion;

typedef struct _arena {
    uint32_t capacity;
    uint32_t size;
    _MemRegion* regions;
    uint32_t regions_arr_size;
    uint32_t regions_arr_capacity;
    char* start;
    char* end;
} MemArena;

// TODO
// Create grow_arena and shrink_arena functions
// MemArena should become _MemBlock and MemArena (new struct) should have an array
// of _MemBlocks. When created, arena has only one block. grow_arena creates a new block.
// shrink_arena clears away all blocks that are empty, except for the first one.
// Perhaps grow_arena and shrink_arena should just be internal to create a seemless
// experience for allocating to the arena?

MemArena new_arena(uint32_t capacity, bool claim_immediately);
void free_arena(MemArena* arena);

#define arena_alloc_item(arena_ptr, type) (type*) _push_to_arena(arena_ptr, sizeof(type))
#define arena_alloc_array(arena_ptr, size, type) (type*) _push_to_arena(arena_ptr, size * sizeof(type))

void free_from_arena(MemArena* arena, void* pos);

#define arena_resize_arr(arena_ptr, arr_start, new_size, type) _arena_resize_arr(arena_ptr, \
										 arr_start, \
										 new_size * sizeof(type))

void* _push_to_arena(MemArena* arena, uint32_t size);
void* _arena_resize_arr(MemArena* arena, void* arr_start, size_t new_size);

#endif
