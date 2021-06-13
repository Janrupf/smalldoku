#pragma once

#define SMALLDOKU_GRID_WIDTH 9
#define SMALLDOKU_GRID_HEIGHT 9
#define SMALLDOKU_SQUARE_WIDTH 3
#define SMALLDOKU_SQUARE_HEIGHT 3
#define SMALLDOKU_GRID(x) smalldoku_cell_t x[SMALLDOKU_GRID_WIDTH][SMALLDOKU_GRID_HEIGHT]

typedef signed char smalldoku_int8_t;
typedef signed short smalldoku_int16_t;
typedef signed int smalldoku_int32_t;
typedef signed long smalldoku_int64_t;

typedef unsigned char smalldoku_uint8_t;
typedef unsigned short smalldoku_uint16_t;
typedef unsigned int smalldoku_uint32_t;
typedef unsigned long smalldoku_uint64_t;

typedef smalldoku_uint64_t smalldoku_ptrdiff_t;

typedef smalldoku_uint8_t(*smalldoku_rng_fn)(smalldoku_uint8_t min, smalldoku_uint8_t max);

/**
 * Determines the type of a cell
 */
enum smalldoku_cell_type {
    /**
     * The cell has been generated
     */
    SMALLDOKU_GENERATED_CELL,

    /**
     * The cell has been filled by the user
     */
    SMALLDOKU_USER_CELL
};

typedef enum smalldoku_cell_type smalldoku_cell_type_t;

/**
 * Represents a single smalldoku cell
 */
struct smalldoku_cell {
    /**
     * The type of the cell
     */
    smalldoku_cell_type_t type;

    /**
     * The value of the cell - this always contains the value the cell is supposed to have for SMALLDOKU_USER_CELL
     * cells, or the value the cell has for SMALLDOKU_GENERATED_CELL cells.
     */
    smalldoku_uint8_t value;

    /**
     * The value the user put into the cell - only valid for SMALLDOKU_USER_CELL cells, always 0 for
     * SMALLDOKU_GENERATED_CELL cells.
     */
    smalldoku_uint8_t user_value;

    /**
     * User specific data, not used by the core library.
     */
    void *user_data;
};

typedef struct smalldoku_cell smalldoku_cell_t;

/**
 * Initializes the sudoku grid
 *
 * @param grid the grid to initialize
 */
void smalldoku_init(SMALLDOKU_GRID(grid));

/**
 * Fills the sudoku grid with random numbers
 *
 * @param grid the grid to fill
 * @param rng the function to use for generating random numbers
 */
void smalldoku_fill_grid(SMALLDOKU_GRID(grid), smalldoku_rng_fn rng);

/**
 * Erases a few numbers from the grid (by simply marking the cells as user cells).
 *
 * @param grid the grid to hammer
 * @param erase_count the number of cells to mark as user cells
 * @param rng the function to use for generating random numbers
 */
void smalldoku_hammer_grid(SMALLDOKU_GRID(grid), smalldoku_uint8_t erase_count, smalldoku_rng_fn rng);

/**
 * Attempts to solve a grid.
 *
 * @param grid the grid to solve
 * @return the number of solutions found
 */
smalldoku_uint32_t smalldoku_solve_grid(SMALLDOKU_GRID(grid));

/**
 * Retrieves the current value of a cell.
 *
 * @param grid the grid to retrieve the value from
 * @param row the row of the cell to retrieve the value for
 * @param col the column of the cell to retrieve the value for
 * @return the current cells value
 */
smalldoku_uint8_t smalldoku_get_cell_value(SMALLDOKU_GRID(grid), smalldoku_uint8_t row, smalldoku_uint8_t col);

