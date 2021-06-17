#pragma once

#include <smalldoku/smalldoku.h>

#include "smalldoku-core-ui/smalldoku-core-graphics.h"

/**
 * Represents the current UI state.
 */
struct smalldoku_core_ui {
    /**
     * The graphics context to use for drawing.
     */
    smalldoku_graphics_t *graphics;

    /**
     * The function to use for generating random numbers.
     */
    smalldoku_rng_fn rng;

    /**
     * The smalldoku grid instance.
     */
    SMALLDOKU_GRID(grid);

    /**
     * The current graphics x coordinate of the grid.
     */
    smalldoku_uint32_t grid_x;

    /**
     * The current graphics y coordinate of the grid.
     */
    smalldoku_uint32_t grid_y;
};

typedef struct smalldoku_core_ui smalldoku_core_ui_t;

/**
 * Creates a new UI state.
 *
 * @param graphics the graphics context to use for drawing
 * @param rng the function to use for generating random numbers
 * @return the created UI state
 */
smalldoku_core_ui_t smalldoku_core_ui_new(smalldoku_graphics_t *graphics, smalldoku_rng_fn rng);

/**
 * Begins a new game for an UI state.
 *
 * @param ui the UI state to begin a new game on
 */
void smalldoku_core_ui_begin_game(smalldoku_core_ui_t *ui);

/**
 * Draws the UI state centered in the graphics context.
 *
 * @param ui the UI stat to draw
 */
void smalldoku_core_ui_draw_centered(smalldoku_core_ui_t *ui);

/**
 * Draws the UI state.
 *
 * @param ui the UI state to draw
 * @param x the x coordinate to start drawing the UI state at
 * @param y the y coordinate to start drawing the UI state at
 */
void smalldoku_core_ui_draw(smalldoku_core_ui_t *ui, smalldoku_uint32_t x, smalldoku_uint32_t y);

/**
 * Handles a click on the UI.
 *
 * @param ui the UI state to handle the click for
 * @param x the x coordinate of the click
 * @param y the y coordinate of the click
 */
void smalldoku_core_ui_click(smalldoku_core_ui_t *ui, smalldoku_uint32_t x, smalldoku_uint32_t y);

/**
 * Handles a key on the UI.
 *
 * @param ui the UI state to handle the key for
 * @param key the key to handle
 */
void smalldoku_core_ui_key(smalldoku_core_ui_t *ui, char key);
