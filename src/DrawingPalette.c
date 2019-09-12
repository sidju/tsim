#include <stdio.h>     /* sscanf */
#include <stdlib.h>    /* malloc, free */ 

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Toggle.h>              /* for XawToggleSet/GetCurrent */
#include <X11/Shell.h>

#include "general.h"
#include "train.h"
#include "bitmap.h"
#include "file.h"
#include "AddToggleWidget.h"
#include "DialogPopup.h"
#include "DrawingPalette.h"

/* codes for radiogroup. 0-99 reserved for railtypes */
#define CARBUTTON 100
#define BULLDOZERBUTTON 101
#define SWITCHBUTTON 102
#define SPEEDBUTTON 103
#define SENSORBUTTON 104
#define CUSTOMBUTTON 1000 /* 1000- for custom bitmaps */

static String OkAndCancelButtons[] = { "ok", "cancel" };
static String OkButton[] = { "ok" };

typedef struct {
  Widget radio_group;
  Widget coordinates;
  tTrainLine train_line;
} tRadioInfo;

typedef struct {
  tTrainLine train_line;
  tRailPos pos;
  Widget popup, dialog;
} tCarDialogInfo;

typedef struct {
  tTrainLine train_line;
  tTrainNo train_no;
} tSpeedDialogInfo;

static tSpeed wanted_speed = 20;

static Boolean SpeedDialogCallback(dialog, text, info, button)
     Widget dialog;
     String text;
     tSpeedDialogInfo *info;
     String button;
{
  if(!strcmp(button, "ok"))
    if(!sscanf(text, "%f", &wanted_speed) ||
               SetSpeed(info->train_line, info->train_no, wanted_speed)) {
      DialogPopup("illegalSpeed", dialog, NULL, NULL, OkButton, 1, 
                  "Set another speed.");
      return(False);
    }
  free(info);
  return(True);
}

static void SpeedDialog(w, train_line, pos)
     Widget w;
     tTrainLine train_line;
     tRailPos pos;

{
  tSpeedDialogInfo *info;
  char speedstr[100];
  sprintf(speedstr,"%.1f",wanted_speed);
  info = NEW(tSpeedDialogInfo);
  if(!CarPresent(train_line, WidgetPos(pos), NULL, &info->train_no)) {
    free(info);
    return;
  }
  info->train_line = train_line;
  DialogPopup("speed", w, SpeedDialogCallback, info, OkAndCancelButtons, 2,
              speedstr);
}

static Boolean AddCarDialogCallback(dialog, text, info, button)
     Widget dialog;
     String text;
     tCarDialogInfo *info;
     String button;
{
  int no_of_cars;
  char direction[100];
  tTrainNo train_no;

  if(!strcmp(button, "ok")) {
    if(sscanf(text, "%d %100s",&no_of_cars, direction) != 2 || no_of_cars < 0)
      {
        DialogPopup("illegalNumber", dialog, NULL, NULL, OkButton, 1, 
                    "Too many cars.");
        return(False);
      }
    if(CreateTrain(info->train_line, info->pos, 
                       StringToDirection(direction), no_of_cars, &train_no)) {
      DialogPopup("cannotCreateTrain", dialog, NULL, NULL, OkButton, 1, 
                  "Place on rail.");
      return(False);
    }
  }
  free(info);
  return(True);
}

static void AddCarDialog(w, train_line, pos)
     Widget w;
     tTrainLine train_line;
     tRailPos pos;
{
  tCarDialogInfo *info;
  static String default_no = NULL;

  info = NEW(tCarDialogInfo);
  info->train_line = train_line;
  info->pos = pos;
  DialogPopup("noOfCarsAndDirection", w, 
              AddCarDialogCallback, info, OkAndCancelButtons, 2, default_no);
}

