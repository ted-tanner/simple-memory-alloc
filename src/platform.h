#ifndef __PLATFORM_H

// PLATFORM SPECIFIC

#if __APPLE__

#include <sys/mman.h>

#else

#error Platform not supported

#endif

// COMMON INTERFACE

#include "int.h"
#include "intrinsics.h"

void *allocate_new_chunk(void *start, u32 size);

// __linux__       Linux
// __sun           Solaris
// __FreeBSD__     FreeBSD
// __NetBSD__      NetBSD
// __OpenBSD__     OpenBSD
// __APPLE__       Mac OS X
// _WIN32          Windows

#define __PLATFORM_H
#endif

