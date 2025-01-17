#include <raylib.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <math.h>

#include "core.h"
#include "../assets/atlas.h"
#include "../assets/world_atlas.h"
#include "../assets/player_atlas.h"
#define CASE_IMPLEMENTATION
#include "case.h"
#define NO_TEMPLATE
#include "puzzle.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            100
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif


#define ASPECT_RATIO (4.f / 3.f)
#define WIDTH 800
#define HEIGHT (WIDTH / ASPECT_RATIO)

#define TIME_LIGHT_MAX 0.1f
#define TIME_LIGHT_MIN 0.02f

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
    PEMPTY = 0b0000,    /* Don't render anything */
    PGROUND = 0b0001,   /* Walkable */
    PBLINDS = 0b0010,   /* -Energy +Light */
    PBED = 0b0011,      /* Restart day / spawn */
    PDOOR = 0b0100,     /* Finish when exit / Big puzzle */
    PPUZZLE1 = 0b0101,  /* Puzzle for exercise */
    PPUZZLE2 = 0b0110,  /* Puzzle for fun */
    PWINDOW = 0b0111,   /* Window for seeing things */
    PBED_END = 0b1000,  /* Bed part II */
    PTABLE = 0b1001,    /* You know */
    PBOSS = 0b1010,     /* You know */
    PTABLE_TL = 0b1011, /* Main table */
    PTABLE_BL = 0b1100, /* Main table */
    PTABLE_TR = 0b1101, /* Main table */
    PTABLE_BR = 0b1110, /* Main table */
    // PWALL = 0b1001, /* Window for seeing things */
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
#define P1 (PPUZZLE1 | H2 | VPINK | VSTRENGTH(0b1111))
#define P2 (PPUZZLE2 | H1 | VBLUE | VSTRENGTH(0b1111))
#define Ld (PWINDOW | UNWALKABLE | PMETA(0b01) | VWHITE | VSTRENGTH(0b1111)) /* Meta off on */
#define Lb (PWINDOW | UNWALKABLE | PMETA(0b01) | VPINK | VSTRENGTH(0b0011))  /* Meta off on */
#define B (PBLINDS | H1)
#define T (PTABLE | H3)
#define Tl (PTABLE_TL | H3)
#define Tr (PTABLE_TR | H3)
#define Bl (PTABLE_BL | H3)
#define Br (PTABLE_BR | H3)

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
        0, G, G, Tl,Tr,G,
        Db,G, G, Bl,Br,G,
        0, G, G, G, P1,G,
        0, P2,G, G, G, G,
        0, G, G, G, G, G,
        0, G, G, G, G, G,
        0, G, G, G, G, G,
        0, 0, 0, 0,D0, 0,
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
    float end_time;
    float init_pain;
    float end_pain;
} Sleep;

typedef struct {
    size_t frame;
    Texture2D atlas;
    Texture2D world_atlas;
    Texture2D player_atlas;
    Puzzle *puzzle_fun;
    Puzzle *puzzle_train;
    Puzzle *puzzle_boss;
    size_t puzzle_fun_id;
    size_t puzzle_train_id;
    World *world;
    GameState state;
    PlayerState pstate;
    Sleep sleep;
    bool blinds_down;
    Shader puzzle_shader;
} GO;

GO go = { 0 };

static char *vs =
	"#version 100\n"
	"\n"
	"// Input vertex attributes\n"
	"attribute vec3 vertexPosition;\n"
	"attribute vec2 vertexTexCoord;\n"
	"attribute vec3 vertexNormal;\n"
	"attribute vec4 vertexColor;\n"
	"\n"
	"// Input uniform values\n"
	"uniform mat4 mvp;\n"
	"\n"
	"// Output vertex attributes (to fragment shader)\n"
	"varying vec2 fragTexCoord;\n"
	"varying vec4 fragColor;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    // Send vertex attributes to fragment shader\n"
	"    fragTexCoord = vertexTexCoord;\n"
	"    fragColor = vertexColor;\n"
	"\n"
	"    // Calculate final vertex position\n"
	"    gl_Position = mvp*vec4(vertexPosition, 1.0);\n"
	"}\n"
	"\n";

