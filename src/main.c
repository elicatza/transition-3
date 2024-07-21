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


// typedef enum {
//     PUZZLE,
//     WORLD,
//     MENU,
// } GameState;

typedef struct {
    Texture2D atlas;
    Puzzle puzzle;
} GO;

GO go = { 0 };



/**
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

    Puzzle puzzle = puzzle_load(puzzle1);
    // case_push(puzzle.player_case, ((Player) { .pos = (Vector2) { 0.f, 0.f }, .state = PHYSICAL, .height = 1 } ));
    go.atlas = LoadTextureFromImage(atlas_img);
    go.puzzle = puzzle;

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
    CloseWindow();
    return 0;
}


void loop(void)
{
    Puzzle *p = &go.puzzle;

    update_puzzle(p);

    BeginDrawing();

    ClearBackground(RAYWHITE);
    render_puzzle(&go.puzzle, go.atlas);

    EndDrawing();
}

