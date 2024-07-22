#include <raylib.h>
#include <stdio.h>
#include <math.h>

#include "core.h"
#include "../assets/atlas.h"
#define CASE_IMPLEMENTATION
#include "case.h"

#include "puzzle.h"


#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#define ASPECT_RATIO (16.f / 9.f)
#define WIDTH 800
#define HEIGHT (WIDTH / ASPECT_RATIO)

/**
 * World format
 * First two bits are ground type
 * 0b00: Ground
 * 0b01: interact
 * 0b10: light
 * 0b11: `reserved`
 *
 * If interact: Next two bits are interact type
 * 0b00xx: window blinds
 * 0b01xx: training puzzle
 * 0b10xx: bed
 * 0b11xx: door
 * 0b11xx: creative puzzles
 *
 * If light: Next two bits are light color
 * 0b00xx: blue
 * 0b01xx: white
 * 0b10xx: pink
 * 0b11xx: black
 * Then 4 bits for the light strength
 *
 * If ground: Next two bits are elevation / type
 * 0b00xx: empty space
 * 0b001xx: Elevation 1
 * 0b010xx: Elevation 2
 * 0b011xx: Elevation 3
 * 0b100xx: Player
 */

enum {
    TGROUND = 0b00,
    TLIGHT = 0b01,
    TINTERACT = 0b10,
} CellType;

#define MASK_TYPE(a) ((a) & 0b11)
#define MASK_INTERACT(a) ((a) & 0b1100)
#define MASK_LIGHT(a) ((a) & 0b1100)
#define MASK_LIGHT_BRIGHTNESS(a) ((a) & 0b11110000)
#define MASK_GROUND(a) ((a) & 0b11100)

/**
 * Header:
 * cols, rows
 *
 * Body: world format (len = width * height)
 *
 */
#define R 0b0100
static u8 world1[] = {
    5, 5,
    R, R, R, R, R,
    R, R, 0, R, R,
    R, R, R, R, R,
    R, R, R, R, R,
    R, R, R, R, R,
};

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
} Player;

typedef struct Cell {
    U32x2 pos;
    u8 info;
} Cell;

typedef struct World {
    Player player;
    size_t cols;
    size_t rows;
    Cell *cell_case;
} World;

typedef struct {
    Texture2D atlas;
    Puzzle *puzzle;
    World *world;
    GameState state;
} GO;

GO go = { 0 };

World *load_world(u8 *byte_map);
void update_world(World *w);
void render_world(World *w);
void free_world(World *w);


/**
 * Puzzle format
 * Header: width, height, padding
 * body cell map
 */
static unsigned char puzzle1[] = { 
    10, 10, 50,
    1, 1, 1, 1, 1, 1, 1, 3,3|G,3,
    1, 1, 0, 1, 1, 1, 1, 2, 3, 3,
    1, 0, 0, 0, 1, 1, 1, 2, 2, 2,
    1, 0, 0, 1, 1, 1, 1, 2, 2, 2,
    1, 1, 1, 1, 1,1|P,1, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
    1, 1, 2, 2, 1, 2,2|G,2, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 3, 3, 3,
};


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
    go.world = load_world(world1);
    go.puzzle = load_puzzle(puzzle1);
    go.state = PUZZLE;

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
    CloseWindow();
    return 0;
}


void loop(void)
{
    Puzzle *p = go.puzzle;

    switch (go.state) {
        case PUZZLE: { update_puzzle(p); } break;
        default: { ASSERT(0, "Not implemented") } break;
    }

    BeginDrawing();

    ClearBackground(RAYWHITE);
    switch (go.state) {
        case PUZZLE: { render_puzzle(p, go.atlas); } break;
        default: { ASSERT(0, "Not implemented"); } break;
    }

    EndDrawing();
}

void update_world(World *w)
{
}

void render_world(World *w)
{
}

void fill_wcells(World *w, u8 *wbody)
{
    size_t row, col;
    for (row = 0; row < w->rows; ++row) {
        for (col = 0; col < w->cols; ++col) {
            u8 type = MASK_TYPE(wbody[row * w->cols + col]);
            switch (type) {
                case TGROUND: { INFO("We hit ground"); } break;
                default: { ASSERT(0, "Invalid world cell type"); } break;
            }
            Cell cell;
            cell.info = wbody[row * w->cols + col];
            cell.pos = (U32x2) { col, row };
            case_push(w->cell_case, cell);
        }
    }
}

World *load_world(u8 *wmap)
{
    World *w = malloc(sizeof *w);
    size_t cols = *wmap; ++wmap;
    size_t rows = *wmap; ++wmap;

    w->cols = cols;
    w->rows = rows;
    w->cell_case = case_init(cols * rows, sizeof *w->cell_case);

    fill_wcells(w, wmap);
    return w;
}

void free_world(World *w)
{
    free(w);
}

