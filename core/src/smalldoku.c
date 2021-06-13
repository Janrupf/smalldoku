#include "smalldoku/smalldoku.h"

/**
 * Checks whether a row already contains a number.
 *
 * @param grid the grid to perform the check on
 * @param row the row to check the number for
 * @param number the number to check for
 * @return 1 if the row already contains the number, 0 otherwise
 */
static int row_contains(SMALLDOKU_GRID(grid), smalldoku_uint8_t row, smalldoku_uint8_t number) {
    for (smalldoku_uint8_t col = 0; col < SMALLDOKU_GRID_WIDTH; col++) {
        if (smalldoku_get_cell_value(grid, row, col) == number) {
            return 1;
        }
    }

    return 0;
}

/**
 * Checks whether a column already contains a number.
 *
 * @param grid the grid to perform the check on
 * @param row the column to check the number for
 * @param number the number to check for
 * @return 1 if the column already contains the number, 0 otherwise
 */
static int column_contains(SMALLDOKU_GRID(grid), smalldoku_uint8_t col, smalldoku_uint8_t number) {
    for (smalldoku_uint8_t row = 0; row < SMALLDOKU_GRID_HEIGHT; row++) {
        if (smalldoku_get_cell_value(grid, row, col) == number) {
            return 1;
        }
    }

    return 0;
}

/**
 * Checks whether a square already contains a number.
 *
 * @param grid the grid to perform the check on
 * @param row the row to find the square for
 * @param col the column to find the square for
 * @param number the number to check for
 * @return 1 if the square already contains the number, 0 otherwise
 */
static int
square_contains(SMALLDOKU_GRID(grid), smalldoku_uint8_t row, smalldoku_uint8_t col, smalldoku_uint8_t number) {
    smalldoku_uint8_t square_index = (col / SMALLDOKU_SQUARE_WIDTH) +
                                     (row / SMALLDOKU_SQUARE_WIDTH) * (SMALLDOKU_GRID_WIDTH / SMALLDOKU_SQUARE_WIDTH);
    smalldoku_uint8_t square_start_cell =
            ((square_index % SMALLDOKU_SQUARE_WIDTH) * SMALLDOKU_SQUARE_WIDTH) /* <- row offset */
            + ((square_index / SMALLDOKU_SQUARE_WIDTH) *
               (SMALLDOKU_GRID_WIDTH * SMALLDOKU_SQUARE_HEIGHT)); /* column offset */

    for (smalldoku_uint8_t square_y = 0; square_y < SMALLDOKU_SQUARE_HEIGHT; square_y++) {
        for (smalldoku_uint8_t square_x = 0; square_x < SMALLDOKU_SQUARE_WIDTH; square_x++) {
            smalldoku_uint8_t current_cell_index = square_start_cell + square_x + (square_y * SMALLDOKU_GRID_WIDTH);

            smalldoku_uint8_t current_row = current_cell_index / SMALLDOKU_GRID_WIDTH;
            smalldoku_uint8_t current_col = current_cell_index % SMALLDOKU_GRID_HEIGHT;

            if (smalldoku_get_cell_value(grid, current_row, current_col) == number) {
                return 1;
            }
        }
    }

    return 0;
}

/**
 * Determines whether the grid is filled completely.
 *
 * @param grid the grid to check
 * @return 1 if the grid is filled completely, 0 otherwise
 */
static int grid_filled(SMALLDOKU_GRID(grid)) {
    for (smalldoku_uint8_t row = 0; row < SMALLDOKU_GRID_HEIGHT; row++) {
        for (smalldoku_uint8_t col = 0; col < SMALLDOKU_GRID_WIDTH; col++) {
            if (smalldoku_get_cell_value(grid, row, col) == 0) {
                return 0;
            }
        }
    }

    return 1;
}

/**
 * Shuffles an array using a random function.
 *
 * @param array the array to shuffle
 * @param size the size of the array
 * @param rng the random function to use
 */
static void shuffle(smalldoku_uint8_t *array, smalldoku_uint8_t size, smalldoku_rng_fn rng) {
    for (smalldoku_uint8_t i = 0; i < size; i++) {
        smalldoku_uint8_t new_i = rng(0, size - 1);

        if (new_i != i) {
            smalldoku_uint8_t tmp = array[i];
            array[i] = array[new_i];
            array[new_i] = tmp;
        }
    }
}

