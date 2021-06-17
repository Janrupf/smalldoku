#include "smalldoku-core-ui/smalldoku-core-graphics.h"

#define RGB(r, g, b) (0xFF000000 | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF))

const static smalldoku_uint32_t SCALE = 80;
const static smalldoku_uint32_t GRID_WIDTH = SCALE * SMALLDOKU_GRID_WIDTH;
const static smalldoku_uint32_t GRID_HEIGHT = SCALE * SMALLDOKU_GRID_HEIGHT;

void smalldoku_core_graphics_draw_grid_centered(
        smalldoku_graphics_t *graphics,
        SMALLDOKU_GRID(grid),
        smalldoku_uint32_t *x,
        smalldoku_uint32_t *y
) {
    smalldoku_uint32_t graphics_width;
    smalldoku_uint32_t graphics_height;
    graphics->query_size(graphics, &graphics_width, &graphics_height);

    *x = (graphics_width / 2) - (GRID_WIDTH / 2);
    *y = (graphics_height / 2) - (GRID_HEIGHT / 2);

    smalldoku_core_graphics_draw_grid(graphics, *x, *y, grid);
}

void smalldoku_core_graphics_draw_grid(
        smalldoku_graphics_t *graphics,
        smalldoku_uint32_t x,
        smalldoku_uint32_t y,
        SMALLDOKU_GRID(grid)
) {
    graphics->set_fill(graphics, RGB(0xFF, 0xFF, 0xFF));
    graphics->draw_rect(graphics, x, y, GRID_WIDTH, GRID_HEIGHT);

    for (smalldoku_uint8_t row = 0; row < SMALLDOKU_GRID_HEIGHT; row++) {
        for (smalldoku_uint8_t col = 0; col < SMALLDOKU_GRID_WIDTH; col++) {
            smalldoku_uint8_t cell_value = smalldoku_get_cell_value(grid, row, col);
            smalldoku_uint32_t cell_rect_x = x + (col * SCALE);
            smalldoku_uint32_t cell_rect_y = y + (row * SCALE);

            if (grid[row][col].type == SMALLDOKU_GENERATED_CELL) {
                graphics->set_fill(graphics, RGB(0xCC, 0xCC, 0xCC));
            } else {
                switch ((smalldoku_uint64_t) grid[row][col].user_data) {
                    case 0x1:
                        graphics->set_fill(graphics, RGB(0xCC, 0xCC, 0x00));
                        break;

                    case 0x2:
                        graphics->set_fill(graphics, RGB(0x55, 0xAA, 0x55));
                        break;

                    case 0x3:
                        graphics->set_fill(graphics, RGB(0xAA, 0x55, 0x55));
                        break;

                    default:
                        graphics->set_fill(graphics, RGB(0xFF, 0xFF, 0xFF));
                        break;
                }
            }

            graphics->draw_rect(graphics, cell_rect_x, cell_rect_y, SCALE, SCALE);

            if (cell_value != 0) {
                char display_text[2] = {(char) ('0' + cell_value), '\0'};

                smalldoku_uint32_t text_width;
                smalldoku_uint32_t text_height;
                graphics->query_text_size(graphics, display_text, &text_width, &text_height);

                smalldoku_uint32_t text_x = x + (col * SCALE) + ((SCALE / 2) - (text_width / 2));
                smalldoku_uint32_t text_y = y + (row * SCALE) + ((SCALE / 2) + (text_height / 2));

                graphics->set_fill(graphics, RGB(0x00, 0x00, 0x00));
                graphics->draw_text(graphics, text_x, text_y, display_text);
            }
        }
    }

    graphics->set_fill(graphics, RGB(0x00, 0x00, 0x00));

    for (smalldoku_uint8_t col = 0; col <= SMALLDOKU_GRID_WIDTH; col++) {
        smalldoku_uint32_t start_x = x + (col * SCALE);

        if (col % SMALLDOKU_SQUARE_WIDTH == 0) {
            graphics->draw_rect(graphics, start_x - 2, y, 5, GRID_HEIGHT);
        } else {
            graphics->draw_rect(graphics, start_x - 1, y, 3, GRID_HEIGHT);
        }
    }

    for (smalldoku_uint8_t row = 0; row <= SMALLDOKU_GRID_HEIGHT; row++) {
        smalldoku_uint32_t start_y = y + (row * SCALE);

        if (row % SMALLDOKU_SQUARE_HEIGHT == 0) {
            graphics->draw_rect(graphics, x, start_y - 2, GRID_WIDTH, 5);
        } else {
            graphics->draw_rect(graphics, x, start_y - 1, GRID_WIDTH, 3);
        }
    }
}

smalldoku_uint32_t smalldoku_core_graphics_get_grid_width(smalldoku_graphics_t *graphics) {
    (void) graphics;
    return GRID_WIDTH;
}

smalldoku_uint32_t smalldoku_core_graphics_get_grid_height(smalldoku_graphics_t *graphics) {
    (void) graphics;
    return GRID_HEIGHT;
}

smalldoku_uint8_t smalldoku_core_graphics_translate_coordinate(
        smalldoku_graphics_t *graphics,
        smalldoku_uint32_t x,
        smalldoku_uint32_t y,
        smalldoku_uint8_t *row,
        smalldoku_uint8_t *col
) {
    if(
            x > smalldoku_core_graphics_get_grid_width(graphics) ||
            y > smalldoku_core_graphics_get_grid_height(graphics)
    ) {
        return 0;
    }

    *col = x / SCALE;
    *row = y / SCALE;

    return 1;
}
