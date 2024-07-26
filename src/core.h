#ifndef CORE_H
#define CORE_H

#include <raylib.h>
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

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#include <stdint.h>
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define C_BLUE CLITERAL(Color){ 0x55, 0xcd, 0xfc, 0xff }
#define C_PINK CLITERAL(Color){ 0xf7, 0xa8, 0xb8, 0xff }

typedef enum {
    PUZZLE,
    PUZZLE_WIN,
    WORLD,
    MENU,
} GameState;

typedef struct {
    float pain;
    float pain_max;
    float energy;
    float energy_max;
    float energy_lim;
} PlayerState;

void render_hud(PlayerState pstate, float offx, Texture2D atlas);

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
