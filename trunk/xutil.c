#include <Python.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>

/* Windows and cursor manipulations not provided by Tkinter or any other
** standard python library
*/

/* the following macro is used to trap x window exceptions and trigger
** a python exception in turn
*/

jmp_buf ErrorEnv;
int xstatus;
int (* oldErrorHandler)(Display *, XErrorEvent *);
int (* oldIOErrorHandler)(Display *);
char XErrorMsg[80];
char ErrorPrefix[] = "XWindows Error!\n";
char ErrorMsg[120];
char IOError[] = "XWindows IO exception.";

static Display *d;
static int screen_num;
static GC gc;
static XWindowAttributes wa;
static int last_win = -1;
static Colormap cmap;
static XColor colorfg, colorbg, color;

extern Display *dpy;
extern int screen;

#define TrapXlibErrors \
    oldIOErrorHandler = XSetIOErrorHandler(&MyXlibIOErrorHandler); \
    oldErrorHandler = XSetErrorHandler(&MyXlibErrorHandler); \
    xstatus = setjmp(ErrorEnv); \
    if ( xstatus != 0) { \
      XSetIOErrorHandler(oldIOErrorHandler); \
      XSetErrorHandler(oldErrorHandler); \
      strncat(ErrorMsg,ErrorPrefix, 20); \
      strncat(ErrorMsg,XErrorMsg, 80); \
      PyErr_SetString(PyExc_EnvironmentError, ErrorMsg); \
      return NULL; \
    }

#define RestoreOldXlibErrorHandlers \
        XSetIOErrorHandler(oldIOErrorHandler); \
        XSetErrorHandler(oldErrorHandler);

int MyXlibErrorHandler(Display *d, XErrorEvent *myerror) {
        XGetErrorText(d, myerror->error_code, XErrorMsg, 80);
        longjmp(ErrorEnv, 1);
        /* Pointless, but it shuts up some compiler warning messages */
        return 0;
}

int MyXlibIOErrorHandler(Display *d) {
  /* just put a constant string in the error message */
  strncat(XErrorMsg,IOError,80);
  longjmp(ErrorEnv, 1);
  /* Pointless, but it shuts up some compiler warning messages */
  return 0;
}

static getdisplaysize(int *width, int *height)
{
	*width = WidthOfScreen (DefaultScreenOfDisplay (d));
	*height = HeightOfScreen (DefaultScreenOfDisplay (d));
}

PyObject *wrap_getdisplaysize(PyObject *self, PyObject *args) {
  int x, y;
  if (!PyArg_ParseTuple(args,"ii", &x, &y))
    return NULL;
  TrapXlibErrors /* macro code to handle xlib exceptions */
  getdisplaysize((void *) x,(void *) y);
  RestoreOldXlibErrorHandlers /* macro */
  Py_INCREF(Py_None);
  return Py_None;
}

void moveCursorTo(int win, int x, int y) {
  Window w;
  /*  Display *XOpenDisplay(char *); */
  int s;
  if (d == NULL) {
    printf("could not open XWindow display\n");
    return;
  }
  w = (Window) win;
	if ( w == 0 ) {
	w = DefaultRootWindow(d);
	}
  s = XGrabPointer(d,w,True,ButtonPressMask | EnterWindowMask,GrabModeSync,
        GrabModeSync,None,None,CurrentTime);
  XWarpPointer(d,None,w,0,0,0,0,x,y);
  XUngrabPointer(d,CurrentTime);
  XFlush(d);
}

PyObject *wrap_moveCursorTo(PyObject *self, PyObject *args) {
  int x, y, w;
  if (!PyArg_ParseTuple(args,"iii",&w, &x, &y))
    return NULL;
  TrapXlibErrors /* macro code to handle xlib exceptions */
  moveCursorTo(w,x,y);
  RestoreOldXlibErrorHandlers /* macro */
  Py_INCREF(Py_None);
  return Py_None;
}

void window_move(int win, int x, int y) {
  Window w;
  XWindowChanges wc;
  int ret;
  wc.x = x;
  wc.y = y;
  w = (Window) win;
  XConfigureWindow(d, w, CWX | CWY, &wc);
  XSync(d, True);
}

PyObject *wrap_window_move(PyObject *self, PyObject *args) {
  int x, y, w;
  if (!PyArg_ParseTuple(args,"iii",&w, &x, &y))
    return NULL;
  TrapXlibErrors /* macro code to handle xlib exceptions */
  window_move(w,x,y);
  RestoreOldXlibErrorHandlers /* macro */
  Py_INCREF(Py_None);
  return Py_None;
}

