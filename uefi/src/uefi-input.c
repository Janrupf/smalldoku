#include "smalldoku-uefi/smalldoku-uefi-input.h"

#include <efilib.h>

static EFI_GUID SIMPLE_POINTER_PROTOCOL_GUID = EFI_SIMPLE_POINTER_PROTOCOL_GUID;
static EFI_GUID SIMPLE_INPUT_EX_PROTOCOL_GUID = EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID;

static void cap(int64_t *v, int64_t min, int64_t max) {
    if(*v < min) {
        *v = min;
    }

    if(*v > max) {
        *v = max;
    }
}

static EFI_STATUS open_all_protocols(
        smalldoku_uefi_application_t *application,
        EFI_GUID *protocol_guid,
        uefi_handle_protocol_pair_t **buffer_out,
        uint32_t *buffer_size
) {
    UINTN handle_count;
    EFI_HANDLE *handles = NULL;

    EFI_STATUS status = application->boot_services->LocateHandleBuffer(
            ByProtocol,
            protocol_guid,
            NULL,
            &handle_count,
            &handles
    );

    if (EFI_ERROR(status)) {
        if (handles) {
            application->boot_services->FreePool(handles);
        }

        return status;
    }

    status = application->boot_services->AllocatePool(
            EfiLoaderData,
            sizeof(uefi_handle_protocol_pair_t) * handle_count,
            (void **) buffer_out
    );

    if (EFI_ERROR(status)) {
        if (handles) {
            application->boot_services->FreePool(handles);
        }

        return status;
    }

    Print(u"Opening %d handles of %g\n", handle_count, protocol_guid);

    for (uint32_t i = 0; i < handle_count; i++) {
        uefi_handle_protocol_pair_t *pair = &(*buffer_out)[i];
        pair->handle = handles[i];

        status = application->boot_services->OpenProtocol(
                pair->handle,
                protocol_guid,
                &pair->protocol,
                application->image_handle,
                NULL,
                EFI_OPEN_PROTOCOL_EXCLUSIVE
        );

        if (EFI_ERROR(status)) {
            for (uint32_t x = 0; x < i - 1; x++) {
                application->boot_services->CloseProtocol(
                        handles[x],
                        protocol_guid,
                        application->image_handle,
                        NULL
                );
            }

            application->boot_services->FreePool(handles);
            application->boot_services->FreePool(buffer_out);
            *buffer_out = NULL;

            return status;
        }
    }

    application->boot_services->FreePool(handles);
    *buffer_size = handle_count;

    return EFI_SUCCESS;
}

static void assign_protocols(
        uefi_handle_protocol_pair_t *pairs,
        uint32_t pair_count,
        uefi_opened_input_protocol_t *opened_protocols,
        EFI_EVENT *event_buffer,
        uint32_t *opened_index_counter,
        uefi_input_type_t input_type
) {
    for (uint32_t i = 0; i < pair_count; i++) {
        uint32_t opened_index = (*opened_index_counter)++;

        opened_protocols[opened_index].handle = pairs[i].handle;
        opened_protocols[opened_index].type = input_type;

        switch (input_type) {
            case UEFI_INPUT_TYPE_KEYBOARD:
                opened_protocols[opened_index].keyboard = pairs[i].protocol;
                event_buffer[opened_index] = opened_protocols[opened_index].keyboard->WaitForKeyEx;
                break;

            case UEFI_INPUT_TYPE_MOUSE:
                opened_protocols[opened_index].mouse = pairs[i].protocol;
                event_buffer[opened_index] = opened_protocols[opened_index].mouse->WaitForInput;
                break;

            default:
                __asm__("ud2");
                break;
        }
    }
}

