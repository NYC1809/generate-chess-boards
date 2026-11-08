#ifndef MAIN_H
#define MAIN_H

typedef struct {
    char squares[8][8][3];
} PuzzlePosition;

PuzzlePosition generate_puzzle_position(void);

#endif
