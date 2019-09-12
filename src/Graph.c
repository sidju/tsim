#include <stdio.h>
#include <X11/Xlib.h>                        /* for Xlib definitions */
#include <X11/StringDefs.h>                /* for useful atom names */
#include <X11/IntrinsicP.h>                /* for toolkit stuff */
#include <X11/Xmu/Converters.h>
#include <X11/Xaw/XawInit.h>            /* for XawInitializeWidgetSet */
#include "Graph.h"
#include "train.h"                      /* for CARNAME */


/* these structures are used elsewhere, but not exported ??, only a typedef */
typedef struct {
  int maxvalues;
  Pixel foreground;
  int        backing_store;
  int first, last, jump;
  int *values;
  Boolean xauto;
  int xmin, xmax;
  int ymin, ymax;
  XFontStruct *font;
  char *label;
  GC gc;
  GC cgc;
  Pixmap pixcach;
} GraphPart;

typedef struct _GraphRec {
  CorePart core;
  GraphPart graph;
} GraphRec;

typedef struct {
  int dummy;
} GraphClassPart;

typedef struct _GraphClassRec {
  CoreClassPart core_class;
  GraphClassPart graph_class;
} GraphClassRec;

extern GraphClassRec graphClassRec;




#define GraphWidth 100
#define GraphHeight 50

#define offset(field) XtOffset(GraphWidget,graph.field)
#define graphoffset(field) XtOffset(Widget,core.field)
#define num_in_list(f,l,m) (l)<(f)?(m)-(f)+(l):(l)-(f)
#define check_pixmap(w) if(!(w)->graph.pixcach) init_pixcach(w)
static Dimension defXDim = 400;
static Dimension defYDim = 400;
static int defMaxValues = 200;
static int defMaxX = 100;
static int defMinX = 0;
static int defMaxY = 100;
static int defMinY = 0;
static Boolean defAutoStep = True;

static XtResource resources[] = {
    { XtNwidth, XtCWidth, XtRDimension, sizeof (Dimension), 
        graphoffset (width), XtRDimension, (caddr_t)&defXDim },
    { XtNheight, XtCHeight, XtRDimension, sizeof (Dimension),
        graphoffset (height), XtRDimension, (caddr_t)&defYDim },
    { XtNmaxvalues, XtCMaxvalues, XtRInt, sizeof (int),
        offset (maxvalues), XtRInt, (caddr_t)&defMaxValues },
    { XtNforeground, XtCForeground, XtRPixel, sizeof (Pixel),
        offset (foreground), XtRString, "black" },
    { XtNbackground, XtCBackground, XtRPixel, sizeof (Pixel),
        graphoffset (background_pixel), XtRString, "white" },
    {XtNbackingStore, XtCBackingStore, XtRBackingStore, sizeof (int),
       offset (backing_store), XtRString, "whenMapped"},
    { XtNxmax, XtCXmax, XtRInt, sizeof (int),
        offset (xmax), XtRInt, (caddr_t) &defMaxX },
    { XtNxmin, XtCXmin, XtRInt, sizeof (int),
        offset (xmin), XtRInt, (caddr_t) &defMinX },
    { XtNymax, XtCYmax, XtRInt, sizeof (int),
        offset (ymax), XtRInt, (caddr_t) &defMaxY },
    { XtNymin, XtCYmin, XtRInt, sizeof (int),
        offset (ymin), XtRInt, (caddr_t) &defMinY },
    { XtNxauto, XtCXauto, XtRBoolean, sizeof (Boolean),
        offset (xauto), XtRBoolean, (caddr_t) &defAutoStep },
    {XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
        offset(font), XtRString, XtDefaultFont},
    {XtNlabel, XtCLabel, XtRString, sizeof(char *),
        offset(label), XtRString, "Graph"},
};

static void ClassInitialize() {
    XawInitializeWidgetSet();
    XtAddConverter( XtRString, XtRBackingStore, XmuCvtStringToBackingStore,
                    NULL, 0 );
}

/*
  widget initialization.
  type is XtInitProc.
*/
static void Initialize (
    Widget request,
    Widget new,
    ArgList	args,
    Cardinal* num_args){

  // suppress compiler warning about unused callback parameter.
  (void)request;
  (void)args;
  (void)num_args;

  GraphWidget w = (GraphWidget) new;

  if (w->core.width <= 0) w->core.width = GraphWidth;
  if (w->core.height <= 0) w->core.height = GraphHeight;
  w->graph.values = NULL;
  w->graph.pixcach = 0; /* NULL; */
  return;
}

static void GetnormalGC(gw)
    GraphWidget gw;
{
  XGCValues        values;

  values.foreground        = gw->graph.foreground;
  values.background        = gw->core.background_pixel;
  
    gw->graph.gc = XCreateGC(XtDisplay(gw), XtWindow(gw),
        GCForeground | GCBackground, &values);
}



