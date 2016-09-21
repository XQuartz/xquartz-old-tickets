#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>

int screen_depth;

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;

typedef struct tagPIXELFORMATDESCRIPTOR {
    WORD nSize;
    WORD nVersion;
    DWORD dwFlags;
    BYTE iPixelType;
    BYTE cColorBits;
    BYTE cRedBits;
    BYTE cRedShift;
    BYTE cGreenBits;
    BYTE cGreenShift;
    BYTE cBlueBits;
    BYTE cBlueShift;
    BYTE cAlphaBits;
    BYTE cAlphaShift;
    BYTE cAccumBits;
    BYTE cAccumRedBits;
    BYTE cAccumGreenBits;
    BYTE cAccumBlueBits;
    BYTE cAccumAlphaBits;
    BYTE cDepthBits;
    BYTE cStencilBits;
    BYTE cAuxBuffers;
    BYTE iLayerType;
    BYTE bReserved;
    DWORD dwLayerMask;
    DWORD dwVisibleMask;
    DWORD dwDamageMask;
} PIXELFORMATDESCRIPTOR;

#define PFD_TYPE_RGBA 0
#define PFD_TYPE_COLORINDEX 1

#define PFD_MAIN_PLANE 0
#define PFD_OVERLAY_PLANE 1
#define PFD_UNDERLAY_PLANE (-1)

#define PFD_DOUBLEBUFFER 0x00000001
#define PFD_STEREO 0x00000002
#define PFD_DRAW_TO_WINDOW 0x00000004
#define PFD_DRAW_TO_BITMAP 0x00000008
#define PFD_SUPPORT_GDI 0x00000010
#define PFD_SUPPORT_OPENGL 0x00000020
#define PFD_GENERIC_FORMAT 0x00000040
#define PFD_NEED_PALETTE 0x00000080
#define PFD_NEED_SYSTEM_PALETTE 0x00000100
#define PFD_SWAP_EXCHANGE 0x00000200
#define PFD_SWAP_COPY 0x00000400
#define PFD_SWAP_LAYER_BUFFERS 0x00000800
#define PFD_GENERIC_ACCELERATED 0x00001000
#define PFD_SUPPORT_COMPOSITION 0x00008000
#define PFD_DEPTH_DONTCARE 0x20000000
#define PFD_DOUBLEBUFFER_DONTCARE 0x40000000
#define PFD_STEREO_DONTCARE 0x80000000

typedef struct wine_glpixelformat {
    int         iPixelFormat;
    GLXFBConfig fbconfig;
    int         fmt_id;
    int         render_type;
    int        offscreenOnly;
    DWORD       dwFlags; /* We store some PFD_* flags in here for emulated bitmap formats */
} WineGLPixelFormat;

// Largely copied from Wine's X11 ChoosePixelFormat() impl.
static int get_render_type_from_fbconfig(Display *display, GLXFBConfig fbconfig)
{
    int render_type=0, render_type_bit;
    glXGetFBConfigAttrib(display, fbconfig, GLX_RENDER_TYPE, &render_type_bit);
    switch(render_type_bit)
    {
        case GLX_RGBA_BIT:
            render_type = GLX_RGBA_TYPE;
            break;
        case GLX_COLOR_INDEX_BIT:
            render_type = GLX_COLOR_INDEX_TYPE;
            break;
        case GLX_RGBA_FLOAT_BIT_ARB:
            render_type = GLX_RGBA_FLOAT_TYPE_ARB;
            break;
        case GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT:
            render_type = GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT;
            break;
        default:
            fprintf(stderr, "Unknown render_type: %x\n", render_type_bit);
    }
    return render_type;
}

/* Check whether a fbconfig is suitable for Windows-style bitmap rendering */
static int check_fbconfig_bitmap_capability(Display *display, GLXFBConfig fbconfig)
{
    int dbuf, value;
    glXGetFBConfigAttrib(display, fbconfig, GLX_DOUBLEBUFFER, &dbuf);
    glXGetFBConfigAttrib(display, fbconfig, GLX_DRAWABLE_TYPE, &value);

    /* Windows only supports bitmap rendering on single buffered formats, further the fbconfig needs to have
     * the GLX_PIXMAP_BIT set. */
    return !dbuf && (value & GLX_PIXMAP_BIT);
}

