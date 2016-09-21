/* to compile:
** cc -I/usr/X11/include problem.c -L/usr/X11/lib -lX11 -o problem
*/

#include <stdio.h>
#include <stdlib.h>

#include <X11/X.h>
#include <X11/Xutil.h>

Display *dpy;
Window win1, win2;
KeyCode quit_code;


static Window init(int set_hints)
{
    Window win;
    Window root;
    int screen = DefaultScreen(dpy);

    root = DefaultRootWindow(dpy);

    win = XCreateSimpleWindow(
	dpy, root,
	set_hints ? 450 : 50, 50, 300, 300,
	0,
	BlackPixel(dpy, screen),
	BlackPixel(dpy, screen)
    );

    quit_code = XKeysymToKeycode(dpy, XStringToKeysym("Q"));

    XSelectInput(
	dpy, win,
	  KeyPressMask
	| StructureNotifyMask
	| ExposureMask
	| ButtonMotionMask
	| ButtonPressMask
    );

    XMapWindow(dpy, win);

    if (set_hints) {
	XSizeHints hints = {0};
	hints.x = set_hints ? 450 : 50;
	hints.y = 50;
	hints.width = 300;
	hints.height = 300;
	hints.flags = USPosition | USSize;
	XSetWMNormalHints(dpy, win, &hints);
    }

    return win;
}

static void deinit(Window win)
{
    XDestroyWindow(dpy, win);
}

static void handle_events(void)
{
    XEvent xev;

    while (1) {
	XNextEvent(dpy, &xev);

	switch(xev.type) {
	    case KeyPress:
		if (xev.xkey.keycode == quit_code) return;

		XWithdrawWindow(dpy, win1, 0);
		XMapRaised(dpy, win1);

		XWithdrawWindow(dpy, win2, 0);
		XMapRaised(dpy, win2);
		break;
	}
    }
}

int main(int argc, char *argv[])
{
    dpy = XOpenDisplay(0);

    if (dpy == NULL) {
	fprintf(stderr, "Failed to open display\n");
	return 1;
    }

    fprintf(stderr,
	"\n"
	"Press 'q' to quit.\n"
	"Press any other key to show wandering window problem.\n"
    );

    win1 = init(0);
    win2 = init(1);
    handle_events();
    deinit(win1);
    deinit(win2);
    XCloseDisplay(dpy);

    return 0;
}

