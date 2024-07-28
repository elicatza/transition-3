#include <raylib.h>
#include <stdio.h>
#include <math.h>

#include "core.h"
#include "../assets/atlas.h"
#include "../assets/world_atlas.h"
#define CASE_IMPLEMENTATION
#include "case.h"
#define NO_TEMPLATE
#include "puzzle.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif


#define ASPECT_RATIO (4.f / 3.f)
#define WIDTH 800
#define HEIGHT (WIDTH / ASPECT_RATIO)

/**
 * New format
 * 0bxxxx0000: physical {type1[empty, ground, blinds, bed, door, puzzle1, puzzle2, window](3), height(1)}
 * 4 bits for type: [empty, ground, blinds, bed, door, puzzle1, puzzle2, window]
 * 2 bit for height: unwalkable, 1, 2, 3
 * 2 bits reserved for metadata
 *
 * 0b0000xxxx: visual {2 type[empty, light, 2`reserved], 2 color, 4 brightness} (1)
 * 2 bits type: [empty, spawn, `reserved`, `reserved`]
 * 2 bits color: [white, blue, pink, black]
 * 4 bits for brightness: 0-15
 */

/* Visual masks */
#define MASK_PHYSICAL(a) ((a) & 0b11111111)
#define MASK_PHYSICAL_T(a) ((a) & 0b00001111)
#define MASK_HEIGHT(a) ((a) & 0b00110000)
#define MASK_META(a) ((a) & 0b11000000)

/* Visual masks */
#define MASK_VISUAL(a) ((a) & (0b1111111 << 8))
#define MASK_VISUAL_T(a) ((a) & (0b0000011 << 8))
#define MASK_COLOR(a) ((a) & 0b00001100 << 8)
#define MASK_BRIGHTNESS(a) ((a) & 0b11110000 << 8)

enum PhysicalType {
    PEMPTY = 0b0000,   /* Don't render anything */
    PGROUND = 0b0001,  /* Walkable */
    PBLINDS = 0b0010,  /* -Energy +Light */
    PBED = 0b0011,     /* Restart day / spawn */
    PDOOR = 0b0100,    /* Finish when exit / Big puzzle */
    PPUZZLE1 = 0b0101, /* Puzzle for exercise */
    PPUZZLE2 = 0b0110, /* Puzzle for fun */
    PWINDOW = 0b0111,  /* Window for seeing things */
    PBED_END = 0b1000, /* Bed part II */
    PTABLE = 0b1001,   /* You know */
    PBOSS = 0b1010,   /* You know */
    // PWALL = 0b1001,  /* Window for seeing things */
};

enum PHeight {
    UNWALKABLE = 0b00 << 4,
    H1         = 0b01 << 4,
    H2         = 0b10 << 4,
    H3         = 0b11 << 4,
};

enum VisualType {
    VEMPTY  = 0b00 << 8,  /* Don't render anything */
    VSPAWN  = 0b01 << 8,  /* Spawn point */
};

enum VisualColor {
    VWHITE = 0b00 << 10,
    VBLUE = 0b01 << 10,
    VPINK = 0b10 << 10,
    VBLACK = 0b11 << 10,
};

#define VSTRENGTH(a) ((a) << 12)
#define PMETA(a) ((a) <<  6)

#define S (PBED | H2)
#define Se (PBED_END | H2 | VSPAWN)
#define G (PGROUND | H1)
#define D0 (PDOOR | H1 | PMETA(0b00))
#define D1 (PDOOR | H1 | PMETA(0b01)) /* Meta roomid */
#define Db (PBOSS | H1 | PMETA(0b10)) /* Boss door */
#define P1 (PPUZZLE1 | H1 | VPINK | VSTRENGTH(0b1111))
#define P2 (PPUZZLE2 | H1 | VBLUE | VSTRENGTH(0b1111))
#define Ld (PWINDOW | UNWALKABLE | PMETA(0b01) | VWHITE | VSTRENGTH(0b1111)) /* Meta off on */
#define Lb (PWINDOW | UNWALKABLE | PMETA(0b01) | VPINK | VSTRENGTH(0b0011))  /* Meta off on */
#define B (PBLINDS | H1)  /* 0 down 1 up */
#define T (PTABLE | H2)   /* 0 down 1 up */

