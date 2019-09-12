
#ifndef TS_Graph_H_
#define TS_Graph_H_

#include <X11/Intrinsic.h>
#include <X11/CoreP.h>

#define XtNxmax "xmax"
#define XtNxmin "xmin"
#define XtNymax "ymax"
#define XtNymin "ymin"

#define XtCXmax "Xmax"
#define XtCXmin "Xmin"
#define XtCYmax "Ymax"
#define XtCYmin "Ymin"

#define XtNmaxvalues "maxvalues"
#define XtCMaxvalues "Maxvalues"

#define XtNxauto "xauto"
#define XtCXauto "Xauto"

typedef struct _GraphRec *GraphWidget;
typedef struct _GraphClassRec *GraphWidgetClass;

extern WidgetClass graphWidgetClass;

void graphCopyArea(
      GraphWidget w,
      Drawable src,
      int src_x,
      int src_y,
      unsigned int width,
      int height,
      int dest_x,
      int dest_y,
      int display_function);

#endif /* TS_Graph_H_ */

