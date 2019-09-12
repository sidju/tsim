/* DialogPopup will popup a modal dialog window with a message, an optional
   input text widget with an optional default value, and a number of buttons
   (at least one)


Boolean acknowledge(dialog, text, proc_data, button_name)
     Widget dialog;
     String text;		Content of text widget
     YourDataType proc_data;
     String button_name;	The same string pointer that was supplied to
     				DialogPopup
  
Return True if you want the dialog to end, false if you want another answer.
*/

#include <stdio.h>                   /* fprintf */
#include <string.h>
#include <stdlib.h>                  /* malloc, free */ 

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>          /* for XtMoveWidget prototype */
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Dialog.h>

#include "general.h"
#include "DialogPopup.h"

/*******************************************************************************
  local
*******************************************************************************/

typedef struct {
  Widget dialog;
  Widget popup;
  Boolean has_value;
  struct _ButtonInfo *button_infos;
  Boolean (*acknowledge)();
  XtPointer proc_data;
} tInfo;

typedef struct _ButtonInfo {
  struct _ButtonInfo *next;
  tInfo *dialog_info;
  String button_name;
} tButtonInfo;

static void ButtonPressed(w, button_info, call_data)
     Widget w;                   // unused callback parameter
     tButtonInfo *button_info;
     XtPointer call_data;
{
  String text;
  tInfo *info = button_info->dialog_info;

  // unused callback parameters (suppresses GCC unused-parameter warning)
  (void)w;
  (void)call_data;

  if(info->has_value) 
    text = strdup(XawDialogGetValueString(info->dialog));
  else
    text = NULL;

  if(!info->acknowledge ||
     info->acknowledge(info->dialog, text, info->proc_data,
		       button_info->button_name)) {
    XtPopdown(info->popup);
    XtDestroyWidget(info->popup);
    {
      tButtonInfo *binfo, *next_binfo;
      /* Too many calls to free(). How to solve that? Don't free! */

      for(binfo = info->button_infos; binfo; /* binfo = binfo->next */) {
	next_binfo = binfo->next;
	/*fprintf (stderr, "binfo = %ld, binfo->next = %ld\n", 
	  	 (long) binfo, (long) next_binfo); */
	free(binfo);
	binfo = next_binfo;
      }
      free(info);      
    }
  }
}

/*******************************************************************************
  exported
*******************************************************************************/

/*
  called to create a popup window
*/
void DialogPopup(name, parent, acknowledge, proc_data, 
		 button_list, no_of_buttons, default_value)
     String name; /* Name of popup resource */
     Widget parent;
     Boolean (*acknowledge)(); /* Optional */
     XtPointer proc_data;
     String *button_list; /*Names of buttons */
     Cardinal no_of_buttons;
     String default_value; /* NULL if determined by resource, "" if no value */
{
  tInfo *info;
  unsigned int width, height;
  info = NEW(tInfo);

/*  fprintf(stderr, "#entering DialogPopup\n"); */

  info->popup = XtCreatePopupShell(name, transientShellWidgetClass, parent,
				   NULL, 0);
  {
    Arg wargs[2];
    Cardinal i = 0;
    if(default_value && (long)default_value != -1) {
      XtSetArg(wargs[i], XtNvalue, default_value); i++; }
    info->dialog = XtCreateManagedWidget("dialog", dialogWidgetClass,
					 info->popup, wargs, i);
  }

  info->has_value = (long)default_value != -1;
  info->acknowledge = acknowledge;
  info->proc_data = proc_data;
  info->button_infos = NULL;
  {
    Cardinal i;
    tButtonInfo *last_button_info, *button_info;
    for(i = 0; i < no_of_buttons; i++) {
      button_info = NEW(tButtonInfo);
      if(i)
	last_button_info->next = button_info;
      else
	info->button_infos = button_info;
      button_info->dialog_info = info;
      button_info->button_name = button_list[i];
      button_info->next = NULL;
      XawDialogAddButton(info->dialog, button_list[i],
			 ButtonPressed, button_info);
      last_button_info = button_info;
    }
  }

  XtRealizeWidget(info->popup); /* So that we can get width & height */
/*  {
    Arg wargs[2];
    Cardinal i = 0;
    XtSetArg(wargs[i], XtNwidth, &width); i++;
    XtSetArg(wargs[i], XtNheight, &height); i++;
    XtGetValues(info->dialog, wargs, i);
  }*/
  width = height = 0;
  /* Adjust popup window's position... */
  {
    int x,y, dummy;
    unsigned int dummy_mask;
    Window dummy_window;
    
    XQueryPointer(XtDisplay(parent), XtWindow(parent), &dummy_window,
		  &dummy_window, &x, &y, &dummy, &dummy, &dummy_mask);
/*    int parent_width = XWidthOfScreen(XtScreen(parent));
    int parent_height = XHeightOfScreen(XtScreen(parent));*/
    /*XGetWindowAttributes(XtDisplay(parent), XtWindow(parent), &xwa);*/

    XtMoveWidget(info->popup, x - width / 2, y - height / 2);
/*    XtSetArg(wargs[i], XtNx, (x - width) / 2); i++;
    XtSetArg(wargs[i], XtNy, (y - height) / 2); i++;
    XtSetValues(info->popup, wargs, i);*/
  }
  XtPopup(info->popup, XtGrabExclusive);
}