/**
 * Header:
 * cols, rows
 * Body: world format (len = width * height)
 *
 */
static u16 worlds[2][103] = {
    {
        6, 7,
        0, 0, 0, 0, D1,0,
        G, G, G, G, G, G,
        T, G, G, G, G, G,
        S, Se,G, G, G ,G,
        G, G, G, G, G ,G,
        G, G, G, B, G, G,
        0, 0, Ld,Ld,0, 0,
    },
    {
        6, 9,
        0, 0, 0, 0, 0, 0,
        0, G, G, G, P2,G,
        Db,G, G, G, G, G,
        0, G, G, G, G, G,
        0, P1,G, G, G, G,
        0, G, G, G, G, G,
        0, G, G, G, G, G,
        0, G, G, G, G, G,
        0, G,G,G,D0,G,
    }
};

typedef struct U32x2 {
    u32 x;
    u32 y;
} U32x2;

typedef struct Player {
    U32x2 pos;
    u8 height;
} Player;

typedef struct Cell {
    U32x2 pos;
    u16 info;
    Color color;
} Cell;

typedef struct World {
    Player player;
    size_t cols;
    size_t rows;
    Cell *cell_case;
    U32x2 camera_pos;
    u16 world_id;

    // Following are set at start of render function and in view space
    float cell_width;
    Vector2 wpos;
    Vector2 wdim;
} World;

typedef struct Sleep {
    float init_energy;
    float end_energy;
    float init_time;
    float step_time;
    float end_time;
} Sleep;

typedef struct {
    Texture2D atlas;
    Texture2D world_atlas;
    Puzzle *puzzle_fun;
    Puzzle *puzzle_train;
    size_t puzzle_fun_id;
    size_t puzzle_train_id;
    World *world;
    GameState state;
    PlayerState pstate;
    Sleep sleep;
} GO;

GO go = { 0 };

/* Door 0, 1, 2, 3, spawnpoint */
World *load_world(u16 world_id, u8 spawn);
GameState update_world(World *w, PlayerState *pstate);
void render_world(World *w, Texture2D atlas);
void free_world(World *w);
GameState update_sleep(World **w, Sleep *s, PlayerState *pstate, GameState gs);
void render_sleep(World *w, Sleep s, PlayerState pstate, Texture2D atlas);



void loop(void);

int main(void)
{
    int width = WIDTH;
    int height = HEIGHT;
    SetConfigFlags(/* FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | */ FLAG_MSAA_4X_HINT);
    InitWindow(width, height, "demo");

    Image atlas_img = {
        .data = ATLAS_DATA,
        .width = ATLAS_WIDTH,
        .height = ATLAS_HEIGHT,
        .format = ATLAS_FORMAT,
        .mipmaps = 1,
    };

    Image world_atlas = {
        .data = WORLD_ATLAS_DATA,
        .width = WORLD_ATLAS_WIDTH,
        .height = WORLD_ATLAS_HEIGHT,
        .format = WORLD_ATLAS_FORMAT,
        .mipmaps = 1,
    };

    go.atlas = LoadTextureFromImage(atlas_img);
    go.world_atlas = LoadTextureFromImage(world_atlas);
    go.world = load_world(0, 4);
    go.state = WORLD;
    go.pstate.energy = 0.3f;
    go.pstate.energy_max = 0.3f;
    go.pstate.energy_lim = 1.0f;
    go.pstate.pain = 0.2f;
    go.pstate.pain_max = 1.f;
    go.pstate.time = 0.25; /* 06:00 */
    go.pstate.did_faint = false;
    go.puzzle_fun_id = 0;
    go.puzzle_train_id = 0;
    go.puzzle_fun = load_puzzle(puzzle_fun_array[go.puzzle_fun_id]);
    go.puzzle_train = load_puzzle(puzzle_train_array[go.puzzle_train_id]);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(loop, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        loop();
    }
#endif

    UnloadTexture(go.atlas);
    free_puzzle(go.puzzle_fun);
    free_puzzle(go.puzzle_train);
    free_world(go.world);
    CloseWindow();
    return 0;
}


