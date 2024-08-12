#include "game_setup.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "game.h"

// Some handy dandy macros for decompression
#define E_CAP_HEX 0x45
#define E_LOW_HEX 0x65
#define G_CAP_HEX 0x47
#define G_LOW_HEX 0x67
#define S_CAP_HEX 0x53
#define S_LOW_HEX 0x73
#define W_CAP_HEX 0x57
#define W_LOW_HEX 0x77
#define DIGIT_START 0x30
#define DIGIT_END 0x39

/** Initializes the board with walls around the edge of the board.
 *
 * Modifies values pointed to by cells_p, width_p, and height_p and initializes
 * cells array to reflect this default board.
 *
 * Returns INIT_SUCCESS to indicate that it was successful.
 *
 * Arguments:
 *  - cells_p: a pointer to a memory location where a pointer to the first
 *             element in a newly initialized array of cells should be stored.
 *  - width_p: a pointer to a memory location where the newly initialized
 *             width should be stored.
 *  - height_p: a pointer to a memory location where the newly initialized
 *              height should be stored.
 */
enum board_init_status initialize_default_board(int** cells_p, size_t* width_p,
                                                size_t* height_p) {
    *width_p = 20;
    *height_p = 10;
    int* cells = malloc(20 * 10 * sizeof(int));
    *cells_p = cells;
    for (int i = 0; i < 20 * 10; i++) {
        cells[i] = PLAIN_CELL;
    }

    // Set edge cells!
    // Top and bottom edges:
    for (int i = 0; i < 20; ++i) {
        cells[i] = FLAG_WALL;
        cells[i + (20 * (10 - 1))] = FLAG_WALL;
    }
    // Left and right edges:
    for (int i = 0; i < 10; ++i) {
        cells[i * 20] = FLAG_WALL;
        cells[i * 20 + 20 - 1] = FLAG_WALL;
    }

    // Set grass cells!
    // Top and bottom edges:
    for (int i = 1; i < 19; ++i) {
        cells[i + 20] = FLAG_GRASS;
        cells[i + (20 * (9 - 1))] = FLAG_GRASS;
    }
    // Left and right edges:
    for (int i = 1; i < 9; ++i) {
        cells[i * 20 + 1] = FLAG_GRASS;
        cells[i * 20 + 19 - 1] = FLAG_GRASS;
    }

    // Add snake
    cells[20 * 2 + 2] = FLAG_SNAKE;

    return INIT_SUCCESS;
}

/** Initialize variables relevant to the game board.
 * Arguments:
 *  - cells_p: a pointer to a memory location where a pointer to the first
 *             element in a newly initialized array of cells should be stored.
 *  - width_p: a pointer to a memory location where the newly initialized
 *             width should be stored.
 *  - height_p: a pointer to a memory location where the newly initialized
 *              height should be stored.
 *  - snake_p: a pointer to your snake struct (not used until part 3!)
 *  - board_rep: a string representing the initial board. May be NULL for
 * default board.
 */
enum board_init_status initialize_game(int** cells_p, size_t* width_p,
                                       size_t* height_p, snake_t* snake_p,
                                       char* board_rep) {
    // TODO: implement!
    g_game_over = 0;
    g_score = 0;
    snake_position = 20 * 2 + 2;
    direction = RIGHT;

    enum board_init_status initBoardStatus;
    if (board_rep != NULL) {
        initBoardStatus = decompress_board_str(cells_p, width_p, height_p,
                                               snake_p, board_rep);
    } else {
        initBoardStatus = initialize_default_board(cells_p, width_p, height_p);
    }
    if (initBoardStatus == INIT_SUCCESS) {
        place_food(*cells_p, *width_p, *height_p);
    }
    return initBoardStatus;
}

