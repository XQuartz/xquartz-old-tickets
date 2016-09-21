#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>

#include <cairo.h>
#include <cairo-xlib.h>

#define PI 3.1415926535

typedef struct win {
    Display *dpy;
    Window win;
    int width, height;
    KeyCode quit_code;
} win_t;


static int push_group = 0;

static cairo_surface_t *surface = NULL;

static void win_draw(win_t *win, int x, int y, int push_group)
{
    cairo_t *cr = NULL;
    int i,j;
    cairo_scaled_font_t *font;

    Visual *visual = DefaultVisual(win->dpy, DefaultScreen (win->dpy));

    if (!surface) {
	surface = cairo_xlib_surface_create(
	    win->dpy, win->win, visual, win->width, win->height
	);
    }

    cr = cairo_create(surface);

    if (push_group) cairo_push_group(cr);

    cairo_save (cr);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint (cr);
    cairo_restore (cr);

    cairo_set_source_rgb(cr, 0, 0, 0);

    cairo_save(cr);

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);

    cairo_select_font_face (
	cr, "serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD
    );
    cairo_set_font_size (cr, 12);
    font = cairo_get_scaled_font(cr);

    for (j = 0; j < 8; j++) {
	char text[200];
	cairo_status_t status;
	cairo_glyph_t *glyphs = NULL;
	int num_glyphs;
	for (i = 0; i < 12; i++) {
	    sprintf(text+i, "%c", 'a' + i%26);
	}
	status = cairo_scaled_font_text_to_glyphs(
	    font,
	    0, 0,
	    text, strlen(text),
	    &glyphs, &num_glyphs,
	    NULL, NULL,
	    NULL
	);
	if (status == CAIRO_STATUS_SUCCESS) {
	    for (i = 0; i < 12; i++) {
		glyphs[i].x += x + 55 + i*3;
		glyphs[i].y += y + 55 + j*15;
	    }
	    cairo_show_glyphs(cr, glyphs, num_glyphs);
	    cairo_glyph_free(glyphs);
	}
    }
    cairo_fill(cr);

    for (i = 0; i < 8; i++)
	for (j = 0; j < 8; j++) {
	    cairo_set_line_width(cr, 0.5 * i);
	    cairo_move_to(cr, x + 10 + i*10, y + 10 + j*10);
	    cairo_line_to(cr, x + 18 + i*10, y + 14 + j*10);
	    cairo_stroke(cr);
	}

    cairo_restore(cr);

    if (push_group) {
	cairo_pop_group_to_source(cr);
	cairo_paint(cr);
    }

    if (cairo_status (cr)) {
	printf(
	    "Cairo is unhappy: %s\n",
	    cairo_status_to_string (cairo_status (cr))
	);
	exit(0);
    }

    if (cr) cairo_destroy(cr), cr = NULL;
}

static void win_init(win_t *win)
{
    Window root;
    int screen = DefaultScreen(win->dpy);

    win->width  = 300;
    win->height = 300;

    root = DefaultRootWindow(win->dpy);

    win->win = XCreateSimpleWindow(
	win->dpy, root, 0, 0,
	win->width, win->height, 0,
	BlackPixel(win->dpy, screen),
	BlackPixel(win->dpy, screen)
    );

    win->quit_code = XKeysymToKeycode(win->dpy, XStringToKeysym("Q"));

    XSelectInput(
	win->dpy, win->win,
	  KeyPressMask
	| StructureNotifyMask
	| ExposureMask
	| ButtonMotionMask
	| ButtonPressMask
    );

    XMapWindow(win->dpy, win->win);
}

static void win_deinit(win_t *win)
{
    XDestroyWindow(win->dpy, win->win);
}

static void win_handle_events(win_t *win)
{
    XEvent xev;
    static int x = -1, y = -1;

    while (1) {
	XNextEvent(win->dpy, &xev);

	switch(xev.type) {
	    case KeyPress:
		if (xev.xkey.keycode == win->quit_code) return;
		break;

	    case ConfigureNotify:
		win->width  = xev.xconfigure.width;
		win->height = xev.xconfigure.height;
		if (surface) cairo_surface_destroy (surface), surface = NULL;
		win_draw(win, x, y, push_group);
		break;

	    case Expose:
		if (xev.xexpose.count == 0) x = -1; y = -1;
		win_draw(win, x, y, push_group);
		break;

	    case MotionNotify:
		if (x<0 && y<0) x = xev.xbutton.x, y = xev.xbutton.y;
		win_draw(win, xev.xbutton.x - x, xev.xbutton.y - y, push_group);
		break;
	}
    }
}

int main(int argc, char *argv[])
{
    win_t win;

    win.dpy = XOpenDisplay(0);

    if (win.dpy == NULL) {
	fprintf(stderr, "Failed to open display\n");
	return 1;
    }

    printf("cairo version: %s\n", CAIRO_VERSION_STRING);

    if (argc > 1) if (0 == strcmp(argv[1], "-pushgroup")) push_group = 1;

    printf("pushgroup: %d\n", push_group);

    win_init(&win);
    win_draw(&win, 0, 0, push_group);
    win_handle_events(&win);
    win_deinit(&win);
    XCloseDisplay(win.dpy);

    return 0;
}