static char *fs =
	"#version 100\n"
	"\n"
	"\n"
	"precision mediump float;\n"
	"\n"
	"varying vec2 fragTexCoord;\n"
	"varying vec4 fragColor;\n"
	"\n"
	"\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
	"uniform vec2 center;\n"
	"uniform float radius;\n"
	"\n"
	"void main()\n"
	"{\n"
    "    vec4 texelColor = texture2D(texture0, fragTexCoord);\n"
    "    float lhs = pow(center.x - gl_FragCoord.x, 2.0) + pow(center.y - gl_FragCoord.y, 2.0);\n"
    "    float rhs = pow(radius, 2.0);\n"
	"    if (lhs < rhs)\n"
	"    {\n"
	"        gl_FragColor = mix(texelColor, vec4(0.0, 0.0, 0.0, 1.0), lhs / rhs);\n"
	"    }\n"
	"    else\n"
	"    {\n"
	"        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
	"    }\n"
	"}\n";


/* Door 0, 1, 2, 3, spawnpoint */
World *load_world(u16 world_id, u8 spawn);
GameState update_world(World *w, PlayerState *pstate);
void render_world(World *w, PlayerState pstate, Texture2D atlas, Texture2D player_atlas);
void free_world(World *w);
GameState update_sleep(World **w, Sleep *s, PlayerState *pstate, GameState gs);
void render_sleep(World *w, Sleep s, PlayerState pstate, Texture2D atlas, Texture2D player_atlas);

GameState update_victory();
void render_victory(World *w, PlayerState pstate, Texture2D atlas, Texture2D player_atlas);

void render_menu(void);
GameState update_menu(void);



void loop(void);

int main(void)
{
    int width = WIDTH;
    int height = HEIGHT;
    SetConfigFlags(/* FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | */ FLAG_MSAA_4X_HINT);
    InitWindow(width, height, "Transition #3");

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

    Image player_atlas = {
        .data = PLAYER_ATLAS_DATA,
        .width = PLAYER_ATLAS_WIDTH,
        .height = PLAYER_ATLAS_HEIGHT,
        .format = PLAYER_ATLAS_FORMAT,
        .mipmaps = 1,
    };

    go.frame = 0;
    go.atlas = LoadTextureFromImage(atlas_img);
    go.world_atlas = LoadTextureFromImage(world_atlas);
    go.player_atlas = LoadTextureFromImage(player_atlas);
    go.world = load_world(0, 4);
    go.state = MENU;
    go.pstate.energy = ENERGY_MAX_INIT;  // orig 0.3f
    go.pstate.energy_max = ENERGY_MAX_INIT;
    go.pstate.energy_lim = ENERGY_MAX_LIM;
    go.pstate.pain = 0.2f;
    go.pstate.pain_max = 1.f;
    go.pstate.time = 0.3334; /* 08:00 */
    go.pstate.did_faint = false;
    go.pstate.face_id = 0;
    go.pstate.is_sleeping = 0;
    go.pstate.light = LIGHT_INIT;  // orig 0.25f
    go.puzzle_fun_id = 0;
    go.puzzle_train_id = 0;
    go.puzzle_fun = load_puzzle(puzzle_fun_array[go.puzzle_fun_id]);
    go.puzzle_train = load_puzzle(puzzle_train_array[go.puzzle_train_id]);
    go.puzzle_boss = load_puzzle(puzzle_boss);
    go.blinds_down = true;
    go.puzzle_shader = LoadShaderFromMemory(vs, fs);


    SetExitKey(0);
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
    UnloadShader(go.puzzle_shader);
    free_puzzle(go.puzzle_fun);
    free_puzzle(go.puzzle_train);
    free_world(go.world);
    CloseWindow();
    return 0;
}


