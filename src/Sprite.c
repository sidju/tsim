/*
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * Author:  Jim Fulton, MIT X Consortium
 *
 */

/* Sprite Widget by Magnus Carlsson */

#include <stdio.h>                        /* for printing error messages */
#include <pwd.h>                        /* for getting username */
#include <sys/stat.h>                        /* for stat() ** needs types.h ***/

#include <X11/cursorfont.h>                /* for cursor constants */
#include <X11/StringDefs.h>                /* for useful atom names */
#include <X11/IntrinsicP.h>                /* for toolkit stuff */
#include <X11/Xaw/XawInit.h>
#include <X11/Xmu/Converters.h>                /* for XmuCvtStringToBitmap */
#include <X11/Xmu/Drawing.h>                /* for XmuCreatePixmapFromBitmap */
#include <X11/extensions/shape.h>

#include "Sprite.h"

/* from "SpriteP.h" */
typedef struct {                        /* new fields for sprite widget */
    /* resources */
    Pixel foreground_pixel;                /* color index of normal state fg */
    Boolean reverseVideo;                /* do reverse video? */
    GC gc;                                /* normal GC to use */
    struct _mbimage {
        Pixmap bitmap, mask;                /* depth 1, describing shape */
        Pixmap pixmap;                        /* full depth pixmap */
        int width, height;                /* geometry of pixmaps */
    } content;
    Boolean shape_it;
    Pixmap shape_cache;
} SpritePart;

/* sub record to Widget, which contains only the "CorePart".
   see "/usr/include/X11/CoreP.h" for CorePart fields.
*/
typedef struct _SpriteRec {                /* full instance record */
    CorePart core;
    SpritePart sprite;
} SpriteRec;


typedef struct {                        /* new fields for mailbox class */
    int dummy;                                /* stupid C compiler */
} SpriteClassPart;

/* full class record declaration */
typedef struct _SpriteClassRec {
    CoreClassPart core_class;
    SpriteClassPart mailbox_class;
} SpriteClassRec;

/*******************************************************************************
  local
*******************************************************************************/



/* Initialization of defaults */
#define offset(field) XtOffset(SpriteWidget,sprite.field)
#define goffset(field) XtOffset(Widget,core.field)
static Dimension defDim = 48;
static Pixmap nopix = None;
static XtResource resources[] = {
    { XtNwidth, XtCWidth, XtRDimension, sizeof (Dimension), 
        goffset (width), XtRDimension, (caddr_t)&defDim },
    { XtNheight, XtCHeight, XtRDimension, sizeof (Dimension),
        goffset (height), XtRDimension, (caddr_t)&defDim },

    { XtNforeground, XtCForeground, XtRPixel, sizeof (Pixel),
        offset (foreground_pixel), XtRString, "black" },
    { XtNbackground, XtCBackground, XtRPixel, sizeof (Pixel),
        goffset (background_pixel), XtRString, "white" },
    { XtNreverseVideo, XtCReverseVideo, XtRBoolean, sizeof (Boolean),
        offset (reverseVideo), XtRString, "FALSE" },

    { XtNwindowPixmapMask, XtCPixmapMask, XtRBitmap, sizeof(Pixmap),
        offset (content.mask), XtRBitmap, (caddr_t) &nopix },
    { XtNcontentPixmap, XtCPixmap, XtRBitmap, sizeof(Pixmap),
        offset (content.bitmap), XtRBitmap, (caddr_t) &nopix },
};
#undef offset
#undef goffset



/*
  widget initialization
*/
static void ClassInitialize() {
  static XtConvertArgRec screenConvertArg[] = {
  { XtWidgetBaseOffset, (caddr_t) XtOffset(Widget, core.screen), sizeof(Screen *) }
  };

  XawInitializeWidgetSet();
  XtAddConverter (XtRString, XtRBitmap, XmuCvtStringToBitmap,
                  screenConvertArg, XtNumber(screenConvertArg));
}



/*
*/
static GC get_sprite_gc (
      SpriteWidget w) {
  XtGCMask valuemask;
  XGCValues xgcv;

  valuemask = GCForeground | GCBackground | GCFunction;
  xgcv.foreground = w->sprite.foreground_pixel;
  xgcv.background = w->core.background_pixel;
  xgcv.function = GXcopy;
  return (XtGetGC ((Widget) w, valuemask, &xgcv));
}



/*
  type must be same as XtInitProc (see "/usr/include/X11/IntrinsicP.h"),
  and is supposed to "initialize subclass fields".
*/
static void Initialize (
      Widget request,
      Widget new,
      ArgList args,
      Cardinal *num_args) {

  // suppress compiler warning about unused callback parameter.
  (void)request;
  (void)args;
  (void)num_args;

  SpriteWidget w = (SpriteWidget) new;
  int shape_event_base, shape_error_base;


  if (w->core.width <= 0) w->core.width = 1;
  if (w->core.height <= 0) w->core.height = 1;

  if (w->sprite.reverseVideo) {
    Pixel tmp;

    tmp = w->sprite.foreground_pixel;
    w->sprite.foreground_pixel = w->core.background_pixel;
    w->core.background_pixel = tmp;
  }

  w->sprite.shape_it = XShapeQueryExtension (XtDisplay (w),
                                             &shape_event_base,
                                             &shape_error_base);
  w->sprite.gc = get_sprite_gc(w);
}



