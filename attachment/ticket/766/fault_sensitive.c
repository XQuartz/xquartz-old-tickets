// Link with: -lGL -lX11 -lXm -lXt

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <X11/Xlib.h>

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>

int main(int argc, char *argv[])
{
    int nruns = (argc > 1) ? atoi(argv[1]) : 1;

    // Request debugging spew
    setenv("LIBGL_DEBUG",         "1", 1);
    setenv("LIBGL_DIAGNOSTIC",    "1", 1);
    setenv("LIBGL_DUMP_VISUALID", "1", 1);

    Display *pDisplay = XOpenDisplay(NULL);

    int iAttrList[] = { GLX_RGBA, GLX_DOUBLEBUFFER, None};
    XVisualInfo *vis = glXChooseVisual(pDisplay, 0, iAttrList);

    Window root = DefaultRootWindow(pDisplay);
    GLXContext glContext = glXCreateContext(pDisplay, vis, NULL, 1);

    for (int i = 0; i < nruns; i++)
    {
        printf("==== RUN %d ====\n", i+1);

        Window win = XCreateSimpleWindow(pDisplay, root, 10, 10, 100, 100, 0, 0, 0);

        glXMakeCurrent(pDisplay, win, glContext);

        // Cleanup
        glXMakeCurrent(pDisplay, None, NULL);
        XDestroyWindow(pDisplay, win);
    }

    return 0;
}

