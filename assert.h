#ifndef __ASSERT_H
#define __ASSERT_H

#ifdef DEBUG

#include <stdlib.h>
#include <stdio.h>

#define assert(condition, msg) if (condition) \
    { ((void)0); } else	\
    { fprintf(stderr, "ASSERTION ERROR: (%s:%d) | %s\r\n", __FILE__, __LINE__, msg); abort(); }

#else

#define assert(condition, msg) ((void)0);

#endif

#endif
