#include <raylib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "../assets/heart.h"
#define CASE_IMPLEMENTATION
#include "case.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#define ASPECT_RATIO (16.f / 9.f)
#define WIDTH 800
#define HEIGHT (WIDTH / ASPECT_RATIO)

typedef enum {
    MIRROR,
    SPLIT,
} ButtonKind;

typedef struct {
    Vector2 center;
    float radius;
    ButtonKind kind;
} Button;

typedef struct {
    Vector2 *player_case;
    size_t rows;
    size_t cols;
    unsigned char *cells;
    Button *button_case;
    Rectangle rec;
    float padding;
} Puzzle;

typedef struct {
    Texture2D heart;
    Puzzle puzzle;
} GO;

GO go = { 0 };


void render(void);
void render_puzzle(Puzzle *p);
void fill_buttons(Puzzle *p);

int main(void)
{

    int width = WIDTH;
    int height = HEIGHT;
    InitWindow(width, height, "demo");

    Image heart_img = {
        .data = HEART_DATA,
        .width = HEART_WIDTH,
        .height = HEART_HEIGHT,
        .format = HEART_FORMAT,
        .mipmaps = 1,
    };

    Puzzle puzzle = {
        .player_case = case_init(64, sizeof *puzzle.player_case),
        .cols = 10,
        .rows = 10,
        .cells = NULL,
        .button_case = case_init(64, sizeof (Button)),
        .padding = 20.f,
    };
    case_push(puzzle.player_case, ((Vector2) { 0.f, 0.f } ));
    go.heart = LoadTextureFromImage(heart_img);
    go.puzzle = puzzle;

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(render, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        render();
    }
#endif

    UnloadTexture(go.heart);
    CloseWindow();
    return 0;
}

void render(void)
{
    // TODO boundary check + animation feedback
    assert(case_len(go.puzzle.player_case) > 0);
    if (IsKeyPressed(KEY_W)) { go.puzzle.player_case[0].y -= 1; }
    if (IsKeyPressed(KEY_A)) { go.puzzle.player_case[0].x -= 1; }
    if (IsKeyPressed(KEY_S)) { go.puzzle.player_case[0].y += 1; }
    if (IsKeyPressed(KEY_D)) { go.puzzle.player_case[0].x += 1; }

    BeginDrawing();

    ClearBackground(RAYWHITE);
    DrawTexture(go.heart, 10, 10, RED);
    render_puzzle(&go.puzzle);

    EndDrawing();
}

void fill_buttons(Puzzle *p)
{
    case_len(p->button_case) = 0;
    float cell_width = p->rec.height / p->cols;
    size_t i;

    size_t row;
    for (row = 1; row < p->rows; ++row) {
        Button btn = {
            .center = (Vector2) {
                .x = p->rec.x,
                .y = row * cell_width + p->rec.y,
            },
            .radius = 10.f,
            .kind = MIRROR,
        };
        case_push(p->button_case, btn);

        btn.center.x = p->rec.x + p->rec.width;
        case_push(p->button_case, btn);
    }

    size_t col;
    for (col = 1; col < p->cols; ++col) {
        Button btn = {
            .center = (Vector2) {
                .x = col * cell_width + p->rec.x,
                .y = p->rec.y,
            },
            .radius = 10.f,
            .kind = MIRROR,
        };
        case_push(p->button_case, btn);

        btn.center.y = p->rec.y + p->rec.height;
        case_push(p->button_case, btn);
    }
}

void render_buttons(Puzzle *p)
{
    size_t i;
    for (i = 0; i < case_len(p->button_case); ++i) {
        DrawCircleV(p->button_case[i].center, p->button_case[i].radius, GREEN);
    }
}

Vector2 vec2d_sub(Vector2 a, Vector2 b)
{
    return (Vector2) {
        .x = a.x - b.x,
        .y = a.y - b.y,
    };
}

bool in_circle(Vector2 center, float radius, Vector2 point)
{
    float lhs = powf(point.x - center.x, 2.f) + powf(point.y - center.y, 2.f);
    float rhs = radius * radius;
    if (lhs <= rhs) {
        return true;
    }
    return false;
}

/**
 * @return -1 if no match is found. Otherwise the index of hover button
 */
int button_hover_id(Button *btn_case)
{
    size_t i;
    Vector2 pos = GetMousePosition();
    for (i = 0; i < case_len(btn_case); ++i) {
        if (in_circle(btn_case[i].center, btn_case[i].radius, pos)) {
            return i;
        }
    }
    return -1;
}

void render_puzzle(Puzzle *p)
{
    int height = GetScreenHeight() - p->padding;
    int width = GetScreenWidth() - p->padding;
    assert(width >= height);
    float cell_width = (float) height / p->cols;

    fill_buttons(p);

    // TODO check for mouse movement first
    int hover_id = button_hover_id(p->button_case);
    if (hover_id != -1) {
        DrawCircleV(p->button_case[hover_id].center, 30.f, MAGENTA);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            DrawCircleV(p->button_case[hover_id].center, 50.f, MAGENTA);
        }
    }

    // TODO Check for window resize
    p->rec = (Rectangle) {
        .width = height,
        .height = height,
        .x = (width - height) / 2.f,
        .y = p->padding / 2.f,
    };
    DrawRectangleLinesEx(p->rec, 10.f, RED);

    // Draw rows
    size_t row;
    for (row = 0; row < p->rows; ++row) {
        Vector2 start = {
            .x = p->rec.x,
            .y = row * cell_width + p->rec.y,
        };
        Vector2 end = {
            .x = p->rec.x + p->rec.width,
            .y = row * cell_width + p->rec.y,
        };
        DrawLineEx(start, end, 1.f, BLUE);
    }

    // Draw columns
    size_t col;
    for (col = 0; col < p->cols; ++col) {
        Vector2 start = {
            .x = col * cell_width + p->rec.x,
            .y = p->rec.y,
        };
        Vector2 end = {
            .x = col * cell_width + p->rec.x,
            .y = p->rec.y + p->rec.height,
        };
        DrawLineEx(start, end, 1.f, BLUE);
    }


    Rectangle player = {
        .x = p->player_case[0].x * cell_width + p->rec.x,
        .y = p->player_case[0].y * cell_width + p->rec.y,
        .width = cell_width,
        .height = cell_width,
    };
    DrawRectangleRec(player, PINK);
    render_buttons(p);
}
