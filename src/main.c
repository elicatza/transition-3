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
 * New format
 * 0bxxxx0000: physical {type1[empty, ground, blinds, bed, door, puzzle1, puzzle2, window](3), height(1)}
 * 4 bits for type: [empty, ground blinds, bed, door, puzzle1, puzzle2, `reserved`]
 * 2 bit for height: unwalkable, 1, 2, 3
 * 2 bits reserved for textures
 *
 * 0b0000xxxx: visual {2 type[empty, light, camera], 2 color, 4 brightness} (1)
 * 2 bits type: [empty, spawn, camera, `reserved`]
 * 2 bits color: [white, blue, pink, black]
 * 4 bits for brightness: 0-15
 */

/* Visual masks */
#define MASK_PHYSICAL(a) ((a) & 0b11111111)
#define MASK_PHYSICAL_T(a) ((a) & 0b00000111)
#define MASK_HEIGHT(a) ((a) & 0b00011000)

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
    VCAMERA = 0b10 << 8,  /* Camera center */
};

#define S (PBED | H2 | VSPAWN)
#define C (PGROUND | H2 | VCAMERA)
#define G (PGROUND | H1 )

/**
 * Header:
 * cols, rows
 * Body: world format (len = width * height)
 *
 */
static u16 world1[] = {
    5, 5,
    0, 0, 0, 0, 0,
    0, C, G, G, 0,
    0, G, S, G, 0,
    0, G, G, G, 0,
    0, 0, 0, 0, 0,
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
    u8 height;
} Player;

typedef struct Cell {
    U32x2 pos;
    u16 info;
} Cell;

typedef struct World {
    Player player;
    size_t cols;
    size_t rows;
    Cell *cell_case;
    U32x2 camera_pos;

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
} GO;

GO go = { 0 };

World *load_world(u16 *byte_map);
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
    INFO("Spawn %X", S);
    INFO("Ground %X", G);

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

        Color color = PURPLE;
        switch ((enum PhysicalType) MASK_PHYSICAL_T(cell.info)) {
            case (PEMPTY): { color = BLACK; } break;
            case (PGROUND): { color = BROWN; } break;
            case (PBLINDS): { color = RED; } break;
            case (PBED): { color = BLUE; } break;
            case (PDOOR): { color = ORANGE; } break;
            case (PPUZZLE1): { color = YELLOW; } break;
            case (PPUZZLE2): { color = VIOLET; } break;
        }
        INFO("Physical %X", MASK_PHYSICAL_T(cell.info));
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

void fill_world(World *w, u16 *wbody)
{
    size_t row, col;
    for (row = 0; row < w->rows; ++row) {
        for (col = 0; col < w->cols; ++col) {
            u16 info = wbody[row * w->cols + col];
            u16 vtype = MASK_VISUAL_T(info);
            switch ((enum VisualType) vtype) {
                case VEMPTY: break;
                case VSPAWN: {
                    w->player.pos = (U32x2) { col, row };
                    w->player.height = MASK_HEIGHT(info);
                } break;
                case VCAMERA: {
                    w->camera_pos = (U32x2) { col, row };
                } break;

            }
            Cell cell;
            cell.pos = (U32x2) { col, row };
            cell.info = info;
            case_push(w->cell_case, cell);
        }
    }
}

World *load_world(u16 *wmap)
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

