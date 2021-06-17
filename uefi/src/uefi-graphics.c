#include "smalldoku-uefi/smalldoku-uefi-graphics.h"

#include <efilib.h>

static EFI_GUID GRAPHICS_PROTOCOL_GUID = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

#define I_MIN(a, b) (((a) < (b)) ? (a) : (b))

static uint32_t strlen(const char *str) {
    uint32_t len = 0;
    while (*str) {
        len++;
        str++;
    }

    return len;
}

static uint32_t convert_rgba_to_mode(uefi_graphics_t *graphics, uint32_t rgb) {
    switch (graphics->protocol->Mode->Info->PixelFormat) {
        case PixelRedGreenBlueReserved8BitPerColor:
            return ((rgb & 0xFF0000) >> 16) |
                   ((rgb & 0x00FF00) >> 8) |
                   ((rgb & 0x0000FF) << 8);

        case PixelBltOnly:
        case PixelBlueGreenRedReserved8BitPerColor:
            return rgb;

        default:
            __asm__("ud2"); /* Should never happen */
            return 0xFFFFFF;
    }
}

static void set_pixel(uefi_graphics_t *graphics, uint32_t x, uint32_t y, uint32_t native_color) {
    if (x > graphics->width || y > graphics->height) {
        return;
    }

    char *framebuffer_base = graphics->pixel_buffer ? graphics->pixel_buffer
                                                    : (char *) graphics->protocol->Mode->FrameBufferBase;
    uint32_t pixels_per_scan_line = graphics->pixel_buffer ? graphics->width
                                                           : graphics->protocol->Mode->Info->PixelsPerScanLine;

    *((uint32_t *) (framebuffer_base + 4 * pixels_per_scan_line * y + 4 * x)) = native_color;
}

static void query_size(uefi_graphics_t *graphics, uint32_t *width, uint32_t *height) {
    *width = graphics->width;
    *height = graphics->height;
}

static void query_text_size(uefi_graphics_t *graphics, const char *text, uint32_t *width, uint32_t *height) {
    if (width) {
        *width = uefi_graphics_text_width(graphics, text);
    }

    if (height) {
        *height = uefi_graphics_text_height(graphics);
    }
}

uefi_graphics_init_status_t uefi_graphics_initialize(smalldoku_uefi_application_t *application, uefi_graphics_t *out) {
    EFI_STATUS status;
    UINTN handle_count;
    EFI_HANDLE *handles = NULL;

    status = application->boot_services->LocateHandleBuffer(
            ByProtocol,
            &GRAPHICS_PROTOCOL_GUID,
            NULL,
            &handle_count,
            &handles
    );

    if (EFI_ERROR(status)) {
        if (handles != NULL) {
            application->boot_services->FreePool(handles);
        }

        return status == EFI_NOT_FOUND ? UEFI_GRAPHICS_NO_PROTOCOL : UEFI_GRAPHICS_UNKNOWN_ERROR;
    }

    Print(u"Found %d graphics protocols\n", handle_count);

    for (uint32_t i = 0; i < handle_count; i++) {
        EFI_GRAPHICS_OUTPUT_PROTOCOL *opened_protocol;

        status = application->boot_services->OpenProtocol(
                handles[i],
                &GRAPHICS_PROTOCOL_GUID,
                (void **) &opened_protocol,
                application->image_handle,
                NULL,
                EFI_OPEN_PROTOCOL_EXCLUSIVE
        );

        if (EFI_ERROR(status)) {
            continue;
        }

        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *mode_information;
        UINTN mode_information_size;
        status = opened_protocol->QueryMode(opened_protocol, 0, &mode_information_size, &mode_information);
        if (EFI_ERROR(status)) {
            if (status == EFI_NOT_STARTED) {
                if (EFI_ERROR(opened_protocol->SetMode(opened_protocol, 0))) {
                    application->boot_services->CloseProtocol(
                            handles[i],
                            &GRAPHICS_PROTOCOL_GUID,
                            application->image_handle,
                            NULL
                    );
                    continue;
                }
            } else {
                application->boot_services->CloseProtocol(
                        handles[i],
                        &GRAPHICS_PROTOCOL_GUID,
                        application->image_handle,
                        NULL
                );
                continue;
            }
        }

        if (mode_information) {
            application->boot_services->FreePool(mode_information);
        }

        uint32_t most_suitable_width = 0;
        uint32_t most_suitable_height = 0;
        uint32_t most_suitable_mode_id = 0;
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION most_suitable_mode;

        for (uint32_t mode_i = 0; mode_i < opened_protocol->Mode->MaxMode; mode_i++) {
            status = opened_protocol->QueryMode(opened_protocol, mode_i, &mode_information_size, &mode_information);

            if (EFI_ERROR(status)) {
                continue;
            }

            DbgPrint(D_INFO,
                     (const unsigned char *) "Considering video mode %d with %dx%d, current mode is %d with %dx%d\n",
                     mode_i, mode_information->HorizontalResolution, mode_information->VerticalResolution,
                     most_suitable_mode_id, most_suitable_width, most_suitable_height);
            if (mode_information->PixelFormat != PixelBitMask) {
                if (
                        (!most_suitable_width || !most_suitable_height) ||
                        (
                                most_suitable_width < mode_information->HorizontalResolution &&
                                mode_information->HorizontalResolution <= 1920 &&
                                most_suitable_height < mode_information->VerticalResolution &&
                                mode_information->VerticalResolution <= 1080
                        )
                        ) {
                    most_suitable_width = mode_information->HorizontalResolution;
                    most_suitable_height = mode_information->VerticalResolution;
                    most_suitable_mode_id = mode_i;

                    application->boot_services->CopyMem(
                            &most_suitable_mode,
                            mode_information,
                            I_MIN(sizeof(most_suitable_mode), mode_information_size)
                    );
                }
            }

            application->boot_services->FreePool(mode_information);
        }

        if (most_suitable_width && most_suitable_height) {
            Print(u"Selected video mode %d: %dx%d\n", most_suitable_mode_id, most_suitable_mode.HorizontalResolution,
                  most_suitable_mode.VerticalResolution);
            if (EFI_ERROR(opened_protocol->SetMode(opened_protocol, most_suitable_mode_id))) {
                application->boot_services->CloseProtocol(
                        handles[i],
                        &GRAPHICS_PROTOCOL_GUID,
                        application->image_handle,
                        NULL
                );
                continue;
            }

            application->boot_services->FreePool(handles);

            out->query_size = (smalldoku_query_size_fn) query_size;
            out->query_text_size = (smalldoku_query_text_size_fn) query_text_size;
            out->set_fill = (smalldoku_set_fill_fn) uefi_graphics_set_fill;
            out->draw_rect = (smalldoku_draw_rect_fn) uefi_graphics_draw_rect;
            out->draw_text = (smalldoku_draw_text_fn) uefi_graphics_draw_text;
            out->request_redraw = (smalldoku_request_redraw_fn) uefi_graphics_request_redraw;

            out->protocol = opened_protocol;
            out->font = NULL;
            out->font_scale = 0;
            out->width = most_suitable_mode.HorizontalResolution;
            out->height = most_suitable_mode.VerticalResolution;
            out->pixel_format = most_suitable_mode.PixelFormat;
            out->should_redraw = TRUE;

            if (most_suitable_mode.PixelFormat != PixelBltOnly) {
                DbgPrint(D_INFO, (const unsigned char *) "Using direct framebuffer for video operations!\n");
                out->pixel_buffer = NULL;
            } else {
                DbgPrint(D_INFO, (const unsigned char *) "Using backbuffer for video operations!\n");
                application->boot_services->AllocatePool(
                        EfiLoaderData,
                        sizeof(uint32_t) * out->width * out->height,
                        &out->pixel_buffer
                );
            }

            return UEFI_GRAPHICS_OK;
        }

        application->boot_services->CloseProtocol(
                handles[i],
                &GRAPHICS_PROTOCOL_GUID,
                application->image_handle,
                NULL
        );
    }

    application->boot_services->FreePool(handles);

    return UEFI_GRAPHICS_NO_SUITABLE_MODE;
}