static WineGLPixelFormat *get_formats(Display *display, int *size_ret, int *onscreen_size_ret)
{
    static WineGLPixelFormat *list;
    static int size, onscreen_size;

    int fmt_id, nCfgs, i, run, bmp_formats;
    GLXFBConfig* cfgs;
    XVisualInfo *visinfo;

    if (list) goto done;

    cfgs = glXGetFBConfigs(display, DefaultScreen(display), &nCfgs);
    if (NULL == cfgs || 0 == nCfgs) {
        if(cfgs != NULL) XFree(cfgs);
        fprintf(stderr, "glXChooseFBConfig returns NULL\n");
        return NULL;
    }

    /* Bitmap rendering on Windows implies the use of the Microsoft GDI software renderer.
     * Further most GLX drivers only offer pixmap rendering using indirect rendering (except for modern drivers which support 'AIGLX' / composite).
     * Indirect rendering can indicate software rendering (on Nvidia it is hw accelerated)
     * Since bitmap rendering implies the use of software rendering we can safely use indirect rendering for bitmaps.
     *
     * Below we count the number of formats which are suitable for bitmap rendering. Windows restricts bitmap rendering to single buffered formats.
     */
    for(i=0, bmp_formats=0; i<nCfgs; i++)
    {
        if(check_fbconfig_bitmap_capability(display, cfgs[i]))
            bmp_formats++;
    }
    fprintf(stderr, "Found %d bitmap capable fbconfigs\n", bmp_formats);

    list = calloc((nCfgs + bmp_formats),sizeof(WineGLPixelFormat));

    /* Fill the pixel format list. Put onscreen formats at the top and offscreen ones at the bottom.
     * Do this as GLX doesn't guarantee that the list is sorted */
    for(run=0; run < 2; run++)
    {
        for(i=0; i<nCfgs; i++) {
            glXGetFBConfigAttrib(display, cfgs[i], GLX_FBCONFIG_ID, &fmt_id);
            visinfo = glXGetVisualFromFBConfig(display, cfgs[i]);

            /* The first run we only add onscreen formats (ones which have an associated X Visual).
             * The second run we only set offscreen formats. */
            if(!run && visinfo)
            {
                /* We implement child window rendering using offscreen buffers (using composite or an XPixmap).
                 * The contents is copied to the destination using XCopyArea. For the copying to work
                 * the depth of the source and destination window should be the same. In general this should
                 * not be a problem for OpenGL as drivers only advertise formats with a similar depth (or no depth).
                 * As of the introduction of composition managers at least Nvidia now also offers ARGB visuals
                 * with a depth of 32 in addition to the default 24 bit. In order to prevent BadMatch errors we only
                 * list formats with the same depth. */
                if(visinfo->depth != screen_depth)
                {
                    XFree(visinfo);
                    continue;
                }

                fprintf(stderr, "Found onscreen format FBCONFIG_ID 0x%x corresponding to iPixelFormat %d at GLX index %d\n", fmt_id, size+1, i);
                list[size].iPixelFormat = size+1; /* The index starts at 1 */
                list[size].fbconfig = cfgs[i];
                list[size].fmt_id = fmt_id;
                list[size].render_type = get_render_type_from_fbconfig(display, cfgs[i]);
                list[size].offscreenOnly = 0;
                list[size].dwFlags = 0;
                size++;
                onscreen_size++;

                /* Clone a format if it is bitmap capable for indirect rendering to bitmaps */
                if(check_fbconfig_bitmap_capability(display, cfgs[i]))
                {
                    fprintf(stderr, "Found bitmap capable format FBCONFIG_ID 0x%x corresponding to iPixelFormat %d at GLX index %d\n", fmt_id, size+1, i);
                    list[size].iPixelFormat = size+1; /* The index starts at 1 */
                    list[size].fbconfig = cfgs[i];
                    list[size].fmt_id = fmt_id;
                    list[size].render_type = get_render_type_from_fbconfig(display, cfgs[i]);
                    list[size].offscreenOnly = 0;
                    list[size].dwFlags = PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_GENERIC_FORMAT;
                    size++;
                    onscreen_size++;
                }
            } else if(run && !visinfo) {
                int window_drawable=0;
                glXGetFBConfigAttrib(display, cfgs[i], GLX_DRAWABLE_TYPE, &window_drawable);

                /* Recent Nvidia drivers and DRI drivers offer window drawable formats without a visual.
                 * This are formats like 16-bit rgb on a 24-bit desktop. In order to support these formats
                 * onscreen we would have to use glXCreateWindow instead of XCreateWindow. Further it will
                 * likely make our child window opengl rendering more complicated since likely you can't use
                 * XCopyArea on a GLX Window.
                 * For now ignore fbconfigs which are window drawable but lack a visual. */
                if(window_drawable & GLX_WINDOW_BIT)
                {
                    fprintf(stderr, "Skipping FBCONFIG_ID 0x%x as an offscreen format because it is window_drawable\n", fmt_id);
                    continue;
                }

                fprintf(stderr, "Found offscreen format FBCONFIG_ID 0x%x corresponding to iPixelFormat %d at GLX index %d\n", fmt_id, size+1, i);
                list[size].iPixelFormat = size+1; /* The index starts at 1 */
                list[size].fbconfig = cfgs[i];
                list[size].fmt_id = fmt_id;
                list[size].render_type = get_render_type_from_fbconfig(display, cfgs[i]);
                list[size].offscreenOnly = 1;
                list[size].dwFlags = 0;
                size++;
            }

            if (visinfo) XFree(visinfo);
        }
    }

    XFree(cfgs);

done:
    if (size_ret) *size_ret = size;
    if (onscreen_size_ret) *onscreen_size_ret = onscreen_size;
    return list;
}

