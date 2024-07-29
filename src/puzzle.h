#ifndef PUZZLE_H
#define PUZZLE_H

#include <raylib.h>
#include "core.h"

/**
* Used to define puzzles
* View MASK_ namespace for more info
*/

#define PUZZEL_BUTTON_SZ 0.25f

typedef struct Puzzle Puzzle;

Puzzle *load_puzzle(unsigned char *bytes);
GameState update_puzzle(Puzzle *p, PlayerState *pstate, GameState default_rv);
void render_puzzle(Puzzle *p, PlayerState pstate, Texture2D atlas, Texture2D player_atlas, Shader fs);
void free_puzzle(Puzzle *p);

void render_puzzle_win(Puzzle *p, PlayerState *pstate, Texture2D atlas, Texture2D player_atlas, Shader fs);
GameState update_puzzle_win(Puzzle *p, GameState default_rv);

#ifndef NO_TEMPLATE
#define P 0b0100
#define G 0b1000
#endif

extern unsigned char puzzle_fun_array[2][103];
extern unsigned char puzzle_train_array[2][103];
extern unsigned char puzzle_boss[20 * 20 + 3];


#endif