static void Realize (gw, valuemaskp, attr)
    Widget gw;
    XtValueMask *valuemaskp;
    XSetWindowAttributes *attr;
{
  GraphWidget w = (GraphWidget) gw;
  /*  Display *dpy = XtDisplay (w);
      Window win = XtWindow (w); */


  *valuemaskp |= CWBitGravity;
  attr->bit_gravity = ForgetGravity;
  switch (w->graph.backing_store) {
  case Always:
  case NotUseful:
  case WhenMapped:
    *valuemaskp |=CWBackingStore;
    attr->backing_store = w->graph.backing_store;
    break;
  }

  XtCreateWindow (gw, InputOutput, (Visual *) CopyFromParent,
                 *valuemaskp, attr);
  
  /*
   * build up the pixmaps that we'll put into the image
   */
  
  GetnormalGC(w);
  w->graph.pixcach = 0; /* NULL; */


  return;
}

static void Destroy (gw)
    Widget gw;
{
    GraphWidget w = (GraphWidget) gw;
    Display *dpy = XtDisplay (w);

    if(w->graph.pixcach)
      XFreePixmap(dpy,w->graph.pixcach);
    XFreeGC (dpy, w->graph.gc);
    XFreeGC (dpy, w->graph.cgc);
    return;
}

/* local
*/
static void Handle_expose(w, event, region)
     GraphWidget w;
     XEvent *event;
     Region region;
{
  // suppress compiler warning about unused callback parameters.
  (void)region;

  Display *dpy = XtDisplay (w);
  Window win = XtWindow (w);
  int x, y, width, height;

  switch(event->type) {
    case Expose: 
      x = event->xexpose.x;
      y = event->xexpose.y;
      width = event->xexpose.width;
      height = event->xexpose.height;
      break;
    case GraphicsExpose:
      x = event->xgraphicsexpose.x;
      y = event->xgraphicsexpose.y;
      width = event->xgraphicsexpose.width;
      height = event->xgraphicsexpose.height;
      break;
    default:
      return;
  }

  if(w->graph.pixcach)
    XCopyPlane(dpy, w->graph.pixcach, win, w->graph.gc,
              x, y, width, height, x, y, 1);

  /*** MAYOR HACK to re-display the trains. ***/

  // Get the children of the parent widget (i.e. the top widget)
  Widget top = XtParent(w);
  Arg wargs[2];
  WidgetList children;
  Cardinal num_children;
  XtSetArg(wargs[0], XtNchildren, &children);
  XtSetArg(wargs[1], XtNnumChildren, &num_children);
  XtGetValues(top, wargs, 2);

  // Find all the children of the top-widget that are cars and call
  // XRaiseWindow on them.
  for (Cardinal i = 0; i < num_children; ++i) {
    Widget child = children[i];
    String child_name = XtName(child);

    // Is this child a car?
    if (strcmp(child_name, CARNAME) == 0) {
      // Yes, we found a car.  Raise the car widget so it is not
      // obscured.  May also generate an expose event.
      XRaiseWindow(dpy, XtWindow(child));
    }
  }
}

static void init_pixcach(w)
     GraphWidget w;
{
    Display *dpy = XtDisplay (w);
    Window win = XtWindow (w);

    w->graph.pixcach = XCreatePixmap(dpy,win,w->core.width,w->core.height,1);
    w->graph.cgc = XCreateGC(XtDisplay(w), w->graph.pixcach,0,NULL);

    XSetFunction(dpy,w->graph.cgc,GXclear);
    XFillRectangle(dpy,w->graph.pixcach,w->graph.cgc,0,0,w->core.width,w->core.height);
    XSetFunction(dpy,w->graph.gc,GXcopy);

}  

/* ARGSUSED */
static Boolean SetValues (gcurrent, grequest, gnew)
    Widget gcurrent, grequest, gnew;
{
  // suppress compiler warning about unused callback parameters.
  (void)grequest;

  GraphWidget current = (GraphWidget) gcurrent;
  GraphWidget new = (GraphWidget) gnew;
  Boolean redisplay = FALSE;
  Display *dpy = XtDisplay (current);
  /*  Window win = XtWindow (current); */

/*
    if (current->graph.foreground != new->graph.foreground ||
        current->core.background_pixel != new->core.background_pixel) {
        XFreeGC (dpy, current->graph.gc);
        GetnormalGC(new);
        redisplay = TRUE;
    }
*/
    if (current->core.width != new->core.width ||
        current->core.height != new->core.height) {
      if(current->graph.pixcach)
        XFreePixmap(dpy,current->graph.pixcach);
      init_pixcach(new);
      redisplay = TRUE;
    }

    return (redisplay);
}

