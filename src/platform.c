#include "platform.h"

#if __WASM__

#elif __APPLE__

FORCEINLINE void *map_new_memory_chunk(void *start, u32 size)
{
    return mmap(start, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

FORCEINLINE i32 unmap_memory_chunk(void *start, u32 size)
{
    return munmap(start, size);
}

#else

#error Platform not supported

#endif