static pointer2wid( int pointx, int pointy) {
/*	char *display_name;
	Display *xdpy;
	if ( (display_name = getenv("DISPLAY")) == (void *)NULL)
	{
		fprintf(stderr, "Error: DISPLAY environment variable not set\n");
		exit(1);
	}
	if ((xdpy = XOpenDisplay(display_name)) == NULL)
	{
		fprintf(stderr, "Error: Can't open display: %s\n", display_name);
		return NULL;
	}*/
	int xdpy = d;
	unsigned int j, nwins;
	int tx, ty,wid;
// 	dpy = XDefaultRootWindow(xdpy);
	Window root_return, parent_return, *wins;
	XWindowAttributes winattr;
	Window root = RootWindowOfScreen(DefaultScreenOfDisplay(xdpy));

	XQueryTree(xdpy, root, &root_return, &parent_return, &wins, &nwins);
	for (j = 1; j < nwins; j++)
	{
		XGetWindowAttributes(xdpy, wins[j], &winattr);
		if (!winattr.override_redirect && winattr.map_state == IsViewable)
		{
			if(wins[j]==winattr.root)
			{
				tx = 0; ty = 0;
			}
			else
			{
				Window child_return;
				XTranslateCoordinates(xdpy, root, winattr.root,
					winattr.x, winattr.y, &tx, &ty, &child_return);
				if ( ( tx < pointx ) && ( ty < pointy ) && ( ( winattr.width + tx ) > pointx ) && ( ( winattr.height + ty ) > pointy ) )
					wid = wins[j] ;
			}
		}
	}
//  	fprintf(stdout,"Widnow ID: %d\n",wid);
	XFree(wins);
 	return wid;
}

PyObject *wrap_pointer2wid(PyObject *self, PyObject *args) {
  int wid, pointx, pointy;
  if (!PyArg_ParseTuple(args,"ii", &pointx,&pointy))
     return NULL;
  TrapXlibErrors /* macro code to handle xlib exceptions */
  wid = pointer2wid( pointx, pointy);
  RestoreOldXlibErrorHandlers /* macro */
//   Py_INCREF(Py_None);
//   return Py_None;
   return Py_BuildValue("i", (int) wid);
}

static getsize(Window win, int *wp, int *hp)
{
	XWindowAttributes attributes;

	if (XGetWindowAttributes(d, win, &attributes) == 0)
		fprintf(stderr, "XGetWindowAttributes(0x%x)\n", win);

	*wp = attributes.width;
	*hp = attributes.height;
}

PyObject *wrap_getsize(PyObject *self, PyObject *args) {
  int x, y, w;
  if (!PyArg_ParseTuple(args,"iii",&w, &x, &y))
    return NULL;
  TrapXlibErrors /* macro code to handle xlib exceptions */
  getsize(w,x,y);
  RestoreOldXlibErrorHandlers /* macro */
  Py_INCREF(Py_None);
  return Py_None;
}

void window_setsize(Window win, int w, int h)
{
    XWindowChanges values;
    unsigned int value_mask;
    int try;
    int nw, nh;
    Display *display = d;
// 	XOpenDisplay(NULL);
    screen_num = DefaultScreen(d);
    values.width = w;
    values.height = h;

    for (try = 0; try < 2; try++) {
	if (XReconfigureWMWindow(display, win, screen_num, CWWidth | CWHeight, &values) == 0)
	          fprintf(stderr, "resize: XReconfigureWMWindow\n");

	getsize(win, &nw, &nh);
	if (values.width == nw && values.height == nh)
	    return;

	/* give window manager a couple of chances to react */
	usleep(100);
	getsize(win, &nw, &nh);
	if (values.width == nw && values.height == nh)
	    return;

	usleep(400);
	getsize(win, &nw, &nh);
	if (values.width == nw && values.height == nh)
	    return;
    }

    /* last chance */
    values.width += values.width - nw;
    values.height += values.height - nh;
    if (XReconfigureWMWindow(display, win, screen_num, CWWidth | CWHeight, &values) == 0)
	fprintf(stderr, "resize: XReconfigureWMWindow 2");
// 	XFlush(display);
	XSync(display, True);
// 	XCloseDisplay(display);
}