void loop(void)
{
    switch (go.state) {
        case PUZZLE_FUN: { go.state = update_puzzle(go.puzzle_fun, &go.pstate, PUZZLE_FUN); } break;
        case PUZZLE_FUN_WIN: {
            go.state = update_puzzle_win(go.puzzle_fun, PUZZLE_FUN_WIN);
            if (go.state == WORLD) {
                go.puzzle_fun_id += 1;
                if (go.puzzle_fun_id >= sizeof puzzle_fun_array / sizeof *puzzle_fun_array) {
                    WARNING("Out of fun puzzles");
                    break;
                }
                Puzzle *p = load_puzzle(puzzle_fun_array[go.puzzle_fun_id]);
                free_puzzle(go.puzzle_fun);
                go.puzzle_fun = p;
            }
        } break;
        case PUZZLE_TRAIN: { go.state = update_puzzle(go.puzzle_train, &go.pstate, PUZZLE_TRAIN); } break;
        case PUZZLE_TRAIN_WIN: {
            go.state = update_puzzle_win(go.puzzle_train, PUZZLE_TRAIN_WIN);
            if (go.state == WORLD) {
                go.puzzle_train_id += 1;
                if (go.puzzle_train_id >= sizeof puzzle_train_array / sizeof *puzzle_train_array) {
                    WARNING("Out of exercise puzzles");
                    break;
                }
                Puzzle *p = load_puzzle(puzzle_train_array[go.puzzle_train_id]);
                free_puzzle(go.puzzle_train);
                go.puzzle_train = p;
            }
        } break;
        case WORLD: { go.state = update_world(go.world, &go.pstate); } break;
        case SLEEP: { go.state = update_sleep(&go.world, &go.sleep, &go.pstate, SLEEP); } break;
        case FAINT: { go.state = update_sleep(&go.world, &go.sleep, &go.pstate, FAINT); } break;
        default: { ASSERT(0, "Not implemented") } break;
    }

    BeginDrawing();

    ClearBackground(BLACK);
    switch (go.state) {
        case PUZZLE_FUN: { render_puzzle(go.puzzle_fun, go.pstate, go.atlas); } break;
        case PUZZLE_FUN_WIN: { render_puzzle_win(go.puzzle_fun, &go.pstate, go.atlas); } break;
        case PUZZLE_TRAIN_WIN: { render_puzzle_win(go.puzzle_train, &go.pstate, go.atlas); } break;
        case PUZZLE_TRAIN: { render_puzzle(go.puzzle_train, go.pstate, go.atlas); } break;
        case WORLD: { render_world(go.world, go.world_atlas); } break;
        case SLEEP: { render_sleep(go.world, go.sleep, go.pstate, go.world_atlas); } break;
        case FAINT: { render_sleep(go.world, go.sleep, go.pstate, go.atlas); } break;
        default: { ASSERT(0, "Not implemented"); } break;
    }

    EndDrawing();
}

Cell cell_at_pos(World *w, U32x2 pos)
{
    return w->cell_case[pos.y * w->cols + pos.x];
}

u8 get_height_at_pos(World *w, U32x2 pos)
{
    return MASK_HEIGHT(w->cell_case[pos.y * w->cols + pos.x].info) >> 4;
}

u8 get_brightness_at_pos(World *w, U32x2 pos)
{
    return MASK_BRIGHTNESS(w->cell_case[pos.y * w->cols + pos.x].info) >> 12;
}

u8 get_meta_at_pos(World *w, U32x2 pos)
{
    return MASK_META(w->cell_case[pos.y * w->cols + pos.x].info) >> 6;
}

enum PhysicalType get_type_at_pos(World *w, U32x2 pos)
{
    return MASK_PHYSICAL_T(w->cell_case[pos.y * w->cols + pos.x].info);
}


Color get_color_at_pos(World *w, U32x2 pos)
{
    enum VisualColor color = MASK_COLOR(w->cell_case[pos.y * w->cols + pos.x].info);
    switch (color) {
        case VWHITE: { return WHITE; } break;
        case VBLUE: { return C_BLUE; } break;
        case VPINK: { return C_PINK; } break;
        case VBLACK: { return BLACK; } break;
        default: {
            ASSERT(0, "Unreachable");
        } break;
    };
}

