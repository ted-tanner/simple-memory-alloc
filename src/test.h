#ifndef __TEST_H
#ifdef TEST_MODE

#include <stdlib.h>

#include "int.h"

#define TEST_NAME_MAX_LEN 256
#define TEST_OUTPUT_BUFFER_SIZE 8192
#define TEST_SET_MAX_TESTS 64
#define TEST_SET_MAX_COUNT 64

#define ASCII_RED "\e[31m"
#define ASCII_GREEN "\e[32m"
#define ASCII_ITALICS "\e[3m"
#define ASCII_DEFAULT "\e[0m"

char TEST_OUTPUT_BUFFER[TEST_OUTPUT_BUFFER_SIZE];

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

void add_test(ModuleTestSet* test_set, char *test_name, TestFunc test_func)
{
    Test test = {
        .test_name = {0},
        .test_func = test_func,
    };

    for (u32 i = 0; i < TEST_OUTPUT_BUFFER_SIZE && test_name[i]; ++i)
        test.test_name[i] = test_name[i];
    
    test_set->tests[test_set->count++] = test;
}

#endif
#define __TEST_H
#endif
