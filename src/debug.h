#ifndef __DEBUG_H

#include "platform.h"

#if defined(DEBUG_MODE) && !defined(TEST_MODE)

enum LOG_LEVEL { DEBUG, INFO, ERROR };
#define log(level, message, ...) fprintf(stdout, "%s: " message "\r\n\t: (%s:%d in '%s')\r\n", \
                                         #level, ##__VA_ARGS__, __FILE__, __LINE__, __FUNCTION__)
#else

#define log(level, message, ...) ((void)0);

#endif

#define __DEBUG_H
#endif 
