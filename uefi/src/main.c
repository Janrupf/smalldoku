#include <efi.h>
#include <efilib.h>

#include <smalldoku/smalldoku.h>
#include <immintrin.h>
#include "smalldoku-uefi/smalldoku-uefi-graphics.h"

const uint32_t SCALE = 80;
const uint32_t WIDTH = SMALLDOKU_GRID_WIDTH * SCALE;
const uint32_t HEIGHT = SMALLDOKU_GRID_HEIGHT * SCALE;
const uint32_t PADDING_X = (1920 / 2) - (WIDTH / 2);
const uint32_t PADDING_Y = (1080 / 2) - (HEIGHT / 2);

#define _STR_MACRO2(x) #x
#define _STR_MACRO(x) _STR_MACRO2(x)

extern uefi_graphics_psf_font_t font_psfu;
__asm__(""
        ".section \".rodata\", \"a\", @progbits\n"
        "font_psfu:"
        ".incbin \"" _STR_MACRO(SMALLDOKU_UEFI_FONT_FILE) "\"\n"
        ".previous");

static void draw(uefi_graphics_t *graphics, SMALLDOKU_GRID(grid)) {
    uefi_graphics_draw_rect(graphics, 0, 0, 1920, 1080, 0xFFFFFF);

    for (uint8_t row = 0; row < SMALLDOKU_GRID_HEIGHT; row++) {
        for (uint8_t col = 0; col < SMALLDOKU_GRID_WIDTH; col++) {
            uint8_t value = smalldoku_get_cell_value(grid, row, col);

            uint32_t rect_start_x = PADDING_X + (col * SCALE);
            uint32_t rect_start_y = PADDING_Y + (row * SCALE);

            if (grid[row][col].type == SMALLDOKU_GENERATED_CELL) {
                uefi_graphics_draw_rect(graphics, rect_start_x, rect_start_y, SCALE, SCALE, 0xCCCCCC);
            } else if (grid[row][col].user_data == (void *) 0x1) {
                uefi_graphics_draw_rect(graphics, rect_start_x, rect_start_y, SCALE, SCALE, 0xCCCC00);
            } else if (grid[row][col].user_data == (void *) 0x2) {
                uefi_graphics_draw_rect(graphics, rect_start_x, rect_start_y, SCALE, SCALE, 0x55AA55);
            } else if (grid[row][col].user_data == (void *) 0x3) {
                uefi_graphics_draw_rect(graphics, rect_start_x, rect_start_y, SCALE, SCALE, 0xAA5555);
            }

            if (value != 0) {
                char display_char = (char) (value + '0');

                char display_text[2];
                display_text[0] = display_char;
                display_text[1] = '\0';

                uint32_t text_width = uefi_graphics_text_width(graphics, display_text);
                uint32_t text_height = uefi_graphics_text_height(graphics);

                uint32_t text_x = PADDING_X + (col * SCALE) + (SCALE / 2) - (text_width / 2);
                uint32_t text_y = PADDING_Y + (row * SCALE) + (SCALE / 2) - (text_height / 2);

                uefi_graphics_draw_text(graphics, text_x, text_y, display_text, 0x000000);
            }
        }
    }

    for (uint8_t col = 0; col <= SMALLDOKU_GRID_WIDTH; col++) {
        uint32_t start_x = PADDING_X + (col * SCALE);
        uint32_t start_y = PADDING_Y;

        if (col % SMALLDOKU_SQUARE_WIDTH == 0) {
            uefi_graphics_draw_rect(graphics, start_x - 2, start_y, 5, HEIGHT, 0x000000);
        } else {
            uefi_graphics_draw_rect(graphics, start_x - 1, start_y, 3, HEIGHT, 0x000000);
        }
    }

    for (uint8_t row = 0; row <= SMALLDOKU_GRID_HEIGHT; row++) {
        uint32_t start_x = PADDING_X;
        uint32_t start_y = PADDING_Y + (row * SCALE);

        if (row % SMALLDOKU_SQUARE_HEIGHT == 0) {
            uefi_graphics_draw_rect(graphics, start_x, start_y - 2, WIDTH, 5, 0x000000);
        } else {
            uefi_graphics_draw_rect(graphics, start_x, start_y - 1, WIDTH, 3, 0x000000);
        }
    }
}

static uint8_t generate_random_number(uint8_t min, uint8_t max) {
    uint32_t generated_value;
    _rdrand32_step(&generated_value);

    return generated_value % (max + 1 - min) + min;
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    InitializeLib(image_handle, system_table);

    Print(u"Smalldoku starting!\n");

    EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics_protocol;
    if (EFI_ERROR(
            system_table->BootServices->LocateProtocol(&GraphicsOutputProtocol, NULL, (void **) &graphics_protocol))) {
        Print(u"Failed to find a graphics buffer!\n");
        system_table->BootServices->Stall(1000 * 1000 * 5);
        return EFI_UNSUPPORTED;
    }

    uint8_t graphics_mode_found = 0;
    for (uint32_t i = 0; i < graphics_protocol->Mode->MaxMode; i++) {
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *graphics_info;
        UINTN graphics_info_size;
        EFI_STATUS graphics_mode_status = graphics_protocol->QueryMode(
                graphics_protocol,
                i,
                &graphics_info_size,
                &graphics_info
        );

        if (!EFI_ERROR(graphics_mode_status)) {
            if (
                    graphics_info->VerticalResolution == 1080 &&
                    graphics_info->HorizontalResolution == 1920
                    ) {
                Print(u"Selecting graphics mode %d!\n", i);

                graphics_mode_found = 1;
                graphics_protocol->SetMode(graphics_protocol, i);
                break;
            }
        }
    }

    if (!graphics_mode_found) {
        Print(u"Failed to find a suitable graphics mode!\n");
        system_table->BootServices->Stall(1000 * 1000 * 5);
        return EFI_UNSUPPORTED;
    }

    uefi_graphics_t graphics = uefi_graphics_initialize(graphics_protocol);
    uefi_graphics_set_font(&graphics, &font_psfu, 3);

    SMALLDOKU_GRID(grid);
    smalldoku_init(grid);
    smalldoku_fill_grid(grid, generate_random_number);
    smalldoku_hammer_grid(grid, 5, generate_random_number);
    draw(&graphics, grid);

    system_table->BootServices->Stall(1000 * 1000 * 30);

    return EFI_SUCCESS;
}
