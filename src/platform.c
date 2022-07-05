#include "platform.h"

#if __WASM__

#elif __APPLE__

FORCEINLINE void *allocate_new_chunk(void *start, u32 size)
{
    return mmap(start, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

#else

#error Platform not supported

#endif
