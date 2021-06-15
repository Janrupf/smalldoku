#include <efi.h>
#include <efilib.h>

#include <smalldoku/smalldoku.h>
#include <immintrin.h>

#include "smalldoku-uefi/smalldoku-uefi.h"
#include "smalldoku-uefi/smalldoku-uefi-graphics.h"

const uint32_t SCALE = 80;
const uint32_t WIDTH = SMALLDOKU_GRID_WIDTH * SCALE;
const uint32_t HEIGHT = SMALLDOKU_GRID_HEIGHT * SCALE;
const uint32_t PADDING_X = (1920 / 2) - (WIDTH / 2);
const uint32_t PADDING_Y = (1080 / 2) - (HEIGHT / 2);

#define _STR_MACRO2(x) #x
#define _STR_MACRO(x) _STR_MACRO2(x)

#define INCLUDE_BINARY(type, name, path)               \
    extern type name;                                  \
    __asm__(""                                         \
            ".section \".rodata\", \"a\", @progbits\n" \
            #name ":\n"                                \
            ".incbin \"" _STR_MACRO(path) "\"\n"       \
            ".previous")

INCLUDE_BINARY(uefi_graphics_psf_font_t, font_psfu, SMALLDOKU_UEFI_FONT_FILE);
INCLUDE_BINARY(char, cursor_raw, SMALLDOKU_UEFI_CURSOR_FILE);

#define I_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define I_MAX(a, b) (((a) > (b)) ? (a) : (b))

static EFI_STATUS report_fatal_error(EFI_SYSTEM_TABLE *system_table, uefi_graphics_t *graphics, const char *error) {
    uefi_graphics_draw_rect(graphics, 0, 0, 1920, 1080, 0xFF0000);
    uefi_graphics_draw_text(graphics, 20, 20, error, 0xFFFFFF);

    system_table->BootServices->Stall(1000 * 1000 * 10);
    return EFI_UNSUPPORTED;
}

static void draw(uefi_graphics_t *graphics, SMALLDOKU_GRID(grid), uint32_t mouse_x, uint32_t mouse_y) {
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

    uefi_graphics_draw_raw(graphics, mouse_x, mouse_y, 24, 24, &cursor_raw);
    uefi_graphics_flush(graphics);
}

static uint8_t generate_random_number(uint8_t min, uint8_t max) {
    static uint8_t counter = 0;
    counter++;

    uint32_t generated_value;
    // _rdrand32_step(&generated_value);
    generated_value = counter;

    return generated_value % (max + 1 - min) + min;
}

__attribute__((unused)) EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    InitializeLib(image_handle, system_table);
    smalldoku_uefi_application_t application = { system_table, system_table->BootServices, image_handle };

    Print(u"Smalldoku starting!\n");

    uefi_graphics_t graphics;
    switch (uefi_graphics_initialize(&application, &graphics)) {
        case UEFI_GRAPHICS_OK:
            break;

        case UEFI_GRAPHICS_NO_PROTOCOL:
            Print(u"No graphics protocol found!");
            return EFI_UNSUPPORTED;

        case UEFI_GRAPHICS_NO_SUITABLE_MODE:
            Print(u"No suitable graphics mode found!");
            return EFI_UNSUPPORTED;

        case UEFI_GRAPHICS_UNKNOWN_ERROR:
            Print(u"An error occurred while initializing the graphics!");
            return EFI_PROTOCOL_ERROR;
    }

    uefi_graphics_set_font(&graphics, &font_psfu, 3);

    EFI_SIMPLE_POINTER_PROTOCOL *pointer_protocol;
    if (EFI_ERROR(
            system_table->BootServices->LocateProtocol(&SimplePointerProtocol, NULL, (void **) &pointer_protocol))) {
        return report_fatal_error(system_table, &graphics, "Failed to find a pointer protocol!");
    }

    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *text_input_protocol;
    if (EFI_ERROR(system_table->BootServices->LocateProtocol(&SimpleTextInputExProtocol, NULL,
                                                             (void **) &text_input_protocol))) {
        return report_fatal_error(system_table, &graphics, "Failed to find a text input protocol!");
    }

    SMALLDOKU_GRID(grid);
    smalldoku_init(grid);
    smalldoku_fill_grid(grid, generate_random_number);
    smalldoku_hammer_grid(grid, 5, generate_random_number);

    if (pointer_protocol->Mode->ResolutionY == 0) {
        return report_fatal_error(system_table, &graphics, "Pointer resolution x is 0!");
    }

    uint32_t mouse_x = 1920 / 2;
    uint32_t mouse_y = 1080 / 2;
    draw(&graphics, grid, mouse_x, mouse_y);

    pointer_protocol->Reset(pointer_protocol, TRUE);

    while (1) {
        uint64_t event_index;
        system_table->BootServices->WaitForEvent(1, &pointer_protocol->WaitForInput, &event_index);

        EFI_SIMPLE_POINTER_STATE pointer_state;
        pointer_protocol->GetState(pointer_protocol, &pointer_state);

        mouse_x = I_MIN(0, I_MAX(1920, mouse_x + pointer_state.RelativeMovementX));
        mouse_y = I_MIN(0, I_MAX(1080, mouse_y + pointer_state.RelativeMovementY));
        draw(&graphics, grid, mouse_x, mouse_y);
        uefi_graphics_draw_text(&graphics, 20, 20, "Pointer moved!", 0x000000);
    }

    system_table->BootServices->Stall(1000 * 1000 * 30);

    return EFI_SUCCESS;
}
