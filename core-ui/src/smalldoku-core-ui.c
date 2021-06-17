#include "smalldoku-core-ui/smalldoku-core-ui.h"

smalldoku_core_ui_t smalldoku_core_ui_new(smalldoku_graphics_t *graphics, smalldoku_rng_fn rng) {
    smalldoku_core_ui_t ui;

    ui.graphics = graphics;
    ui.rng = rng;
    smalldoku_init(ui.grid);
    ui.grid_x = 0;
    ui.grid_y = 0;

    return ui;
}

void smalldoku_core_ui_begin_game(smalldoku_core_ui_t *ui) {
    smalldoku_init(ui->grid);
    smalldoku_fill_grid(ui->grid, ui->rng);
    smalldoku_hammer_grid(ui->grid, 5, ui->rng);
    ui->graphics->request_redraw(ui->graphics);
}

void smalldoku_core_ui_draw_centered(smalldoku_core_ui_t *ui) {
    smalldoku_core_graphics_draw_grid_centered(ui->graphics, ui->grid, &ui->grid_x, &ui->grid_y);
}

void smalldoku_core_ui_draw(smalldoku_core_ui_t *ui, smalldoku_uint32_t x, smalldoku_uint32_t y) {
    ui->grid_x = x;
    ui->grid_y = y;
    smalldoku_core_graphics_draw_grid(ui->graphics, x, y, ui->grid);
}

void smalldoku_core_ui_click(smalldoku_core_ui_t *ui, smalldoku_uint32_t x, smalldoku_uint32_t y) {
    for (smalldoku_uint8_t row = 0; row < SMALLDOKU_GRID_HEIGHT; row++) {
        for (smalldoku_uint8_t col = 0; col < SMALLDOKU_GRID_WIDTH; col++) {
            ui->grid[row][col].user_data = 0x0;
        }
    }

    smalldoku_uint32_t grid_click_x = x - ui->grid_x;
    smalldoku_uint32_t grid_click_y = y - ui->grid_y;

    smalldoku_uint8_t row;
    smalldoku_uint8_t col;

    if (smalldoku_core_graphics_translate_coordinate(
            ui->graphics,
            grid_click_x,
            grid_click_y,
            &row,
            &col
    )) {
        if (ui->grid[row][col].type == SMALLDOKU_USER_CELL) {
            ui->grid[row][col].user_data = (void *) 0x1;
        }
    }

    ui->graphics->request_redraw(ui->graphics);
}

void smalldoku_core_ui_key(smalldoku_core_ui_t *ui, char key) {
    switch (key) {
        case 'r': {
            smalldoku_core_ui_begin_game(ui);
            return;
        }

        case 'c': {
            for (smalldoku_uint8_t row = 0; row < SMALLDOKU_GRID_HEIGHT; row++) {
                for (smalldoku_uint8_t col = 0; col < SMALLDOKU_GRID_WIDTH; col++) {
                    if (ui->grid[row][col].type == SMALLDOKU_USER_CELL) {
                        if(ui->grid[row][col].user_value == ui->grid[row][col].value) {
                            ui->grid[row][col].user_data = (void *) 0x2;
                        } else {
                            ui->grid[row][col].user_data = (void *) 0x3;
                        }
                    }
                }
            }

            ui->graphics->request_redraw(ui->graphics);
            return;
        }

        default: {
            if (key >= '0' && key <= '9') {
                smalldoku_uint8_t number = key - '0';

                for (smalldoku_uint8_t row = 0; row < SMALLDOKU_GRID_HEIGHT; row++) {
                    for (smalldoku_uint8_t col = 0; col < SMALLDOKU_GRID_WIDTH; col++) {
                        if (ui->grid[row][col].user_data == (void *) 0x1) {
                            ui->grid[row][col].user_value = number;
                        }
                    }
                }

                ui->graphics->request_redraw(ui->graphics);
            }
            return;
        }
    }
}
