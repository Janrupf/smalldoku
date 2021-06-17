#pragma once

#include <efi/efi.h>

#include <smalldoku-core-ui/smalldoku-core-ui.h>

#include "smalldoku-uefi/smalldoku-uefi.h"
#include "smalldoku-uefi/smalldoku-uefi-graphics.h"

/**
 * Determines of which type an opened protocol is.
 */
enum uefi_input_type {
    /**
     * The protocol is a keyboard protocol.
     */
    UEFI_INPUT_TYPE_KEYBOARD,

    /**
     * The protocol is a mouse protocol.
     */
    UEFI_INPUT_TYPE_MOUSE
};

typedef enum uefi_input_type uefi_input_type_t;

/**
 * Combines together a protocol and the handle it has been opened with.
 */
struct uefi_handle_protocol_pair {
    /**
     * The handle the protocol has been opened on.
     */
    EFI_HANDLE handle;

    /**
     * The opened protocol.
     */
    void *protocol;
};

typedef struct uefi_handle_protocol_pair uefi_handle_protocol_pair_t;

/**
 * Holder for an input protocol and its associated type.
 */
struct uefi_opened_input_protocol {
    /**
     * The type of the protocol.
     */
    uefi_input_type_t type;

    /**
     * The handle the protocol has been opened on.
     */
    EFI_HANDLE handle;

    union {
        /**
         * Keyboard protocol for when type == UEFI_INPUT_TYPE_KEYBOARD
         */
        EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *keyboard;

        /**
         * Mouse protocol for when type == UEFI_INPUT_TYPE_MOUSE
         */
        EFI_SIMPLE_POINTER_PROTOCOL *mouse;
    };
};

typedef struct uefi_opened_input_protocol uefi_opened_input_protocol_t;

/**
 * State of the input system.
 */
struct uefi_input_system {
    /**
     * The count of opened protocols.
     */
    uint32_t protocol_count;

    /**
     * All opened protocols used by this input system.
     */
    uefi_opened_input_protocol_t *opened_protocols;

    /**
     * Buffer of all opened events.
     */
    EFI_EVENT *event_buffer;

    /**
     * The current X position of the mouse.
     */
    uint32_t mouse_x;

    /**
     * The current Y position of the mouse.
     */
    uint32_t mouse_y;
};

typedef struct uefi_input_system uefi_input_system_t;

/**
 * Attempts to initialize the input system.
 *
 * @param application the application to initialize the system for
 * @param graphics the graphics system to initialize input for
 * @param out the system to initialize
 * @return EFI_SUCCESS if the initialization succeeded, an error code otherwise
 */
EFI_STATUS uefi_input_system_initialize(
        smalldoku_uefi_application_t *application,
        uefi_graphics_t *graphics,
        uefi_input_system_t *out
);

/**
 * Processes the events and dispatches them to the UI state.
 *
 * @param application the application the event input system belongs to
 * @param input_system the input system to process events for
 * @param graphics the graphics system to request redraws from
 * @param ui the UI state to dispatch events to
 * @return EFI_SUCCESS if the handling succeeded, an error code otherwise
 */
EFI_STATUS uefi_input_system_process_event(
        smalldoku_uefi_application_t *application,
        uefi_input_system_t *input_system,
        uefi_graphics_t *graphics,
        smalldoku_core_ui_t *ui
);
