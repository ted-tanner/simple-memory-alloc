#ifndef __DYN_ARRAY_H
#define __DYN_ARRAY_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "assert.h"

typedef struct _dynarr {
    uint32_t capacity;
    uint32_t size;
    size_t element_size;
    char* arr;
} DynArray;

#define new_dynarr(start_capacity, type) _create_dynarr(start_capacity, sizeof(type))
#define dynarr_push(arr_ptr, item) _dynarr_push(arr_ptr, &item)
#define dynarr_get(arr_ptr, pos, type) *((type*) _dynarr_get(arr_ptr, pos))
#define dynarr_get_ptr(arr_ptr, pos, type) (type*) _dynarr_get(arr_ptr, pos)

void free_dynarr(DynArray* arr);
void dynarr_shrink(DynArray* arr);

DynArray _create_dynarr(uint32_t start_capacity, size_t element_size);
void _dynarr_push(DynArray* arr, void* item_ptr);
char* _dynarr_get(const DynArray* arr, uint32_t pos);

#endif
