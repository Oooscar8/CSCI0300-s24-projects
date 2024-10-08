#include "game.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "linked_list.h"
#include "mbstrings.h"

/** Updates the game by a single step, and modifies the game information
 * accordingly. Arguments:
 *  - cells: a pointer to the first integer in an array of integers representing
 *    each board cell.
 *  - width: width of the board.
 *  - height: height of the board.
 *  - snake_p: pointer to your snake struct (not used until part 3!)
 *  - input: the next input.
 *  - growing: 0 if the snake does not grow on eating, 1 if it does.
 */
void update(int* cells, size_t width, size_t height, snake_t* snake_p,
            enum input_key input, int growing) {
    // `update` should update the board, your snake's data, and global
    // variables representing game information to reflect new state. If in the
    // updated position, the snake runs into a wall or itself, it will not move
    // and global variable g_game_over will be 1. Otherwise, it will be moved
    // to the new position. If the snake eats food, the game score (`g_score`)
    // increases by 1. This function assumes that the board is surrounded by
    // walls, so it does not handle the case where a snake runs off the board.

    // TODO: implement!

    // update g_game_over, g_score, snake_position, direction
    if (g_game_over == 1) {
        return;
    }

    switch (input) {
        case INPUT_RIGHT:
            if (snake_p->direction == LEFT &&
                length_list(snake_p->snake_position_list) >= 2) {
                break;
            }
            snake_p->direction = RIGHT;
            break;
        case INPUT_LEFT:
            if (snake_p->direction == RIGHT &&
                length_list(snake_p->snake_position_list) >= 2) {
                break;
            }
            snake_p->direction = LEFT;
            break;
        case INPUT_UP:
            if (snake_p->direction == DOWN &&
                length_list(snake_p->snake_position_list) >= 2) {
                break;
            }
            snake_p->direction = UP;
            break;
        case INPUT_DOWN:
            if (snake_p->direction == UP &&
                length_list(snake_p->snake_position_list) >= 2) {
                break;
            }
            snake_p->direction = DOWN;
            break;
        default:
            break;
    }

    int next;
    switch (snake_p->direction) {
        case RIGHT:
            next = *(int*)snake_p->snake_position_list->data + 1;
            break;
        case LEFT:
            next = *(int*)(snake_p->snake_position_list->data) - 1;
            break;
        case UP:
            next = *(int*)(snake_p->snake_position_list->data) - width;
            break;
        case DOWN:
            next = *(int*)(snake_p->snake_position_list->data) + width;
            break;
    }

    if (cells[next] == FLAG_WALL) {
        g_game_over = 1;
        return;
    }

    node_t* iterator = snake_p->snake_position_list;
    while (iterator->next) {
        if (next == *(int*)iterator->data) {
            g_game_over = 1;
            return;
        }
        iterator = iterator->next;
    }

    insert_first(&snake_p->snake_position_list, &next, sizeof(int));

    if (cells[next] & FLAG_FOOD) {
        place_food(cells, width, height);
        g_score += 1;
    }
    if (!(cells[next] & FLAG_FOOD) || growing == 0) {
        int* last_position = remove_last(&snake_p->snake_position_list);
        cells[*last_position] ^= FLAG_SNAKE;  // snake leave the cell
        free(last_position);
    }

    if (cells[next] & FLAG_GRASS) {
        cells[next] = FLAG_SNAKE | FLAG_GRASS;
    } else {
        cells[next] = FLAG_SNAKE;
    }
}

/** Sets a random space on the given board to food.
 * Arguments:
 *  - cells: a pointer to the first integer in an array of integers representing
 *    each board cell.
 *  - width: the width of the board
 *  - height: the height of the board
 */
void place_food(int* cells, size_t width, size_t height) {
    /* DO NOT MODIFY THIS FUNCTION */
    unsigned food_index = generate_index(width * height);
    // check that the cell is empty or only contains grass
    if ((*(cells + food_index) == PLAIN_CELL) ||
        (*(cells + food_index) == FLAG_GRASS)) {
        *(cells + food_index) |= FLAG_FOOD;
    } else {
        place_food(cells, width, height);
    }
    /* DO NOT MODIFY THIS FUNCTION */
}

/** Prompts the user for their name and saves it in the given buffer.
 * Arguments:
 *  - `write_into`: a pointer to the buffer to be written into.
 */
void read_name(char* write_into) {
    // TODO: implement! (remove the call to strcpy once you begin your
    // implementation)
    while (true) {
        printf("Name > ");
        fflush(0);
        int bytes = (int)read(0, write_into, 1000);
        if (bytes > 1) {
            write_into[bytes - 1] = '\0';
            break;
        }
        printf("\nName Invalid: must be longer than 0 characters.\n");
    }
}

/** Cleans up on game over — should free any allocated memory so that the
 * LeakSanitizer doesn't complain.
 * Arguments:
 *  - cells: a pointer to the first integer in an array of integers representing
 *    each board cell.
 *  - snake_p: a pointer to your snake struct. (not needed until part 3)
 */
void teardown(int* cells, snake_t* snake_p) {
    // TODO: implement!
    free(cells);
    while (snake_p->snake_position_list) {
        free(remove_last(&snake_p->snake_position_list));
    }
}
