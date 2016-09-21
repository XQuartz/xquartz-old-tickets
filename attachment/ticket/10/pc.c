/* gcc -DEBUG pc.c -o pc -L/usr/X11/lib -lX11 */
/* gcc pc.c -o pc -L/usr/X11/lib -lX11 */

/*
 * Cobbled together from:
 *   http://bellet.info/XVideo/testxv.c
 *   http://users.actcom.co.il/~choo/lupg/tutorials/xlib-programming/xlib-programming.html
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/* trick to verbosely do an expression */
#ifdef	EBUG
#define	debug(expr, fmt, args...) \
	((void)fprintf(stderr, "%s = " fmt "\n", # expr, expr, ## args))
#else
#define	debug(expr, fmt, args...)       (void)(expr)
#endif

/* Used to fill in the name class array */
#define	NC(c)   { # c, c }

/* Used to match class name strings to the constants used by XLib */
static struct {
	char    *name;
	int     class;
} nc[] = {
	NC(PseudoColor),
	NC(GrayScale),
	NC(DirectColor),
	NC(TrueColor),
	NC(StaticColor),
	NC(StaticGray)
};

/* Number of elements in an array */
#define	NELEM(a)        (sizeof (a)/sizeof ((a)[0]))

/* gross way of marking the falied lookup, but whatever */
static int
nclkup(char *name)
{
	int i;

	for (i = 0; i < NELEM(nc); i++) {
		debug(i, "%d:");
		debug(nc[i].name, "%s");
		debug(nc[i].class, "%d");

		if (strcmp(nc[i].name, name) != 0)
			continue;

		return (nc[i].class);
	}

	/* Mark that the lookup failed */
	i = name[0];
	name[0] = 0;

	return (i);
}

int
main(int argc, char *argv[])
{
	int width = 320;
	int height = 200;
	Display* dpy;
	XSetWindowAttributes xswa;
	XVisualInfo xvi;
	Window window;
	XGCValues vals;
	GC gc;
	XEvent event;
	unsigned long mask;
	int res, screen, class, depth;

	if (argc != 3) {
		(void)fprintf(stderr, "Usage %s class depth\n", argv[0]);
		(void)fprintf(stderr, "  examples:\n");
		(void)fprintf(stderr, "    %s PseudoColor 8\n", argv[0]);
		(void)fprintf(stderr, "    %s TrueColor 24\n", argv[0]);
		return (2);
	}

	class = nclkup(argv[1]);
	if (argv[1][0] == 0) {
		argv[1][0] = class;
		(void)fprintf(stderr, "%s: bad class: %s\n", argv[0], argv[1]);
		return (1);
	}

	errno = 0;
	debug(depth = strtoul(argv[2], NULL, 10), "%d, argv[2] = %s", argv[2]);
	if (errno != 0) {
		(void)fprintf(stderr, "%s: bad depth argument: %s\n",
		    argv[0], strerror(errno));
		return (1);
	}

	debug(dpy = XOpenDisplay(NULL),
	      "0x%x");
	if (dpy == NULL) {
		(void)fprintf(stderr, "Cannot connect to X server\n");
		return (1);
	}

	debug(screen = DefaultScreen(dpy), "%d");
	if (screen < 0) {
		fprintf(stderr, "%s: screen not found\n", argv[0]);
		return (1);
	}

	debug(res = XMatchVisualInfo(dpy, screen, depth, class, &xvi), "%d");
	if (res == 0) {
		fprintf(stderr, "%s: visual not found\n", argv[0]);
		return (1);
	}

	debug(xswa.colormap = XCreateColormap(dpy, DefaultRootWindow(dpy),
	    xvi.visual, AllocNone), "0x%x");
	xswa.event_mask = StructureNotifyMask | ExposureMask;
	xswa.background_pixel = 0;
	xswa.border_pixel = 0;

	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	debug(window = XCreateWindow(dpy, DefaultRootWindow(dpy),
	    0, 0,
	    width,
	    height,
	    0, xvi.depth,
	    InputOutput,
	    xvi.visual,
	    mask, &xswa), "0x%x");

	debug(XMapWindow(dpy, window), "%d");

	do {
		XNextEvent(dpy, &event);
	}
	while (event.type != MapNotify || event.xmap.event != window);

	debug(gc = XCreateGC(dpy, window, 0, &vals), "0x%x");

	debug(XSetForeground(dpy, gc, BlackPixel(dpy, screen)), "%d");
	debug(XSetBackground(dpy, gc, WhitePixel(dpy, screen)), "%d");

	debug(XSetLineAttributes(dpy, gc, 2, LineSolid, CapButt, JoinBevel),
	    "%d");
	debug(XSetFillStyle(dpy, gc, FillSolid), "%d");

	debug(XSync(dpy, False), "%d");

	debug(XDrawLine(dpy, window, gc, width / 2, 0, width / 2, height),
	    "%d");

	debug(XSync(dpy, False), "%d");

	debug(XSetForeground(dpy, gc, WhitePixel(dpy, screen)), "%d");
	debug(XSetBackground(dpy, gc, BlackPixel(dpy, screen)), "%d");

	debug(XSetLineAttributes(dpy, gc, 2, LineSolid, CapButt, JoinBevel),
	    "%d");
	debug(XSetFillStyle(dpy, gc, FillSolid), "%d");

	debug(XSync(dpy, False), "%d");

	debug(XDrawLine(dpy, window, gc, 0, height / 2, width, height / 2),
	    "%d");

	debug(XFlush(dpy), "%d");

	sleep(10);

	XCloseDisplay(dpy);

	return (0);
}
