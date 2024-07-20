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

#define M_BLUE       CLITERAL(Color){ 0, 121, 241, 100 }     // Blue

typedef enum {
    MIRROR,
    SPLIT,
} ButtonKind;

typedef struct {
    Vector2 center;
    float radius;
    ButtonKind kind;
} Button;

typedef enum {
    PREVIEW,
    PHYSICAL,
} PlayerState;

typedef struct {
    Vector2 pos;
    PlayerState state;
} Player;

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
    int clicked_button;  /* id if button is clicked. Else -1 */
} Puzzle;

typedef struct {
    Texture2D heart;
    Puzzle puzzle;
} GO;

GO go = { 0 };


void loop(void);
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
        .padding = 30.f,
        .clicked_button = -1,
    };
    case_push(puzzle.player_case, ((Player) { .pos = (Vector2) { 0.f, 0.f }, .state = PHYSICAL } ));
    go.heart = LoadTextureFromImage(heart_img);
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

    UnloadTexture(go.heart);
    CloseWindow();
    return 0;
}

/**
 * Can be cast from keys
 */
typedef enum {
    NONE = 0,
    UP = KEY_W,
    DOWN = KEY_S,
    LEFT = KEY_A,
    RIGHT = KEY_D,
} Direction;

/**
 * O(n)
 */
bool is_valid_pos(Puzzle *p, Vector2 pos)
{
    if (!(pos.x >= 0 && pos.x < p->cols)) { return false; }
    if (!(pos.y >= 0 && pos.y < p->rows)) { return false; }

    size_t i;
    for (i = 0; i < case_len(p->player_case); ++i) {
        if ((int) pos.x + 0.5f == (int) p->player_case[i].pos.x + 0.5f &&
            (int) pos.y + 0.5f == (int) p->player_case[i].pos.y + 0.5f) {
            // Colliding with other object
            return false;
        }
    }
    return true;
}

void move_square(Puzzle *p, Player *player, Direction dir)
{
    // TODO play animation when can't move + when move
    Vector2 new_pos = player->pos;
    switch (dir) {
        case UP: { new_pos.y += -1; } break;
        case DOWN: { new_pos.y += 1; } break;
        case LEFT: { new_pos.x += -1; } break;
        case RIGHT: { new_pos.x += 1; } break;
            default: {
            WARNING("This function should not be run");
        } break;
    };

    // Check collisions
    if (is_valid_pos(p, new_pos)) {
        player->pos.x = new_pos.x;
        player->pos.y = new_pos.y;
    }
}

int players_cmp_ud(const void *a, const void *b)
{
    return ((Player *) a)->pos.y - ((Player *) b)->pos.y;
}

int players_cmp_du(const void *a, const void *b)
{
    return ((Player *) b)->pos.y - ((Player *) a)->pos.y;
}

int players_cmp_lr(const void *a, const void *b)
{
    return ((Player *) a)->pos.x - ((Player *) b)->pos.x;
}

int players_cmp_rl(const void *a, const void *b)
{
    return ((Player *) b)->pos.x - ((Player *) a)->pos.x;
}

