#ifndef __ASSERT_H

#ifdef DEBUG_MODE

#ifdef _MSC_VER
#define _FUNC_ __FUNCSIG__
#else
#define _FUNC_ __PRETTY_FUNCTION__
#endif

#include <stdlib.h>
#include <stdio.h>

#ifdef TEST_MODE

#include "test.h"

#define assert(condition, msg) if (condition)                           \
    { ((void)0); } else                                                 \
    { snprintf(TEST_OUTPUT_BUFFER,                                      \
               TEST_OUTPUT_BUFFER_SIZE,                                 \
               "\t\t  ASSERTION ERROR: %s\r\n\t\t\t  : '%s' evaluated to 0\r\n\t\t\t  : in function '%s'\r\n\t\t\t  : at '%s' line %d.\r\n", \
              msg, #condition, _FUNC_, __FILE__, __LINE__); return TEST_FAIL; }

#else // TEST_MODE

#define assert(condition, msg) if (condition)                           \
    { ((void)0); } else                                                 \
    { fprintf(stderr, "ASSERTION ERROR: %s\r\n\t: '%s' evaluated to 0\r\n\t: in function '%s'\r\n\t: at '%s' line %d.\r\n", \
              msg, #condition, _FUNC_, __FILE__, __LINE__); abort(); }

#endif // TEST_MODE

#else // DEBUG_MODE

#define assert(condition, msg) ((void)0);

#endif // DEBUG_MODE

#define __ASSERT_H
#endif