/** Takes in a string `compressed` and initializes values pointed to by
 * cells_p, width_p, and height_p accordingly. Arguments:
 *      - cells_p: a pointer to the pointer representing the cells array
 *                 that we would like to initialize.
 *      - width_p: a pointer to the width variable we'd like to initialize.
 *      - height_p: a pointer to the height variable we'd like to initialize.
 *      - snake_p: a pointer to your snake struct (not used until part 3!)
 *      - compressed: a string that contains the representation of the board.
 * Note: We assume that the string will be of the following form:
 * B24x80|E5W2E73|E5W2S1E72... To read it, we scan the string row-by-row
 * (delineated by the `|` character), and read out a letter (E, S or W) a number
 * of times dictated by the number that follows the letter.
 */
enum board_init_status decompress_board_str(int** cells_p, size_t* width_p,
                                            size_t* height_p, snake_t* snake_p,
                                            char* compressed) {
    // TODO: implement!
    int i = 0, count = 0;
    while (compressed[i] != '\0') {
        if (compressed[i] == '|') {
            count += 1;
        }
        ++i;
    }

    const char delim[] = "x|";

    // complete judge before 'x'
    char* token = strtok(compressed, delim);
    if (token[0] != 'B') {
        return INIT_ERR_BAD_CHAR;
    }
    token += 1;
    if (token[0] - '0' != count) {
        return INIT_ERR_INCORRECT_DIMENSIONS;
    }
    *height_p = count;

    // save the board's width at the begining
    token = strtok(NULL, delim);
    *width_p = atoi(token);
    // if 'x' does not exist, return error
    if (*width_p == 0) {
        return INIT_ERR_BAD_CHAR;
    }

    *cells_p = malloc((*width_p) * (*height_p) * sizeof(int));

    // B7x10|W10|W1E4W5|W2E5G2W1|W1E8W1|W1E4W1E3W1|W1E2S1E1W1E3W1|W10
    // initially token = 'w10',then token = 'W1E4W5'...
    int has_snake = 0;    // number of snakes
    int current_row = 0;  // current operating row
    token = strtok(NULL, delim);
    while (token != NULL) {
        int current_column = 0;  // current operating column
        int j = 0;
        char current_alpha;
        while (token[j] != '\0') {
            if (is_valid_alphabet(token[j])) {
                current_alpha = token[j];
            } else {
                return INIT_ERR_BAD_CHAR;
            }
            ++j;
            int num = 0;  // the number of each wall/grass/empty/snake
            /**
             * Code below is buggy! need fixed! num calculation is wrong and while condition is wrong!
            while (!is_valid_alphabet(token[j]) && token[j] != '\0') {
                num += token[j] - '0';
                ++j;
            }
            */
            if (current_alpha == 'S') {
                if (num != 1 || has_snake == 1) {
                    return INIT_ERR_WRONG_SNAKE_NUM;
                }
                has_snake = 1;
                initialize(current_alpha, num, current_row, current_column,
                           cells_p, width_p, height_p);
            } else {
                initialize(current_alpha, num, current_row, current_column,
                           cells_p, width_p, height_p);
            }
            current_column += num;
        }
        if (current_column != (int)*width_p) {
            return INIT_ERR_INCORRECT_DIMENSIONS;
        }
        token = strtok(NULL, delim);
        current_row += 1;
    }
    return INIT_SUCCESS;
}

int is_valid_alphabet(char alpha) {
    if (alpha == 'W' || alpha == 'E' || alpha == 'S' || alpha == 'G') {
        return 1;
    }
    return 0;
}

void initialize(char alpha, int num, int row, int column, int** cells_p,
                size_t* width_p, size_t* height_p) {
    int* cells = *cells_p;
    if (alpha == 'W') {
        for (int i = 0; i < num; ++i) {
            cells[row * *width_p + column + i] = FLAG_WALL;
        }
    }
    if (alpha == 'E') {
        for (int i = 0; i < num; ++i) {
            cells[row * *width_p + column + i] = PLAIN_CELL;
        }
    }
    if (alpha == 'G') {
        for (int i = 0; i < num; ++i) {
            cells[row * *width_p + column + i] = FLAG_GRASS;
        }
    }
    if (alpha == 'S') {
        cells[row * *width_p + column] = FLAG_SNAKE;
    }
}