/**
 * Copies the grid.
 *
 * @param dst the grid to copy to
 * @param src the grid to copy from
 */
static void copy_grid(SMALLDOKU_GRID(dst), SMALLDOKU_GRID(src)) {
    for (smalldoku_uint8_t row = 0; row < SMALLDOKU_GRID_HEIGHT; row++) {
        for (smalldoku_uint8_t col = 0; col < SMALLDOKU_GRID_WIDTH; col++) {
            dst[row][col] = src[row][col];
        }
    }
}

/**
 * Recursive function to fill a grid completely.
 *
 * @param grid the grid to fill
 * @param rng the random function to use
 * @return 1 if the grid could be filled, 0 otherwise
 */
static int fill_grid_internal(SMALLDOKU_GRID(grid), smalldoku_rng_fn rng) {
    SMALLDOKU_GRID(backup);
    copy_grid(backup, grid);

    for (smalldoku_uint8_t cell_index = 0;
         cell_index < SMALLDOKU_GRID_WIDTH * SMALLDOKU_GRID_HEIGHT; cell_index++) {
        smalldoku_uint8_t row = cell_index / SMALLDOKU_GRID_WIDTH;
        smalldoku_uint8_t col = cell_index % SMALLDOKU_GRID_HEIGHT;

        if (grid[row][col].value == 0) {
            smalldoku_uint8_t numbers[SMALLDOKU_GRID_WIDTH];
            for (smalldoku_uint8_t i = 1; i <= SMALLDOKU_GRID_WIDTH; i++) {
                numbers[i - 1] = i;
            }
            shuffle(numbers, SMALLDOKU_GRID_WIDTH, rng);

            for (smalldoku_uint8_t number_index = 0; number_index < SMALLDOKU_GRID_WIDTH; number_index++) {
                smalldoku_uint8_t cell_value = numbers[number_index];

                if (
                        !row_contains(grid, row, cell_value) &&
                        !column_contains(grid, col, cell_value) &&
                        !square_contains(grid, row, col, cell_value)
                        ) {
                    grid[row][col].value = cell_value;

                    if (grid_filled(grid) || fill_grid_internal(grid, rng)) {
                        return 1;
                    }
                }
            }

            break;
        }
    }

    copy_grid(grid, backup);
    return 0;
}

static int solve_grid_internal(SMALLDOKU_GRID(grid), smalldoku_uint32_t *solve_count, smalldoku_uint8_t start_cell_index) {
    for (smalldoku_uint8_t cell_index = start_cell_index;
         cell_index < SMALLDOKU_GRID_WIDTH * SMALLDOKU_GRID_HEIGHT; cell_index++) {
        smalldoku_uint8_t row = cell_index / SMALLDOKU_GRID_WIDTH;
        smalldoku_uint8_t col = cell_index % SMALLDOKU_GRID_HEIGHT;

        if (smalldoku_get_cell_value(grid, row, col) != 0) {
            continue;
        }

        for (smalldoku_uint8_t i = 1; i <= 9; i++) {
            if (
                    !row_contains(grid, row, i) &&
                    !column_contains(grid, col, i) &&
                    !square_contains(grid, row, col, i)
                    ) {

                if (grid[row][col].type == SMALLDOKU_GENERATED_CELL) {
                    grid[row][col].value = i;
                } else {
                    grid[row][col].user_value = i;
                }

                if (grid_filled(grid)) {
                    (*solve_count)++;
                } else {
                    solve_grid_internal(grid, solve_count, cell_index + 1);
                }

                if (grid[row][col].type == SMALLDOKU_GENERATED_CELL) {
                    grid[row][col].value = 0;
                } else {
                    grid[row][col].user_value = 0;
                }
            }
        }
    }

    return 0;
}

