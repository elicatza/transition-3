#ifndef PUZZLE_H
#define PUZZLE_H

#include <raylib.h>

/**
* Used to define puzzles
* View MASK_ namespace for more info
*/
#define P 0b0100
#define G 0b1000

#define PUZZEL_BUTTON_SZ 0.25f

typedef struct Puzzle Puzzle;

Puzzle *load_puzzle(unsigned char *bytes);
void update_puzzle(Puzzle *p);
void render_puzzle(Puzzle *p, Texture2D atlas);
void free_puzzle(Puzzle *p);

#endif