void loop(void)
{
    go.frame += 1;
    switch (go.state) {
        case MENU: { go.state = update_menu(); } break;
        case PUZZLE_FUN: { go.state = update_puzzle(go.puzzle_fun, &go.pstate, PUZZLE_FUN); } break;
        case PUZZLE_FUN_WIN: {
            go.state = update_puzzle_win(go.puzzle_fun, &go.pstate, PUZZLE_FUN_WIN);
            if (go.state == WORLD) {
                go.puzzle_fun_id += 1;
                if (go.puzzle_fun_id >= sizeof puzzle_fun_array / sizeof *puzzle_fun_array) {
                    // TODO give user feedback
                    WARNING("Out of puzzles. I think you are ready now");
                    break;
                }
                Puzzle *p = load_puzzle(puzzle_fun_array[go.puzzle_fun_id]);
                free_puzzle(go.puzzle_fun);
                go.puzzle_fun = p;
            }
        } break;
        case PUZZLE_TRAIN: { go.state = update_puzzle(go.puzzle_train, &go.pstate, PUZZLE_TRAIN); } break;
        case PUZZLE_TRAIN_WIN: {
            go.state = update_puzzle_win(go.puzzle_train, &go.pstate, PUZZLE_TRAIN_WIN);
            if (go.state == WORLD) {
                go.puzzle_train_id += 1;
                if (go.puzzle_train_id >= sizeof puzzle_train_array / sizeof *puzzle_train_array) {
                    // TODO give user feedback
                    WARNING("Out of puzzles. I think you are ready now");
                    break;
                }
                Puzzle *p = load_puzzle(puzzle_train_array[go.puzzle_train_id]);
                free_puzzle(go.puzzle_train);
                go.puzzle_train = p;
            }
        } break;
        case PUZZLE_BOSS: { go.state = update_puzzle(go.puzzle_boss, &go.pstate, PUZZLE_BOSS); } break;
        case PUZZLE_BOSS_WIN: { go.state = update_victory(); } break;
        case WORLD: { go.state = update_world(go.world, &go.pstate); } break;
        case SLEEP: { go.state = update_sleep(&go.world, &go.sleep, &go.pstate, SLEEP); } break;
        case FAINT: { go.state = update_sleep(&go.world, &go.sleep, &go.pstate, FAINT); } break;
    }

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_Q)) {
        go.state = MENU;
    }

#ifdef DEBUG
    if (IsKeyPressed(KEY_C)) {
        char *screenshot_buf;
        asprintf(&screenshot_buf, "screenshot_%zu.png", go.frame);
        TakeScreenshot(screenshot_buf);
        free(screenshot_buf);
        INFO("Screen capture `%s` taken", screenshot_buf);
    }
#endif


    BeginDrawing();

    ClearBackground(BLACK);
    switch (go.state) {
        case MENU: { render_menu(); } break;
        case PUZZLE_FUN: { render_puzzle(go.puzzle_fun, go.pstate, go.atlas, go.player_atlas, go.puzzle_shader); } break;
        case PUZZLE_FUN_WIN: { render_puzzle_win(go.puzzle_fun, &go.pstate, go.atlas, go.player_atlas, go.puzzle_shader); } break;
        case PUZZLE_TRAIN_WIN: { render_puzzle_win(go.puzzle_train, &go.pstate, go.atlas, go.player_atlas, go.puzzle_shader); } break;
        case PUZZLE_TRAIN: { render_puzzle(go.puzzle_train, go.pstate, go.atlas, go.player_atlas, go.puzzle_shader); } break;
        case PUZZLE_BOSS_WIN: { render_victory(go.world, go.pstate, go.atlas, go.player_atlas); } break;
        case PUZZLE_BOSS: { render_puzzle(go.puzzle_boss, go.pstate, go.atlas, go.player_atlas, go.puzzle_shader); } break;
        case WORLD: { render_world(go.world, go.pstate, go.world_atlas, go.player_atlas); } break;
        case SLEEP: { render_sleep(go.world, go.sleep, go.pstate, go.world_atlas, go.player_atlas); } break;
        case FAINT: { render_sleep(go.world, go.sleep, go.pstate, go.atlas, go.player_atlas); } break;
    }

    EndDrawing();
}