PyObject *wrap_window_setsize(PyObject *self, PyObject *args) {
  int x, y, w;
  if (!PyArg_ParseTuple(args,"iii",&w, &x, &y))
    return NULL;
  TrapXlibErrors /* macro code to handle xlib exceptions */
  window_setsize(w,x,y);
  RestoreOldXlibErrorHandlers /* macro */
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject *wrap_getWindowUnderCursor(PyObject *self, PyObject *args) {
	if (d == NULL) 
		{
		printf("could not open XWindow display\n");
		return -1;
        	}
	Window window = None;
	Window wroot;
	int dummyi;
	unsigned int dummy;

	XQueryPointer(d, RootWindow(d, DefaultScreen(d)),
			&wroot, &window,
			&dummyi, &dummyi, &dummyi, &dummyi, &dummy);
        return Py_BuildValue("{s:i}",
                "window",(int) window);
}

int getWindowID(void) {
   Window w;
   int revert;
   /*  Display *XOpenDisplay(char *); */
   if (d == NULL) {
     printf("could not open XWindow display\n");
     return -1;
        }
   XGetInputFocus(d,&w,&revert);
   XFlush(d);
   return (int) w;
}

PyObject *wrap_getWindowID(PyObject *self, PyObject *args) {
  int result;
  TrapXlibErrors /* macro code to handle xlib exceptions */
  result = getWindowID();
  RestoreOldXlibErrorHandlers /* macro */
  return Py_BuildValue("i",result);
}

void mouse_click(int button) {
	Display *display = d;
// 	XOpenDisplay(NULL);
	XEvent event;

	memset(&event, 0x00, sizeof(event));
	event.type = ButtonPress;
	event.xbutton.button = button;
	event.xbutton.same_screen = True;

	XQueryPointer(display,
		RootWindow(display,
		DefaultScreen(display)),
		&event.xbutton.root,
		&event.xbutton.window,
		&event.xbutton.x_root,
		&event.xbutton.y_root,
		&event.xbutton.x,
		&event.xbutton.y,
		&event.xbutton.state);

	event.xbutton.subwindow = event.xbutton.window;

	while(event.xbutton.subwindow)
	{
		event.xbutton.window = event.xbutton.subwindow;
		XQueryPointer(display,
			event.xbutton.window,
			&event.xbutton.root,
			&event.xbutton.subwindow,
			&event.xbutton.x_root,
			&event.xbutton.y_root,
			&event.xbutton.x,
			&event.xbutton.y,
			&event.xbutton.state);
	}

	if(XSendEvent(display, PointerWindow,
		True, 0xfff, &event)==0)
		fprintf(stderr, "XSendEvent()\n");

	XFlush(display);
	usleep(100000);

	event.type = ButtonRelease;
	event.xbutton.state = 0x100;

	if(XSendEvent(display, PointerWindow,
		True, 0xfff, &event)==0)
		fprintf(stderr, "XSendEvent()\n");

	XFlush(display);
// 	XCloseDisplay(display);
}

PyObject *wrap_mouse_click(PyObject *self, PyObject *args) {
  int button;
  if (!PyArg_ParseTuple(args,"i",&button))
    return NULL;
  TrapXlibErrors /* macro code to handle xlib exceptions */
  mouse_click(button);
  RestoreOldXlibErrorHandlers /* macro */
  Py_INCREF(Py_None);
  return Py_None;
}

int getDeepestVisual(void) {
  XVisualInfo *visualList;
  int i, visualsMatched, maxDepth;
  maxDepth = 1;
  if (d == NULL) {
    printf("could not open XWindow display\n");
    return -1;
  }
  visualList = XGetVisualInfo (d, VisualNoMask, NULL, &visualsMatched);
  for (i=0;i<visualsMatched;i++) {
    if (visualList[i].depth > maxDepth) {
      maxDepth = visualList[i].depth;
    }
  }
  XFree(visualList);
  XFlush(d);
  return maxDepth;
}

PyObject *wrap_getDeepestVisual(PyObject *self, PyObject *args) {
  int depth;
  TrapXlibErrors /* macro code to handle xlib exceptions */
  depth = getDeepestVisual();
  RestoreOldXlibErrorHandlers /* macro */
  return Py_BuildValue("i",depth);
}

void setFocusTo(int win) {
  Window w;
  /* Display *XOpenDisplay(char *); */
  w = (Window) win;
  if (d == NULL) {
    printf("could not open XWindow display\n");
    return;
  }
  XSetInputFocus(d,w,0,CurrentTime);
  XFlush(d);
}

PyObject *wrap_setFocusTo(PyObject *self, PyObject *args) {
  int win;
  if (!PyArg_ParseTuple(args,"i", &win))
    return NULL;
  TrapXlibErrors /* macro code to handle xlib exceptions */
  setFocusTo(win);
  RestoreOldXlibErrorHandlers /* macro */
  Py_INCREF(Py_None);
  return Py_None;
}

void setBackingStore(int win) {
  Window w;
  XWindowAttributes wa;
  XSetWindowAttributes  NewWinAttributes;
  Status XGetWindowAttributes(Display *, Window, XWindowAttributes *);
  int XChangeWindowAttributes(Display *, Window, unsigned long,
                               XSetWindowAttributes *);
  w = (Window) win;
  if (d == NULL) {
    printf("could not open XWindow display\n");
    return;
  }
  XGetWindowAttributes(d, w, &wa);
  if (XDoesBackingStore(wa.screen) != NotUseful) {
    NewWinAttributes.backing_store = Always;
    XChangeWindowAttributes(d, w, CWBackingStore, &NewWinAttributes);
  }
  XFlush(d);
}

PyObject *wrap_setBackingStore(PyObject *self, PyObject *args) {
  int win;
  if (!PyArg_ParseTuple(args,"i", &win))
    return NULL;
  TrapXlibErrors /* macro code to handle xlib exceptions */
    setBackingStore(win);
  RestoreOldXlibErrorHandlers /* macro */
  Py_INCREF(Py_None);
  return Py_None;
}

void getWindowAttributes(int win, XWindowAttributes *winAttr, char **visual) {
  Window w;
  XVisualInfo visual_info;
  static char *visual_class[] = {
          "StaticGray",
          "GrayScale",
          "StaticColor",
          "PseudoColor",
          "TrueColor",
          "DirectColor"
  };
  int i;
  int screen_num;
  Status XGetWindowAttributes(Display *, Window, XWindowAttributes *);
  i = 5;
  w = (Window) win;
  if (d == NULL) {
    printf("could not open XWindow display\n");
    return;
  }
  XGetWindowAttributes(d, w, winAttr);
  screen_num = DefaultScreen(d);
  while (!XMatchVisualInfo(d, screen_num, DefaultDepth(d, screen_num),
                 i--, &visual_info));
  *visual = visual_class[++i];
}

PyObject *wrap_getWindowAttributes(PyObject *self, PyObject *args) {
        int win, viewable;
        XWindowAttributes wa;
        char *visual;
        if (!PyArg_ParseTuple(args,"i", &win))
                return NULL;
        TrapXlibErrors /* macro code to handle xlib exceptions */
        getWindowAttributes(win, &wa, &visual);
        if (wa.map_state == IsViewable) {
          viewable = 1;
        } else {
          viewable = 0;
        }
        RestoreOldXlibErrorHandlers /* macro */
        return Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:s}",
                "x", wa.x,
                "y", wa.y,
                "rootID", (int) wa.root,
                "width", wa.width,
                "height", wa.height,
                "borderWidth", wa.border_width,
                                                 "viewable", viewable,
                "depth", wa.depth,
                "visualClass", visual);
}