static WineGLPixelFormat *choose_fbconfig(Display *dpy, int scr, PIXELFORMATDESCRIPTOR *pfd) {
    WineGLPixelFormat *list;
    int onscreen_size;
    int value = 0;
    int i = 0;
    int bestFormat = -1;
    int bestDBuffer = -1;
    int bestStereo = -1;
    int bestColor = -1;
    int bestAlpha = -1;
    int bestDepth = -1;
    int bestStencil = -1;
    int bestAux = -1;

    if (!(list = get_formats(dpy, NULL, &onscreen_size ))) return 0;

    for(i=0; i<onscreen_size; i++)
    {
        int dwFlags = 0;
        int iPixelType = 0;
        int alpha=0, color=0, depth=0, stencil=0, aux=0;
        WineGLPixelFormat *fmt = &list[i];

        /* Pixel type */
        glXGetFBConfigAttrib(dpy, fmt->fbconfig, GLX_RENDER_TYPE, &value);
        if (value & GLX_RGBA_BIT)
            iPixelType = PFD_TYPE_RGBA;
        else
            iPixelType = PFD_TYPE_COLORINDEX;

        if (pfd->iPixelType != iPixelType)
        {
            fprintf(stderr, "pixel type mismatch for iPixelFormat=%d\n", i+1);
            continue;
        }

        /* Only use bitmap capable for formats for bitmap rendering.
         * See get_formats for more info. */
        if( (pfd->dwFlags & PFD_DRAW_TO_BITMAP) != (fmt->dwFlags & PFD_DRAW_TO_BITMAP))
        {
            fprintf(stderr, "PFD_DRAW_TO_BITMAP mismatch for iPixelFormat=%d\n", i+1);
            continue;
        }

        glXGetFBConfigAttrib(dpy, fmt->fbconfig, GLX_DOUBLEBUFFER, &value);
        if (value) dwFlags |= PFD_DOUBLEBUFFER;
        glXGetFBConfigAttrib(dpy, fmt->fbconfig, GLX_STEREO, &value);
        if (value) dwFlags |= PFD_STEREO;
        glXGetFBConfigAttrib(dpy, fmt->fbconfig, GLX_BUFFER_SIZE, &color); /* cColorBits */
        glXGetFBConfigAttrib(dpy, fmt->fbconfig, GLX_ALPHA_SIZE, &alpha); /* cAlphaBits */
        glXGetFBConfigAttrib(dpy, fmt->fbconfig, GLX_DEPTH_SIZE, &depth); /* cDepthBits */
        glXGetFBConfigAttrib(dpy, fmt->fbconfig, GLX_STENCIL_SIZE, &stencil); /* cStencilBits */
        glXGetFBConfigAttrib(dpy, fmt->fbconfig, GLX_AUX_BUFFERS, &aux); /* cAuxBuffers */

        /* The behavior of PDF_STEREO/PFD_STEREO_DONTCARE and PFD_DOUBLEBUFFER / PFD_DOUBLEBUFFER_DONTCARE
         * is not very clear on MSDN. They specify that ChoosePixelFormat tries to match pixel formats
         * with the flag (PFD_STEREO / PFD_DOUBLEBUFFERING) set. Otherwise it says that it tries to match
         * formats without the given flag set.
         * A test on Windows using a Radeon 9500pro on WinXP (the driver doesn't support Stereo)
         * has indicated that a format without stereo is returned when stereo is unavailable.
         * So in case PFD_STEREO is set, formats that support it should have priority above formats
         * without. In case PFD_STEREO_DONTCARE is set, stereo is ignored.
         *
         * To summarize the following is most likely the correct behavior:
         * stereo not set -> prefer no-stereo formats, else also accept stereo formats
         * stereo set -> prefer stereo formats, else also accept no-stereo formats
         * stereo don't care -> it doesn't matter whether we get stereo or not
         *
         * In Wine we will treat no-stereo the same way as don't care because it makes
         * format selection even more complicated and second drivers with Stereo advertise
         * each format twice anyway.
         */

        /* Doublebuffer, see the comments above */
        if( !(pfd->dwFlags & PFD_DOUBLEBUFFER_DONTCARE) ) {
            if( ((pfd->dwFlags & PFD_DOUBLEBUFFER) != bestDBuffer) &&
                ((dwFlags & PFD_DOUBLEBUFFER) == (pfd->dwFlags & PFD_DOUBLEBUFFER)) )
            {
                bestDBuffer = dwFlags & PFD_DOUBLEBUFFER;
                bestStereo = dwFlags & PFD_STEREO;
                bestAlpha = alpha;
                bestColor = color;
                bestDepth = depth;
                bestStencil = stencil;
                bestAux = aux;
                bestFormat = i;
                continue;
            }
            if(bestDBuffer != -1 && (dwFlags & PFD_DOUBLEBUFFER) != bestDBuffer)
                continue;
        }

        /* Stereo, see the comments above. */
        if( !(pfd->dwFlags & PFD_STEREO_DONTCARE) ) {
            if( ((pfd->dwFlags & PFD_STEREO) != bestStereo) &&
                ((dwFlags & PFD_STEREO) == (pfd->dwFlags & PFD_STEREO)) )
            {
                bestDBuffer = dwFlags & PFD_DOUBLEBUFFER;
                bestStereo = dwFlags & PFD_STEREO;
                bestAlpha = alpha;
                bestColor = color;
                bestDepth = depth;
                bestStencil = stencil;
                bestAux = aux;
                bestFormat = i;
                continue;
            }
            if(bestStereo != -1 && (dwFlags & PFD_STEREO) != bestStereo)
                continue;
        }

        /* Below we will do a number of checks to select the 'best' pixelformat.
         * We assume the precedence cColorBits > cAlphaBits > cDepthBits > cStencilBits -> cAuxBuffers.
         * The code works by trying to match the most important options as close as possible.
         * When a reasonable format is found, we will try to match more options.
         * It appears (see the opengl32 test) that Windows opengl drivers ignore options
         * like cColorBits, cAlphaBits and friends if they are set to 0, so they are considered
         * as DONTCARE. At least Serious Sam TSE relies on this behavior. */

        /* Color bits */
        if(pfd->cColorBits) {
            if( ((pfd->cColorBits > bestColor) && (color > bestColor)) ||
                ((color >= pfd->cColorBits) && (color < bestColor)) )
            {
                bestDBuffer = dwFlags & PFD_DOUBLEBUFFER;
                bestStereo = dwFlags & PFD_STEREO;
                bestAlpha = alpha;
                bestColor = color;
                bestDepth = depth;
                bestStencil = stencil;
                bestAux = aux;
                bestFormat = i;
                continue;
            } else if(bestColor != color) {  /* Do further checks if the format is compatible */
                fprintf(stderr, "color mismatch for iPixelFormat=%d\n", i+1);
                continue;
            }
        }

        /* Alpha bits */
        if(pfd->cAlphaBits) {
            if( ((pfd->cAlphaBits > bestAlpha) && (alpha > bestAlpha)) ||
                ((alpha >= pfd->cAlphaBits) && (alpha < bestAlpha)) )
            {
                bestDBuffer = dwFlags & PFD_DOUBLEBUFFER;
                bestStereo = dwFlags & PFD_STEREO;
                bestAlpha = alpha;
                bestColor = color;
                bestDepth = depth;
                bestStencil = stencil;
                bestAux = aux;
                bestFormat = i;
                continue;
            } else if(bestAlpha != alpha) {
                fprintf(stderr, "alpha mismatch for iPixelFormat=%d\n", i+1);
                continue;
            }
        }

        /* Depth bits */
        if(pfd->cDepthBits) {
            if( ((pfd->cDepthBits > bestDepth) && (depth > bestDepth)) ||
                ((depth >= pfd->cDepthBits) && (depth < bestDepth)) )
            {
                bestDBuffer = dwFlags & PFD_DOUBLEBUFFER;
                bestStereo = dwFlags & PFD_STEREO;
                bestAlpha = alpha;
                bestColor = color;
                bestDepth = depth;
                bestStencil = stencil;
                bestAux = aux;
                bestFormat = i;
                continue;
            } else if(bestDepth != depth) {
                fprintf(stderr, "depth mismatch for iPixelFormat=%d\n", i+1);
                continue;
            }
        }

        /* Stencil bits */
        if(pfd->cStencilBits) {
            if( ((pfd->cStencilBits > bestStencil) && (stencil > bestStencil)) ||
                ((stencil >= pfd->cStencilBits) && (stencil < bestStencil)) )
            {
                bestDBuffer = dwFlags & PFD_DOUBLEBUFFER;
                bestStereo = dwFlags & PFD_STEREO;
                bestAlpha = alpha;
                bestColor = color;
                bestDepth = depth;
                bestStencil = stencil;
                bestAux = aux;
                bestFormat = i;
                continue;
            } else if(bestStencil != stencil) {
                fprintf(stderr, "stencil mismatch for iPixelFormat=%d\n", i+1);
                continue;
            }
        }

        /* Aux buffers */
        if(pfd->cAuxBuffers) {
            if( ((pfd->cAuxBuffers > bestAux) && (aux > bestAux)) ||
                ((aux >= pfd->cAuxBuffers) && (aux < bestAux)) )
            {
                bestDBuffer = dwFlags & PFD_DOUBLEBUFFER;
                bestStereo = dwFlags & PFD_STEREO;
                bestAlpha = alpha;
                bestColor = color;
                bestDepth = depth;
                bestStencil = stencil;
                bestAux = aux;
                bestFormat = i;
                continue;
            } else if(bestAux != aux) {
                fprintf(stderr, "aux mismatch for iPixelFormat=%d\n", i+1);
                continue;
            }
        }
    }

    if(bestFormat == -1)
        fprintf(stderr, "No matching mode was found\n");

    return &list[bestFormat];
}

