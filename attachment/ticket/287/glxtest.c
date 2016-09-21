#include <X11/Xlib.h>
#include <GL/glx.h>

int main(void) {
	Display *dpy = XOpenDisplay(NULL);
	int scr = DefaultScreen(dpy), n;
	Window win = DefaultRootWindow(dpy);
	Visual *vis = DefaultVisual(dpy, scr);
	XVisualInfo tmpl, *vinfo;
	GLXContext glrc;

	tmpl.visualid = XVisualIDFromVisual(vis);
	vinfo = XGetVisualInfo(dpy, VisualIDMask, &tmpl, &n);
	if (vinfo) {
		glrc = glXCreateContext(dpy, vinfo, None, GL_TRUE);
	} else glrc = NULL;
	if (glrc) {
		glXMakeCurrent(dpy, win, glrc);
		glXMakeCurrent(dpy, None, NULL);
		glXDestroyContext(dpy, glrc);
	}
	XCloseDisplay(dpy);
	return 0;
}

