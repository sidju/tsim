/* AddToggleWidget Magnus Carlsson 1990-09-06 */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Toggle.h>

#include "AddToggleWidget.h"

/* AddToggleWidget will create a managed ToggleWidget in a radio_group,
   and set translation tables for correct behavior.
   Set radio_group to NULL to create the first widget in the group. */
Widget AddToggleWidget(
      String name,
      Widget parent,
      Widget radio_group,
      XtPointer radio_data,
      ArgList args,
      Cardinal num_args) {
  static String translation_table =
    "<EnterWindow>:        highlight(Always) \n \
     <LeaveWindow>:        unhighlight() \n \
     <Btn1Down>,<Btn1Up>:        set() notify()";
  static XtTranslations toggle_translations;

  Widget tw;
  Arg tw_args[2];
  Cardinal i;

  if(!toggle_translations)
    toggle_translations = XtParseTranslationTable(translation_table);

  i = 0;
  XtSetArg(tw_args[i], XtNradioData, radio_data); i++;
  XtSetArg(tw_args[i], XtNradioGroup, radio_group); i++;
  tw = XtCreateManagedWidget(name, toggleWidgetClass, parent, tw_args, i);
  
  XtSetValues(tw, args, num_args);
  XtOverrideTranslations(tw, toggle_translations);
  return(tw);
}