float light_from_time(PlayerState pstate)
{
    float day_time = (pstate.time - floorf(pstate.time)) * PI * 2.f;
    return MAX(sinf(day_time - PI / 2.f), TIME_LIGHT_MIN) * TIME_LIGHT_MAX;  /* Light from 6 18 */
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
    return w->cell_case[pos.y * w->cols + pos.x].color;
}

Color get_colorinfo_at_pos(World *w, U32x2 pos)
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


void apply_lighting(World *w, PlayerState pstate)
{
    size_t col, row;
    for (row = 0; row < w->rows; ++row) {
        for (col = 0; col < w->cols; ++col) {
            U32x2 pos = { col, row };

            float b = get_brightness_at_pos(w, pos);
            if (b == 0) continue;
            if (get_type_at_pos(w, pos) == PWINDOW) {
                b *= light_from_time(pstate) / TIME_LIGHT_MAX;
            }

            if (go.blinds_down && get_type_at_pos(w, pos) == PWINDOW) continue;

            Color color = get_colorinfo_at_pos(w, pos);
            int nx, ny;
            for (nx = 0; nx < (int) w->cols; ++nx) {
                for (ny = 0; ny < (int) w->rows; ++ny) {
                    float val = 1.f / powf(abs((int) col - nx) + abs((int) row - ny), 2.f);
                    if (nx == (int) col && ny == (int) row) {
                        val = 1.f;
                    }
                    // inverse square law light
                    size_t i = ny * w->cols + nx;
                    w->cell_case[i].color = blend(w->cell_case[i].color, color, val * b);
                }
            }
        }
    }
}

Sleep init_sleep(PlayerState *pstate)
{
    if (pstate->energy < 0.f) pstate->energy = 0.0f;
    if (pstate->energy >= pstate->energy_max) pstate->energy = pstate->energy_max - 0.001;
    float stime = (15.f / (1 + powf(M_E, -2 * (1.f - (pstate->energy / pstate->energy_max)))) - 5.f) / 24.f;
    float stime_max = (15.f / (1 + powf(M_E, -2)) - 5.f) / 24.f;
    pstate->is_sleeping = true;
    INFO("Hours: %.2f", stime * 24.f);
    INFO("Percent: %.2f", (stime / stime_max) * 100.f);

    Sleep rv;
    rv.init_time = pstate->time;
    rv.end_time = pstate->time + stime;
    rv.init_energy = pstate->energy;
    rv.end_energy = MIN(pstate->energy * 0.3 + pstate->energy_max * (stime / stime_max) * 0.8f, pstate->energy_max); /* TODO Factor in lightness (0.8f) */
    rv.init_pain = pstate->pain;
    rv.end_pain = pstate->pain * 0.333f;
    return rv;
}

Sleep init_faint(PlayerState pstate)
{
    if (pstate.energy < 0.f) pstate.energy = 0.0f;
    if (pstate.energy >= pstate.energy_max) pstate.energy = pstate.energy_max - 0.001;
    float stime = (15.f / (1 + powf(M_E, -2 * (1.f - (pstate.energy / pstate.energy_max)))) - 5.f) / 24.f;
    float stime_max = (15.f / (1 + powf(M_E, -2)) - 6.f) / 24.f;
    Sleep rv;
    rv.init_time = pstate.time;
    rv.end_time = pstate.time + stime;
    rv.init_energy = pstate.energy;
    rv.end_energy = MIN(pstate.energy_max * (stime / stime_max) * 0.5f, pstate.energy_max); /* TODO Factor in lightness (0.8f) */
    rv.init_pain = pstate.pain;
    rv.end_pain = pstate.pain * 0.666f;
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
        if (pstate->light > 0.05) {
            pstate->light -= 0.05f;
        }
        return SLEEP;
    }

    update_world(*w, pstate);
    pstate->time += SLEEP_SPEED * GetFrameTime();
    if (pstate->time >= s->end_time) {
        pstate->energy = s->end_energy;
        pstate->did_faint = false;
        pstate->is_sleeping = false;
        // pstate->time = cur_time;
        return WORLD;
    }

    // Good math (first try)
    pstate->pain = s->init_pain + (s->end_pain - s->init_pain) * ((pstate->time - s->init_time)  / (s->end_time - s->init_time));
    pstate->energy = s->init_energy + (s->end_energy - s->init_energy) * ((pstate->time - s->init_time)  / (s->end_time - s->init_time));
    return SLEEP;
}