static Boolean RailPosOk(dw, event, train_line, pos)
     Widget dw;
     XEvent *event;
     tTrainLine train_line;
     tRailPos *pos;
{
  tRailPos space;
  int x, y;
  Window dummy_window;

  XTranslateCoordinates(XtDisplay(dw), XtWindow(dw),
                        XtWindow(train_line->back_widget),
                        event->xbutton.x, event->xbutton.y,
                        &x, &y, &dummy_window);
  space = Space(train_line);
  *pos = RailPos(Pos(x, y));
  if(pos->x < 0 || pos->y < 0 || pos->x >= space.x || pos->y >= space.y)
    return False;
  return True;
}

static void MotionEventHandler(
     Widget dw,
     tRadioInfo *radio_info,
     XEvent *event,
     Boolean *cont) {
  tRailPos pos;
  char buf[1000];
  static tRailPos old_pos = { -1, -1 };

  // suppress compiler warning about unused callback parameters.
  (void)cont;

  if(!RailPosOk(dw, event, radio_info->train_line, &pos))
    return;
  if(old_pos.x != pos.x || old_pos.y != pos.y) {
    old_pos = pos;
    sprintf(buf, "%d %d", pos.x, pos.y);
    XtVaSetValues(radio_info->coordinates, XtNlabel, buf, NULL);
  }
}

static void ButtonEventHandler(dw, radio_info, event, cont)
     Widget dw;
     tRadioInfo *radio_info;
     XEvent *event;
     Boolean *cont;
{
  tRailPos pos;
  long radio_data;
  tTrainLine train_line = radio_info->train_line;

  // suppress compiler warning about unused callback parameters.
  (void)cont;

  if(!RailPosOk(dw, event, train_line, &pos))
    return;

  /* XawToggle... returns XtPointer (which is void *). 
     What should I do with that? */
  radio_data = (long) XawToggleGetCurrent(radio_info->radio_group);
  /*  fprintf (stderr, "radio_data = %d\n", radio_data); */

  switch(radio_data) {
  case CARBUTTON:
    AddCarDialog(dw, train_line, pos);
    break;
  case BULLDOZERBUTTON:
    SetRailType(train_line, pos, NoRail); 
    break;
  case SWITCHBUTTON:
    switch(SwitchState(train_line, pos)) {
    case LeftSwitch:
      SetSwitch(train_line, pos, RightSwitch);
      break;
    case RightSwitch:
      SetSwitch(train_line, pos, LeftSwitch);
      break;
    default:
      break;
    }
    break; 
  case SPEEDBUTTON:
    SpeedDialog(dw, train_line, pos);
    break;
  case SENSORBUTTON:
    if(SensorState(train_line, pos) == NoSensor)
      SetSensorState(train_line, pos, Sensor);
    else
      SetSensorState(train_line, pos, NoSensor);
    break;
  default:
    if(radio_data >= CUSTOMBUTTON) {
      int i;
      void *entry = NULL;
      tBitmapEntry *custom_bitmap;
      for(i = CUSTOMBUTTON; i<= radio_data; i++)
        CustomBitmaps(&entry, &custom_bitmap);
      SetRailBitmap(train_line, pos, custom_bitmap);
    }
    else
      SetRailType(train_line, pos, (tRailType)radio_data);
    break;
  }
}

static Boolean SaveCallback(dialog, filename, train_line, button)
     Widget dialog;
     String filename;
     tTrainLine train_line;
     String button;
{
  if(!strcmp(button, "ok"))
    if(SaveTrainLine(filename, train_line)) {
      DialogPopup("cannotSave", dialog, NULL, NULL, OkButton, 1, "-1");
      return(False);
    }
  return(True);
}

/*******************************************************************************
  local
*******************************************************************************/

/*
  type is XtCallbackProc.
*/
static void SavePressed(
     Widget w,
     XtPointer train_line, /* closure. data the application registered */
     XtPointer call_data) { /* callback specific data */

  // suppress compiler warning about unused callback parameters.
  (void)call_data;

  DialogPopup("save", w, SaveCallback, train_line, OkAndCancelButtons, 2,
              ((tTrainLine)train_line)->file_name);
}