void uefi_graphics_set_font(uefi_graphics_t *graphics, uefi_graphics_psf_font_t *font, uint8_t font_scale) {
    graphics->font = font;
    graphics->font_scale = font_scale;
}

void uefi_graphics_set_fill(uefi_graphics_t *graphics, uint32_t color) {
    graphics->fill_color = convert_rgba_to_mode(graphics, color);
}

void uefi_graphics_draw_rect(uefi_graphics_t *graphics, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    uint32_t color = graphics->fill_color;

    for (uint32_t cx = x; cx <= x + width; cx++) {
        for (uint32_t cy = y; cy <= y + height; cy++) {
            set_pixel(graphics, cx, cy, color);
        }
    }
}

void uefi_graphics_draw_text(uefi_graphics_t *graphics, uint32_t x, uint32_t y, const char *text) {
    uint32_t bytes_per_line = (graphics->font->width + 7) / 8;
    uint32_t scale = graphics->font_scale;

    y -= graphics->font->height * graphics->font_scale;

    while (*text) {
        char c = *text;

        uint8_t *glyph = ((uint8_t *) graphics->font) +
                         graphics->font->header_size +
                         (c > 0 && c < graphics->font->glyph_count ? c : 0) * graphics->font->bytes_per_glyph;

        for (uint32_t current_y = 0; current_y < graphics->font->height; current_y++) {
            uint32_t mask = 1 << (graphics->font->width - 1);

            for (uint32_t current_x = 0; current_x < graphics->font->width; current_x++) {
                if (*((uint32_t *) glyph) & mask) {
                    uefi_graphics_draw_rect(graphics, (current_x * scale) + x, (current_y * scale) + y, scale, scale);
                }

                mask >>= 1;
            }

            glyph += bytes_per_line;
        }

        text++;
        x += graphics->font->width * scale + 1;
    }
}

uint32_t uefi_graphics_text_width(uefi_graphics_t *graphics, const char *text) {
    uint32_t len = strlen(text);
    return (len * graphics->font->width + len) * graphics->font_scale;
}

uint32_t uefi_graphics_text_height(uefi_graphics_t *graphics) {
    return graphics->font->height * graphics->font_scale;
}

void uefi_graphics_draw_raw(
        uefi_graphics_t *graphics,
        uint32_t x,
        uint32_t y,
        uint32_t width,
        uint32_t height,
        const void *data
) {
    const uint32_t *img = data;

    for (uint32_t img_y = 0; img_y < height; img_y++) {
        for (uint32_t img_x = 0; img_x < width; img_x++) {
            uint32_t color = convert_rgba_to_mode(graphics, img[img_x + img_y * width]);

            if (color & 0x000000FF) {
                set_pixel(graphics, x + img_x, y + img_y, color);
            }
        }
    }
}

void uefi_graphics_request_redraw(uefi_graphics_t *graphics) {
    graphics->should_redraw = TRUE;
}

void uefi_graphics_flush(uefi_graphics_t *graphics) {
    if (graphics->pixel_buffer) {
        graphics->protocol->Blt(
                graphics->protocol,
                graphics->pixel_buffer,
                EfiBltBufferToVideo,
                0, 0,
                0, 0,
                graphics->width, graphics->height,
                0
        );
    }

    graphics->should_redraw = FALSE;
}