void render_menu(void)
{
    float width = GetScreenWidth();
    float height = GetScreenHeight();

    char *msg = "Resume / play [enter]";
    Vector2 sz = MeasureTextEx(GetFontDefault(), msg, FONT_SIZE_BIG, 4.f);
    Vector2 pos = {
        .x = width * 0.2f,
        .y = height * 0.2f,
    };
    DrawTextEx(GetFontDefault(), msg, pos, FONT_SIZE_BIG, 4.f, WHITE);

    char *controls = "Controls:";
    Vector2 cpos = {
        .x = pos.x,
        .y = pos.y + sz.y * LINE_SPACE * LINE_SPACE,
    };
    DrawTextEx(GetFontDefault(), controls, cpos, FONT_SIZE_BIG, 4.f, WHITE);

    char *interact = "interact        [enter]";
    Vector2 interact_sz = MeasureTextEx(GetFontDefault(), interact, FONT_SIZE_MID, 4.f);
    Vector2 interact_pos = {
        .x = pos.x,
        .y = cpos.y + sz.y * LINE_SPACE,
    };
    DrawTextEx(GetFontDefault(), interact, interact_pos, FONT_SIZE_MID, 4.f, WHITE);

    char *movement = "movement       [w|a|s|d]";
    Vector2 movement_pos = {
        .x = pos.x,
        .y = interact_pos.y + interact_sz.y * LINE_SPACE,
    };
    DrawTextEx(GetFontDefault(), movement, movement_pos, FONT_SIZE_MID, 4.f, WHITE);

#ifdef PLATFORM_WEB
    char *modifier = "move modifier  [u]";
#else
    char *modifier = "move modifier  [shift|u]";
#endif
    Vector2 modifier_pos = {

        .x = pos.x,
        .y = movement_pos.y + interact_sz.y * LINE_SPACE,
    };
    DrawTextEx(GetFontDefault(), modifier, modifier_pos, FONT_SIZE_MID, 4.f, WHITE);

    char *mirror = "puzzle: mirror [left mouse]";
    Vector2 mirror_pos = {
        .x = pos.x,
        .y = modifier_pos.y + interact_sz.y * LINE_SPACE,
    };
    DrawTextEx(GetFontDefault(), mirror, mirror_pos, FONT_SIZE_MID, 4.f, WHITE);

#ifdef PLATFORM_WEB
    char *menu = "open menu      [q]";
#else
    char *menu = "open menu      [escape|q]";
#endif
    Vector2 menu_pos = {
        .x = pos.x,
        .y = mirror_pos.y + interact_sz.y * LINE_SPACE,
    };
    DrawTextEx(GetFontDefault(), menu, menu_pos, FONT_SIZE_MID, 4.f, WHITE);

}

GameState update_menu(void)
{
    if (IsKeyPressed(KEY_ENTER)) {
        return WORLD;
    }
    return MENU;
}


GameState update_victory()
{
    if (IsKeyPressed(KEY_ENTER)) {
        return MENU;
    }
    return PUZZLE_BOSS_WIN;
}

void render_victory(World *w, PlayerState pstate, Texture2D atlas, Texture2D player_atlas)
{
    (void) player_atlas;
    render_hud_lhs(pstate, w->wpos.x + w->wdim.x, atlas);

    char *msg = "You've made it back in the world!";
    Vector2 sz = MeasureTextEx(GetFontDefault(), msg, FONT_SIZE_BIG, 4.f);
    Vector2 pos = {
        .x = (GetScreenWidth() - sz.x) / 2.f,
        .y = 0.4f * (GetScreenHeight()),
    };
    DrawTextEx(GetFontDefault(), msg, pos, FONT_SIZE_BIG, 4.f, WHITE);


    char *instructions = "Thanks for playing <3";
    Vector2 isz = MeasureTextEx(GetFontDefault(), instructions, FONT_SIZE_MID, 4.f);
    Vector2 ipos = {
        .x = pos.x,
        .y = 0.6f * GetScreenHeight(),
    };
    DrawTextEx(GetFontDefault(), instructions, ipos, FONT_SIZE_MID, 4.f, WHITE);

    char *author = "- elicatza";
    Vector2 apos = {
        .x = pos.x,
        .y = ipos.y + isz.y * 1.35f,
    };
    DrawTextEx(GetFontDefault(), author, apos, FONT_SIZE_MID, 4.f, WHITE);
}


