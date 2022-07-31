#ifndef __PLATFORM_H

#include "intrinsics.h"

// PLATFORM SPECIFIC

#if __WASM__

#define _NO_STD_LIB

void abort();
void *memcpy(void *restrict dest, const void *restrict src, size_t size);
void *memmove(void *dest, const void *src, size_t size);

// TODO
#define fprintf(stream, message, ...) 

#elif defined(__APPLE__) || defined (__linux__)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/mman.h>

#else

#error Platform not supported

#endif

// COMMON INTERFACE

void *map_new_memory_chunk(u32 size);
i32 unmap_memory_chunk(void *start, u32 size);

// __linux__       Linux
// __sun           Solaris
// __FreeBSD__     FreeBSD
// __NetBSD__      NetBSD
// __OpenBSD__     OpenBSD
// __APPLE__       Mac OS X
// _WIN32          Windows

#define __PLATFORM_H
#endif

