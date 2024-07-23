#include "puzzle.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "case.h"
#include "core.h"

#define TEXTURE_BUTTON_OFFX 0
#define TEXTURE_BUTTON_OFFY 1

#define M_BLUE CLITERAL(Color){ 0x55, 0xcd, 0xfc, 100 }     // Blue


/**
 * First 2 bits are for height (0b00, 0b01, 0b10, 0b11)
 * Second 2 are for type (regular, player, goal, `reserved`) (0b00xx, 0b01xx, 0b10xx, 0b11xx)
 */
#define MASK_HEIGHT(a) ((a) & 0b11)
#define MASK_TYPE(a) ((a) & 0b1100)

#define MIRROR_UP (1 << 0)
#define MIRROR_DOWN (1 << 1)
#define MIRROR_LEFT (1 << 2)
#define MIRROR_RIGHT (1 << 3)
#define MIRROR_PREVIEW (1 << 4)

/**
 * Puzzle format
 * Header: width, height, padding
 * body cell map
 */
// static unsigned char puzzle1[] = { 
unsigned char puzzle_array[1][103] = {
    {
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
    },
};

typedef enum {
    MIRROR,
    SPLIT,
} ButtonKind;

typedef struct Button {
    Vector2 center;
    float radius;
    ButtonKind kind;
    bool is_highlighted;
} Button;

typedef enum {
    PREVIEW,
    PHYSICAL,
} PlayerState;

typedef struct {
    Vector2 pos;
    PlayerState state;
    unsigned char height;
} Player;

typedef struct {
    unsigned char info;  /* Texture offset for height  */
    Vector2 pos;           /* Stored in world space (ws) */
} Cell;

typedef struct Puzzle {
    Player *player_case;
    size_t rows;
    size_t cols;

    Cell *cell_case;
    Button *button_case;
    Rectangle rec;
    float padding;
    int clicked_button;  /* id if button is clicked. Else -1 */
    int hover_button;  /* id of hovered button. Else -1 */
} Puzzle;
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

// Note: not defined in stdlib when building for web
#ifdef PLATFORM_WEB
typedef int (*__compar_fn_t)(const void *, const void *);
#endif


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

/**
 * Converts from position in world space to position in view space
 */
Vector2 vs_pos_of_ws(Puzzle *p, Vector2 pos)
{
    float cell_width = p->rec.width / p->cols;
    return (Vector2) {
        .x = pos.x * cell_width + p->rec.x,
        .y = pos.y * cell_width + p->rec.y,
    };
}

/**
 * Converts from button in world space to button in view space
 */
Button vs_button_of_ws(Puzzle *p, Button btn)
{
    float cell_width = p->rec.width / p->cols;
    Button rt = btn;
    rt.center.x = btn.center.x * cell_width + p->rec.x;
    rt.center.y = btn.center.y * cell_width + p->rec.y;
    rt.radius = btn.radius * cell_width;
    return rt;
}

/**
 * Converts from cell in world space to cell in view space
 */
Cell vs_cell_of_ws(Puzzle *p, Cell cell)
{
    Cell rt = cell;
    rt.pos = vs_pos_of_ws(p, rt.pos);
    return rt;
}

/**
 * Converts from player in world space to player in view space
 */
Player vs_player_of_ws(Puzzle *p, Player player)
{
    Player rt = player;
    rt.pos = vs_pos_of_ws(p, rt.pos);
    return rt;
}

int cell_height_at_pos(Puzzle *p, Vector2 pos)
{
    return MASK_HEIGHT(p->cell_case[((int) (pos.y + 0.5f)) * p->cols + (int) (pos.x + 0.5f)].info);
}

/**
 * O(n) where n is the amount of players
 */
bool is_valid_pos(Puzzle *p, Player player)
{
    Vector2 pos = player.pos;
    if (!(pos.x >= 0 && pos.x < p->cols)) { return false; }
    if (!(pos.y >= 0 && pos.y < p->rows)) { return false; }
    if (cell_height_at_pos(p, pos) > player.height) { return false; }

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

void move_player(Puzzle *p, Player *player, Direction dir)
{
    // TODO play animation when can't move + when move
    Player new_player = *player;
    switch (dir) {
        case UP: { new_player.pos.y += -1; } break;
        case DOWN: { new_player.pos.y += 1; } break;
        case LEFT: { new_player.pos.x += -1; } break;
        case RIGHT: { new_player.pos.x += 1; } break;
            default: {
            WARNING("This function should not be run");
        } break;
    };

    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT) || IsKeyDown(KEY_U)) {
        new_player.height += 1;
    }
    // Check collisions
    if (is_valid_pos(p, new_player)) {
        new_player.height = cell_height_at_pos(p, new_player.pos);
        memcpy(player, &new_player, sizeof new_player);
    }
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
        if (CheckCollisionPointCircle(pos, vs_button.center, vs_button.radius)) {
            return i;
        }
    }
    return -1;
}

