#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

int main() {
  Display *display;
  Window win, subwin;
  int screen;
  int depth;
  Pixmap pixmap; 
  Visual *visual;
  XSetWindowAttributes attributes; 
  unsigned int white;
  unsigned int black;
  XEvent event;
  int i;
 
  display = XOpenDisplay(NULL);
  screen  = DefaultScreen(display);
  visual  = DefaultVisual(display,screen); 
  depth   = DefaultDepth(display,screen); 
  
  white = WhitePixel(display,screen);
  black = BlackPixel(display,screen);

  attributes.background_pixel      = white;
  attributes.border_pixel          = black;
  attributes.override_redirect     = 0; 

  /* Create main window */

  win = XCreateWindow(display, XRootWindow(display,screen),
		200, 200, 400, 400, 0, depth, InputOutput, visual,
		CWBackPixel | CWBorderPixel | CWOverrideRedirect, 
		&attributes);

  XSelectInput(display,win,ExposureMask | ButtonPressMask | KeyPressMask);
  XMapWindow(display, win);
  XFlush(display);
 
  /* Create subwindow. The borders are not shown in 1.5 servers */
 
  subwin = XCreateSimpleWindow(display, win, 100, 100, 200, 200, 10, black, white);
  XMapWindow(display, subwin);
  XFlush(display);

  i = 1;
  do {
    XNextEvent(display,&event); 
    switch(event.type) {
    case ButtonPress:
    case KeyPress:
      i = 0;
      break;
    }
  } while (i);

  return(0);
}

