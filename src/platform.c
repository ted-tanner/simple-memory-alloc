#include "platform.h"

#if __WASM__

extern byte __heap_base;
static byte *bump_pointer = &__heap_base;

void *map_new_memory_chunk(u32 size)
{
    void *new_chunk = bump_pointer;
    bump_pointer += size;
    return new_chunk;
}

// WARNING: The WASM version of this function ignores start pointer and
//          simply unmaps off the top of the heap.
i32 unmap_memory_chunk(void *start, u32 size)
{
    bump_pointer -= size;
    return 0;
}

void abort()
{
    // TODO
}

void *memcpy(void *restrict dest, const void *restrict src, size_t size)
{
    byte *d = dest;
    const byte *s = src;

    while (--size)
        *d++ = *s++;

    return dest;
}
    
void *memmove(void *dest, const void *src, size_t size)
{
    byte *d = dest;
    const byte *s = src;

    if (d < s)
    {
        while (--size)
            *d++ = *s++;
    }
    else
    {
        
        for (byte *curr = d + size; curr != dest;)
            *(--d) = *(--s);
    }
    
    return dest;
}

#elif defined(__APPLE__) || defined (__linux__)

void *map_new_memory_chunk(u32 size)
{
    static void *starting_point = 0;

    void *new_chunk = mmap(starting_point, size,
                           PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
                           -1, 0);

    starting_point = new_chunk + size;
    
    return new_chunk;
}

FORCEINLINE i32 unmap_memory_chunk(void *start, u32 size)
{
    return munmap(start, size);
}

#else

#error Platform not supported

#endif