/**
 * @return check MIRROR_ namespace
 */
int get_mirror_direction(Puzzle *p)
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

/**
 * @param options. Check MIRROR_ namespace for options.
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

        Player mirrored_p;
        memcpy(&mirrored_p, &p->player_case[i], sizeof mirrored_p);
        mirrored_p.pos = mirrored;

        if (is_valid_pos(p, mirrored_p)) {
            mirrored_p.height = cell_height_at_pos(p, mirrored_p.pos);
            if (options & MIRROR_PREVIEW) {
                mirrored_p.state = PREVIEW;
                case_push(p->player_case, mirrored_p);
            } else {
                // In bound of puzzle
                mirrored_p.state = PHYSICAL;
                case_push(p->player_case, mirrored_p);
                INFO("added new at %.0f, %.0f", mirrored.x, mirrored.y);
            }
        }
    }
}

void update_puzzle(Puzzle *p)
{
    Direction dir;
    __compar_fn_t cmp_fn = NULL;

    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
        dir = UP;
        cmp_fn = players_cmp_ud;
    } else if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) {
        dir = LEFT;
        cmp_fn = players_cmp_lr;
    } else if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
        dir = DOWN;
        cmp_fn = players_cmp_du;
    } else if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) {
        dir = RIGHT;
        cmp_fn = players_cmp_rl;
    } else {
        dir = NONE;
    }
    if (dir != NONE) {
        qsort(p->player_case, case_len(p->player_case), sizeof *(p->player_case), cmp_fn);
        size_t i;
        for (i = 0; i < case_len(p->player_case); ++i) {
            move_player(p, &p->player_case[i], dir);
        }
    }

    p->hover_button = button_hover_id(p, p->button_case);

    if (p->hover_button != -1 && p->clicked_button == -1) {
        p->button_case[p->hover_button].is_highlighted = true;
    }

    if (p->clicked_button == -1) {
        // Step 1: selecting button
        if (p->hover_button != -1 && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            p->clicked_button = p->hover_button;
        }
    } else {
        // Step 2: selecting side

        int options = get_mirror_direction(p);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            // Write changes
            if (CheckCollisionPointRec(GetMousePosition(), p->rec)) {
                mirror_over_line(p, p->clicked_button, options);
            }
            p->clicked_button = -1;
        } else {
            // Preview changes
            if (CheckCollisionPointRec(GetMousePosition(), p->rec)) {
                mirror_over_line(p, p->clicked_button, options | MIRROR_PREVIEW);
            }
        }
    }

}

void render_button(Puzzle *p, Button *btn, Texture2D atlas)
{
    Button vs_btn = vs_button_of_ws(p, *btn);
    Rectangle src = {
        .x = 8.f * TEXTURE_BUTTON_OFFX + 0.1f, .y = 8.f * TEXTURE_BUTTON_OFFY + 0.1f,
        .width = 7.8f, .height = 7.8f,
    };
    Rectangle dest = {
        .x = vs_btn.center.x, .y = vs_btn.center.y,
        .width = vs_btn.radius * 2.f, .height = vs_btn.radius * 2.f,
    };
    if (btn->is_highlighted) {
        DrawTexturePro(atlas, src, dest, (Vector2) { vs_btn.radius, vs_btn.radius} , 45.f, WHITE);
        btn->is_highlighted = 0;
    } else {
        DrawTexturePro(atlas, src, dest, (Vector2) { vs_btn.radius, vs_btn.radius} , 0.f, WHITE);
    }
}

void render_height_lines(Puzzle *p)
{
    size_t row, col;
    float cell_width = p->rec.width / p->cols;
    for (row = 0; row < p->rows; ++row) {
        for (col = 0; col < p->cols; ++col) {
            Cell c = p->cell_case[row * p->cols + col];

            if (case_len(p->cell_case) > (row + 1) * p->cols + col) {
                // Has cell down
                Cell cd = p->cell_case[(row + 1) * p->cols + col];
                if (cd.pos.x != c.pos.x) continue;
                if (MASK_HEIGHT(c.info) != MASK_HEIGHT(cd.info)) {
                    Vector2 start = vs_pos_of_ws(p, cd.pos);
                    Vector2 end = { start.x + cell_width, start. y};
                    float diff = fabsf((float) MASK_HEIGHT(c.info) - MASK_HEIGHT(cd.info));
                    DrawLineEx(start, end, diff * 2.f, RED);
                }
            }

            if (case_len(p->cell_case) > row * p->cols + col + 1) {
                // Has cell Right
                Cell cr = p->cell_case[row * p->cols + col + 1];
                if (cr.pos.y != c.pos.y) continue;
                if (MASK_HEIGHT(c.info) != MASK_HEIGHT(cr.info)) {
                    Vector2 start = vs_pos_of_ws(p, cr.pos);
                    Vector2 end = { start.x, start.y + cell_width};
                    float diff = fabsf((float) MASK_HEIGHT(c.info) - MASK_HEIGHT(cr.info));
                    DrawLineEx(start, end, diff * 2.f, RED);
                }
            }

        }
    }
}

/**
 * Should be lighter based on hight / darker based on depth
 */