Color get_color_at_i(World *w, size_t i)
{
    enum VisualColor color = MASK_COLOR(w->cell_case[i].info);
    switch (color) {
        case VWHITE: { return WHITE; } break;
        case VBLUE: { return C_BLUE; } break;
        case VPINK: { return C_PINK; } break;
        case VBLACK: { return BLACK; } break;
        default: {
            ASSERT(0, "Unreachable");
        } break;
    };
}


bool is_valid_wspos(World *w, U32x2 pos, u32 height)
{
    if (pos.x >= w->cols) {
        return false;
    }
    if (pos.y >= w->rows) {
        return false;
    }
    u8 cell_height = get_height_at_pos(w, pos);
    if (cell_height == UNWALKABLE) {
        return false;
    }
    if (cell_height > height) {
        return false;
    }

    return true;
}

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

Color apply_shade(Color c, float factor)
{
    return (Color) {
        .a = c.a,
        .r = c.r * factor,
        .g = c.g * factor,
        .b = c.b * factor,
    };
}

Color apply_tint(Color c, u32 lightness)
{
    float factor = 0.5 + (lightness / 30.f);
    return (Color) {
        .r = c.r + ((255 - c.r) * factor),
        .g = c.g + ((255 - c.g) * factor),
        .b = c.b + ((255 - c.b) * factor),
        .a = c.a,
    };
}


void apply_lighting(World *w)
{
    size_t col, row;
    for (row = 0; row < w->rows; ++row) {
        for (col = 0; col < w->cols; ++col) {
            U32x2 pos = { col, row };

            u8 b = get_brightness_at_pos(w, pos);
            if (b == 0) continue;

            u8 off = get_meta_at_pos(w, pos);
            if (off) continue;

            Color color = get_color_at_pos(w, pos);
            int nx, ny;
            for (nx = 0; nx < (int) w->cols; ++nx) {
                for (ny = 0; ny < (int) w->rows; ++ny) {
                    if (nx == (int) col && ny == (int) row) continue;
                    // inverse square law light
                    float val = 1.f / powf(abs((int) col - nx) + abs((int) row - ny), 2.f);
                    size_t i = ny * w->cols + nx;
                    w->cell_case[i].color = blend(w->cell_case[i].color, color, val * b);
                }
            }
        }
    }
}

void toggle_blinds(World *w, U32x2 pos) {
    // Loop over windows in 3x3 area. Disable them.
    // Factor out to toggle blinds function
    int x, y;
    for (x = -1; x <= 1; ++x) {
        for (y = -1; y <= 1; ++y) {
            if (x == 0 && y == 0) continue;
            int newx = pos.x + x;
            int newy = pos.y + y;
            if (newx < 0 || newx >= (int) w->cols) continue;
            if (newy < 0 || newy >= (int) w->rows) continue;
            Cell *xycell = &w->cell_case[newy * w->cols + newx];
            if ((enum PhysicalType) MASK_PHYSICAL_T(xycell->info) == PWINDOW) {
                INFO("Found valid lightsource %d", MASK_PHYSICAL_T(xycell->info));
                xycell->info ^= 0b01000000;
            }
        }
    }
}

Sleep init_sleep(PlayerState pstate)
{
    Sleep rv;
    float sleep_time = (3.f / 8.f) * (1.f - (pstate.energy / pstate.energy_max));
    rv.init_time = pstate.time;
    rv.step_time = 0.05f / ((3.f / 8.f) / sleep_time);
    rv.end_time = pstate.time + sleep_time;
    rv.init_energy = pstate.energy;
    rv.end_energy = ((3.f / 8.f) / sleep_time) * 0.8f * pstate.energy_max; /* TODO Factor in lightness (0.8f) */
    return rv;
}

Sleep init_faint(PlayerState pstate)
{
    Sleep rv;
    float sleep_time = (3.f / 8.f) * (1.f - (pstate.energy / pstate.energy_max));
    rv.init_time = pstate.time;
    rv.step_time = 0.05f / ((3.f / 8.f) / sleep_time);
    rv.end_time = pstate.time + sleep_time;
    rv.init_energy = pstate.energy;
    rv.end_energy = ((3.f / 8.f) / sleep_time) * 0.5f * pstate.energy_max; /* TODO Factor in lightness (0.8f) */
    return rv;
}

