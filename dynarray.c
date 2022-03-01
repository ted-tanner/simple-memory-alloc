#include "dynarray.h"

void free_dynarr(DynArray* arr)
{
    free(arr->arr);
}

void dynarr_shrink(DynArray* arr)
{
    size_t new_capacity = arr->size * arr->element_size;
    char* new_arr = realloc(arr->arr, new_capacity);

    if (!new_arr)
    {
	fprintf(stderr, "REALLOC FAILURE (%s:%d)", __FILE__, __LINE__);
	abort();
    }

    arr->arr = new_arr;
    arr->capacity = new_capacity;
}

DynArray _create_dynarr(uint32_t start_capacity, size_t element_size)
{
    char* arr = malloc(element_size * start_capacity);

    if (!arr)
    {
	fprintf(stderr, "MALLOC FAILURE (%s:%d)", __FILE__, __LINE__);
	abort();
    }

    DynArray vec = {
	.capacity = start_capacity,
	.size = 0,
	.element_size = element_size,
	.arr = arr,
    };

    return vec;
}

void _dynarr_push(DynArray* arr, void* item)
{
    if (arr->size == arr->capacity)
    {
	char* new_arr = realloc(arr->arr, arr->capacity * arr->element_size * 2);
	
	if (!new_arr)
	{
	    fprintf(stderr, "REALLOC FAILURE (%s:%d)", __FILE__, __LINE__);
	    abort();
	}
	
	arr->arr = new_arr;
	arr->capacity *= 2;
    }

    memcpy(arr->arr + arr->size * arr->element_size, item, arr->element_size);
    ++arr->size;
}

char* _dynarr_get(const DynArray* arr, uint32_t pos)
{
    assert(pos < arr->size, "Access out of bounds");
    return arr->arr + pos * arr->element_size;
}