void render_sleep(World *w, Sleep s, PlayerState pstate, Texture2D atlas, Texture2D player_atlas)
{
    (void) s;
    render_world(w, pstate, atlas, player_atlas);

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
    

    char *instructions = pstate.did_faint ? "Darkness" : "Good";
    Vector2 isz = MeasureTextEx(GetFontDefault(), instructions, w->wdim.y / 20.f, 2.f);
    Vector2 ipos = {
        .x = w->wpos.x + 0.5f * (w->wdim.x  - isz.x),
        .y = w->wpos.y + 0.3f * (w->wdim.y - isz.y) + sz.y + tsz.y,
    };
    DrawTextEx(GetFontDefault(), instructions, ipos, w->wdim.y / 20.f, 4.f, WHITE);
}

GameState update_world(World *w, PlayerState *pstate)
{
    Player new_p = w->player;
    Direction dir = NONE;
    if ((IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) && !pstate->is_sleeping) {
        new_p.pos.y += -1;
        dir = UP;
    } else if ((IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) && !pstate->is_sleeping) {
        new_p.pos.x += -1;
        dir = LEFT;
    } else if ((IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) && !pstate->is_sleeping) {
        new_p.pos.y += 1;
        dir = DOWN;
    } else if ((IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) && !pstate->is_sleeping) {
        new_p.pos.x += 1;
        dir = RIGHT;
    }

    if (memcmp(&new_p, &w->player.pos, sizeof new_p) != 0) {
        bool penalty = false;
        if ((IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT) || IsKeyDown(KEY_U)) && !pstate->is_sleeping) {
            penalty = true;
            new_p.height += 1;
        }
        if (is_valid_wspos(w, new_p.pos, new_p.height)) {
            if (penalty) {
                pstate->face_id = new_face_id(pstate->face_id, dir);
                INFO("Starting animation");
                apply_energy_loss(pstate);
                if (should_faint(*pstate)) return FAINT;
            }
            w->player = new_p;
            w->player.height = get_height_at_pos(w, new_p.pos);
        }
    }

    if ((IsKeyPressed(KEY_I) || IsKeyPressed(KEY_ENTER)) && !pstate->is_sleeping) {
        // Interact
        Cell cell = cell_at_pos(w, w->player.pos);
        switch ((enum PhysicalType) MASK_PHYSICAL_T(cell.info)) {
            case PTABLE_BL: break;
            case PTABLE_BR: break;
            case PTABLE_TL: break;
            case PTABLE_TR: break;
            case PGROUND: break;
            case PEMPTY: break;
            case PWINDOW: break;
            case PTABLE: break;
            case PBLINDS: { go.blinds_down = !go.blinds_down; } break;
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
                return PUZZLE_FUN;
            } break;
            case PPUZZLE2: {
                return PUZZLE_TRAIN;
            } break;
            case PBOSS: {
                return PUZZLE_BOSS;
            } break;
            case PBED: {
                go.sleep = init_sleep(pstate);
                return SLEEP;
            } break;
            case PBED_END: {
                go.sleep = init_sleep(pstate);
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
    update_pstate(pstate);
    pstate->light_tmp = 0;
    pstate->light_tmp += light_from_time(*pstate);
    apply_lighting(w, *pstate);
    return WORLD;
}

Vector2 vspos_of_ws(World *w, U32x2 ws)
{
    Vector2 vs;
    vs.x = w->wpos.x + ws.x * w->cell_width;
    vs.y = w->wpos.y + ws.y * w->cell_width;
    return vs;
}

void render_world_cells(World *w, PlayerState pstate, Texture2D atlas)
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

        Color color = WHITE;
        float rotation = 0.f;
        switch ((enum PhysicalType) MASK_PHYSICAL_T(cell.info)) {
            case (PEMPTY): { continue; } break;
            case (PGROUND): { src.x = 0.f * 16.f; src.y = 0.f; } break;
            case (PBED): { src.x = 1.f * 16.f; src.y = 0.f; } break;
            case (PBED_END): { src.x = 2.f * 16.f; src.y = 0.f; } break;
            case (PWINDOW): { if (go.blinds_down) continue; src.x = 3.f * 16.f; src.y = 0.f; } break;
            case (PDOOR): { src.x = 4.f * 16.f, src.y = 0.f; } break;
            case (PTABLE): { src.x = 5.f * 16.f; src.y = 0.f; } break;
            case (PBLINDS): { src.x = 6.f * 16.f; src.y = 0.f; } break;
            case (PBOSS): { src.x = 7.f * 16.f; src.y = 0.f; } break;
            case (PTABLE_TL): { src.x = 8.f * 16.f; src.y = 0.f; } break;
            case (PTABLE_TR): { src.x = 9.f * 16.f; src.y = 0.f; } break;
            case (PTABLE_BL): { src.x = 10.f * 16.f; src.y = 0.f; } break;
            case (PTABLE_BR): { src.x = 11.f * 16.f; src.y = 0.f; } break;
            case (PPUZZLE1): { src.x = 12.f * 16.f; src.y = 0.f; } break;
            case (PPUZZLE2): { src.x = 13.f * 16.f; src.y = 0.f; } break;
        }

        // color = apply_shade(color, 0.4f);
        // color = blend(color, cell.color, 0.5);
        color = ColorTint(color, cell.color);
        color = ColorBrightness(color, MAX(pstate.light + pstate.light_tmp, 0.25f));
        w->cell_case[i].color = color;
        DrawTexturePro(atlas, src, dest, center, rotation, color); // Draw a part of a texture defined by a rectangle with 'pro' parameters
        // cell.lighting = 0.5 + (lightness / 30.f);
        // color = blend(color, C_BLUE, cell.lighting + 5);
        // color = apply_tint(color, cell.lighting);
        // color = lerp(color, (Color) { 0, 0, 0, 255 }, 0.5 + (cell.lighting / 30.f));


        // DrawRectangleV(vspos, dim, color);
    }
}

