#include <raylib.h>
#include <stdio.h>
#include <math.h>

#include "core.h"
#include "../assets/heart.h"
#define CASE_IMPLEMENTATION
#include "case.h"


#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#define ASPECT_RATIO (16.f / 9.f)
#define WIDTH 800
#define HEIGHT (WIDTH / ASPECT_RATIO)

#define PUZZEL_BUTTON_SZ 0.25f

typedef enum {
    MIRROR,
    SPLIT,
} ButtonKind;

typedef struct {
    Vector2 center;
    float radius;
    ButtonKind kind;
} Button;

typedef Vector2 Player;

/**
 * All values are to be defined in world space
 * Translations are done to convert to view space
 */
typedef struct {
    Player *player_case;
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
Button vs_button_of_ws(Puzzle *p, Button btn);
Player vs_player_of_ws(Puzzle *p, Player player);

int main(void)
{

    int width = WIDTH;
    int height = HEIGHT;
    SetConfigFlags(/* FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | */ FLAG_MSAA_4X_HINT);
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

/**
 * Can be cast from keys
 */
typedef enum {
    UP = KEY_W,
    DOWN = KEY_S,
    LEFT = KEY_A,
    RIGHT = KEY_D,
} Direction;

// void move_square(Puzzle *p, Player player, Direction dir)
// {
//     switch (dir) {
//         case UP: {
//             if (player.y 
//             player.y -= 1;
//         } break;
//         case DOWN: {
//         } break;
//         case LEFT: {
//         } break;
//         case RIGHT: {
//         } break;
//     };
// }

void render(void)
{
    // TODO boundary check + animation feedback
    ASSERT(case_len(go.puzzle.player_case) > 0);
    size_t i;
    if (IsKeyPressed(KEY_W)) {
        for (i = 0; i < case_len(go.puzzle.player_case); ++i) {
            go.puzzle.player_case[i].y -= 1;
        }
    }
    if (IsKeyPressed(KEY_A)) {
        for (i = 0; i < case_len(go.puzzle.player_case); ++i) {
			go.puzzle.player_case[i].x -= 1;
        }
	}
    if (IsKeyPressed(KEY_S)) {
        for (i = 0; i < case_len(go.puzzle.player_case); ++i) {
			go.puzzle.player_case[i].y += 1;
        }
	}
    if (IsKeyPressed(KEY_D)) {
        for (i = 0; i < case_len(go.puzzle.player_case); ++i) {
			go.puzzle.player_case[i].x += 1;
        }
	}

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
                .x = 0.f,
                .y = row,
            },
            .radius = PUZZEL_BUTTON_SZ,
            .kind = MIRROR,
        };
        case_push(p->button_case, btn);

        // btn.center.x = p->rec.x + p->rec.width;
        // case_push(p->button_case, btn);
    }

    size_t col;
    for (col = 1; col < p->cols; ++col) {
        Button btn = {
            .center = (Vector2) {
                .x = col,
                .y = 0.f,
            },
            .radius = PUZZEL_BUTTON_SZ,
            .kind = MIRROR,
        };
        case_push(p->button_case, btn);

        // btn.center.y = p->rec.y + p->rec.height;
        // case_push(p->button_case, btn);
    }
}

void render_buttons(Puzzle *p)
{
    size_t i;
    float cell_width = p->rec.width / p->cols;
    for (i = 0; i < case_len(p->button_case); ++i) {
        Button vs_btn = vs_button_of_ws(p, p->button_case[i]);
        DrawCircleV(vs_btn.center, vs_btn.radius, GREEN);
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
 * Converts from button in world space to button in view space
 */
Button vs_button_of_ws(Puzzle *p, Button btn)
{
    float cell_width = p->rec.width / p->cols;
    return (Button) {
        .center = (Vector2) {
            .x = btn.center.x * cell_width + p->rec.x,
            .y = btn.center.y * cell_width + p->rec.y,
        },
        .radius = btn.radius * cell_width,
        .kind = btn.kind,
    };
}

Player vs_player_of_ws(Puzzle *p, Player player)
{
    float cell_width = p->rec.width / p->cols;
    return (Player) {
        .x = player.x * cell_width + p->rec.x,
        .y = player.y * cell_width + p->rec.y,
    };
}

/**
 * @return -1 if no match is found. Otherwise the index of hover button
 */
int button_hover_id(Puzzle *p, Button *btn_case)
{
    size_t i;
    Vector2 pos = GetMousePosition();
    float cell_width = p->rec.width / p->cols;
    for (i = 0; i < case_len(btn_case); ++i) {
        Button vs_button = vs_button_of_ws(p, btn_case[i]);
        if (in_circle(vs_button.center, vs_button.radius, pos)) {
            return i;
        }
    }
    return -1;
}

void mirror_over_line(Puzzle *p, int button_id)
{
    Button btn = p->button_case[button_id];
    float cell_width = p->rec.width / p->cols;
    Vector2 btn_cen_grid = {
        .x = (btn.center.x - p->rec.x) / cell_width,
        .y = (btn.center.y - p->rec.y) / cell_width,
    };
    if (btn.center.x == 0.f) {
        // Horizontal mirror
    } else if (btn.center.y == 0.f) {
        // Vertical mirror
        Vector2 mirrored = {
            .x = (2 * btn_cen_grid.x) - p->player_case[0].x - 1,
            .y = p->player_case[0].y,
        };
        if (mirrored.x < p->rec.width / cell_width) {
            // In bound of puzzle
            case_push(p->player_case, mirrored);
            INFO("added new at %.0f, %.0f", mirrored.x, mirrored.y);
        }
    } else {
        ASSERT(0, "Unreachable. invalid button position");
    }
}

void render_puzzle(Puzzle *p)
{
    int height = GetScreenHeight() - p->padding;
    int width = GetScreenWidth() - p->padding;
    ASSERT(width >= height);
    float cell_width = (float) height / p->cols;

    fill_buttons(p);

    // TODO check for mouse movement first
    int hover_id = button_hover_id(p, p->button_case);
    if (hover_id != -1) {
        Button vs_btn = vs_button_of_ws(p, p->button_case[hover_id]);
        DrawCircleV(vs_btn.center, 30.f, MAGENTA);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            DrawCircleV(vs_btn.center, 50.f, MAGENTA);
            mirror_over_line(p, hover_id);
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


    // Draws player objects
    size_t i;
    for (i = 0; i < case_len(p->player_case); ++i) {
        Vector2 vs_pos = vs_player_of_ws(p, p->player_case[i]);
        Rectangle player = {
            .x = vs_pos.x,
            .y = vs_pos.y,
            .width = cell_width,
            .height = cell_width,
        };
        DrawRectangleRec(player, PINK);
        // INFO("Drawing square %zu at %0.f, %0.f", i, player.x, player.y);
    }
    render_buttons(p);
}