/* // FIXME: never used?
static void graphDrawLine(
      GraphWidget w,
      int x1,
      int y1,
      int x2,
      int y2) {
  register Display *dpy = XtDisplay (w);
  register Window win = XtWindow (w);
  GC gc = w->graph.gc;

  check_pixmap(w);

  XDrawLine(dpy, w->graph.pixcach, gc, x1, y1, x2, y2);
  XDrawLine(dpy, win, gc, x1, y1, x2, y2);
}
*/

/* // FIXME: never used?
static void graphDrawPoint(
      GraphWidget w,
      int x1,
      int y1) {
  register Display *dpy = XtDisplay (w);
  register Window win = XtWindow (w);
  GC gc = w->graph.gc;

  check_pixmap(w);

  XDrawPoint(dpy, win, gc, x1, y1);
  XDrawPoint(dpy, w->graph.pixcach, gc, x1, y1);
}
*/

/*
*/
void graphCopyArea(
      GraphWidget w,
      Drawable src,
      int src_x,
      int src_y,
      unsigned int width,
      int height,
      int dest_x,
      int dest_y,
      int display_function) {
  register Display *dpy = XtDisplay (w);
  register Window win = XtWindow (w);
  GC cgc;

  check_pixmap(w);
  cgc =  w->graph.cgc;
  XSetFunction(dpy, cgc, display_function);
#if 1
  XCopyArea(dpy, src, w->graph.pixcach, cgc, src_x, src_y, width, height,
               dest_x, dest_y);
  XCopyPlane(dpy, w->graph.pixcach, win, w->graph.gc, dest_x, dest_y, width, height,
               dest_x, dest_y, 1);
#else
  XCopyArea(dpy, src, win, gc, src_x, src_y, width, height,
               dest_x, dest_y);
  XCopyArea(dpy, src, w->graph.pixcach, gc, src_x, src_y, width, height,
               dest_x, dest_y);
#endif
  XSetFunction(dpy, cgc, GXcopy);
}

/* // FIXME: never used?
static void graphClearPoint(
      GraphWidget w,
      int x1,
      int y1) {
  register Display *dpy = XtDisplay (w);
  register Window win = XtWindow (w);
  GC gc = w->graph.gc;

  check_pixmap(w);

  XSetFunction(dpy,gc,GXset);
  XDrawPoint(dpy, win, gc, x1, y1);
  XDrawPoint(dpy, w->graph.pixcach, gc, x1, y1);
  XSetFunction(dpy,gc,GXcopy);
}
*/

/*  // FIXME: never used?
static void graphClear(
      GraphWidget w) {
  register Display *dpy = XtDisplay (w);
  register Window win = XtWindow (w);

  check_pixmap(w);

  XClearWindow (dpy, win);
}
*/

/*  // FIXME: never used?
static void graphDrawText(
      GraphWidget w,
      char *s) {
  register Display *dpy = XtDisplay (w);
  register Window win = XtWindow (w);
  GC gc = w->graph.gc;

  check_pixmap(w);

  XDrawString(dpy, win, gc,
                2, 2+w->graph.font->ascent, s, strlen(s));
  XDrawString(dpy,w->graph.pixcach, gc,
                2, 2+w->graph.font->ascent, s, strlen(s));
}
*/

/*******************************************************************************
  local
*******************************************************************************/

GraphClassRec graphClassRec = {
  { /* core fields */
    /* superclass                */        &widgetClassRec,
    /* class_name                */        "Graph",
    /* widget_size                */        sizeof(GraphRec),
    /* class_initialize                */        ClassInitialize,
    /* class_part_initialize        */        NULL,
    /* class_inited                */        FALSE,
    /* initialize                */        Initialize,
    /* initialize_hook                */        NULL,
    /* realize                        */        Realize,
    /* actions                        */        NULL,
    /* num_actions                */        0,
    /* resources                */        resources,
    /* resource_count                */        XtNumber(resources),
    /* xrm_class                */        0, /* NULL */
    /* compress_motion                */        TRUE,
    /* compress_exposure        */        TRUE,
    /* compress_enterleave        */        TRUE,
    /* visible_interest                */        FALSE,
    /* destroy                        */        Destroy,
    /* resize                        */        NULL,
    /* expose                        */        Handle_expose,
    /* set_values                */        SetValues,
    /* set_values_hook                */        NULL,
    /* set_values_almost        */        XtInheritSetValuesAlmost,
    /* get_values_hook                */        NULL,
    /* accept_focus                */        NULL,
    /* version                        */        XtVersion,
    /* callback_private                */        NULL,
    /* tm_table                        */        NULL,
    /* query_geometry                */        XtInheritQueryGeometry,
    /* display_accelerator        */        XtInheritDisplayAccelerator,
    /* extension                */        NULL
    },{0} /* dummy initialization */
};

/*******************************************************************************
  exported
*******************************************************************************/
  
WidgetClass graphWidgetClass = (WidgetClass) &graphClassRec;


