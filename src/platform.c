#include "platform.h"

#if __WASM__

#elif __APPLE__

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
