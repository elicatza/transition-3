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
 * 0b01xx: Regular ground
 * 0b10xx: Player spawn
 * 0b11xx: `reserved
 * where xx is 0b00
 *
 * Then two bits for elevation
 * 0b00xxxx: elevation 0
 * 0b01xxxx: elevation 1
 * 0b10xxxx: elevation 2
 * 0b11xxxx: elevation 3
 */

enum CellType {
    TGROUND = 0b00,
    TINTERACT = 0b01,
    TLIGHT = 0b10,
};

enum GrndType {
    GRND_EMTPY = 0b0000,
    GRND_NORMAL = 0b0100,
    GRND_SPAWN = 0b1000,
};

enum LightType {
    LIGHT_BLUE = 0b0000,
    LIGHT_WHITE = 0b0100,
    LIGHT_PINK = 0b1000,
    LIGHT_BLACK = 0b1100,
};

#define MASK_TYPE(a) ((a) & 0b11)
#define MASK_INTERACT(a) ((a) & 0b1100)
#define MASK_LIGHT(a) ((a) & 0b1100)
#define MASK_LIGHT_BRIGHTNESS(a) ((a) & 0b11110000)
#define MASK_GROUND(a) ((a) & 0b1100)

/**
 * Header:
 * cols, rows
 *
 * Body: world format (len = width * height)
 *
 */
#define G 0b0100  /* Regular ground height 1 */
#define P 0b1000  /* Player init position */
static u8 world1[] = {
    5, 5,
    G, G, G, G, G,
    G, G, 0, G, G,
    G, G, G, G, G,
    G, P, G, G, G,
    G, G, G, G, G,
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

    // Following are set at start of render function
    float cell_width;
    Vector2 wpos;
    Vector2 wdim;
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
    go.world = load_world(world1);
    go.puzzle = load_puzzle(puzzle_array[0]);
    go.state = WORLD;

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
    switch (go.state) {
        case PUZZLE: { update_puzzle(go.puzzle); } break;
        case WORLD: { update_world(go.world); } break;
        default: { ASSERT(0, "Not implemented") } break;
    }

    BeginDrawing();

    ClearBackground(RAYWHITE);
    switch (go.state) {
        case PUZZLE: { render_puzzle(go.puzzle, go.atlas); } break;
        case WORLD: { render_world(go.world, go.atlas); } break;
        default: { ASSERT(0, "Not implemented"); } break;
    }

    EndDrawing();
}

void update_world(World *w)
{
    (void) w;
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
        // INFO("Pos %.2f, %.2f", vspos.x, vspos.y);
        // INFO("dim %.2f, %.2f", dim.x, dim.y);

        Color color;
        switch ((enum CellType) MASK_TYPE(cell.info)) {
            case TGROUND: { switch ((enum GrndType) MASK_GROUND(cell.info)) {
                case GRND_SPAWN: { color = GREEN; } break;
                case GRND_EMTPY: { color = GRAY; } break;
                case GRND_NORMAL: { color = BROWN; } break;
                default: { color = PURPLE; } break;
            } break; } break;
            case TLIGHT: { switch ((enum LightType) MASK_LIGHT(cell.info)) {
                case LIGHT_BLUE: { color = BLUE; } break;
                case LIGHT_WHITE: { color = WHITE; } break;
                case LIGHT_PINK: { color = PINK; } break;
                case LIGHT_BLACK: { color = BLACK; } break;
                default: { color = PURPLE; } break;
                } break; } break;
            case TINTERACT: { color = ORANGE; } break;
            default: { ASSERT(0, "invalid map value"); } break;
        }
        DrawRectangleV(vspos, dim, color);
    }
}

void render_world(World *w, Texture2D atlas)
{
    // Figure out where player is.
    // Two approaches:
    //   Draw world around player
    //   Draw rooms (static camera) More control over rendering + coherent

    int width = GetScreenWidth();
    int height = GetScreenHeight();
    int min_dim = MIN(width, height);

    w->wpos.x = (width - min_dim) / 2.f;
    w->wpos.y = (height - min_dim) / 2.f;
    w->wdim.x = min_dim;
    w->wdim.y = min_dim;
    w->cell_width = (float) min_dim / (float) MAX(w->rows, w->cols);


    render_world_cells(w, atlas);
    DrawRectangleLinesEx((Rectangle) { w->wpos.x, w->wpos.y, w->wdim.x, w->wdim.y }, 2.f, RED);
}

void fill_world(World *w, u8 *wbody)
{
    size_t row, col;
    for (row = 0; row < w->rows; ++row) {
        for (col = 0; col < w->cols; ++col) {
            u8 info = wbody[row * w->cols + col];
            u8 type = MASK_TYPE(info);
            u8 gtype = MASK_GROUND(info);

            if (type == TGROUND && gtype == GRND_SPAWN) {
                Player p;
                p.pos = (U32x2) { col, row };
                w->player = p;
                INFO("Added player");
            }

            Cell cell;
            cell.info = info;
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

    fill_world(w, wmap);
    return w;
}

void free_world(World *w)
{
    free(w);
}

