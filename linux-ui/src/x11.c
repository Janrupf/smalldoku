#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <smalldoku/smalldoku.h>
#include "smalldoku-linux/smalldoku-x11.h"

const int32_t OUTER_PADDING = 20;
const int32_t SCALE = 80;

static void handle_click(SMALLDOKU_GRID(grid), int x, int y) {
    for (uint8_t row = 0; row < SMALLDOKU_GRID_HEIGHT; row++) {
        for (uint8_t col = 0; col < SMALLDOKU_GRID_WIDTH; col++) {
            grid[row][col].user_data = 0x0;
        }
    }

    if (x < OUTER_PADDING || y < OUTER_PADDING) {
        return;
    }

    uint8_t col = (x - OUTER_PADDING) / SCALE;
    uint8_t row = (y - OUTER_PADDING) / SCALE;

    if (col > SMALLDOKU_GRID_WIDTH || row > SMALLDOKU_GRID_HEIGHT) {
        return;
    }

    if (grid[row][col].type == SMALLDOKU_USER_CELL) {
        grid[row][col].user_data = (void *) 0x1;
    }
}

static int enter_number(SMALLDOKU_GRID(grid), uint8_t value) {
    for (uint8_t row = 0; row < SMALLDOKU_GRID_HEIGHT; row++) {
        for (uint8_t col = 0; col < SMALLDOKU_GRID_WIDTH; col++) {
            if (grid[row][col].user_data == (void *) 0x1) {
                grid[row][col].user_value = value;
                return 1;
            }
        }
    }

    return 0;
}

