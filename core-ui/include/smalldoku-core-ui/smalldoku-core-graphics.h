#pragma once

#include <smalldoku/smalldoku.h>

struct smalldoku_graphics;
typedef struct smalldoku_graphics smalldoku_graphics_t;

/**
 * Function to query the width and height of the canvas.
 *
 * @param graphics the graphics context to operate on
 * @param width the pointer to write the width to
 * @param height the pointer to write the height to
 */
typedef void(*smalldoku_query_size_fn)(
        smalldoku_graphics_t *graphics,
        smalldoku_uint32_t *width,
        smalldoku_uint32_t *height
);

/**
 * Function to query the size of a text.
 *
 * @param graphics the graphics context to operate on
 * @param text the text to query the size for
 * @param width the pointer to write the width to, or NULL, to not calculate the width
 * @param height the pointer to write the height to, or NULL, to not calculate the height
 */
typedef void(*smalldoku_query_text_size_fn)(
        smalldoku_graphics_t *graphics,
        const char *text,
        smalldoku_uint32_t *width,
        smalldoku_uint32_t *height
);


/**
 * Function to set the fill color.
 *
 * @param graphics the graphics context to operate on
 * @param color the color to set as the fill color, in the format 0xAARRGGBB
 */
typedef void(*smalldoku_set_fill_fn)(
        smalldoku_graphics_t *graphics,
        smalldoku_uint32_t color
);

/**
 * Function to draw a rectangle.
 *
 * @param graphics the graphics context to operate on
 * @param x the x coordinate to start drawing at
 * @param y the y coordinate to start drawing at
 * @param width the width of the rectangle to draw
 * @param height the height of the rectangle to draw
 */
typedef void(*smalldoku_draw_rect_fn)(
        smalldoku_graphics_t *graphics,
        smalldoku_uint32_t x,
        smalldoku_uint32_t y,
        smalldoku_uint32_t width,
        smalldoku_uint32_t height
);

/**
 * Function to draw text.
 *
 * @param graphics the graphics context to operate on
 * @param x the x coordinate to start drawing at
 * @param y the y coordinate to start drawing at
 * @param text the text to draw
 */
typedef void(*smalldoku_draw_text_fn)(
        smalldoku_graphics_t *graphics,
        smalldoku_uint32_t x,
        smalldoku_uint32_t y,
        const char *text
);

/**
 * Function to request a redraw.
 *
 * @param graphics the graphics context to operate on
 */
typedef void(*smalldoku_request_redraw_fn)(
        smalldoku_graphics_t *graphics
);

#define SMALLDOKU_GRAPHICS_STRUCT_MEMBERS         \
    smalldoku_query_size_fn query_size;           \
    smalldoku_query_text_size_fn query_text_size; \
    smalldoku_set_fill_fn set_fill;               \
    smalldoku_draw_rect_fn draw_rect;             \
    smalldoku_draw_text_fn draw_text;             \
    smalldoku_request_redraw_fn request_redraw

/**
 * Base struct for graphics implementations.
 *
 * Implementations are meant to include this structure at the start of their implementation and then
 * may extend upon.
 */
struct smalldoku_graphics {
    SMALLDOKU_GRAPHICS_STRUCT_MEMBERS;
};

/**
 * Draws the grid center on the graphics context.
 *
 * @param graphics the graphics context to operate on
 * @param grid the grid to draw
 * @param x pointer to write the x position of the grid to
 * @param y pointer to write the y position of the grid to
 */
void smalldoku_core_graphics_draw_grid_centered(
        smalldoku_graphics_t *graphics,
        SMALLDOKU_GRID(grid),
        smalldoku_uint32_t *x,
        smalldoku_uint32_t *y
);

/**
 * Draws the grid at the given coordinates.
 *
 * @param graphics the graphics context to operate on
 * @param x the x coordinate to start drawing at
 * @param y the y coordinate to start drawing at
 * @param grid the grid to draw
 */
void smalldoku_core_graphics_draw_grid(
        smalldoku_graphics_t *graphics,
        smalldoku_uint32_t x,
        smalldoku_uint32_t y,
        SMALLDOKU_GRID(grid)
);

/**
 * Retrieves the width of the grid.
 *
 * @param graphics the graphics context to operate on
 * @return the grid width
 */
smalldoku_uint32_t smalldoku_core_graphics_get_grid_width(smalldoku_graphics_t *graphics);

/**
 * Retrieves the grid height
 *
 * @param graphics the graphics context to operate on
 * @return the grid height
 */
smalldoku_uint32_t smalldoku_core_graphics_get_grid_height(smalldoku_graphics_t *graphics);

/**
 * Tries to translate a grid coordinate to row and column.
 *
 * @param graphics the graphics context to operate on
 * @param x the x coordinate relative to the grid
 * @param y the y coordinate relative to the grid
 * @param row the pointer to write the row to
 * @param col the pointer to write the column to
 * @return 1 if the translation succeeded, 0 otherwise
 */
smalldoku_uint8_t smalldoku_core_graphics_translate_coordinate(
        smalldoku_graphics_t *graphics,
        smalldoku_uint32_t x,
        smalldoku_uint32_t y,
        smalldoku_uint8_t *row,
        smalldoku_uint8_t *col
);
