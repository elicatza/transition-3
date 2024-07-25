#include <raylib.h>
#include <stdio.h>
#include <math.h>

#include "core.h"
#include "../assets/atlas.h"
#define CASE_IMPLEMENTATION
#include "case.h"
#define NO_TEMPLATE
#include "puzzle.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif


#define ASPECT_RATIO (16.f / 9.f)
#define WIDTH 800
#define HEIGHT (WIDTH / ASPECT_RATIO)
#define PADDING 50.f

#define MAX_ENERGY 1.f

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
    PEMPTY = 0b000,   /* Don't render anything */
    PGROUND = 0b001,  /* Walkable */
    PBLINDS = 0b010,  /* -Energy +Light */
    PBED = 0b011,     /* Restart day / spawn */
    PDOOR = 0b100,    /* Finish when exit / Big puzzle */
    PPUZZLE1 = 0b101, /* Puzzle for exercise */
    PPUZZLE2 = 0b110, /* Puzzle for fun */
    PWINDOW = 0b111,  /* Window for seeing things */
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

#define S (PBED | H2 | VSPAWN)
#define G (PGROUND | H1)
#define D0 (PDOOR | H2 | PMETA(0b00))
#define D1 (PDOOR | H2 | PMETA(0b01))
#define Ld (PWINDOW | UNWALKABLE | VWHITE | VSTRENGTH(0b1111))
#define Lb (PWINDOW | UNWALKABLE | VPINK | VSTRENGTH(0b0011))

/**
 * Header:
 * cols, rows
 * Body: world format (len = width * height)
 *
 */
static u16 worlds[2][103] = {
    {
        6, 7,
        0, 0, 0, 0, D1, 0,
        G, G, G, G, G, G,
        G, G, G, G, G, G,
        S, S, G, G, G, G,
        G, G, G, G, G, G,
        G, G, G, G, G, G,
        0, 0, Ld,Ld,0, 0,
    },
    {
        6, 9,
        0, 0, 0, 0, 0, 0,
        0, G, G, G, G, G,
        0, G, G, G, G, G,
        0, G, G, G, G, G,
        0, G, G, G, G, G,
        0, S, G, G, G, G,
        0, G, G, G, G, G,
        0, G, G, G, G, G,
        0, 0, 0, 0, D0, 0,
    }
};

#define C_BLUE CLITERAL(Color){ 0x55, 0xcd, 0xfc, 0xff }
#define C_PINK CLITERAL(Color){ 0xf7, 0xa8, 0xb8, 0xff }

typedef enum {
    PUZZLE,
    WORLD,
    MENU,
} GameState;

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

typedef struct {
    Texture2D atlas;
    Puzzle *puzzle;
    World *world;
    GameState state;
    float energy;
    float energy_max;
    float pain;
    float pain_max;
} GO;

GO go = { 0 };

/* Door 0, 1, 2, 3, spawnpoint */
World *load_world(u16 world_id, u8 spawn);
void update_world(World *w);
void render_world(World *w, Texture2D atlas);
void free_world(World *w);




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

    go.atlas = LoadTextureFromImage(atlas_img);
    go.world = load_world(0, 4);
    go.puzzle = load_puzzle(puzzle_array[0]);
    go.state = WORLD;
    go.energy = 0.3f;
    go.energy_max = 0.3f;
    go.pain = 0.2f;
    go.pain_max = 1.f;

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
    free_puzzle(go.puzzle);
    free_world(go.world);
    CloseWindow();
    return 0;
}


void loop(void)
{
    switch (go.state) {
        case PUZZLE: { update_puzzle(go.puzzle); } break;
        case WORLD: { update_world(go.world); } break;
        default: { ASSERT(0, "Not implemented") } break;
    }

    BeginDrawing();

    ClearBackground(BLACK);
    switch (go.state) {
        case PUZZLE: { render_puzzle(go.puzzle, go.atlas); } break;
        case WORLD: { render_world(go.world, go.atlas); } break;
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

Color get_color_at_pos(World *w, U32x2 pos)
{
    enum VisualColor color = MASK_COLOR(w->cell_case[pos.y * w->cols + pos.x].info);
    switch (color) {
        case VWHITE: { return WHITE; } break;
        case VBLUE: { return C_BLUE; } break;
        case VPINK: { return C_PINK; } break;
        case VBLACK: { return BLACK; } break;
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
    };
}


bool is_valid_wspos(World *w, U32x2 pos, u32 height)
{
    if (pos.x < 0 || pos.x >= w->cols) {
        return false;
    }
    if (pos.y < 0 || pos.y >= w->rows) {
        return false;
    }
    Cell cell = cell_at_pos(w, pos);
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
            u8 brightness = get_brightness_at_pos(w, pos);
            Color color = get_color_at_pos(w, pos);
            int b = brightness;
            int nx, ny;
            for (nx = 0; nx < w->cols; ++nx) {
                for (ny = 0; ny < w->rows; ++ny) {
                    float val = 1.f / powf(abs((int) col - nx) + abs((int) row - ny), 2.f);
                    size_t i = ny * w->cols + nx;
                    Color old = w->cell_case[i].color;
                    w->cell_case[i].color = blend(w->cell_case[i].color, color, val * b);
                    old = w->cell_case[i].color;
                }
            }
        }
    }
}

