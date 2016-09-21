/*
    gcc -o ${HBIN:-.}/xnestxftbug XQuartzXNestXftBug.c -I/opt/X11/include -I/opt/X11/include/freetype2 -L/opt/X11/lib -lX11 -lXft
*/

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

void main (void)
{
    Display *dsp = XOpenDisplay (0);
    if (dsp)
    {
        int scr = DefaultScreen (dsp);

        Window p = XCreateSimpleWindow (dsp, RootWindow (dsp, scr), 100, 100, 300, 100, 1,
                                             BlackPixel (dsp, scr), WhitePixel (dsp, scr));
        XSelectInput (dsp, p, ExposureMask);
        XMapWindow (dsp, p);

        XftColor xft_color;
        XftColorAllocName (dsp, DefaultVisual (dsp, scr), DefaultColormap (dsp, scr), "tan", &xft_color);

        XftFont *font = XftFontOpenName (dsp, scr, "sans:bold:pixelsize=18");
        XftDraw *draw = XftDrawCreate (dsp, p, DefaultVisual (dsp, scr), DefaultColormap (dsp, scr));

        for (;;)
        {
            char *txt = "Lorem ipsum";
            XEvent e;
            XNextEvent (dsp, &e);
            if (e.type == Expose)
                XftDrawString8 (draw, &xft_color, font, 20, 30, (XftChar8*)(txt), strlen(txt));
        }
    }
}
