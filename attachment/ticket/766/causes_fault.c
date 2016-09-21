// Link with: -lGL -lX11 -lXm -lXt

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>

#include <Xm/Xm.h>
#include <Xm/DrawingAP.h>

int main(int argc, char *argv[])
{
    int nruns = (argc > 1) ? atoi(argv[1]) : 1;

    // Request debugging spew
    setenv("LIBGL_DEBUG",         "1", 1);
    setenv("LIBGL_DIAGNOSTIC",    "1", 1);
    setenv("LIBGL_DUMP_VISUALID", "1", 1);

    XtToolkitInitialize();
    XtAppContext appContext = XtCreateApplicationContext();

    Display *pDisplay = XtOpenDisplay( appContext, NULL,
        (String)NULL, "glxtest", (XrmOptionDescList)NULL, 0,
        &argc, argv);

    int iAttrList[] = {GLX_RGBA, GLX_DOUBLEBUFFER, None};
    XVisualInfo *pVisInfo = glXChooseVisual(pDisplay, 0, iAttrList);

    GLXContext glContext = glXCreateContext(pDisplay, pVisInfo, NULL, GL_TRUE);
    
    for (int i = 0; i < nruns; i++)
    {
        printf("==== RUN %d ====\n", i+1);
        
        Widget wParent   = XtAppCreateShell(NULL, "glxtest", applicationShellWidgetClass, pDisplay, NULL, 0);
        Widget wDrawable = XtCreateManagedWidget("_window", xmDrawingAreaWidgetClass, wParent,      NULL, 0);
        XtRealizeWidget(wParent);

        glXMakeCurrent(pDisplay, XtWindow(wDrawable), glContext);

        // Cleanup; doesn't appear to affect the bug
        glXMakeCurrent(pDisplay, None, NULL);
        XtDestroyWidget(wParent);
        XSync(pDisplay, False);
    }

    return 0;
}