typedef int (*__compar_fn_t)(const void *, const void *);
void loop(void)
{
    Puzzle *p = &go.puzzle;

    Direction dir;
    __compar_fn_t cmp_fn = NULL;

    if (IsKeyPressed(KEY_W)) {
        dir = UP;
        cmp_fn = players_cmp_ud;
    } else if (IsKeyPressed(KEY_A)) {
        dir = LEFT;
        cmp_fn = players_cmp_lr;
    } else if (IsKeyPressed(KEY_S)) {
        dir = DOWN;
        cmp_fn = players_cmp_du;
    } else if (IsKeyPressed(KEY_D)) {
        dir = RIGHT;
        cmp_fn = players_cmp_rl;
    } else {
        dir = NONE;
    }
    if (dir != NONE) {
        qsort(p->player_case, case_len(p->player_case), sizeof *(p->player_case), cmp_fn);
        size_t i;
        for (i = 0; i < case_len(go.puzzle.player_case); ++i) {
            move_square(&go.puzzle, &go.puzzle.player_case[i], dir);
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

void render_button(Puzzle *p, Button btn)
{
    Button vs_btn = vs_button_of_ws(p, btn);
    DrawCircleV(vs_btn.center, vs_btn.radius, GREEN);
}

Vector2 vec2d_add(Vector2 a, Vector2 b)
{
    return (Vector2) {
        .x = a.x + b.x,
        .y = a.y + b.y,
    };
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
        .pos = {
            .x = player.pos.x * cell_width + p->rec.x,
            .y = player.pos.y * cell_width + p->rec.y,
        },
        .state = player.state,
    };
}

/**
 * @return -1 if no match is found. Otherwise the index of hover button
 */
int button_hover_id(Puzzle *p, Button *btn_case)
{
    size_t i;
    Vector2 pos = GetMousePosition();
    for (i = 0; i < case_len(btn_case); ++i) {
        Button vs_button = vs_button_of_ws(p, btn_case[i]);
        if (in_circle(vs_button.center, vs_button.radius, pos)) {
            return i;
        }
    }
    return -1;
}

#define MIRROR_UP (1 << 0)
#define MIRROR_DOWN (1 << 1)
#define MIRROR_LEFT (1 << 2)
#define MIRROR_RIGHT (1 << 3)
#define MIRROR_PREVIEW (1 << 4)

void render_player(Puzzle *p, Player player);

/**
 * @param options. Check MIRROR_ namespace for options, 
 */
void mirror_over_line(Puzzle *p, int button_id, int options)
{
    ASSERT(options != 0, "Invalid options");
    Button ws_btn = p->button_case[button_id];


    size_t len = case_len(p->player_case);
    size_t i;
    for (i = 0; i < len; ++i) {

        Vector2 mirrored = { 0 };
        if (options & MIRROR_UP || options & MIRROR_DOWN) {
            // Horizontal mirror
            mirrored = (Vector2) {
                .x = p->player_case[i].pos.x,
                .y = (2 * ws_btn.center.y) - p->player_case[i].pos.y - 1,
            };
            if ((mirrored.y >= ws_btn.center.y && options & MIRROR_UP) ||
                (mirrored.y < ws_btn.center.y && options & MIRROR_DOWN)) {
                continue;
            }
        } else if (options & MIRROR_RIGHT || options & MIRROR_LEFT) {
            // Vertical mirror
            mirrored = (Vector2) {
                .x = (2 * ws_btn.center.x) - p->player_case[i].pos.x - 1,
                .y = p->player_case[i].pos.y,
            };
            if ((mirrored.x >= ws_btn.center.x && options & MIRROR_LEFT) ||
                (mirrored.x < ws_btn.center.x && options & MIRROR_RIGHT)) {
                continue;
            }
        }

        if (is_valid_pos(p, mirrored)) {
            if (options & MIRROR_PREVIEW) {
                // Vector2 size = vec2d_sub(*(Vector2 *) &p->rec.width, vs_btn.center);
                // Draw rectangle over section
                Player mirr_player = { .pos = mirrored, .state = PREVIEW };
                case_push(p->player_case, mirr_player);
            } else {
                // In bound of puzzle
                Player mirr_player = { .pos = mirrored, .state = PHYSICAL };
                case_push(p->player_case, mirr_player);
                INFO("added new at %.0f, %.0f", mirrored.x, mirrored.y);
            }
        }

    }
}

int get_mirror_direction(Puzzle *p, int btn_id)
{
    int options = 0;
    Vector2 m_pos = GetMousePosition();
    Button sel_ws = p->button_case[p->clicked_button];
    Button sel_vs = vs_button_of_ws(p, sel_ws);
    if (sel_ws.center.x == 0.f) {
        if (m_pos.y >= sel_vs.center.y) {
            options |= MIRROR_DOWN;
        } else {
            options |= MIRROR_UP;
        }
    } else if (sel_ws.center.y == 0.f) {
        if (m_pos.x >= sel_vs.center.x) {
            options |= MIRROR_RIGHT;
        } else {
            options |= MIRROR_LEFT;
        }
    } else {
        ASSERT(0, "Unreachable. New dimension?");
    }
    return options;
}

void render_player(Puzzle *p, Player player)
{
    float cell_width = p->rec.height / p->cols;
    Player vs_pos = vs_player_of_ws(p, player);
    Rectangle dims = {
        .x = vs_pos.pos.x,
        .y = vs_pos.pos.y,
        .width = cell_width,
        .height = cell_width,
    };
    Color color;
    switch (player.state) {
        case PHYSICAL: { color = PINK; } break;
        case PREVIEW: { color = GRAY; } break;
    }
    DrawRectangleRec(dims, color);
}

void render_selection(Puzzle *p, Button sel_ws)
{
    Button sel_vs = vs_button_of_ws(p, sel_ws);
    int options = get_mirror_direction(p, p->clicked_button);
    Rectangle rec = { 0 };
    if (options & MIRROR_LEFT) {
        rec.x = p->rec.x;
        rec.y = p->rec.y;
        rec.width = sel_vs.center.x - p->rec.x;
        rec.height = p->rec.height;
    } else if (options & MIRROR_RIGHT) {
        rec.x = sel_vs.center.x;
        rec.y = p->rec.y;
        rec.width = p->rec.x + p->rec.width - sel_vs.center.x;
        rec.height = p->rec.height;
    } else if (options & MIRROR_UP) {
        rec.x = p->rec.x;
        rec.y = p->rec.y;
        rec.width = p->rec.width;
        rec.height = sel_vs.center.y - p->rec.y;
    } else if (options & MIRROR_DOWN) {
        rec.x = p->rec.x;
        rec.y = sel_vs.center.y;
        rec.width = p->rec.width;
        rec.height = p->rec.y + p->rec.height - sel_vs.center.y ;
    }
    DrawRectangleRec(rec, M_BLUE);
}

void render_puzzle(Puzzle *p)
{
    int height = GetScreenHeight() - p->padding;
    int width = GetScreenWidth() - p->padding;
    ASSERT(width >= height);
    float cell_width = (float) height / p->cols;

    fill_buttons(p);


    int hover_id = button_hover_id(p, p->button_case);
    Button vs_btn = vs_button_of_ws(p, p->button_case[hover_id]);
    if (hover_id != -1 && p->clicked_button == -1) {
        DrawCircleV(vs_btn.center, 50.f, MAGENTA);
    }

    if (p->clicked_button == -1) {
        // Step 1: selecting button
        if (hover_id != -1 && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            // Move to next step
            p->clicked_button = hover_id;
        }
    } else {
        // Step 2: selecting side

        if (IsKeyPressed(KEY_C)) {
            // TODO: Is outside of rectangle
            p->clicked_button = -1;
        }

        int options = get_mirror_direction(p, p->clicked_button);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            // Finalize changes. Restart steps
            mirror_over_line(p, p->clicked_button, options);
            p->clicked_button = -1;
        } else {
            // Preview changes
            mirror_over_line(p, p->clicked_button, options | MIRROR_PREVIEW);
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
        render_player(p, p->player_case[i]);
    }
    for (i = 0; i < case_len(p->player_case); ++i) {
        if (p->player_case[i].state == PREVIEW) {
            case_len(p->player_case) = i;
        }
    }

    if (p->clicked_button != -1) {
        Button sel_ws = p->button_case[p->clicked_button];
        render_selection(p, sel_ws);
        render_button(p, sel_ws);
    } else {
        for (i = 0; i < case_len(p->button_case); ++i) {
            render_button(p, p->button_case[i]);
        }
    }
}
