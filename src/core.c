#include "core.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/**
 * @param intencity value from [0, inf>
 */
Color blend(Color main, Color blend, float intencity)
{
    return (Color) {
        .r = (main.r + intencity * blend.r) / (1.f + intencity),
        .g = (main.g + intencity * blend.g) / (1.f + intencity),
        .b = (main.b + intencity * blend.b) / (1.f + intencity),
        .a = (main.a + intencity * blend.a) / (1.f + intencity),
    };
}


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
    float minute = (t - floorf(t)) * 59.f;
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

void render_player(Vector2 vs_pos, Vector2 dim, PlayerState pstate, Texture2D player_atlas, Color color)
{
    Rectangle dest = {
        .x = vs_pos.x,
        .y = vs_pos.y,
        .width = dim.x,
        .height = dim.y,
    };
    Rectangle src = {
        .x = 0.f,
        .y = 0.f,
        .width = 16.f,
        .height = 16.f,
    };
    if (pstate.face_id == 1 || pstate.face_id == 3) {
        src.x = 0.f;
        src.y = 0.f;
    }
    else if (pstate.face_id == 2 || pstate.face_id == 4) {
        src.x = 16.f;
        src.y = 0.f;
    }
    else if (pstate.face_id == 0 || pstate.face_id == 5) {
        src.x = 2.f * 16.f;
        src.y = 0.f;
    }

    Color fade = Fade(pstate.ani_color, pstate.ani_time_remaining / pstate.ani_time_max);

    DrawRectangleV(vs_pos, dim, color);
    DrawRectangleV(vs_pos, dim, fade);

    DrawTexturePro(player_atlas, src, dest, (Vector2) { 0.f, 0.f }, 0, color);
}

void player_start_animation(PlayerState *pstate, Color color)
{
    pstate->ani_color = color;
    pstate->ani_time_max = 0.5f;
    pstate->ani_time_remaining = 0.5f;
}

void update_pstate(PlayerState *pstate)
{
    pstate->ani_time_remaining -= GetFrameTime();
    if (pstate->ani_time_remaining < 0.f) {
        pstate->ani_time_remaining = 0.f;
    }
}

void apply_pain(PlayerState *pstate)
{
    player_start_animation(pstate, C_PINK);
    pstate->time += PENALTY_PAIN_TIME;
    pstate->pain += PENALTY_PAIN;
}

void apply_energy_loss(PlayerState *pstate)
{
    player_start_animation(pstate, C_BLUE);
    pstate->time += PENALTY_ENERGY_TIME;
    pstate->energy -= PENALTY_ENERGY;
}

bool should_faint(PlayerState pstate)
{
    if (pstate.energy < 0) {
        pstate.energy = 0.f;
        return true;
    }
    if (pstate.pain > pstate.pain_max) {
        pstate.pain_max = pstate.pain_max;
        return true;
    }
    return false;
}

int new_face_id(int face_id, Direction dir)
{
    switch (dir) {
        case UP: { switch (face_id) {
            case 0: { return 2; } break;
            case 1: { return 2; } break;
            case 2: { return 5; } break;
            case 3: { return 2; } break;
            case 4: { return 0; } break;
            case 5: { return 4; } break;
        } } break;
        case DOWN: { switch (face_id) {
            case 0: { return 4; } break;
            case 1: { return 4; } break;
            case 2: { return 0; } break;
            case 3: { return 4; } break;
            case 4: { return 5; } break;
            case 5: { return 2; } break;
        } } break;
        case LEFT: { switch (face_id) {
            case 0: { return 1; } break;
            case 1: { return 5; } break;
            case 2: { return 1; } break;
            case 3: { return 0; } break;
            case 4: { return 1; } break;
            case 5: { return 3; } break;
        } } break;
        case RIGHT: { switch (face_id) {
            case 0: { return 3; } break;
            case 1: { return 0; } break;
            case 2: { return 3; } break;
            case 3: { return 5; } break;
            case 4: { return 3; } break;
            case 5: { return 1; } break;
        } } break;
        case NONE: { ASSERT(0, "Verify direction before calling"); } break;
    }
    ASSERT(0, "Unreachable");
}

