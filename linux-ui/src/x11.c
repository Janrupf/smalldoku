#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#include <X11/Xlib.h>

#include <smalldoku/smalldoku.h>
#include "smalldoku-linux/smalldoku-x11.h"

const int32_t OUTER_PADDING = 20;
const int32_t SCALE = 80;

static void create_window(Display **display, int *screen, Window *window, Atom *delete_window_atom) {
    *display = XOpenDisplay(NULL);

    if (!*display) {
        fprintf(stderr, "Can't open X11 display!\n");
        exit(1);
    }

    *delete_window_atom = XInternAtom(*display, "WM_DELETE_WINDOW", False);

    *screen = DefaultScreen(*display);

    Window root_window = RootWindow(*display, *screen);
    unsigned long black_pixel = BlackPixel(*display, *screen);
    unsigned long white_pixel = WhitePixel(*display, *screen);

    *window = XCreateSimpleWindow(
            *display,
            root_window,
            10,
            10,
            SCALE * SMALLDOKU_GRID_WIDTH + OUTER_PADDING * 2,
            SCALE * SMALLDOKU_GRID_HEIGHT + OUTER_PADDING * 2,
            1,
            black_pixel,
            white_pixel
    );
    XSetWMProtocols(*display, *window, delete_window_atom, 1);

    XSelectInput(*display, *window, ExposureMask | KeyPressMask | ButtonPressMask);
    XMapWindow(*display, *window);
}

static GC create_gc(Display *display, int x11_screen, Window window) {
    XGCValues gc_values;
    GC gc = XCreateGC(display, window, 0, &gc_values);
    if (!gc) {
        fprintf(stderr, "Failed to create GC!\n");
        exit(1);
    }

    XSetForeground(display, gc, BlackPixel(display, x11_screen));
    XSetBackground(display, gc, WhitePixel(display, x11_screen));
    XSync(display, False);

    return gc;
}

static XFontStruct *load_font(Display *display, const char *font_specification) {
    XFontStruct *font = XLoadQueryFont(display, font_specification);

    if (!font) {
        fprintf(stderr, "Failed to load font for specification %s!\n", font_specification);
        exit(1);
    }

    return font;
}

static uint8_t generate_random_number(uint8_t min, uint8_t max) {
    return rand() % (max + 1 - min) + min;
}

static void get_window_size(smalldoku_x11_graphics_t *graphics, smalldoku_uint32_t *width, smalldoku_uint32_t *height) {
    Window root_window;
    int32_t x_pos;
    int32_t y_pos;
    uint32_t border_width;
    uint32_t depth;

    XGetGeometry(graphics->display, graphics->window, &root_window, &x_pos, &y_pos, width, height, &border_width,
                 &depth);
}

static void get_text_size(
        smalldoku_x11_graphics_t *graphics,
        const char *text,
        smalldoku_uint32_t *width,
        smalldoku_uint32_t *height
) {
    int extent_direction, extent_ascend, extent_descent;
    XCharStruct extent_overall;
    XTextExtents(graphics->font, text, (int) strlen(text), &extent_direction, &extent_ascend, &extent_descent,
                 &extent_overall);

    if (width) {
        *width = extent_overall.width;
    }

    if (height) {
        *height = extent_ascend;
    }
}

static void set_fill(smalldoku_x11_graphics_t *graphics, smalldoku_uint32_t color) {
    XSetForeground(graphics->display, graphics->gc, color);
}

static void draw_rect(
        smalldoku_x11_graphics_t *graphics,
        smalldoku_uint32_t x,
        smalldoku_uint32_t y,
        smalldoku_uint32_t width,
        smalldoku_uint32_t height
) {
    XFillRectangle(graphics->display, graphics->window, graphics->gc, (int) x, (int) y, width, height);
}

static void draw_text(
        smalldoku_x11_graphics_t *graphics,
        smalldoku_uint32_t x,
        smalldoku_uint32_t y,
        const char *text
) {
    XDrawString(graphics->display, graphics->window, graphics->gc, (int) x, (int) y, text, (int) strlen(text));
}

static void request_redraw(smalldoku_x11_graphics_t *graphics) {
    XClearArea(graphics->display, graphics->window, 0, 0, 0, 0, 1);
}

int smalldoku_run_x11(int argc, const char **argv) {
    Display *display;
    int x11_screen;
    Window window;
    Atom delete_window_atom;
    create_window(&display, &x11_screen, &window, &delete_window_atom);
    GC gc = create_gc(display, x11_screen, window);
    XFontStruct *dejavu_font = load_font(display, "-*-dejavu sans mono-medium-r-normal-*-*-200-*-*-m-*-ascii-*");

    smalldoku_x11_graphics_t graphics = {
            .query_size = (smalldoku_query_size_fn) get_window_size,
            .query_text_size = (smalldoku_query_text_size_fn) get_text_size,
            .set_fill = (smalldoku_set_fill_fn) set_fill,
            .draw_rect = (smalldoku_draw_rect_fn) draw_rect,
            .draw_text = (smalldoku_draw_text_fn) draw_text,
            .request_redraw = (smalldoku_request_redraw_fn) request_redraw,
            .gc = gc,
            .display = display,
            .window = window,
            .font = dejavu_font
    };

    smalldoku_core_ui_t ui = smalldoku_core_ui_new((smalldoku_graphics_t *) &graphics, generate_random_number);
    smalldoku_core_ui_begin_game(&ui);

    XSetFont(display, gc, dejavu_font->fid);

    srand(time(NULL));

    while (1) {
        XEvent event;
        XNextEvent(display, &event);

        switch (event.type) {
            case Expose: {
                smalldoku_core_ui_draw_centered(&ui);
                break;
            }

            case KeyPress: {
                KeySym sym = XLookupKeysym(&event.xkey, 0);

                if(sym <= 255) {
                    smalldoku_core_ui_key(&ui, (char) sym);
                }

                if (event.xkey.keycode == 0x09) {
                    goto exit_program;
                }
                break;
            }

            case ButtonPress: {
                smalldoku_core_ui_click(&ui, event.xbutton.x, event.xbutton.y);
                break;
            }

            case ClientMessage: {
                if (event.xclient.data.l[0] == delete_window_atom) {
                    goto exit_program;
                }
            }

            case UnmapNotify:
            case DestroyNotify:
                goto exit_program;
        }
    }

    exit_program:
    XFreeFont(display, dejavu_font);
    XUnmapWindow(display, window);
    XDestroyWindow(display, window);
    XCloseDisplay(display);

    exit(0);
}
