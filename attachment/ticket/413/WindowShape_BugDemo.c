/*
    gcc -o ${HBIN:-.}/windowshape_bugdemo WindowShape_BugDemo.c -lX11 -lXext -L/usr/X11R6/lib -I/usr/X11R6/include
*/
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>

#define BORDER 6

void main (void)
{
    int		    i, j;
    XColor	    col, tmp;

    Display * dsp = XOpenDisplay (0);
    int scr = XDefaultScreen (dsp);

    XAllocNamedColor (dsp, XDefaultColormap (dsp, scr), "bisque", &col, &tmp);

    Window win = XCreateSimpleWindow (dsp, XRootWindow (dsp, scr),
			0, 0, 300, 200, BORDER, XWhitePixel (dsp, scr), col.pixel);

    XAllocNamedColor (dsp, XDefaultColormap (dsp, scr), "aquamarine", &col, &tmp);
    XSetWindowBorder (dsp, win, col.pixel);

    if ((True == XShapeQueryExtension (dsp, &i, &j)) && XShapeQueryVersion (dsp, &i, &j))
    {
	XSelectInput (dsp, win, StructureNotifyMask);
	XMoveWindow (dsp, win, 1, 1); /*trigger setting initial shape*/
    }

    XMapWindow (dsp, win);
    for (;;)
    {
	XEvent e;
	XNextEvent (dsp, &e);
	if (e.type == ConfigureNotify)
	{
	    XRectangle r;
	    r.x = -BORDER;  /* fill hole */
	    r.y = -BORDER;
	    r.width = e.xconfigure.width + 2*BORDER;
	    r.height = e.xconfigure.height + 2*BORDER;
	    XShapeCombineRectangles (dsp, e.xconfigure.window, ShapeBounding, 0, 0, &r, 1, ShapeSet, Unsorted);

	    if (e.xconfigure.width > 4*BORDER && e.xconfigure.height > 4*BORDER)
	    {
		r.x = BORDER;  /* set shape: clip & bounding */
		r.y = BORDER;
		r.width = e.xconfigure.width - 2*BORDER;
		r.height = e.xconfigure.height - 2*BORDER;
		XShapeCombineRectangles (dsp, e.xconfigure.window, ShapeClip, 0, 0, &r, 1, ShapeSet, Unsorted);

		r.x = 2*BORDER;
		r.y = 2*BORDER;
		r.width = e.xconfigure.width - 4*BORDER;
		r.height = e.xconfigure.height - 4*BORDER;
		XShapeCombineRectangles (dsp, e.xconfigure.window, ShapeBounding, 0, 0, &r, 1, ShapeSubtract, Unsorted);
	    }
	}
    }
}