PyObject *wrap_getPointerPosition(PyObject *self, PyObject *args) {
        int win;
        Window w;
        Bool inScreen;
        Window root, child;
        int root_x, root_y, win_x, win_y;
        unsigned int mask;
        Bool XQueryPointer(Display *, Window, Window *, Window *,
                int *, int *, int *, int *, unsigned int *);
        if (!PyArg_ParseTuple(args,"i", &win))
                return NULL;
        w = (Window) win;
        TrapXlibErrors /* macro code to handle xlib exceptions */
        if (d == NULL) {
                printf("could not open XWindow display\n");
                RestoreOldXlibErrorHandlers /* macro */
                return NULL;
        }
        inScreen = XQueryPointer(d, w, &root, &child, &root_x, &root_y,
                &win_x, &win_y, &mask);
        RestoreOldXlibErrorHandlers /* macro */
        return Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i,s:i}",
                "inScreen", (int) inScreen,
                "rootID", (int) root,
                "childID", (int) child,
                "root_x", root_x,
                "root_y", root_y,
                "win_x", win_x,
                "win_y", win_y);
}

PyObject *wrap_getParentID(PyObject *self, PyObject *args) {

        int win;
        Window w;
        Window root, parent, *children;
        unsigned int nchildren;
        /*      Display *XOpenDisplay(char *); */
        Status XQueryTree(Display *, Window, Window *, Window *,
                Window **, unsigned int *);
        if (!PyArg_ParseTuple(args,"i", &win))
                return NULL;
        w = (Window) win;
        if ((w==PointerRoot) | (w==None)) {
          return Py_BuildValue("i", (int) w);
        }
        TrapXlibErrors /* macro code to handle xlib exceptions */
        if (d == NULL) {
                printf("could not open XWindow display\n");
                RestoreOldXlibErrorHandlers /* macro */
                return NULL;
        }
        XQueryTree(d, w, &root, &parent, &children, &nchildren);
        XFree(children);
        RestoreOldXlibErrorHandlers /* macro */
        if (root == parent) {
          Py_INCREF(Py_None);
          return Py_None;
        } else {
          return Py_BuildValue("i", (int) parent);
        }
}

