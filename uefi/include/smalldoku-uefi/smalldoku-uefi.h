#pragma once

#include <efi.h>

/**
 * Contains information about an application instance.
 */
struct smalldoku_uefi_application {
    /**
     * Access to the system table.
     */
    EFI_SYSTEM_TABLE *system;

    /**
     * Access to the boot services.
     */
    EFI_BOOT_SERVICES *boot_services;

    /**
     * The image handle of the application owning EFI image.
     */
    EFI_HANDLE image_handle;
};

typedef struct smalldoku_uefi_application smalldoku_uefi_application_t;