static GLXContext create_glxcontext(Display *display, WineGLPixelFormat *fmt, XVisualInfo *vis, GLXContext shareList)
{
    GLXContext ctx;

    /* We use indirect rendering for rendering to bitmaps. See get_formats for a comment about this. */
    int indirect = (fmt->dwFlags & PFD_DRAW_TO_BITMAP) ? 0 : 1;

    if(vis)
        ctx = glXCreateContext(display, vis, shareList, indirect);
    else /* Create a GLX Context for a pbuffer */
        ctx = glXCreateNewContext(display, fmt->fbconfig, fmt->render_type, shareList, 1);

    return ctx;
}

int main(void) {
#if 0
    static int attribs[] = {
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_BUFFER_SIZE, 24,
        GLX_DEPTH_SIZE, 32,
        GLX_NONE
    };
#else
	static PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		24,
		0, 0, 0, 0, 0, 0, 0, 0,
		0,
		0, 0, 0, 0,
		32,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};
#endif
    Display *dpy;
	WineGLPixelFormat *pf;
    GLXContext glc;
    XVisualInfo *vis;
    Window win, root;
    XSetWindowAttributes attr;
    int errbase, eventbase, scr, numfbc;
	int maj, min;
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Can't open X Display %s\n", getenv("DISPLAY"));
        return 1;
    }
    if (!glXQueryExtension(dpy, &errbase, &eventbase)) {
        fprintf(stderr, "glX not supported\n");
        XCloseDisplay(dpy);
        return 1;
    }
    scr = DefaultScreen(dpy);
	screen_depth = DefaultDepth(dpy, scr);
    fprintf(stderr, "glX is up and running, errbase=%d, eventbase=%d\n", errbase, eventbase);