EFI_STATUS uefi_input_system_initialize(
        smalldoku_uefi_application_t *application,
        uefi_graphics_t *graphics,
        uefi_input_system_t *out
) {
    EFI_STATUS status;

    uint32_t mouse_protocol_count;
    uefi_handle_protocol_pair_t *mouse_protocols;

    status = open_all_protocols(application, &SIMPLE_POINTER_PROTOCOL_GUID, &mouse_protocols, &mouse_protocol_count);
    if (EFI_ERROR(status)) {
        return status;
    }

    uint32_t keyboard_protocol_count;
    uefi_handle_protocol_pair_t *keyboard_protocols;

    status = open_all_protocols(application, &SIMPLE_INPUT_EX_PROTOCOL_GUID, &keyboard_protocols,
                                &keyboard_protocol_count);
    if (EFI_ERROR(status)) {
        application->boot_services->FreePool(mouse_protocols);
        return status;
    }

    uint32_t opened_protocols_count = mouse_protocol_count + keyboard_protocol_count;
    uefi_opened_input_protocol_t *opened_protocols;

    for(uint32_t i = 0; i < mouse_protocol_count; i++) {
        Print(u"Opened mouse protocol 0x%x with handle 0x%x\n", mouse_protocols[i].protocol, mouse_protocols[i].handle);
    }

    for(uint32_t i = 0; i < keyboard_protocol_count; i++) {
        Print(u"Opened keyboard protocol 0x%x with handle 0x%x\n", keyboard_protocols[i].protocol, keyboard_protocols[i].handle);
    }


    status = application->boot_services->AllocatePool(
            EfiLoaderData,
            sizeof(uefi_opened_input_protocol_t) * opened_protocols_count,
            (void **) &opened_protocols
    );

    if (EFI_ERROR(status)) {
        application->boot_services->FreePool(mouse_protocols);
        application->boot_services->FreePool(keyboard_protocols);
        return status;
    }

    EFI_EVENT *event_buffer;
    status = application->boot_services->AllocatePool(
            EfiLoaderData,
            sizeof(EFI_EVENT) * opened_protocols_count,
            (void **) &event_buffer
    );

    if (EFI_ERROR(status)) {
        application->boot_services->FreePool(mouse_protocols);
        application->boot_services->FreePool(keyboard_protocols);
        application->boot_services->FreePool(opened_protocols);
        return status;
    }

    uint32_t protocol_i = 0;
    assign_protocols(
            mouse_protocols,
            mouse_protocol_count,
            opened_protocols,
            event_buffer,
            &protocol_i,
            UEFI_INPUT_TYPE_MOUSE
    );
    assign_protocols(
            keyboard_protocols,
            keyboard_protocol_count,
            opened_protocols,
            event_buffer,
            &protocol_i,
            UEFI_INPUT_TYPE_KEYBOARD
    );

    application->boot_services->FreePool(mouse_protocols);
    application->boot_services->FreePool(keyboard_protocols);

    out->protocol_count = opened_protocols_count;
    out->opened_protocols = opened_protocols;
    out->event_buffer = event_buffer;
    out->mouse_x = graphics->width / 2;
    out->mouse_y = graphics->height / 2;

    return EFI_SUCCESS;
}

EFI_STATUS uefi_input_system_process_event(
        smalldoku_uefi_application_t *application,
        uefi_input_system_t *input_system,
        uefi_graphics_t *graphics,
        smalldoku_core_ui_t *ui
) {
    Print(u"Selecting from %d events\n", input_system->protocol_count);

    for(uint32_t i = 0; i < input_system->protocol_count; i++) {
        EFI_STATUS status = application->boot_services->CheckEvent(input_system->event_buffer[i]);
        uint8_t is_valid = status == EFI_SUCCESS || status == EFI_NOT_READY;

        if(!is_valid) {
            Print(u"Event %d is invalid: %r\n", input_system->event_buffer[i], status);
        }
    }

    UINTN event_index;
    EFI_STATUS status = application->boot_services->WaitForEvent(
            input_system->protocol_count,
            input_system->event_buffer,
            &event_index
    );

    if (EFI_ERROR(status)) {
        Print(u"WaitForEvent failed: %r\n", status);
        return status;
    }

    uefi_opened_input_protocol_t *event_protocol = &input_system->opened_protocols[event_index];
    switch (event_protocol->type) {
        case UEFI_INPUT_TYPE_KEYBOARD: {
            EFI_KEY_DATA data;
            event_protocol->keyboard->ReadKeyStrokeEx(event_protocol->keyboard, &data);

            if (data.Key.UnicodeChar <= 255) {
                char key = (char) data.Key.UnicodeChar;
                smalldoku_core_ui_key(ui, key);
            }
            break;
        }

        case UEFI_INPUT_TYPE_MOUSE: {
            EFI_SIMPLE_POINTER_STATE pointer_state;
            event_protocol->mouse->GetState(event_protocol->mouse, &pointer_state);

            int64_t new_mouse_x = input_system->mouse_x + (pointer_state.RelativeMovementX /
                                                           ((int64_t) event_protocol->mouse->Mode->ResolutionX));
            int64_t new_mouse_y = input_system->mouse_y + (pointer_state.RelativeMovementY /
                                                           ((int64_t) event_protocol->mouse->Mode->ResolutionY));

            cap(&new_mouse_x, 0, graphics->width);
            cap(&new_mouse_y, 0, graphics->height);

            if (new_mouse_x != input_system->mouse_x || new_mouse_y != input_system->mouse_y) {
                uefi_graphics_request_redraw(graphics);
            }

            input_system->mouse_x = new_mouse_x;
            input_system->mouse_y = new_mouse_y;

            if (pointer_state.LeftButton || pointer_state.RightButton) {
                smalldoku_core_ui_click(ui, input_system->mouse_x, input_system->mouse_y);
            }

            break;
        }
    }

    return EFI_SUCCESS;
}