void spawn_player(World *w, u8 spawnid)
{
    size_t i;
    for (i = 0; i < case_len(w->cell_case); ++i) {
        Cell c = w->cell_case[i];
        if (spawnid == 4) {
            u16 vtype = MASK_VISUAL_T(c.info);
            switch ((enum VisualType) vtype) {
                case VEMPTY: break;
                case VSPAWN: {
                    Player p;
                    p.pos = (U32x2) { i % w->cols, i / w->cols };
                    p.height = MASK_HEIGHT(c.info) >> 4;
                    w->player = p;
                    break;
                } break;
            }
        } else {
            if (MASK_PHYSICAL_T(c.info) == PDOOR) {
                u16 meta = MASK_META(c.info) >> 6;
                INFO("Door meta %d", meta);
                if (meta == spawnid) {
                    Player p;
                    p.pos = (U32x2) { i % w->cols, i / w->cols };
                    p.height = MASK_HEIGHT(c.info) >> 4;
                    w->player = p;
                    break;
                }
            }
        }
    }
}

GameState update_sleep(World **w, Sleep *s, PlayerState *pstate, GameState gs)
{
    if (gs == FAINT) {
        free_world(*w);
        *w = load_world(0, 4);
        spawn_player(*w, 4);
        *s = init_faint(*pstate);
        pstate->did_faint = true;
        return SLEEP;
    }

    pstate->time += s->step_time * GetFrameTime();
    if (pstate->time >= s->end_time) {
        pstate->energy = s->end_energy;
        pstate->did_faint = false;
        // pstate->time = cur_time;
        return WORLD;
    }
    return SLEEP;
}

void render_sleep(World *w, Sleep s, PlayerState pstate, Texture2D atlas)
{
    (void) s;
    render_world(w, atlas);

    Color bg = BLACK;
    bg.a = 128;
    Rectangle border = {
        .x = w->wpos.x + 0.2f * w->wdim.x,
        .y = w->wpos.y + 0.2f * w->wdim.y,
        .width = (1.f - 0.4f) * w->wdim.x,
        .height = (1.f - 0.4f) * w->wdim.y,
    };
    DrawRectangleRec(border, bg);
    DrawRectangleLinesEx(border, 5.f, BLACK);

    char *msg = pstate.did_faint ? "Fainted" : "Sleeping";
    Vector2 sz = MeasureTextEx(GetFontDefault(), msg, w->wdim.y / 10.f, 4.f);
    Vector2 pos = {
        .x = w->wpos.x + 0.5f * (w->wdim.x  - sz.x),
        .y = w->wpos.y + 0.3f * (w->wdim.y - sz.y),
    };
    DrawTextEx(GetFontDefault(), msg, pos, w->wdim.x / 10.f, 4.f, WHITE);


    char time[6];
    format_time(pstate, time, sizeof time);
    Vector2 tsz = MeasureTextEx(GetFontDefault(), time, w->wdim.y / 10.f, 4.f);
    Vector2 tpos = {
        .x = w->wpos.x + 0.5f * (w->wdim.x  - tsz.x),
        .y = w->wpos.y + 0.3f * (w->wdim.y - tsz.y) + sz.y,
    };
    DrawTextEx(GetFontDefault(), time, tpos, w->wdim.x / 10.f, 4.f, WHITE);
    

    char *instructions = "[enter]";
    Vector2 isz = MeasureTextEx(GetFontDefault(), instructions, w->wdim.y / 20.f, 2.f);
    Vector2 ipos = {
        .x = w->wpos.x + 0.5f * (w->wdim.x  - isz.x),
        .y = w->wpos.y + 0.3f * (w->wdim.y - isz.y) + sz.y + tsz.y,
    };
    DrawTextEx(GetFontDefault(), instructions, ipos, w->wdim.y / 20.f, 4.f, WHITE);
}