__attribute__((unused)) static void print_grid(SMALLDOKU_GRID(grid), void(*printf)(const char *fmt, ...)) {
    printf("   ");
    for (smalldoku_uint8_t x = 0; x < SMALLDOKU_GRID_WIDTH; x++) {
        printf("%d ", x);
    }
    printf("\n");
    printf("  ┏");
    for (smalldoku_uint8_t x = 0; x < SMALLDOKU_GRID_WIDTH; x++) {
        printf("━");

        if (x != SMALLDOKU_GRID_WIDTH - 1) {
            if (x != 0 && ((x + 1) % SMALLDOKU_SQUARE_WIDTH) == 0) {
                printf("┳");
            } else {
                printf("┯");
            }
        }
    }
    printf("┓\n");

    for (smalldoku_uint8_t row = 0; row < SMALLDOKU_GRID_HEIGHT; row++) {
        printf("%d ┃", row);
        for (smalldoku_uint8_t col = 0; col < SMALLDOKU_GRID_WIDTH; col++) {
            printf("%d", smalldoku_get_cell_value(grid, row, col));

            if (((col + 1) % SMALLDOKU_SQUARE_WIDTH) == 0) {
                printf("┃");
            } else {
                printf("│");
            }
        }
        printf("\n");
        if (row != SMALLDOKU_GRID_HEIGHT - 1) {
            printf("  ┠");
            for (smalldoku_uint8_t col = 0; col < SMALLDOKU_GRID_WIDTH; col++) {
                if ((row + 1) % SMALLDOKU_SQUARE_HEIGHT == 0) {
                    printf("━");
                    if (col != SMALLDOKU_GRID_WIDTH - 1) {
                        if (((col + 1) % SMALLDOKU_SQUARE_WIDTH) == 0) {
                            printf("╋");
                        } else {
                            printf("┿");
                        }
                    }
                } else {
                    printf("─");
                    if (col != SMALLDOKU_GRID_WIDTH - 1) {
                        if (((col + 1) % SMALLDOKU_SQUARE_WIDTH) == 0) {
                            printf("╂");
                        } else {
                            printf("┼");
                        }
                    }
                }
            }
            printf("┨\n");
        }
    }

    printf("  ┗");
    for (smalldoku_uint8_t x = 0; x < SMALLDOKU_GRID_WIDTH; x++) {
        printf("━");

        if (x != SMALLDOKU_GRID_WIDTH - 1) {
            if (x != 0 && ((x + 1) % SMALLDOKU_SQUARE_WIDTH) == 0) {
                printf("┻");
            } else {
                printf("┷");
            }
        }
    }
    printf("┛\n");
}

void smalldoku_init(SMALLDOKU_GRID(grid)) {
    for (smalldoku_uint8_t row = 0; row < SMALLDOKU_GRID_HEIGHT; row++) {
        for (smalldoku_uint8_t col = 0; col < SMALLDOKU_GRID_WIDTH; col++) {
            smalldoku_cell_t cell = {SMALLDOKU_GENERATED_CELL, 0, 0, 0};
            grid[row][col] = cell;
        }
    }
}

void smalldoku_fill_grid(SMALLDOKU_GRID(grid), smalldoku_rng_fn rng) {
    fill_grid_internal(grid, rng);
}

void smalldoku_hammer_grid(SMALLDOKU_GRID(grid), smalldoku_uint8_t erase_count, smalldoku_rng_fn rng) {
    for(smalldoku_uint8_t c = 0; c < erase_count; c++) {
        while (1) {
            smalldoku_uint8_t to_erase = rng(0, 80);

            smalldoku_uint8_t row = to_erase / SMALLDOKU_GRID_WIDTH;
            smalldoku_uint8_t col = to_erase % SMALLDOKU_GRID_HEIGHT;

            if(grid[row][col].type == SMALLDOKU_GENERATED_CELL) {
                grid[row][col].type = SMALLDOKU_USER_CELL;
                grid[row][col].user_value = 0;

                if(smalldoku_solve_grid(grid) == 1) {
                    break;
                } else {
                    grid[row][col].type = SMALLDOKU_GENERATED_CELL;
                }
            }
        }
    }
}

smalldoku_uint32_t smalldoku_solve_grid(SMALLDOKU_GRID(grid)) {
    smalldoku_uint32_t solve_count = 0;
    solve_grid_internal(grid, &solve_count, 0);

    return solve_count;
}

smalldoku_uint8_t smalldoku_get_cell_value(SMALLDOKU_GRID(grid), smalldoku_uint8_t row, smalldoku_uint8_t col) {
    smalldoku_cell_t cell = grid[row][col];

    switch (cell.type) {
        case SMALLDOKU_GENERATED_CELL:
            return cell.value;

        case SMALLDOKU_USER_CELL:
            return cell.user_value;
    }

    __asm__("ud2");
    return -1;
}
