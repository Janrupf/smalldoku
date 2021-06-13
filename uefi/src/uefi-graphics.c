#include "smalldoku-uefi/smalldoku-uefi-graphics.h"

static uint32_t convert_rgba_to_mode(uefi_graphics_t *graphics, uint32_t rgb) {
    switch (graphics->protocol->Mode->Info->PixelFormat) {
        case PixelRedGreenBlueReserved8BitPerColor:
            return ((rgb & 0xFF0000) >> 24) |
                   ((rgb & 0x00FF00) >> 8) |
                   ((rgb & 0x0000FF) << 8);

        case PixelBlueGreenRedReserved8BitPerColor:
            return rgb;

        default:
            __asm__("ud2"); /* Should never happen */
            return 0;
    }
}

static void set_pixel(uefi_graphics_t *graphics, uint32_t x, uint32_t y, uint32_t native_color) {
    EFI_PHYSICAL_ADDRESS framebuffer_base = graphics->protocol->Mode->FrameBufferBase;
    uint32_t pixels_per_scan_line = graphics->protocol->Mode->Info->PixelsPerScanLine;

    *((uint32_t *) (framebuffer_base + 4 * pixels_per_scan_line * y + 4 * x)) = native_color;
}

static uint32_t strlen(const char *str) {
    uint32_t len = 0;
    while (*str) {
        len++;
        str++;
    }

    return len;
}

uefi_graphics_t uefi_graphics_initialize(EFI_GRAPHICS_OUTPUT_PROTOCOL *protocol) {
    uefi_graphics_t graphics = {protocol, NULL};
    return graphics;
}

void uefi_graphics_set_font(uefi_graphics_t *graphics, uefi_graphics_psf_font_t *font, uint8_t font_scale) {
    graphics->font = font;
    graphics->font_scale = font_scale;
}

void uefi_graphics_draw_rect(
        uefi_graphics_t *graphics,
        uint32_t x,
        uint32_t y,
        uint32_t width,
        uint32_t height,
        uint32_t color
) {
    uint32_t real_color = convert_rgba_to_mode(graphics, color);

    for (uint32_t cx = x; cx <= x + width; cx++) {
        for (uint32_t cy = y; cy <= y + height; cy++) {
            set_pixel(graphics, cx, cy, real_color);
        }
    }
}

uint32_t uefi_graphics_text_width(uefi_graphics_t *graphics, const char *text) {
    uint32_t len = strlen(text);
    return (len * graphics->font->width + len) * graphics->font_scale;
}

uint32_t uefi_graphics_text_height(uefi_graphics_t *graphics) {
    return graphics->font->height * graphics->font_scale;
}

void uefi_graphics_draw_text(uefi_graphics_t *graphics, uint32_t x, uint32_t y, const char *text, uint32_t color) {
    uint32_t bytes_per_line = (graphics->font->width + 7) / 8;
    uint32_t scale = graphics->font_scale;

    while (*text) {
        char c = *text;

        uint8_t *glyph = ((uint8_t *) graphics->font) +
                         graphics->font->header_size +
                         (c > 0 && c < graphics->font->glyph_count ? c : 0) * graphics->font->bytes_per_glyph;

        for (uint32_t current_y = 0; current_y < graphics->font->height; current_y++) {
            uint32_t mask = 1 << (graphics->font->width - 1);

            for (uint32_t current_x = 0; current_x < graphics->font->width; current_x++) {
                if (*((uint32_t *) glyph) & mask) {
                    uefi_graphics_draw_rect(graphics, (current_x * scale) + x, (current_y * scale) + y, scale, scale,
                                            color);
                }

                mask >>= 1;
            }

            glyph += bytes_per_line;
        }

        text++;
        x += graphics->font->width + 1;
    }
}
