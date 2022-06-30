#ifndef __MEMORY_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>

#include "assert.h"
#include "debug.h"
#include "int.h"
#include "intrinsics.h"

#define __REGION_ARR_INIT_SIZE PAGE_SIZE

typedef struct {
    byte *region;
    u32 size;
} MemRegion;

typedef struct {
    byte *arena;
     
    u32 grow_size;
    u32 size;
    u32 used;
    
    MemRegion *regions_arr;
    u32 regions_count;
    u32 regions_capacity;
} MemArena;

MemArena create_arena(u32 init_size, u32 grow_size);

#define alloc_array(arena_ptr, arr_ptr, count) _alloc_data((arena_ptr), (arr_ptr), sizeof(*(arr)) * (count))
#define alloc_struct(arena_ptr, structure) _alloc_data((arena_ptr), (&structure), sizeof(structure))

void *_alloc_data(MemArena *restrict arena, void *restrict struct_ptr, size_t size);

// TODO: Functions to create and use a TransitoryArena within the MemArena with less allocation overhead that
//       can only be freed all at once

#define __MEMORY_H
#endif