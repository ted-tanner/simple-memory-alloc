#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "memory.h"
#include "test.h"

static void abort_handler(int signum) {
    siglongjmp(ENV_JUMP_BUFFER, 1);
}

int main(int argc, char **argv)
{
    ModuleTestSet test_sets[TEST_MAX_SET_COUNT] = {0};

    u32 test_set_count = 0;
    test_sets[test_set_count++] = memory_h_register_tests();

    printf("Running tests...\n");

    u32 passed = 0;
    u32 failed = 0;

    signal(SIGABRT, abort_handler);
    
    for (int i = 0; i < test_set_count; ++i)
    {
        ModuleTestSet *set = test_sets + i;

        printf("\n%s\n", set->module_name);

        for (int j = 0; j < set->count; ++j)
        {
            Test *test = set->tests + j;
            
            printf("\t* Testing \"%s\" --> ", test->test_name);
            fflush(stdout);

            // Redirect stdout
            int temp_stdout;
            int pipes[2];
            temp_stdout = dup(fileno(stdout));
            pipe(pipes);
            dup2(pipes[1], fileno(stdout));
            
            TEST_RESULT result = TEST_FAIL;

            // Set jmp_return so assertions can jump back here
            int jmp_return = sigsetjmp(ENV_JUMP_BUFFER, 1);
            if (!jmp_return)
                result = test->test_func();

            // Terminate test output with a zero
            write(pipes[1], "", 1);

            // Restore stdout
            fflush(stdout);
            dup2(temp_stdout, fileno(stdout));

            if (result == TEST_PASS)
            {
                printf("%s%s%s\n", ASCII_GREEN, "PASS", ASCII_DEFAULT);
                ++passed;
            }
            else
            {
                printf("%s%s%s\n", ASCII_RED, "FAIL", ASCII_DEFAULT);
                ++failed;

                // Read output from test
                bool is_first = true;
                while (1)
                {
                    char c;
                    read(pipes[0], &c, 1);

                    if (!c)
                    {
                        if (!is_first)
                            putc('\n', stdout);
                        
                        break;
                    }


                    if (c == '\n' || is_first)
                        printf("\n\t\t");
                    else
                        putc(c, stdout);

                    is_first = false;
                }
            }
        }
    }

    if (failed == 0)
        printf("\n%s%s%s - ", ASCII_GREEN, "PASS", ASCII_DEFAULT);
    else
        printf("\n%s%s%s - ", ASCII_RED, "FAIL", ASCII_DEFAULT);

    printf("%u test(s) passed, %u test(s) failed\n", passed, failed);
    
    return 0;
}
