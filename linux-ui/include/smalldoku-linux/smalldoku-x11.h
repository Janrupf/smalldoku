#pragma once

#include <X11/Xlib.h>

#include <smalldoku-core-ui/smalldoku-core-ui.h>

/**
 * X11 implementation of the smalldoku graphics.
 */
struct smalldoku_x11_graphics {
    SMALLDOKU_GRAPHICS_STRUCT_MEMBERS;

    /**
     * The X11 graphics context.
     */
    GC gc;

    /**
     * The X11 display handle.
     */
    Display *display;

    /**
     * The X11 window handle.
     */
    Window window;

    /**
     * The font used by the graphics context.
     */
    XFontStruct *font;
};

typedef struct smalldoku_x11_graphics smalldoku_x11_graphics_t;

int smalldoku_run_x11(int argc, const char **argv);
