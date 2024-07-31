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

#define PENALTY_ENERGY_TIME 0.02f
#define PENALTY_PAIN_TIME 0.02f
#define PENALTY_PAIN 0.05f
#define PENALTY_ENERGY 0.025f
#define SLEEP_SPEED 0.04f

#define FONT_SIZE_BIG (GetScreenWidth() / 18.f)
#define FONT_SIZE_MID (GetScreenWidth() / 30.f)
#define FONT_SIZE_SMALL (GetScreenWidth() / 45.f)
#define LINE_SPACE 1.35f

typedef enum {
    PUZZLE_FUN,
    PUZZLE_FUN_WIN,
    PUZZLE_TRAIN,
    PUZZLE_TRAIN_WIN,
    PUZZLE_BOSS,
    PUZZLE_BOSS_WIN,
    WORLD,
    MENU,
    SLEEP,
    FAINT,
} GameState;

typedef struct {
    float pain;
    float pain_max;
    float energy;
    float energy_max;
    float light; /* [0, 1] */
    float light_tmp;
    float time;  /* 0-1 ranges a whole day */
    float energy_lim;
    float brightness;  /* 0..1 */
    bool did_faint;
    bool is_sleeping;
    int face_id;

    Color ani_color;
    float ani_time_max;
    float ani_time_remaining;
} PlayerState;

Color blend(Color main, Color blend, float intencity);
void render_hud_rhs(PlayerState pstate, float offx, Texture2D atlas);
void render_hud_lhs(PlayerState pstate, float offx, Texture2D atlas);
void format_time(PlayerState pstate, char *dest, size_t sz);
void render_player(Vector2 vs_pos, Vector2 dim, PlayerState pstate, Texture2D player_atlas, Color color);
void player_start_animation(PlayerState *pstate, Color color);
void update_pstate(PlayerState *pstate);
void apply_energy_loss(PlayerState *pstate);
void apply_pain(PlayerState *pstate);
bool should_faint(PlayerState pstate);

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
