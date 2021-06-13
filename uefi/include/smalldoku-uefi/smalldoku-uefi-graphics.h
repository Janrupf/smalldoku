#pragma once

#include <efi.h>

/**
 * Represents a simple PSF font.
 */
struct uefi_graphics_psf_font {
    uint32_t magic;
    uint32_t version;
    uint32_t header_size;
    uint32_t flags;
    uint32_t glyph_count;
    uint32_t bytes_per_glyph;
    uint32_t height;
    uint32_t width;
};

typedef struct uefi_graphics_psf_font uefi_graphics_psf_font_t;

/**
 * Container for an UEFI graphics context.
 */
struct uefi_graphics {
    /**
     * The protocol used for drawing.
     */
    EFI_GRAPHICS_OUTPUT_PROTOCOL *protocol;

    /**
     * The font currently in use.
     */
    uefi_graphics_psf_font_t *font;

    /**
     * The scale of the PSF font.
     */
    uint8_t font_scale;
};

typedef struct uefi_graphics uefi_graphics_t;

/**
 * Initializes an uefi graphics using the given protocol.
 *
 * @param protocol the protocol to use for initialization
 * @return the initialized context
 */
uefi_graphics_t uefi_graphics_initialize(EFI_GRAPHICS_OUTPUT_PROTOCOL *protocol);

/**
 * Sets the active graphics context font.
 *
 * @param graphics the graphics context to set the font for
 * @param font the new font to use
 * @param font_scale the multiplier to use when rendering
 */
void uefi_graphics_set_font(uefi_graphics_t *graphics, uefi_graphics_psf_font_t *font, uint8_t font_scale);

/**
 * Draws a rectangle.
 *
 * @param graphics the graphics context to draw with
 * @param x the upper left x coordinate of the rectangle
 * @param y the upper left y coordinate of the rectangle
 * @param width the width of the rectangle
 * @param height the height of the rectangle
 * @param color the color of the rectangle
 */
void uefi_graphics_draw_rect(
        uefi_graphics_t *graphics,
        uint32_t x,
        uint32_t y,
        uint32_t width,
        uint32_t height,
        uint32_t color
);

/**
 * Calculates the width of a text in pixels.
 *
 * @param graphics the graphics context to calculate the width for
 * @param text the text to calculate the width for
 * @return the calculated width in pixels
 */
uint32_t uefi_graphics_text_width(uefi_graphics_t *graphics, const char *text);

/**
 * Calculates the height of a text in pixels.
 *
 * @param graphics the graphics context to calculate the height for
 * @param text the text to calculate the height for
 * @return the calculated height in pixels
 */
uint32_t uefi_graphics_text_height(uefi_graphics_t *graphics);

/**
 * Draws a string.
 *
 * @param graphics the graphics context to draw with
 * @param x the x coordinate to start drawing at
 * @param y the y coordinate to start drawing at
 * @param text the text to draw
 * @param color the color to draw the text in
 */
void uefi_graphics_draw_text(uefi_graphics_t *graphics, uint32_t x, uint32_t y, const char *text, uint32_t color);
