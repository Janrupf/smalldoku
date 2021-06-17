#include <efi.h>
#include <efilib.h>

#include <immintrin.h>

#include <smalldoku-core-ui/smalldoku-core-ui.h>

#include "smalldoku-uefi/smalldoku-uefi.h"
#include "smalldoku-uefi/smalldoku-uefi-graphics.h"

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

#define I_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define I_MAX(a, b) (((a) > (b)) ? (a) : (b))

static uint8_t generate_random_number(uint8_t min, uint8_t max) {
    uint32_t generated_value;
    _rdrand32_step(&generated_value); // TODO: Better RNG

    return generated_value % (max + 1 - min) + min;
}

static EFI_STATUS report_fatal_error(EFI_SYSTEM_TABLE *system_table, uefi_graphics_t *graphics, const char *error) {
    uefi_graphics_set_fill(graphics, 0xFFFF0000);
    uefi_graphics_draw_rect(graphics, 0, 0, graphics->width, graphics->height);

    uefi_graphics_set_fill(graphics, 0xFFFFFFFF);
    uefi_graphics_draw_text(graphics, 20, 20, error);

    system_table->BootServices->Stall(1000 * 1000 * 10);
    return EFI_UNSUPPORTED;
}

static void redraw(uefi_graphics_t *graphics, smalldoku_core_ui_t *ui, uint32_t mouse_x, uint32_t mouse_y) {
    uefi_graphics_set_fill(graphics, 0xFFFFFF00);
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

    if (pointer_protocol->Mode->ResolutionY == 0) {
        return report_fatal_error(system_table, &graphics, "Pointer resolution x is 0!");
    }

    smalldoku_core_ui_t ui = smalldoku_core_ui_new((smalldoku_graphics_t *) &graphics, generate_random_number);
    smalldoku_core_ui_begin_game(&ui);
    smalldoku_core_ui_draw_centered(&ui);

    int64_t mouse_x = graphics.width / 2;
    int64_t mouse_y = graphics.height / 2;

    redraw(&graphics, &ui, mouse_x, mouse_y);

    pointer_protocol->Reset(pointer_protocol, TRUE);

    while (TRUE) {
        EFI_EVENT events[2] = {pointer_protocol->WaitForInput, text_input_protocol->WaitForKeyEx};
        uint64_t event_index;
        if (EFI_ERROR(system_table->BootServices->WaitForEvent(2, events, &event_index))) {
            return report_fatal_error(system_table, &graphics, "Failed to wait for events!");
        }

        switch (event_index) {
            case 0: {
                EFI_SIMPLE_POINTER_STATE pointer_state;
                pointer_protocol->GetState(pointer_protocol, &pointer_state);

                int64_t new_mouse_x = I_MAX(0, I_MIN(graphics.width, mouse_x + (pointer_state.RelativeMovementX /
                                                                                ((int64_t) pointer_protocol->Mode->ResolutionX))));
                int64_t new_mouse_y = I_MAX(0, I_MIN(graphics.height, mouse_y + (pointer_state.RelativeMovementY /
                                                                                 ((int64_t) pointer_protocol->Mode->ResolutionY))));

                if (new_mouse_x != mouse_x || new_mouse_y != mouse_y) {
                    uefi_graphics_request_redraw(&graphics);
                }

                mouse_x = new_mouse_x;
                mouse_y = new_mouse_y;

                if (pointer_state.LeftButton || pointer_state.RightButton) {
                    smalldoku_core_ui_click(&ui, mouse_x, mouse_y);
                }

                break;
            }

            case 1: {
                EFI_KEY_DATA data;
                text_input_protocol->ReadKeyStrokeEx(text_input_protocol, &data);

                if (data.Key.UnicodeChar <= 255) {
                    char key = (char) data.Key.UnicodeChar;
                    smalldoku_core_ui_key(&ui, key);
                }
                break;
            }

            default:
                break;
        }

        if (graphics.should_redraw) {
            redraw(&graphics, &ui, mouse_x, mouse_y);
        }
    }

    system_table->BootServices->Stall(1000 * 1000 * 30);

    return EFI_SUCCESS;
}