GameState update_world(World *w, PlayerState *pstate)
{
    if (IsKeyPressed(KEY_T)) {
        pstate->time += 0.002f;
    }
    Player new_p = w->player;
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
        new_p.pos.y += -1;
    } else if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) {
        new_p.pos.x += -1;
    } else if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
        new_p.pos.y += 1;
    } else if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) {
        new_p.pos.x += 1;
    }

    if (memcmp(&new_p, &w->player.pos, sizeof new_p) != 0) {
        bool penalty = false;
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT) || IsKeyDown(KEY_U)) {
            penalty = true;
            new_p.height += 1;
        }
        if (is_valid_wspos(w, new_p.pos, new_p.height)) {
            if (penalty) {
                pstate->energy -= PENALTY_ENERGY;
                if (pstate->energy < 0) {
                    pstate->energy = 0.f;
                    return FAINT;
                }
            }
            w->player = new_p;
            w->player.height = get_height_at_pos(w, new_p.pos);
        }
    }

    if (IsKeyPressed(KEY_I) || IsKeyPressed(KEY_ENTER)) {
        // Interact
        Cell cell = cell_at_pos(w, w->player.pos);
        switch ((enum PhysicalType) MASK_PHYSICAL_T(cell.info)) {
            case PGROUND: break;
            case PEMPTY: break;
            case PBOSS: {
                ASSERT(0, "Not implemented");
            } break;
            case PWINDOW: break;
            case PTABLE: break;
            case PBLINDS: { toggle_blinds(w, cell.pos); } break;
            case PDOOR: {
                int meta = MASK_META(cell.info) >> 6;
                INFO("Door %d", meta);
                World *oldw = w;
                w = load_world(meta, w->world_id);
                go.world = w;
                free_world(oldw);
                return WORLD;
            } break;
            case PPUZZLE1: {
                INFO("Puzzle 1");
                return PUZZLE_FUN;
            } break;
            case PPUZZLE2: {
                go.sleep = init_sleep(*pstate);
                return PUZZLE_TRAIN;
            } break;
            case PBED: {
                go.sleep = init_sleep(*pstate);
                return SLEEP;
            } break;
            case PBED_END: {
                go.sleep = init_sleep(*pstate);
                return SLEEP;
            } break;
            // default: {
            //     ASSERT(0, "Not implemented");
            // } break;
        }
    }

    size_t i;
    for (i = 0; i < case_len(w->cell_case); ++i) {
        w->cell_case[i].color = BLACK;
    }
    apply_lighting(w);
    return WORLD;
}

Vector2 vspos_of_ws(World *w, U32x2 ws)
{
    Vector2 vs;
    vs.x = w->wpos.x + ws.x * w->cell_width;
    vs.y = w->wpos.y + ws.y * w->cell_width;
    return vs;
}

void render_world_cells(World *w, Texture2D atlas)
{
    (void) atlas;
    size_t i;
    for (i = 0; i < case_len(w->cell_case); ++i) {
        Cell cell = w->cell_case[i];
        Vector2 vspos = vspos_of_ws(w, cell.pos);

        Rectangle src = {
            .x = 0,
            .y = 0,
            .width = 16.f,
            .height = 16.f,
        };
        Rectangle dest = {
            .x = vspos.x,
            .y = vspos.y,
            .width = w->cell_width,
            .height = w->cell_width,
        };
        Vector2 center = { 0.f, 0.f };

        Color color = PURPLE;
        float rotation = 0.f;
        switch ((enum PhysicalType) MASK_PHYSICAL_T(cell.info)) {
            case (PEMPTY): { color = BLACK; } break;
            case (PGROUND): { color = WHITE; src.x = 0.f; src.y = 0.f; } break;
            case (PBLINDS): { color = WHITE; src.x = 6.f * 16.f; src.y = 0.f; } break;
            case (PWINDOW): { color = WHITE; src.x = 48.f; src.y = 0.f; } break;
            case (PBED): { color = WHITE; src.x = 16.f; src.y = 0.f; } break;
            case (PBED_END): { color = WHITE; src.x = 32.f; src.y = 0.f; } break;
            case (PDOOR): { color = WHITE; src.x = 4.f * 16.f, src.y = 0.f; } break;
            case (PPUZZLE1): { color = YELLOW; } break;
            case (PPUZZLE2): { color = VIOLET; } break;
            case (PTABLE): { color = WHITE; src.x = 5.f * 16.f; src.y = 0.f; } break;
            case (PBOSS): { color = WHITE; src.x = 7.f * 16.f; src.y = 0.f; } break;
        }

        color = apply_shade(color, 0.4f);
        color = blend(color, cell.color, 0.4);
        DrawTexturePro(atlas, src, dest, center, rotation, color); // Draw a part of a texture defined by a rectangle with 'pro' parameters
        // cell.lighting = 0.5 + (lightness / 30.f);
        // color = blend(color, C_BLUE, cell.lighting + 5);
        // color = apply_tint(color, cell.lighting);
        // color = lerp(color, (Color) { 0, 0, 0, 255 }, 0.5 + (cell.lighting / 30.f));


        // DrawRectangleV(vspos, dim, color);
    }
}

