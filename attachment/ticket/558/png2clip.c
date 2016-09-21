/* cc -o png2clip png2clip.c -L/usr/X11R6/lib -lX11 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

Atom XA_TARGETS;
Atom XA_multiple;
Atom XA_image_png;
Atom XA_text_uri_list;

unsigned char *file_data = NULL;
int file_size = 0;
char file_uri[2048];

static const char *GetAtomName(Display *dpy, Atom a)
{
    if (a == None) return "None";
    else	   return XGetAtomName(dpy, a);
}

static int read_whole_file(const char *name, unsigned char **data)
{
    FILE *file = NULL;
    int size = 0;

    *data = NULL;

    file = fopen(name, "rb");
    if (!file) return 0;

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);

    *data = (unsigned char *) malloc(size);
    if (!(*data)) { fclose(file); return 0; }

    fread(*data, 1, size, file);
    fclose(file);

    return size;
}

void set_targets_property(Display* dpy, Window w, Atom property)
{
    int i;
    Atom targets[5];

    targets[0] = XA_TARGETS;
    targets[1] = XA_multiple;
    targets[2] = XA_image_png;
    targets[3] = XA_text_uri_list;
    targets[4] = XA_STRING;

    printf("Offering: ");
    for (i=0; i < 5; i++) printf("%s ", GetAtomName(dpy, targets[i]));
    printf("\n");

    XChangeProperty(
	dpy, w, property, XA_ATOM, 32, PropModeReplace,
	(unsigned char *) targets, 5
    );
}

void process_selection_request(XEvent e)
{
    if (e.type == SelectionRequest) {
	Window owner	 = e.xselectionrequest.owner;
	Atom selection	 = e.xselectionrequest.selection;
	Atom target	 = e.xselectionrequest.target;
	Atom property	 = e.xselectionrequest.property;
	Window requestor = e.xselectionrequest.requestor;
	Time timestamp	 = e.xselectionrequest.time;
	Display *dpy	 = e.xselection.display;
	XEvent s;

	printf("A selection request has arrived!\n");
	printf("Selection atom = %s\n", GetAtomName(dpy, selection));
	printf("Target atom    = %s\n", GetAtomName(dpy, target));
	printf("Property atom  = %s\n", GetAtomName(dpy, property));

	    /* Start by constructing a refusal request. */

	s.xselection.type	= SelectionNotify;
	s.xselection.requestor	= requestor;
	s.xselection.selection	= selection;
	s.xselection.target	= target;
	s.xselection.property	= property;
	s.xselection.time	= timestamp;
	/* s.xselection.serial     - filled in by server */
	/* s.xselection.send_event - filled in by server */
	/* s.xselection.display    - filled in by server */

	if (target == XA_TARGETS) {

	    printf("Replying with a target list.\n");

	    set_targets_property(dpy, requestor, property);

	} else if (target == XA_image_png) {

		/* We're asked to convert to one the formats we know about */

	    printf("Replying with image/png\n");

	    XChangeProperty(
		dpy, requestor, property, target, 8, PropModeReplace,
		(unsigned char *) file_data, file_size
	    );

	} else if (target == XA_text_uri_list) {

		/* We're asked to convert to one the formats we know about */

	    printf("Replying with text/uri-list\n");

	    XChangeProperty(
		dpy, requestor, property, target, 8, PropModeReplace,
		(unsigned char *) file_uri, strlen(file_uri)
	    );

	} else if (target == XA_STRING) {

		/* We're asked to convert to one the formats we know about */

	    printf("Replying with STRING\n");

	    XChangeProperty(
		dpy, requestor, property, target, 8, PropModeReplace,
		(unsigned char *) file_uri, strlen(file_uri)
	    );

	} else {

		/* We've been asked to converto to something we don't know about. */

	    printf("No valid conversion. Replying with refusal.\n");

	    s.xselection.property = None;	/* This means refusal */
	}

	XSendEvent(dpy, e.xselectionrequest.requestor, True, 0, &s);
    }
}

int main(int argc, char**argv)
{
    Display* dpy;
    Window root, w;
    int screen;
    XEvent e;
    Atom selection;

    if (argc <= 1) return 0;

    dpy = XOpenDisplay(NULL);
    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    w = XCreateSimpleWindow(dpy, root, 0, 0, 100, 100, 0,
	BlackPixel(dpy, screen), BlackPixel(dpy, screen));

    selection = XInternAtom(dpy, "CLIPBOARD", 0);

    XA_TARGETS		= XInternAtom(dpy, "TARGETS", False);
    XA_multiple		= XInternAtom(dpy, "MULTIPLE", False);
    XA_image_png	= XInternAtom(dpy, "image/png", False);
    XA_text_uri_list	= XInternAtom(dpy, "text/uri-list", False);

    file_size = read_whole_file(argv[1], &file_data);
    sprintf(file_uri, "file://%s", argv[1]);	/* hack, not correct encoding */
    printf("uri: %s file size: %d\n", file_uri, file_size);

	/* All your selection are belong to us... */

    XSetSelectionOwner(dpy, selection, w, CurrentTime);
    XFlush(dpy);

    for(;;) {
	XNextEvent(dpy, &e);

	    /* Wait until something asks for the selection or until
	    ** we loose the selection. */

	if (e.type == SelectionClear) {
	    printf("SelectionClear event received. Quitting.\n");
	    return 0;
	} else if(e.type == SelectionRequest) {
	    process_selection_request(e);
	}
    }

    return 0;
}

