#define TEST_MODE

#include <stdio.h>

#include "memory.h"
#include "test.h"

int main(int argc, char **argv)
{
    ModuleTestSet test_sets[TEST_SET_MAX_COUNT] = {0};

    u32 test_set_count = 0;
    test_sets[test_set_count++] = memory_h_register_tests();

    printf("Running tests...\n");

    u32 passed = 0;
    u32 failed = 0;
    
    for (int i = 0; i < test_set_count; ++i)
    {
        ModuleTestSet *set = test_sets + i;

        printf("\n%s\n", set->module_name);

        for (int j = 0; j < set->count; ++j)
        {
            Test *test = set->tests + j;

            TEST_OUTPUT_BUFFER[0] = 0;
            
            TEST_RESULT result = test->test_func();

            if (result == TEST_PASS)
            {
                printf("\t%s%s%s | ", ASCII_GREEN, "PASS", ASCII_DEFAULT);
                ++passed;
            }
            else
            {
                printf("\t%s%s%s | ", ASCII_RED, "FAIL", ASCII_DEFAULT);
                ++failed;
            }

            printf("%s\n", test->test_name);

            if (TEST_OUTPUT_BUFFER[0])
                printf("\n%s%s%s\n", ASCII_ITALICS, TEST_OUTPUT_BUFFER, ASCII_DEFAULT);
        }
    }

    if (failed == 0)
        printf("\n%s%s%s - ", ASCII_GREEN, "PASS", ASCII_DEFAULT);
    else
        printf("\n%s%s%s - ", ASCII_RED, "FAIL", ASCII_DEFAULT);

    printf("%u test(s) passed, %u test(s) failed\n", passed, failed);
    
    return 0;
}