// RLAPI Color Fade(Color color, float alpha);                                 // Get color with alpha applied, alpha goes from 0.0f to 1.0f
// RLAPI Vector4 ColorNormalize(Color color);                                  // Get Color normalized as float [0..1]
// RLAPI Color ColorFromNormalized(Vector4 normalized);                        // Get Color from normalized values [0..1]
// RLAPI Color ColorTint(Color color, Color tint);                             // Get color multiplied with another color
// RLAPI Color ColorContrast(Color color, float contrast);                     // Get color with contrast correction, contrast values between -1.0f and 1.0f
// RLAPI Color ColorBrightness(Color color, float factor);                     // Get color with brightness correction, brightness factor goes from -1.0f to 1.0f
// RLAPI Color ColorAlphaBlend(Color dst, Color src, Color tint);              // Get src alpha-blended into dst color with tint

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


void render_world(World *w, PlayerState pstate, Texture2D atlas, Texture2D player_atlas)
{
    float width = GetScreenWidth();
    float height = GetScreenHeight();

    w->cell_width = MIN(width / w->cols, height / w->rows);
    w->wpos.x = (width - (w->cell_width * w->cols)) / 2.f;
    w->wpos.y = (height - (w->cell_width * w->rows)) / 2.f;
    w->wdim.x = w->cell_width * w->cols;
    w->wdim.y = w->cell_width * w->rows;

    render_world_cells(w, pstate, atlas);
    
    Color color = get_color_at_pos(w, w->player.pos);
    render_player(vspos_of_ws(w, w->player.pos),
                  (Vector2) { w->cell_width, w->cell_width },
                  pstate,
                  player_atlas,
                  color);
    // render_world_height_lines(w);
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
            cell.color = WHITE;
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