void render_player(Puzzle *p, Player player);


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
    int options = get_mirror_direction(p);
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

void render_cell(Puzzle *p, Cell cell, Texture2D atlas)
{
    Cell vs_cell = vs_cell_of_ws(p, cell);
    float cell_width = p->rec.width / p->cols;
    Rectangle src = {
        .x = 8.f * MASK_HEIGHT(cell.info), .y = 0.f,
        .width = 8.f, .height = 8.f,
    };
    Rectangle dest = {
        .x = vs_cell.pos.x, .y = vs_cell.pos.y,
        .width = cell_width, .height = cell_width,
    };
    DrawTexturePro(atlas, src, dest, (Vector2) { 0.f, 0.f} , 0.f, WHITE);
    if (MASK_TYPE(cell.info) == G) {
        DrawRectangleRec(dest, RED);
    }
}

void render_puzzle_grid(Puzzle *p)
{
    // Draw rows
    float cell_width = p->rec.height / p->cols;
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
        DrawLineEx(start, end, 1.f, BLACK);
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
        DrawLineEx(start, end, 1.f, BLACK);
    }
}

void render_puzzle(Puzzle *p, Texture2D atlas)
{
    int height = GetScreenHeight() - p->padding;
    int width = GetScreenWidth() - p->padding;
    ASSERT(width >= height);

    p->rec = (Rectangle) {
        .width = height,
        .height = height,
        .x = (width - height) / 2.f,
        .y = p->padding / 2.f,
    };

    // Draw cells
    size_t i;
    for (i = 0; i < case_len(p->cell_case); ++i) {
        render_cell(p, p->cell_case[i], atlas);
    }

    // Draws players
    for (i = 0; i < case_len(p->player_case); ++i) {
        render_player(p, p->player_case[i]);
    }
    for (i = 0; i < case_len(p->player_case); ++i) {
        if (p->player_case[i].state == PREVIEW) {
            case_len(p->player_case) = i;
        }
    }

    render_puzzle_grid(p);

    render_height_lines(p);

    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT) || IsKeyDown(KEY_U)) {
        DrawRectangleLinesEx(p->rec, 3.f, RED);
    } else {
        DrawRectangleLinesEx(p->rec, 3.f, GREEN);
    }

    if (p->clicked_button != -1) {
        Button sel_ws = p->button_case[p->clicked_button];
        if (CheckCollisionPointRec(GetMousePosition(), p->rec)) {
            render_selection(p, sel_ws);
        }
        render_button(p, &sel_ws, atlas);
    } else {
        for (i = 0; i < case_len(p->button_case); ++i) {
            render_button(p, &p->button_case[i], atlas);
        }
    }
}

void fill_cells(Puzzle *p, unsigned char *puzzle_body)
{
    case_len(p->cell_case) = 0;

    size_t row, col;
    for (row = 0; row < p->rows; ++row) {
        for (col = 0; col < p->cols; ++col) {
            Cell cell = {
                .pos = (Vector2) {
                    .x = col,
                    .y = row,
                },
                .info = puzzle_body[row * p->cols + col],
            };
            case_push(p->cell_case, cell);

        }
    }
}

void fill_players(Puzzle *p, unsigned char *puzzle_body)
{
    case_len(p->player_case) = 0;

    size_t row, col;
    for (row = 0; row < p->rows; ++row) {
        for (col = 0; col < p->cols; ++col) {
            if ((puzzle_body[row * p->cols + col] & 0b1100) == P) {
                INFO("Found player");
                Player player = {
                    .pos = (Vector2) {
                        .x = col,
                        .y = row,
                    },
                    .height = puzzle_body[row * p->cols + col],
                    .state = PHYSICAL,
                };
                case_push(p->player_case, player);
            }
        }
    }
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
            .is_highlighted = false,
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

Puzzle *load_puzzle(unsigned char *bytes)
{
    Puzzle *p = malloc(sizeof *p);
    p->clicked_button = -1;
    p->hover_button = -1;
    p->cols = bytes[0];
    p->rows = bytes[1];
    p->padding = bytes[2];
    p->cell_case = case_init(p->cols * p->rows, sizeof *p->cell_case);
    p->player_case = case_init(64, sizeof *p->player_case);
    p->button_case = case_init(p->cols + p->rows, sizeof *p->button_case);

    fill_cells(p, &bytes[3]);
    fill_players(p, &bytes[3]);
    fill_buttons(p);

    return p;
}

void free_puzzle(Puzzle *p)
{
    case_free(p->button_case);
    case_free(p->player_case);
    case_free(p->cell_case);
    free(p);
}

// TODO:
// Consider adding cell_width to Puzzle object
// Does Direction need enumeration