void render_world_player(World *w)
{
    Vector2 vs_pos = vspos_of_ws(w, w->player.pos);
    DrawRectangleV(vs_pos, (Vector2) { w->cell_width, w->cell_width }, PINK);
}

void render_world_height_lines(World *w)
{
    size_t row, col;
    float cell_width = w->wdim.x / w->cols;
    for (row = 0; row < w->rows; ++row) {
        for (col = 0; col < w->cols; ++col) {
            Cell c = w->cell_case[row * w->cols + col];
            u8 c_height = get_height_at_pos(w, (U32x2) { col, row });

            if (case_len(w->cell_case) > (row + 1) * w->cols + col) {
                // Has cell down
                Cell cd = w->cell_case[(row + 1) * w->cols + col];
                u8 cd_height = get_height_at_pos(w, (U32x2) { col, row + 1 });
                if (cd.pos.x != c.pos.x) continue;
                if (c_height != cd_height) {
                    Vector2 start = vspos_of_ws(w, cd.pos);
                    Vector2 end = { start.x + cell_width, start.y};
                    float diff = fabsf((float) c_height - cd_height);
                    DrawLineEx(start, end, diff * 2.f, BLACK);
                }
            }

            if (case_len(w->cell_case) > row * w->cols + col + 1) {
                // Has cell Right
                Cell cr = w->cell_case[row * w->cols + col + 1];
                u8 cr_height = get_height_at_pos(w, (U32x2) { col + 1, row });
                if (cr.pos.y != c.pos.y) continue;
                if (c_height != cr_height) {
                    Vector2 start = vspos_of_ws(w, cr.pos);
                    Vector2 end = { start.x, start.y + cell_width};
                    float diff = fabsf((float) c_height - cr_height);
                    DrawLineEx(start, end, diff * 2.f, BLACK);
                }
            }

        }
    }
}


void render_world(World *w, Texture2D atlas)
{
    float width = GetScreenWidth();
    float height = GetScreenHeight();

    w->cell_width = MIN(width / w->cols, height / w->rows);
    w->wpos.x = (width - (w->cell_width * w->cols)) / 2.f;
    w->wpos.y = (height - (w->cell_width * w->rows)) / 2.f;
    w->wdim.x = w->cell_width * w->cols;
    w->wdim.y = w->cell_width * w->rows;

    render_world_cells(w, atlas);
    render_world_player(w);
    render_world_height_lines(w);
    render_hud_rhs(go.pstate, w->wpos.x + w->wdim.x, atlas);
    render_hud_lhs(go.pstate, w->wpos.x + w->wdim.x, atlas);
    // DrawRectangleLinesEx((Rectangle) { w->wpos.x, w->wpos.y, w->wdim.x, w->wdim.y }, 2.f, RED);
}

void fill_world(World *w, u16 *wbody)
{
    size_t row, col;
    for (row = 0; row < w->rows; ++row) {
        for (col = 0; col < w->cols; ++col) {
            u16 info = wbody[row * w->cols + col];
            Cell cell;
            cell.pos = (U32x2) { col, row };
            cell.info = info;
            cell.color = BLACK;
            case_push(w->cell_case, cell);
        }
    }
}


World *load_world(u16 world_id, u8 spawn)
{
    u16 *wmap = worlds[world_id];
    World *w = malloc(sizeof *w);
    size_t cols = *wmap; ++wmap;
    size_t rows = *wmap; ++wmap;
    INFO("new dims %zu, %zu", cols, rows);

    w->world_id = world_id;
    w->cols = cols;
    w->rows = rows;
    w->cell_case = case_init(cols * rows, sizeof *w->cell_case);

    fill_world(w, wmap);
    INFO("Spawnid %d", spawn);
    spawn_player(w, spawn);
    INFO("Player pos %d, %d", w->player.pos.x, w->player.pos.y);
    return w;
}

void free_world(World *w)
{
    free(w);
}

