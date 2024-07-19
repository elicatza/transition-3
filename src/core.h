#ifndef CORE_H
#define CORE_H

#include <stdio.h>
#include <stdlib.h>

#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_RED "\x1b[31m"
#define ANSI_RESET "\x1b[0m"

#define INFO(fmt, ...) fprintf(stderr, "["ANSI_GREEN"INFO"ANSI_RESET"] "fmt" (in %s at %s:%d)\n", ##__VA_ARGS__, __func__, __FILE__, __LINE__)
#define WARNING(fmt, ...) fprintf(stderr, "["ANSI_YELLOW"WARNING"ANSI_RESET"] "fmt" (in %s at %s:%d)\n", ##__VA_ARGS__, __func__, __FILE__, __LINE__)
#define ERROR(fmt, ...) fprintf(stderr, "["ANSI_RED"ERROR"ANSI_RESET"] "fmt" (in %s at %s:%d)\n", ##__VA_ARGS__, __func__, __FILE__, __LINE__)
#define ASSERT(cond, ...) { if (!(cond)) { ERROR(__VA_ARGS__); exit(1);} }

#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

#include <stdint.h>
typedef uint32_t u32;
typedef uint8_t u8;

#ifdef TEST
#define TEST_ASSERT(statement) test_assert(statement, #statement)

#include <stdbool.h>
static int passed = 0;
static int tests = 0;

void test_assert(bool statement, char *msg)
{
    ++tests;

    if (statement) {
        printf("[\x1b[32mPASSED\x1b[0m] %s\n", msg);
        ++passed;
    } else {
        printf("[\x1b[31mFAILED\x1b[0m] %s\n", msg);
    }
}
void test_result_print(void)
{
    printf("[%d/%d] Tests passed!\n", passed, tests);
}
#endif  /* TEST */

#endif  /* CORE_H */
