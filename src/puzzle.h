#ifndef PUZZLE_H
#define PUZZLE_H

#include <stddef.h>
#include <raylib.h>

/**
 * First 2 bits are for height (0b00, 0b01, 0b10, 0b11)
 * Second 2 are for type (regular, player, goal, `reserved`) (0b00xx, 0b01xx, 0b10xx, 0b11xx)
 */
#define MASK_HEIGHT(a) ((a) & 0b11)
#define MASK_TYPE(a) ((a) & 0b1100)
#define P 0b0100
#define G 0b1000

#define MIRROR_UP (1 << 0)
#define MIRROR_DOWN (1 << 1)
#define MIRROR_LEFT (1 << 2)
#define MIRROR_RIGHT (1 << 3)
#define MIRROR_PREVIEW (1 << 4)

#define PUZZEL_BUTTON_SZ 0.25f
#define TEXTURE_BUTTON_OFFX 0
#define TEXTURE_BUTTON_OFFY 1

#define M_BLUE       CLITERAL(Color){ 0x55, 0xcd, 0xfc, 100 }     // Blue



typedef enum {
    MIRROR,
    SPLIT,
} ButtonKind;

typedef struct {
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

/**
 * All values are to be defined in world space
 * Translations are done to convert to view space
 */
typedef struct {
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


void update_puzzle(Puzzle *p);
void render_puzzle(Puzzle *p, Texture2D atlas);
Puzzle puzzle_load(unsigned char *bytes);

#endif
