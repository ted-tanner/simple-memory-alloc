#ifndef __MEMORY_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "assert.h"
#include "debug.h"
#include "int.h"
#include "intrinsics.h"
#include "platform.h"

#define __REGION_ARR_INIT_SIZE  (2 * PAGE_SIZE)

typedef struct {
    byte *start;
    u32 size;
    bool is_alive;
} MemRegion;

typedef struct {
    byte *arena;   
     
    u32 grow_size;
    u32 size;
    u32 used;
    
    MemRegion *regions_arr;
    MemRegion *regions_arr_region;
    u32 regions_count;
    u32 regions_capacity;
} MemArena;

MemArena create_arena(u32 init_size, u32 grow_size);

void *arena_alloc(MemArena *restrict arena, u32 size);
void *arena_realloc(MemArena *restrict arena, void *restrict ptr, u32 new_size);
void arena_free(MemArena *restrict arena, void *restrict ptr);

#define alloc_array(arena_ptr, count, type) ((type*) arena_alloc((arena_ptr), sizeof(type) * (count)))
#define alloc_struct(arena_ptr, type) ((type*) arena_alloc((arena_ptr), sizeof(type)))



// TODO: Realloc

// TODO: Functions to create and use a TransitoryArena within the MemArena with less allocation overhead that
//       can only be freed all at once

#ifdef TEST_MODE

#include "test.h"

ModuleTestSet memory_h_register_tests();

#endif

#define __MEMORY_H
#endif
