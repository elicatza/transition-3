#include "core.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void render_hud_rhs(PlayerState pstate, float offx, Texture2D atlas)
{
    (void) atlas;

    float s_width = GetScreenWidth();
    float s_height = GetScreenHeight();
    float padx = (s_width - offx) / 10.f;
    float x = offx;
    float width = s_width - x;
    float ysec = s_height * (1.f / 9.f);

    DrawRectangleLinesEx((Rectangle) {
        .width = width - 2.f * padx,
        .height = 3 * (pstate.energy_max / pstate.energy_lim) * ysec,
        .x = x + padx,
        .y = ysec + 3 * (1.f - (pstate.energy_max / pstate.energy_lim)) * ysec,
    }, 2.f, C_BLUE);

    DrawRectangleRec((Rectangle) {
        .width = width - 2.f * padx,
        .height = 3 * (pstate.energy / pstate.energy_lim) * ysec,
        .x = x + padx,
        .y = ysec + 3 * (1.f - (pstate.energy / pstate.energy_lim)) * ysec,
    }, C_BLUE);

    DrawRectangleLinesEx((Rectangle) {
        .width = width - 2.f * padx,
        .height = 3 * ysec,
        .x = x + padx,
        .y = ysec,
    }, 2.f, WHITE);

    DrawText("Energy", x + padx, ysec * 0.5f, 24.f, C_BLUE);

    DrawRectangleRec((Rectangle) {
        .width = width - 2.f * padx,
        .height = 3 * (pstate.pain / pstate.pain_max) * ysec,
        .x = x + padx,
        .y = 5 * ysec + 3 * (1.f - (pstate.pain / pstate.pain_max)) * ysec,
    }, C_PINK);

    DrawRectangleLinesEx((Rectangle) {
        .width = width - 2.f * padx,
        .height = 3 * ysec,
        .x = x + padx,
        .y = 5 * ysec,
    }, 2.f, WHITE);

    DrawText("Pain", x + padx, 4 * ysec + ysec * 0.5f, 24.f, C_PINK);
}

void format_date(PlayerState pstate, char *dest, size_t sz)
{
    ASSERT(sz >= 32);
    int day = (int) floorf(pstate.time);
    int weekday = day % 7;
    char buf[12];
    switch (weekday) {
        case 0: { memcpy(buf, "Wednesday", sizeof "Wednesday"); } break;
        case 1: { memcpy(buf, "Thursday", sizeof "Thursday"); } break;
        case 2: { memcpy(buf, "Friday", sizeof "Friday"); } break;
        case 3: { memcpy(buf, "Saturday", sizeof "Saturday"); } break;
        case 4: { memcpy(buf, "Sunday", sizeof "Sunday"); } break;
        case 5: { memcpy(buf, "Monday", sizeof "Monday"); } break;
        case 6: { memcpy(buf, "Tuesday", sizeof "Tuesday"); } break;
    }
    snprintf(dest, sz, "Day %d\n\n%s", day, buf);
}

void format_time(PlayerState pstate, char *dest, size_t sz)
{
    ASSERT(sz >= 6, "Buffer is too small");
    float t = (pstate.time - floorf(pstate.time)) * 24.f;
    float hour = floorf(t);
    float minute = (t - floorf(t)) * 60.f;
    snprintf(dest, sz, "%02.0f:%02.0f", hour, minute);
}

void render_hud_lhs(PlayerState pstate, float offx, Texture2D atlas)
{
    (void) atlas;
    (void) offx;

    float s_width = GetScreenWidth();
    float s_height = GetScreenHeight();
    float padx = (s_width - offx) / 10.f;
    float x = 0;
    // float width = s_width - offx;
    float ysec = s_height * (1.f / 9.f);


    char day[38];
    format_date(pstate, day, sizeof day);
    size_t len = strlen(day);
    day[len + 0] = '\n';
    day[len + 1] = '\n';
    format_time(pstate, (char *) (day + len + 2), 6);
    // INFO("%s", day);
    // INFO("%s", day + len + 1);
    DrawText(day, x + padx, ysec * 0.5f, 19.f, WHITE);
}