/*
  type is XtCallbackProc.
*/
static void QuitPressed(
      Widget w,
      XtPointer train_line, /* closure. data the application registered */
      XtPointer call_data) { /* callback specific data */

  // suppress compiler warning about unused callback parameters.
  (void)w;
  (void)train_line;
  (void)call_data;

  exit(0);
}

static void AddButton(
      String name,
      Widget parent,
      XtCallbackProc action,
      XtPointer data) {
  Widget w;
  w = XtCreateManagedWidget(name, commandWidgetClass, parent, NULL, 0);
  XtAddCallback(w, XtNcallback, action, data);
}

static Widget AddBitmapToggle(
      String name,
      Widget parent,
      Widget radio_group,
      XtPointer radio_data,
      Pixmap bitmap) {
  int i;
  Arg wargs[1];
  Widget tw;

  i = 0;
  if(bitmap) {
    XtSetArg(wargs[i], XtNbitmap, bitmap); i++;
  }
  tw = AddToggleWidget(name, parent, radio_group, radio_data, wargs, i);
  return(tw);
}

/*******************************************************************************
  exported
*******************************************************************************/

/*
*/
Widget CreateDrawingPalette(
      String name,
      Widget parent,
      tTrainLine train_line) {
  Widget shell, composite, tw, coordinates;
  tRadioInfo *radio_info;

  shell = XtCreatePopupShell(name, topLevelShellWidgetClass, parent, NULL, 0);
  composite = XtCreateManagedWidget("palette", boxWidgetClass, shell, NULL, 0);
  
  /* Create huge radio group */
  tw = NULL;

  {
    tBitmapEntry *custom_bitmap;
    void *entry = NULL;
    long i = CUSTOMBUTTON;
    for(CustomBitmaps(&entry, &custom_bitmap); custom_bitmap;
        CustomBitmaps(&entry, &custom_bitmap))
      tw = AddBitmapToggle("customButton", composite, tw, (XtPointer)(i++),
                           custom_bitmap->bitmap);
  }

  {
    tRailType r;
    for(r = HorizontalRail; r <= LeftStopRail; r++)
      tw = AddBitmapToggle("railButton", composite, tw, (XtPointer)r, RailBitmap[r]);
  }

  AddBitmapToggle("sensorButton", composite, tw, (XtPointer)SENSORBUTTON,
                  SensorBitmap[HorizontalRail]);
  AddBitmapToggle("trainButton", composite, tw, (XtPointer)CARBUTTON,
                  CarBitmap[Engine][Left][ForegroundBitmap]);
  AddBitmapToggle("bulldozerButton", composite, tw, (XtPointer)BULLDOZERBUTTON, 0);
  AddBitmapToggle("switchButton", composite, tw, (XtPointer)SWITCHBUTTON, 0);
  AddBitmapToggle("speedButton", composite, tw, (XtPointer)SPEEDBUTTON, 0);
  /* XawToggle... needs an XtPointer as second argument */
  XawToggleSetCurrent(tw, (XtPointer)HorizontalRail);

  coordinates = 
    XtCreateManagedWidget("coordinates", labelWidgetClass, composite, NULL, 0);
  AddButton("save", composite, SavePressed, train_line);
  AddButton("quit", composite, QuitPressed, train_line);

  radio_info = NEW(tRadioInfo);
  radio_info->train_line = train_line;
  radio_info->radio_group = tw;
  radio_info->coordinates = coordinates;
  XtAddEventHandler(train_line->top, ButtonPressMask,
                    False, (XtEventHandler)ButtonEventHandler, radio_info);
  XtAddEventHandler(train_line->top, PointerMotionMask,
                    False, (XtEventHandler)MotionEventHandler, radio_info);
  XtPopup(shell, XtGrabNone);
  return(shell);
}