/*
  action procedures
*/
static Pixmap make_pixmap(
      Display *dpy,
      SpriteWidget w,
      Pixmap bitmap,
      int depth,
      int *widthp,
      int *heightp) {
  Window root;
  int x, y;
  unsigned int width, height, bw, dep;
  Dimension border_width = 0;
  unsigned long fore, back;

  if (!XGetGeometry (dpy, bitmap, &root, &x, &y, &width, &height, &bw, &dep))
    return None;

  *widthp = (int) width;
  *heightp = (int) height;
  XtResizeWidget((Widget)w, width, height, border_width);
       /* width & height should also be Dimesion, which is "unsigned short" */
  fore = w->sprite.foreground_pixel;
  back = w->core.background_pixel;
  return XmuCreatePixmapFromBitmap(dpy, w->core.window, bitmap, 
                                   width, height, depth, fore, back);
}


/*
  type of this function is the same as "XtRealizeProc",
  see "/usr/include/X11/IntrinsicP.h", but the Widget is
  supposedly always a SpriteWidget, so we can safely down-cast it.
*/
static void Realize (
      Widget gw,
      XtValueMask *valuemaskp,
      XSetWindowAttributes *attr) {
  SpriteWidget w = (SpriteWidget) gw;
  /*    register Display *dpy = XtDisplay (w);
        int depth = w->core.depth;
  */
  *valuemaskp |= (CWBitGravity);
  attr->bit_gravity = ForgetGravity;

  XtCreateWindow (gw, InputOutput, (Visual *) CopyFromParent,
                  *valuemaskp, attr);

  w->sprite.content.pixmap = 
       make_pixmap(XtDisplay(gw), w, w->sprite.content.bitmap,
                   w->core.depth, &w->sprite.content.width, 
                   &w->sprite.content.height);
  w->sprite.shape_cache = None;
}



/*
*/
static void Destroy(
      Widget gw) {
  SpriteWidget w = (SpriteWidget) gw;
  Display *dpy = XtDisplay (gw);

  XtDestroyGC (w->sprite.gc);
#define freepix(p) if (p) XFreePixmap (dpy, p)
  freepix (w->sprite.content.pixmap);                /* until cvter does ref cnt */
}



/*
  same type as "XtSetValuesFunc".
  task: "set subclass resource values"
*/
static Boolean SetValues(
      Widget gcurrent,   /* old */
      Widget grequest,   /* request */
      Widget gnew,       /* new */
      ArgList args,
      Cardinal* num_args) {

  // suppress compiler warning about unused callback parameter.
  (void)grequest;
  (void)args;
  (void)num_args;

  SpriteWidget current = (SpriteWidget) gcurrent;
  SpriteWidget new = (SpriteWidget) gnew;
  Boolean redisplay = FALSE;
  Display *dpy = XtDisplay (gcurrent);

  XRaiseWindow(dpy, XtWindow(new));
  if(current->sprite.content.bitmap != new->sprite.content.bitmap) {
    freepix(current->sprite.content.pixmap);
#undef freepix
    new->sprite.content.pixmap = 
      make_pixmap(XtDisplay(gnew), new, new->sprite.content.bitmap,
                  new->core.depth, &new->sprite.content.width, 
                  &new->sprite.content.height);
    redisplay = TRUE;
  }

  if (current->sprite.foreground_pixel != new->sprite.foreground_pixel ||
    current->core.background_pixel != new->core.background_pixel) {
    XtDestroyGC (current->sprite.gc);
    new->sprite.gc = get_sprite_gc (new);
    redisplay = TRUE;
  }
  return redisplay;
}



/*
  same type as "XtExposeProc".
  task is to "rediplay window".
*/
static void Handle_expose(
      Widget _w,
      XEvent *event,
      Region region) {

  // suppress compiler warning about unused callback parameter.
  (void)region;

  SpriteWidget w = (SpriteWidget)_w;
  register Display *dpy = XtDisplay (w);
  register Window win = XtWindow (w);
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

  XCopyArea (dpy, w->sprite.content.pixmap, win, w->sprite.gc, x, y,
             width, height, x, y);
  
  if (w->sprite.shape_it && w->sprite.content.mask != w->sprite.shape_cache) {
    XShapeCombineMask (dpy, win, ShapeBounding, 0, 0,
                       w->sprite.content.mask, ShapeSet);
    w->sprite.shape_cache = w->sprite.content.mask;
  }
}

/* used in global */
SpriteClassRec spriteClassRec = {
    { /* core fields */
    /* superclass                */        &widgetClassRec,
    /* class_name                */        "Sprite",
    /* widget_size                */        sizeof(SpriteRec),
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
    /* xrm_class                */        NULLQUARK,  /* 0 / NULL */
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
    },
    {0} /* dummy initialization */
};

/*******************************************************************************
  exported
*******************************************************************************/

/* why a global ? */
WidgetClass spriteWidgetClass = (WidgetClass) &spriteClassRec;


