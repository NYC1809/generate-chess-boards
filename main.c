#include "main.h"
#include "webserver.h"

#include <string.h>

static void set_piece(PuzzlePosition *position, int row, int col, const char *piece_code) {
    strncpy(position->squares[row][col], piece_code, sizeof(position->squares[row][col]) - 1);
    position->squares[row][col][sizeof(position->squares[row][col]) - 1] = '\0';
}

PuzzlePosition generate_puzzle_position(void) {
    PuzzlePosition position;
    int row;
    int col;

    for (row = 0; row < 8; row++) {
        for (col = 0; col < 8; col++) {
            set_piece(&position, row, col, "--");
        }
    }
    return position;
}

int main(void) {
    return run_webserver();
}
