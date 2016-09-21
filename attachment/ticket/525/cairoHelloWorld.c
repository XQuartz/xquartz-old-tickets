/* Adapted from Hello World (C, cairo) at:
 *
 * http://en.literateprograms.org/Hello_World_(C,_Cairo)
 *
 * To compile on MacOSX 10.8.2, with XQuartz 2.7.4 installed:
 *
 *   gcc cairoHelloWorld.c -o cairoHelloWorld -I/opt/X11/include -L/opt/X11/lib -lX11 -lcairo
 *
 * To run showing the incomplete painting:
 *
 *   $ ./cairoHelloWorld
 *
 * To run with a "fix":
 *
 *   $ ./cairoHelloWorld 1
 *
 */
#include<stdio.h>
#include<stdlib.h>
#include<cairo/cairo.h>
#include<cairo/cairo-xlib.h> 
#include<X11/Xlib.h>

int main(int argc, char* argv[])
{
	Display *dpy;
	Window rootwin;
	Window win;
	Colormap cmap;
	XEvent e;
	int scr;
	GC gc;
        int SIZEX = 512;
        int SIZEY = 512;
        int applyFix = 0;

        if (argc > 1) applyFix = atoi(argv[1]);


	if(!(dpy=XOpenDisplay(NULL))) {
		fprintf(stderr, "ERROR: could not open display\n");
		exit(1);
	}
	scr = DefaultScreen(dpy);
	rootwin = RootWindow(dpy, scr);
	cmap = DefaultColormap(dpy, scr);

	win=XCreateSimpleWindow(dpy, rootwin, 1, 1, SIZEX, SIZEY, 0, 
			WhitePixel(dpy, scr), WhitePixel(dpy, scr));
	XStoreName(dpy, win, "hello");

	gc=XCreateGC(dpy, win, 0, NULL);
	XSetForeground(dpy, gc, WhitePixel(dpy, scr));
	XSelectInput(dpy, win, ExposureMask|ButtonPressMask);
	XMapWindow(dpy, win);

	cairo_surface_t* cs=cairo_xlib_surface_create(dpy, win, DefaultVisual(dpy, 0), SIZEX, SIZEY);

	while(1) {
		XNextEvent(dpy, &e);
		if(e.type==Expose && e.xexpose.count<1) {
                  /* XDrawString(dpy, win, gc, 10, 10, "Hello World!", 12); */
                  cairo_t *c=cairo_create(cs);
                  if (applyFix) {
                    cairo_push_group(c);
                  }

#if 0
                  /*  This is literateprogramming's original example; NOTE it does not 
                   * exhibit the problem!
                   */
                  cairo_rectangle(c, 0.0, 0.0, SIZEX, SIZEY);
                  cairo_set_source_rgb(c, 0.0, 0.0, 0.5);
                  cairo_fill(c);
                  cairo_move_to(c, 10.0, 10.0);
                  cairo_set_source_rgb(c, 1.0, 1.0, 0.0);
                  cairo_show_text(c, "Hello World!");
#else
                  /* This is verbatum from example "text" at http://cairographics.org/samples */
                  cairo_select_font_face (c, "Sans", CAIRO_FONT_SLANT_NORMAL,
                                          CAIRO_FONT_WEIGHT_BOLD);
                  cairo_set_font_size (c, 90.0);
                  cairo_move_to (c, 10.0, 135.0);
                  cairo_show_text (c, "Hello");
                  cairo_move_to (c, 70.0, 165.0);
                  cairo_text_path (c, "void");
                  cairo_set_source_rgb (c, 0.5, 0.5, 1);
                  cairo_fill_preserve (c);
                  cairo_set_source_rgb (c, 0, 0, 0);
                  cairo_set_line_width (c, 2.56);
                  cairo_stroke (c);
                  /* draw helping lines */
                  cairo_set_source_rgba (c, 1, 0.2, 0.2, 0.6);
                  cairo_arc (c, 10.0, 135.0, 5.12, 0, 2*3.14159);
                  cairo_close_path (c);
                  cairo_arc (c, 70.0, 165.0, 5.12, 0, 2*3.14159);
                  cairo_fill (c);
                  /* end of code from example "text" */
#endif

                  if (applyFix) {
                    cairo_pop_group_to_source(c);
                    cairo_paint(c);
                  }
                  /* NOTE with original "Hello World" code, nothing is drawn 
                   * until a window resize is performed without the XSync().  Doesn't
                   * make much difference to the cairo "text" code.                   */
                  XSync(dpy, 0);  
}
		else if(e.type==ButtonPress) break;
	}

	XCloseDisplay(dpy);
	return 0;
}
