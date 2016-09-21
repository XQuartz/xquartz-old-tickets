/*
    gcc -o depth15text XQuartzBugDepth15.c -I/opt/X11/include -I/opt/X11/include/freetype2 -L/opt/X11/lib -lX11 -lXft
*/

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#define DEPTH 24    /* 15, 24 */

main (int argc, char **argv)
{
    Display *dsp = XOpenDisplay (0);
    int scr = DefaultScreen (dsp);

    Window p = XCreateSimpleWindow (dsp, RootWindow (dsp, scr), 100, 100, 300, 100, 1,
                                    BlackPixel (dsp, scr), WhitePixel (dsp, scr));
    XMapWindow (dsp, p);

    XVisualInfo xvis;
    XMatchVisualInfo (dsp, scr, DEPTH, TrueColor, &xvis);

    XSetWindowAttributes att;
    att.event_mask = ExposureMask;
    att.colormap = XCreateColormap (dsp, RootWindow (dsp, scr), xvis.visual, AllocNone);

    XRenderColor render_color;
    XftColor xft_color;
    render_color.red = render_color.green = render_color.blue = 200 * 257;
    render_color.alpha = 0xffff;
    XftColorAllocValue (dsp, xvis.visual, att.colormap, &render_color, &xft_color);

    att.border_pixel = xft_color.pixel;

    Window  w = XCreateWindow (dsp, p, 10, 10, 200, 50, 2, DEPTH, InputOutput, xvis.visual,
                                (CWEventMask | CWColormap | CWBorderPixel), &att);
    XMapWindow (dsp, w);

    XftDraw *draw = XftDrawCreate (dsp, w, xvis.visual, att.colormap);
    XftFont *font = XftFontOpenName (dsp, scr, "sans:bold:pixelsize=18");

    char *str = "Lorem ipsum";
    XEvent ev;

    for (;;)
    {
        XNextEvent (dsp, &ev);
        if (ev.type == Expose)
            XftDrawString8 (draw, &xft_color, font, 20, 30, (XftChar8*)(str), strlen(str));
    }
}