void initXGraphics(void) {

  d = XOpenDisplay(NULL);
  if (d == NULL) {
    printf("could not open XWindow display\n");
    return;
  }
  screen_num = DefaultScreen(d);
}

PyObject *wrap_initXGraphics(PyObject *self, PyObject *args) {

  TrapXlibErrors /* macro code to handle xlib exceptions */
  initXGraphics();
  RestoreOldXlibErrorHandlers /* macro */
  Py_INCREF(Py_None);
  return Py_None;
}

void closeXGraphics(void) {

  XCloseDisplay(d);
}

PyObject *wrap_closeXGraphics(PyObject *self, PyObject *args) {

  TrapXlibErrors /* macro code to handle xlib exceptions */
  closeXGraphics();
  RestoreOldXlibErrorHandlers /* macro */
  Py_INCREF(Py_None);
  return Py_None;
}

void drawCursor(int win, double x, double y, int width, int height) {
  /* plot cursor at x and y where x,y are normalized (range from 0 to 1) */
  Window w;

  w = (Drawable) win;
  if (d == NULL) {
    printf("could not open XWindow display\n");
    return;
  }
  /* only get a graphics context if the window id changes */
  if (win != last_win) {
    last_win = win;
    gc = XCreateGC(d, w, 0, NULL);
    if (!XGetWindowAttributes(d, w, &wa)) {
      printf("Problem getting window attributes\n");
      return;
    }
    cmap = wa.colormap;
    if (!XParseColor(d, cmap, "red", &colorfg)) {
      printf("could not parse color string\n");
      return;
    }
    if (!XParseColor(d, cmap, "black", &colorbg)) {
      printf("could not parse color string\n");
      return;
    }
    if (!(XAllocColor(d, cmap, &colorfg) && XAllocColor(d, cmap, &colorbg))) {
       printf("Problem allocating colors for cursor color determination\n");
       return;
    }
    color.pixel = colorfg.pixel ^ colorbg.pixel;
    XSetFunction(d, gc, GXxor);
    XSetForeground(d, gc, color.pixel);
  }
  /*if (!XGetGeometry(d,w,&wroot, &xr, &yr, &width, &height, &border, &depth)) {
    printf("could not get window geometry\n");
    return;
  }*/
  XDrawLine(d, w, gc, (int) (x*width), 0, (int) (x*width),  height);
  XDrawLine(d, w, gc, 0, (int) ((1.-y)*height),  width, (int) ((1.-y)*height));
  XFlush(d);
}

PyObject *wrap_drawCursor(PyObject *self, PyObject *args) {

  double x, y;
  int width, height;
  int win;

  if (!PyArg_ParseTuple(args, "iddii", &win, &x, &y, &width, &height))
    return NULL;
  TrapXlibErrors /* macro code to handle xlib exceptions */
  drawCursor(win, x, y, width, height);
  RestoreOldXlibErrorHandlers /* macro */
  Py_INCREF(Py_None);
  return Py_None;
}

static PyMethodDef xutilMethods[] = {
  { "pointer2wid",wrap_pointer2wid, 1},
  { "getdisplaysize",wrap_getdisplaysize, 1},
  { "moveCursorTo",wrap_moveCursorTo, 1},
  { "getWindowUnderCursor", wrap_getWindowUnderCursor, 1},
  { "getsize",wrap_getsize, 1},
  { "window_move",wrap_window_move, 1},
  { "window_setsize",wrap_window_setsize, 1},
  { "mouse_click",wrap_mouse_click, 1},
  { "getWindowID",wrap_getWindowID, 1},
  { "setFocusTo",wrap_setFocusTo, 1},
  { "setBackingStore",wrap_setBackingStore, 1},
  { "getWindowAttributes",wrap_getWindowAttributes, 1},
  { "getPointerPosition",wrap_getPointerPosition, 1},
  { "getParentID",wrap_getParentID, 1},
  { "getDeepestVisual", wrap_getDeepestVisual, 1},
  { "initXGraphics",wrap_initXGraphics, 1},
  { "closeXGraphics",wrap_closeXGraphics, 1},
  { "drawCursor",wrap_drawCursor, 1},
  {NULL, NULL}
};

void initxutil(void) {
  PyObject *m;
  m = Py_InitModule("xutil", xutilMethods);
}