static void draw_grid(SMALLDOKU_GRID(grid), Display *display, Window window, GC gc, XFontStruct *font) {
    XSetFont(display, gc, font->fid);

    for (uint8_t row = 0; row < SMALLDOKU_GRID_HEIGHT; row++) {
        for (uint8_t col = 0; col < SMALLDOKU_GRID_WIDTH; col++) {
            uint8_t value = smalldoku_get_cell_value(grid, row, col);

            int32_t rect_start_x = OUTER_PADDING + (col * SCALE);
            int32_t rect_start_y = OUTER_PADDING + (row * SCALE);

            if (grid[row][col].type == SMALLDOKU_GENERATED_CELL) {
                XSetForeground(display, gc, 0xCCCCCC);
                XFillRectangle(display, window, gc, rect_start_x, rect_start_y, SCALE, SCALE);
            } else if (grid[row][col].user_data == (void *) 0x1) {
                XSetForeground(display, gc, 0xCCCC00);
                XFillRectangle(display, window, gc, rect_start_x, rect_start_y, SCALE, SCALE);
            } else if (grid[row][col].user_data == (void *) 0x2) {
                XSetForeground(display, gc, 0x55AA55);
                XFillRectangle(display, window, gc, rect_start_x, rect_start_y, SCALE, SCALE);
            } else if (grid[row][col].user_data == (void *) 0x3) {
                XSetForeground(display, gc, 0xAA5555);
                XFillRectangle(display, window, gc, rect_start_x, rect_start_y, SCALE, SCALE);
            }

            XSetForeground(display, gc, 0x000000);

            if (value != 0) {
                char display_char = (char) (value + '0');

                int extent_direction, extent_ascend, extent_descent;
                XCharStruct extent_overall;
                XTextExtents(font, &display_char, 1, &extent_direction, &extent_ascend, &extent_descent,
                             &extent_overall);

                int32_t text_x = OUTER_PADDING + (col * SCALE) + (SCALE / 2) - (extent_overall.width / 2);
                int32_t text_y = OUTER_PADDING + (row * SCALE) + (SCALE / 2) +
                                 ((extent_overall.ascent + extent_overall.descent) / 2);

                XDrawString(display, window, gc, text_x, text_y, &display_char, 1);
            }
        }
    }

    for (uint8_t col = 0; col <= SMALLDOKU_GRID_WIDTH; col++) {
        int32_t start_x = OUTER_PADDING + (col * SCALE);
        int32_t start_y = OUTER_PADDING;
        int32_t end_x = start_x;
        int32_t end_y = start_y + (SMALLDOKU_GRID_HEIGHT * SCALE);

        if (col % SMALLDOKU_SQUARE_WIDTH == 0) {
            XSetLineAttributes(display, gc, 5, LineSolid, CapButt, JoinBevel);
        } else {
            XSetLineAttributes(display, gc, 2, LineSolid, CapButt, JoinBevel);
        }

        XDrawLine(display, window, gc, start_x, start_y, end_x, end_y);
    }

    for (uint8_t row = 0; row <= SMALLDOKU_GRID_HEIGHT; row++) {
        int32_t start_x = OUTER_PADDING;
        int32_t start_y = OUTER_PADDING + (row * SCALE);
        int32_t end_x = start_x + (SMALLDOKU_GRID_WIDTH * SCALE);
        int32_t end_y = start_y;

        if (row % SMALLDOKU_SQUARE_HEIGHT == 0) {
            XSetLineAttributes(display, gc, 5, LineSolid, CapButt, JoinBevel);
        } else {
            XSetLineAttributes(display, gc, 2, LineSolid, CapButt, JoinBevel);
        }

        XDrawLine(display, window, gc, start_x, start_y, end_x, end_y);
    }
}

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
    if (gc < 0) {
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

static void regenerate_grid(SMALLDOKU_GRID(grid)) {
    smalldoku_init(grid);
    smalldoku_fill_grid(grid, generate_random_number);
    smalldoku_hammer_grid(grid, 20, generate_random_number);
}

static void check_grid(SMALLDOKU_GRID(grid)) {
    for (uint8_t row = 0; row < SMALLDOKU_GRID_HEIGHT; row++) {
        for (uint8_t col = 0; col < SMALLDOKU_GRID_WIDTH; col++) {
            if (grid[row][col].type == SMALLDOKU_USER_CELL) {
                if(grid[row][col].user_value == grid[row][col].value) {
                    grid[row][col].user_data = (void *) 0x2;
                } else {
                    grid[row][col].user_data = (void *) 0x3;
                }
            }
        }
    }
}

int smalldoku_run_x11(int argc, const char **argv) {
    Display *display;
    int x11_screen;
    Window window;
    Atom delete_window_atom;
    create_window(&display, &x11_screen, &window, &delete_window_atom);
    GC gc = create_gc(display, x11_screen, window);

    XFontStruct *dejavu_font = load_font(display, "-*-dejavu sans mono-medium-r-normal-*-*-200-*-*-m-*-ascii-*");

    srand(time(NULL));

    SMALLDOKU_GRID(grid);
    regenerate_grid(grid);

    while (1) {
        XEvent event;
        XNextEvent(display, &event);

        switch (event.type) {
            case Expose: {
                draw_grid(grid, display, window, gc, dejavu_font);
                break;
            }

            case KeyPress: {
                KeySym sym = XLookupKeysym(&event.xkey, 0);

                switch (sym) {
                    case XK_Escape:
                        goto exit_program;

                    case XK_r:
                        regenerate_grid(grid);
                        XClearArea(display, window, 0, 0, 0, 0, 1);
                        break;

                    case XK_c:
                        check_grid(grid);
                        XClearArea(display, window, 0, 0, 0, 0, 1);
                        break;

                    default: {
                        if (sym >= XK_1 && sym <= XK_9) {
                            uint8_t num = sym - XK_0;
                            if (enter_number(grid, num)) {
                                XClearArea(display, window, 0, 0, 0, 0, 1);
                            }
                        }
                    }
                }

                if (event.xkey.keycode == 0x09) {
                    goto exit_program;
                }
                break;
            }

            case ButtonPress: {
                handle_click(grid, event.xbutton.x, event.xbutton.y);
                XClearArea(display, window, 0, 0, 0, 0, 1);
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