#if 0
    fbcs = glXChooseFBConfig(dpy, scr, attribs, &numfbc);
    if (!fbcs || numfbc < 1) {
        fprintf(stderr, "No suitable FBConfigs!\n");
        XCloseDisplay(dpy);
        return 1;
    }
    fbc = fbcs[0];
#else
	pf = choose_fbconfig(dpy, scr, &pfd);
	if (!pf) {
		fprintf(stderr, "No suitable FBConfigs!\n");
		XCloseDisplay(dpy);
		return 1;
	}
#endif
    vis = glXGetVisualFromFBConfig(dpy, pf->fbconfig);
    root = DefaultRootWindow(dpy);
    attr.bit_gravity = NorthWestGravity;
    attr.event_mask = ExposureMask | PointerMotionMask | ButtonReleaseMask | ButtonPressMask | EnterWindowMask | KeyPressMask | KeyReleaseMask | FocusChangeMask | KeymapStateMask | StructureNotifyMask | PropertyChangeMask;
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap(dpy, root, vis->visual, AllocNone);
    win = XCreateWindow(dpy, root, 10, 10, 200, 200, 0, vis->depth, InputOutput, vis->visual, CWEventMask | CWBitGravity | CWColormap | CWBorderPixel, &attr);
	XFree(vis);
    if (!win) {
        fprintf(stderr, "Couldn't create window!\n");
        XCloseDisplay(dpy);
        return 1;
    }
    glc = create_glxcontext(dpy, pf, vis, NULL);
    if (!glc) {
        fprintf(stderr, "Couldn't create glX context!\n");
        XDestroyWindow(dpy, win);
        XCloseDisplay(dpy);
        return 1;
    }
    if (!glXMakeCurrent(dpy, win, glc)) {
        fprintf(stderr, "Couldn't make context current!\n");
        glXDestroyContext(dpy, glc);
        XDestroyWindow(dpy, win);
        XCloseDisplay(dpy);
        return 1;
    }
    XCloseDisplay(dpy);
    return 0;
}
