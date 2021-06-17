#include <efi.h>
#include <efilib.h>

#include <immintrin.h>

#include <smalldoku-core-ui/smalldoku-core-ui.h>

#include "smalldoku-uefi/smalldoku-uefi.h"
#include "smalldoku-uefi/smalldoku-uefi-graphics.h"
#include "smalldoku-uefi/smalldoku-uefi-input.h"

const uint32_t SCALE = 80;

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

static uint8_t generate_random_number(uint8_t min, uint8_t max) {
    uint32_t generated_value;
    _rdrand32_step(&generated_value); // TODO: Better RNG

    return generated_value % (max + 1 - min) + min;
}

static EFI_STATUS report_fatal_error(
        EFI_SYSTEM_TABLE *system_table,
        EFI_STATUS status,
        uefi_graphics_t *graphics,
        const char *error
) {
    Print(u"FATAL ERROR: %r => %a\n", status, error);

    uefi_graphics_set_fill(graphics, 0xFFFF0000);
    uefi_graphics_draw_rect(graphics, 0, 0, graphics->width, graphics->height);

    uefi_graphics_set_fill(graphics, 0xFFFFFFFF);
    uefi_graphics_draw_text(graphics, 20, 20, error);

    uefi_graphics_flush(graphics);

    system_table->BootServices->Stall(1000 * 1000 * 10);
    return status;
}

static void redraw(uefi_graphics_t *graphics, smalldoku_core_ui_t *ui, uint32_t mouse_x, uint32_t mouse_y) {
    uefi_graphics_set_fill(graphics, 0xFFFFFFFF);
    uefi_graphics_draw_rect(graphics, 0, 0, graphics->width, graphics->height);

    smalldoku_core_ui_draw_centered(ui);
    uefi_graphics_draw_raw(graphics, mouse_x, mouse_y, 24, 24, &cursor_raw);
    uefi_graphics_flush(graphics);
}

__attribute__((unused)) EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    InitializeLib(image_handle, system_table);

    smalldoku_uefi_application_t application = {system_table, system_table->BootServices, image_handle};

    Print(u"Smalldoku starting!\n");

    uefi_graphics_t graphics;
    switch (uefi_graphics_initialize(&application, &graphics)) {
        case UEFI_GRAPHICS_OK:
            break;

        case UEFI_GRAPHICS_NO_PROTOCOL:
            Print(u"No graphics protocol found!\n");
            return EFI_UNSUPPORTED;

        case UEFI_GRAPHICS_NO_SUITABLE_MODE:
            Print(u"No suitable graphics mode found!\n");
            return EFI_UNSUPPORTED;

        case UEFI_GRAPHICS_UNKNOWN_ERROR:
            Print(u"An error occurred while initializing the graphics!\n");
            return EFI_PROTOCOL_ERROR;
    }

    uefi_graphics_set_font(&graphics, &font_psfu, 3);

    smalldoku_core_ui_t ui = smalldoku_core_ui_new((smalldoku_graphics_t *) &graphics, generate_random_number);
    uefi_input_system_t input_system;

    EFI_STATUS status = uefi_input_system_initialize(&application, &graphics, &input_system);
    if (EFI_ERROR(status)) {
        return report_fatal_error(system_table, status, &graphics, "Failed to initialize input system!");
    }

    smalldoku_core_ui_begin_game(&ui);
    smalldoku_core_ui_draw_centered(&ui);

    redraw(&graphics, &ui, input_system.mouse_x, input_system.mouse_y);

    Print(u"Initial draw done!\n");

    while (TRUE) {
        status = uefi_input_system_process_event(&application, &input_system, &graphics, &ui);
        if(EFI_ERROR(status)) {
            return report_fatal_error(system_table, status, &graphics, "Failed to process events!");
        }

        if (graphics.should_redraw) {
            redraw(&graphics, &ui, input_system.mouse_x, input_system.mouse_y);
        }
    }

    system_table->BootServices->Stall(1000 * 1000 * 30);

    return EFI_SUCCESS;
}