void update_world(World *w)
{
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
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT) || IsKeyDown(KEY_U)) {
            new_p.height += 1;
        }
        if (is_valid_wspos(w, new_p.pos, new_p.height)) {
            w->player = new_p;
            w->player.height = get_height_at_pos(w, new_p.pos);
        }
    }

    if (IsKeyPressed(KEY_I)) {
        // Interact
        Cell cell = cell_at_pos(w, w->player.pos);
        switch ((enum PhysicalType) MASK_PHYSICAL_T(cell.info)) {
            case PDOOR: {
                int meta = MASK_META(cell.info) >> 6;
                INFO("Door %d", meta);
                World *oldw = w;
                w = load_world(meta, w->world_id);
                go.world = w;
                free_world(oldw);
            } break;
        }
    }

    size_t i;
    for (i = 0; i < case_len(w->cell_case); ++i) {
        w->cell_case[i].color = WHITE;
    }
    apply_lighting(w);
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
    size_t i;
    for (i = 0; i < case_len(w->cell_case); ++i) {
        Cell cell = w->cell_case[i];
        Vector2 vspos = vspos_of_ws(w, cell.pos);
        Vector2 dim;
        dim.x = w->cell_width;
        dim.y = w->cell_width;

        Color color = PURPLE;
        switch ((enum PhysicalType) MASK_PHYSICAL_T(cell.info)) {
            case (PEMPTY): { color = BLACK; } break;
            case (PGROUND): { color = BROWN; } break;
            case (PBLINDS): { color = RED; } break;
            case (PWINDOW): { color = WHITE; } break;
            case (PBED): { color = BLUE; } break;
            case (PDOOR): { color = ORANGE; } break;
            case (PPUZZLE1): { color = YELLOW; } break;
            case (PPUZZLE2): { color = VIOLET; } break;
        }
        // cell.lighting = 0.5 + (lightness / 30.f);
        // color = blend(color, C_BLUE, cell.lighting + 5);
        // color = apply_tint(color, cell.lighting);
        // color = lerp(color, (Color) { 0, 0, 0, 255 }, 0.5 + (cell.lighting / 30.f));
        color = apply_shade(color, 0.4f);
        color = blend(color, cell.color, 0.2);
        DrawRectangleV(vspos, dim, color);
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
                    DrawLineEx(start, end, diff * 2.f, RED);
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
                    DrawLineEx(start, end, diff * 2.f, RED);
                }
            }

        }
    }
}

void render_hud(World *w, Texture2D atlas)
{
    (void) atlas;

    float s_width = w->wpos.x + w->wdim.x;
    float s_height = GetScreenHeight();
    float padx = PADDING * 0.1;
    float ysec = w->wdim.y * (1.f / 9.f);
    DrawRectangleRec((Rectangle) {
        .width = PADDING,
        .height = w->wdim.y,
        .x = w->wpos.x + w->wdim.x,
        .y = 0,
    }, GRAY);


    DrawRectangleLinesEx((Rectangle) {
        .width = PADDING - 2.f * padx,
        .height = 3 * (go.energy_max / MAX_ENERGY) * ysec,
        .x = w->wpos.x + w->wdim.x + padx,
        .y = ysec + 3 * (1.f - (go.energy_max / MAX_ENERGY)) * ysec,
    }, 2.f, C_BLUE);

    DrawRectangleRec((Rectangle) {
        .width = PADDING - 2.f * padx,
        .height = 3 * (go.energy / MAX_ENERGY) * ysec,
        .x = w->wpos.x + w->wdim.x + padx,
        .y = ysec + 3 * (1.f - (go.energy / MAX_ENERGY)) * ysec,
    }, C_BLUE);

    DrawRectangleLinesEx((Rectangle) {
        .width = PADDING - 2.f * padx,
        .height = 3 * ysec,
        .x = w->wpos.x + w->wdim.x + padx,
        .y = ysec,
    }, 2.f, BLACK);

    DrawText("Energy", w->wpos.x + w->wdim.x + padx, ysec * 0.5f, 12.f, C_BLUE);

    DrawRectangleRec((Rectangle) {
        .width = PADDING - 2.f * padx,
        .height = 3 * (go.pain / go.pain_max) * ysec,
        .x = w->wpos.x + w->wdim.x + padx,
        .y = 5 * ysec + 3 * (1.f - (go.pain / go.pain_max)) * ysec,
    }, C_PINK);

    DrawRectangleLinesEx((Rectangle) {
        .width = PADDING - 2.f * padx,
        .height = 3 * ysec,
        .x = w->wpos.x + w->wdim.x + padx,
        .y = 5 * ysec,
    }, 2.f, BLACK);

    DrawText("Pain", w->wpos.x + w->wdim.x + padx, 4 * ysec + ysec * 0.5f, 12.f, C_PINK);
}

void render_world(World *w, Texture2D atlas)
{
    float s_width = GetScreenWidth();
    float s_height = GetScreenHeight();
    float width = s_width - PADDING;
    float height = s_height;
    float padx = PADDING;
    float pady = 0.f;

    int min_dim = MIN(width, height);
    int max_dim = MAX(width, height);


    w->cell_width = MIN(width / w->cols, height / w->rows);

    w->wpos.x = (max_dim - height) / 2.f + (w->cell_width * (MAX(w->rows, w->cols) - w->rows) / 2.f) + padx / 2.f;
    w->wpos.y = (max_dim - width) / 2.f + (w->cell_width * (MAX(w->rows, w->cols) - w->rows) / 2.f) + pady / 2.f;
    w->wdim.x = w->cell_width * w->cols;
    w->wdim.y = w->cell_width * w->rows;

    render_world_cells(w, atlas);
    render_world_player(w);
    render_world_height_lines(w);
    render_hud(w, atlas);
    DrawRectangleLinesEx((Rectangle) { w->wpos.x, w->wpos.y, w->wdim.x, w->wdim.y }, 2.f, RED);
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
            // cell.lighting = 0;
            cell.color = WHITE;
            case_push(w->cell_case, cell);
        }
    }
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

