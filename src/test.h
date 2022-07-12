#ifndef __TEST_H

#include <setjmp.h>
#include <stdlib.h>

#include "int.h"

#define TEST_NAME_MAX_LEN 256
#define TEST_SET_MAX_TESTS 64
#define TEST_MAX_SET_COUNT 64

#define ASCII_RED "\e[31m"
#define ASCII_GREEN "\e[32m"
#define ASCII_DEFAULT "\e[0m"

extern jmp_buf ENV_JUMP_BUFFER;

typedef enum { TEST_PASS, TEST_FAIL } TEST_RESULT;
typedef TEST_RESULT (*TestFunc)();

typedef struct {
    char test_name[TEST_NAME_MAX_LEN];
    TestFunc test_func;
} Test;

typedef struct {
    char module_name[TEST_NAME_MAX_LEN];
    Test tests[TEST_SET_MAX_TESTS];

    u32 count;
} ModuleTestSet;

#define register_test(test_set, test_func) _register_test(test_set, #test_func, test_func)
void _register_test(ModuleTestSet* test_set, char *test_name, TestFunc test_func);

#define __TEST_H
#endif
