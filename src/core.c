#include "core.h"
#include <math.h>
#include <stdio.h>

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
    float padx = (s_width - MIN(s_width, s_height)) / 10.f;
    float x = 0;
    // float width = s_width - offx;
    float ysec = s_height * (1.f / 9.f);

    char msg[6];
    format_time(pstate, msg, sizeof msg);
    DrawText(msg, x + padx, ysec * 0.5f, 24.f, C_BLUE);
}
