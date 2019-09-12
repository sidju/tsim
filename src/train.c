#include <stdio.h>
#include <math.h>
#include <stdlib.h>                     /* malloc, abs */

#include <X11/Xlib.h>                        /* for Xlib definitions */
#include <X11/StringDefs.h>                /* for useful atom names */
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>             /* for XtMoveWidget prototype */
#include <X11/Shell.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Label.h>

#include "general.h"
#include "Graph.h"
#include "Sprite.h"
#include "bitmap.h"
#include "file.h"
#include "sound.h"
#include "pipe.h"
#include "train.h"
#include "timer.h"

/* globals! */
unsigned int RailWidth;
int WheelOffset;
Pixmap 
  CarBitmap[eCarType_size][Left+1][MaskBitmap+1],
  RailBitmap[LeftStopRail+1],
  SwitchBitmap[Left+1][RightSwitch+1],
  SensorBitmap[LeftStopRail+1];
  
  
  
static tDirection NewDirs[LeftStopRail+1][Left+1] = {
/*  NoDir        Up        Right        Down        Left */
  { NoDir,        NoDir,        NoDir,        NoDir,        NoDir }, /* NoRail */
  { NoDir,        NoDir,        Right,        NoDir,        Left }, /* HorizontalRail */
  { NoDir,        Up,        NoDir,        Down,        NoDir }, /* VerticalRail */
  { NoDir,        NoDir,        Up,        Left,        NoDir }, /* UpLeftRail */
  { NoDir,        NoDir,        NoDir,        Right,        Up }, /* UpRightRail */
  { NoDir,        Left,        Down,        NoDir,        NoDir }, /* DownLeftRail */
  { NoDir,        Right,        NoDir,        NoDir,        Down }, /* DownRightRail */
  { NoDir,        NoDir,        NoDir,        NoDir,        NoDir }, /* UpStopRail */
  { NoDir,        NoDir,        NoDir,        NoDir,        NoDir }, /* DownStopRail */
  { NoDir,        NoDir,        NoDir,        NoDir,        NoDir }, /* LeftStopRail*/
  { NoDir,        NoDir,        NoDir,        NoDir,        NoDir }, /* RightStopRail */
};

tPos Pos(
     int x,
     int y) {
  tPos p;
  p.x = x;
  p.y = y;
  return(p);
}

static tDirection OppositeDir(d)
     tDirection d;
{
  switch(d) {
  case Up:    return(Down);
  case Right: return(Left);
  case Down:  return(Up);
  case Left:  return(Right);
  default:    return(NoDir);
  }
}

static tDirection Clockwise(d)
     tDirection d;
{
  switch(d) {
  case Up:    return(Right);
  case Right: return(Down);
  case Down:  return(Left);
  case Left:  return(Up);
  default:    return(NoDir);
  }
}
    
static tDirection CommonEntry(rail)
     tRail *rail;
{
  /* Find out the common entry direction for a switch rail */
  tDirection dir = Up;
  do {
    if(NewDirs[rail->type[LeftSwitch]][dir] != NoDir &&
       NewDirs[rail->type[RightSwitch]][dir] != NoDir)
      return(dir);
    dir = Clockwise(dir);
  } while(dir != Up);
  return(NoDir);
}

static Boolean IsStopRail(tRailType railtype){
  return(railtype == UpStopRail ||
         railtype == RightStopRail ||
         railtype == DownStopRail ||
         railtype == LeftStopRail);
}

Boolean IsCustom(tRail *rail) {
  return(rail->custom && rail->bitmap_entry);
}

static Boolean IsCovered(tRail *rail, tRailPos *bitmap_pos){
  *bitmap_pos = rail->bitmap_pos;
  return(rail->custom && !rail->bitmap_entry);
}

static Boolean IsStop(tRail *rail){
  return(IsStopRail(rail->type[LeftSwitch]));
}

static Boolean IsCrossing(tRail *rail){
  return((rail->type[LeftSwitch] == HorizontalRail &&
          rail->type[RightSwitch] == VerticalRail) ||
         (rail->type[LeftSwitch] == VerticalRail &&
          rail->type[RightSwitch] == HorizontalRail));
}

static Boolean IsEmptyRail(tRail *rail){
  return(rail->type[LeftSwitch] == NoRail);
}

static Boolean IsCompositeRail(tRail *rail){
  return(rail->type[RightSwitch] != NoRail);
}

static Boolean IsSwitchRail(tRail *rail){
  return(IsCompositeRail(rail) && !IsCrossing(rail));
}

Boolean CarPresent(
      tTrainLine train_line,
      tPos pos,
      tTrainPart *not_this_car,
      tTrainNo *train) {
  tTrainPart *test_car;
  tTrain *test_train;
  int min_dist = RailWidth / 2;

  for(test_train = train_line->trains;
      test_train;
      test_train = test_train->next)
    for(test_car = test_train->parts; test_car;
        test_car = test_car->next) 
      if(test_car != not_this_car &&
         abs(test_car->widget_pos.x - pos.x) < min_dist &&
         abs(test_car->widget_pos.y - pos.y) < min_dist) {
        *train = test_train->no;
        return(True);
      }
  return(False);
}

static tRailPos BitmapSize(
      tBitmapEntry *bitmap_entry) {
  tRailPos pos;
  pos.x = (bitmap_entry->width - 1) / RailWidth + 1;
  pos.y = (bitmap_entry->height - 1) / RailWidth + 1;
  return pos;
}

static tPos AdjustCarPos(
      tDirection direction,
      tPos pos) {

  // suppress compiler warning about unused callback parameter.
  (void)direction;

  pos.x += (RailWidth - WheelOffset - 1);
  pos.y -= (RailWidth - WheelOffset - 1);
  return(pos);
}
        
static tSwitchState FlipSwitch(
      tSwitchState sw) {
  if(sw == LeftSwitch)
    return(RightSwitch);
  else
    return(LeftSwitch);
}

static tDirection NewDirection(
      tRail *rail,
      tDirection direction) {
  tDirection new_dir;
  
  new_dir = NewDirs[rail->type[rail->sw]][direction];
  if(new_dir == NoDir && IsCrossing(rail))
    new_dir = NewDirs[rail->type[FlipSwitch(rail->sw)]][direction];
  return(new_dir);
}

static tPos AddPos(
      tPos pos,
      tDirection direction,
      tRailOffset offset) {
  switch(direction) {
  case Up:                pos.y -= offset; break;
  case Right:        pos.x += offset; break;
  case Down:        pos.y += offset; break;
  case Left:        pos.x -= offset; break;
  case NoDir:        break;
  }
  return(pos);
}

static tStatus CheckRailPos(train_line, pos)
     tTrainLine train_line;
     tRailPos pos;
{
  if(pos.x < 0 || pos.y < 0 || 
     pos.x >= train_line->space.x || 
     pos.y >= train_line->space.y) return(IllegalRailPosStatus);
  else
    return(SuccessStatus);
}

static tEvent MovePos(train_line, distance, offset, dir, pos,
                    new_offset, new_dir, new_pos)
     tTrainLine train_line;
     tRailOffset offset, *new_offset, distance;
     tDirection dir, *new_dir;
     tPos pos, *new_pos;
{
  offset += distance;
  while(offset >= (tRailOffset)RailWidth || offset < 0) {
    if(distance < 0) { /* reverse! */
      offset += RailWidth;
      if(IsStop(&train_line->rails[pos.x][pos.y])) {
        SoundStopCrash(train_line);
        return(StopCollisionEvent);
      }
      dir = OppositeDir(NewDirection(&train_line->rails[pos.x][pos.y],
                                  OppositeDir(dir)));
      if(dir == NoDir) {
        SoundDerailCrash(train_line);
        return(DerailmentEvent);
      }
      pos = AddPos(pos, dir, -1);
      if(CheckRailPos(train_line,pos)) {
        SoundDerailCrash(train_line);
        return(DerailmentEvent);
      }
    } else {
      offset -= RailWidth;
      pos = AddPos(pos, dir, 1);
      if(CheckRailPos(train_line,pos)) {
        SoundDerailCrash(train_line);
        return(DerailmentEvent);
      }
      if(IsStop(&train_line->rails[pos.x][pos.y])) {
        SoundStopCrash(train_line);
        return(StopCollisionEvent);
      }
      dir = NewDirection(&train_line->rails[pos.x][pos.y], dir);
      if(dir == NoDir) {
        SoundDerailCrash(train_line);
        return(DerailmentEvent);
      }
    }
  }
  *new_dir = dir;
  *new_pos = pos;
  *new_offset = offset;
  return(NoEvent);
}

static Widget NewSprite(train_line, pos, bitmap_pair)
     tTrainLine train_line;
     tPos pos;
     Pixmap bitmap_pair[];
{
  Widget w;
  Arg wargs[4];
  int i = 0;

  XtSetArg(wargs[i], XtNx, pos.x); i++;
  XtSetArg(wargs[i], XtNy, pos.y); i++;
  XtSetArg(wargs[i], XtNwindowPixmapMask, bitmap_pair[MaskBitmap]); i++;
  XtSetArg(wargs[i], XtNcontentPixmap, bitmap_pair[ForegroundBitmap]); i++;
  w = XtCreateManagedWidget(CARNAME, spriteWidgetClass,
                          train_line->top, wargs, i);
  return(w);
}

static void SetSpritePos(sprite, pos)
     Widget sprite;
     tPos pos;
{
  XtMoveWidget(sprite, pos.x, pos.y);
}

static void SetSpritePixmap(sprite, bitmap_pair)
     Widget sprite;
     Pixmap bitmap_pair[];
{
  Arg wargs[2];
  int i = 0;

  XtSetArg(wargs[i], XtNwindowPixmapMask, bitmap_pair[MaskBitmap]); i++;
  XtSetArg(wargs[i], XtNcontentPixmap, bitmap_pair[ForegroundBitmap]); i++;
  XtSetValues(sprite, wargs, i);
}

static void DrawRail(train_line, pos)
     tTrainLine train_line;
     tRailPos pos;
{
  tPos widget_pos;
  tRail *rail = &train_line->rails[pos.x][pos.y];

#define DrawR(bmap,function) \
  graphCopyArea((GraphWidget)train_line->back_widget, (bmap), 0, 0,\
                 RailWidth, RailWidth, widget_pos.x, widget_pos.y, (function))
  
  widget_pos = WidgetPos(pos);
  if(IsCustom(rail))
    graphCopyArea((GraphWidget)train_line->back_widget, 
                  rail->bitmap_entry->bitmap, 0, 0,
                  rail->bitmap_entry->width, rail->bitmap_entry->height,
                  widget_pos.x, widget_pos.y, GXcopy);
  else {
    tRailPos pos;
    if(IsCovered(rail, &pos))
       DrawRail(train_line, pos);
    else {
       DrawR(RailBitmap[rail->type[LeftSwitch]], GXcopy);
       DrawR(RailBitmap[rail->type[RightSwitch]], GXor);
       if(IsSwitchRail(rail))
         DrawR(SwitchBitmap[CommonEntry(rail)][rail->sw], GXor);
       
       if(rail->sensor == Sensor)
         DrawR(SensorBitmap[rail->type[LeftSwitch]], GXor);
     }
  }
}

tTrainLine CreateTrainLine(
      Widget parent, /* must be mapped */
      tRailPos space) {
  unsigned int dummy4;
  int dummy5;
  tTrainLine train_line;

  train_line = NEW(sTrainLine);
  train_line->space = space;
  train_line->free_no = 1;
  train_line->top =
    XtCreateManagedWidget(TRAINLINENAME, viewportWidgetClass, parent, NULL, 0);

  ReadCar("HorizontalCar", Car, Right, ForegroundBitmap);
  ReadCar("HorizontalCarMask", Car, Right, MaskBitmap);
  ReadCar("LeftEngine", Engine, Left, ForegroundBitmap);
  ReadCar("LeftEngineMask", Engine, Left, MaskBitmap);
  ReadCar("RightEngine", Engine, Right, ForegroundBitmap);
  ReadCar("RightEngineMask", Engine, Right, MaskBitmap);
  ReadCar("UpEngine", Engine, Up, ForegroundBitmap);
  ReadCar("UpEngineMask", Engine, Up, MaskBitmap);
  ReadCar("DownEngine", Engine, Down, ForegroundBitmap);
  ReadCar("DownEngineMask", Engine, Down, MaskBitmap);
  ReadCar("VerticalCar", Car, Up, ForegroundBitmap);
  ReadCar("VerticalCarMask", Car, Up, MaskBitmap);

  ReadRail("HorizontalRail", HorizontalRail, &RailWidth, &WheelOffset);
  ReadRail("UpLeftRail", UpLeftRail, &dummy4, &dummy5);
  ReadRail("DownLeftRail", DownLeftRail, &dummy4, &dummy5);
  ReadRail("RightStopRail", RightStopRail, &dummy4, &dummy5);
  ReadRail("LeftStopRail", LeftStopRail, &dummy4, &dummy5);
  ReadRail("NoRail", NoRail, &dummy4, &dummy5);
  ReadRail("VerticalRail", VerticalRail, &dummy4, &dummy5);
  ReadRail("DownRightRail", DownRightRail, &dummy4, &dummy5);
  ReadRail("UpRightRail", UpRightRail, &dummy4, &dummy5);
  ReadRail("UpStopRail", UpStopRail, &dummy4, &dummy5);
  ReadRail("DownStopRail", DownStopRail, &dummy4, &dummy5);

  ReadSwitch("LeftSwitchLeft", Left, LeftSwitch);
  ReadSwitch("RightSwitchLeft", Left, RightSwitch);
  ReadSwitch("RightSwitchDown", Down, RightSwitch);
  ReadSwitch("LeftSwitchDown", Down, LeftSwitch);
  ReadSwitch("RightSwitchUp", Up, RightSwitch);
  ReadSwitch("LeftSwitchUp", Up, LeftSwitch);
  ReadSwitch("LeftSwitchRight", Right, LeftSwitch);
  ReadSwitch("RightSwitchRight", Right, RightSwitch);

/*
  ReadSemaphore("StopSemaphore", ForeGroundBitmap, StopSemaphore);
  ReadSemaphore("GoSemaphore", ForeGroundBitmap, GoSemaphore);
*/

  ReadSensor("UpLeftSensor", UpLeftRail);
  ReadSensor("UpRightSensor", UpRightRail);
  ReadSensor("DownLeftSensor", DownLeftRail);
  ReadSensor("DownRightSensor", DownRightRail);
  ReadSensor("HorizontalSensor", HorizontalRail);
  ReadSensor("VerticalSensor", VerticalRail);

  ReadCustomBitmaps(parent);
 
  {
    tBitmapType i;

    for(i = ForegroundBitmap; i <= MaskBitmap; i++) {
      CarBitmap[Car][Left][i] = CarBitmap[Car][Right][i];
      CarBitmap[Car][Down][i] = CarBitmap[Car][Up][i];
    }
  }

  {
    int x,y;
    
    for(x = 0; x < train_line->space.x; x++) {
      for(y = 0; y < train_line->space.y; y++) {
        train_line->rails[x][y].type[LeftSwitch] = NoRail;
        train_line->rails[x][y].type[RightSwitch] = NoRail;
        train_line->rails[x][y].sensor = NoSensor;
        train_line->rails[x][y].sw = LeftSwitch;
        train_line->rails[x][y].semaphore = NoSemaphore;
        train_line->rails[x][y].custom = False;
      }
    }
  }
  {
    Arg wargs[2];
    int i = 0;
    tPos widget_space;

    widget_space = WidgetPos(space);
    XtSetArg(wargs[i], XtNwidth, widget_space.x); i++;
    XtSetArg(wargs[i], XtNheight, widget_space.y); i++;
    train_line->back_widget = 
      XtCreateManagedWidget(BACKGROUNDNAME, graphWidgetClass,
                            train_line->top, wargs, i);
  }
  XtRealizeWidget(parent); /* Hack so that drawing will work... */
  AddTimeOut(train_line);
  train_line->file_name = NULL;
  return(train_line);
}

void DeleteTrainLine(tTrainLine train_line){
  while(train_line->trains)
    (void)DeleteTrain(train_line, train_line->trains->no);
  XtDestroyWidget(train_line->top);
}

tRailPos Space(tTrainLine train_line) {
  return(train_line->space);
}

tRailPos RailPos(tPos widget_pos){
  tRailPos rp;
  rp.x = widget_pos.x / RailWidth;
  rp.y = widget_pos.y / RailWidth;
  return(rp);
}

tPos WidgetPos(tRailPos rail_pos){
  tPos p;
  p.x = rail_pos.x * RailWidth;
  p.y = rail_pos.y * RailWidth;
  return(p);
}

static void ResetRail(rail, type)
     tRail *rail;
     tRailType type;
{
  rail->type[LeftSwitch] = type;
  rail->type[RightSwitch] = NoRail;
  rail->sw = LeftSwitch;
  rail->custom = False;
  rail->sensor = NoSensor;
}

static void SortRailTypes(rail)
     tRail *rail;
{
  /* Check if the two rail types in a switched rail are sorted so 
     that rail[LeftSwitch] really brings the train on a path to the left 
     of rail[RightSwitch]. If not, swap them */

  tDirection in_dir;

  in_dir = CommonEntry(rail);
  {
    tDirection dir, start_dir;
    dir = start_dir = Clockwise(OppositeDir(in_dir));
    do {
      if(NewDirs[rail->type[LeftSwitch]][in_dir] == dir)
        return; /* No swapping needed */
      else
        if(NewDirs[rail->type[RightSwitch]][in_dir] == dir) {
          tRailType temp = rail->type[LeftSwitch];
          rail->type[LeftSwitch] = rail->type[RightSwitch];
          rail->type[RightSwitch] = temp;
          return;
        }
      dir = Clockwise(dir);
    } while(dir != start_dir);
  }
}
  
static void ClearRail(train_line, pos)
     tTrainLine train_line;
     tRailPos pos;
{
  tTrainNo train_no;
  tRail *rail;

  if(CarPresent(train_line, WidgetPos(pos), NULL, &train_no))
    (void)DeleteTrain(train_line, train_no);
  rail = &train_line->rails[pos.x][pos.y];
  if(IsCustom(rail)) {
    int x,y;
    tRailPos size;
    size = BitmapSize(rail->bitmap_entry);
    for(x = pos.x; x < pos.x + size.x; x++)
      for(y = pos.y; y < pos.y + size.y; y++) {
        ResetRail(&train_line->rails[x][y], NoRail);
        DrawRail(train_line, Pos(x,y));
      }
  } else
    {
      tRailPos bitmap_pos;
      if(IsCovered(rail, &bitmap_pos))
        ClearRail(train_line, bitmap_pos);
      else 
        ResetRail(rail, NoRail);
    }
}


void SetRailType(
      tTrainLine train_line,
      tRailPos pos,
      tRailType new) {
  tRail *rail;

  rail = &train_line->rails[pos.x][pos.y];
  if(rail->custom)
    ClearRail(train_line, pos);
  /* Sort out illegal combinations... */
  if(IsStopRail(new) || 
     new == NoRail ||
     IsStop(rail) ||
     IsEmptyRail(rail) ||
     IsCompositeRail(rail))
    ResetRail(rail, new);
  else
    {
      tRailType current = rail->type[LeftSwitch];
      if((current == UpLeftRail && new == DownRightRail) ||
         (current == UpRightRail && new == DownLeftRail) ||
         (current == DownLeftRail && new == UpRightRail) ||
         (current == DownRightRail && new == UpLeftRail) ||
         (current == new))
        ResetRail(rail, new);
      else {
        /* We have a legal combination! */
        rail->type[RightSwitch] = new;
        if(!IsCrossing(rail))
          SortRailTypes(rail);
      }
    }
  DrawRail(train_line, pos);
}

void SetRailBitmap(
      tTrainLine train_line,
      tRailPos pos,
      tBitmapEntry *bitmap_entry) {
  tRail *rail;
  tRailPos size;
  rail = &train_line->rails[pos.x][pos.y];
  size = BitmapSize(bitmap_entry);

  if(pos.x + size.x <= train_line->space.x &&
     pos.y + size.y <= train_line->space.y) {
    {
      int x,y;
      for(x = 0; x < size.x; x++)
        for(y = 0; y < size.y; y++) {
          ClearRail(train_line, Pos(pos.x + x, pos.y + y));
          if(x || y) {
            tRail *covered_rail = &train_line->rails[pos.x + x][pos.y + y];
            covered_rail->custom = True;
            covered_rail->bitmap_entry = NULL;
            covered_rail->bitmap_pos = pos;
          }
        }
    }
    rail->custom = True;
    rail->bitmap_entry = bitmap_entry;
    DrawRail(train_line, pos);
  }
}

tRailType RailType(
      tTrainLine train_line,
      tRailPos pos,
      tSwitchState sw) {
  return(train_line->rails[pos.x][pos.y].type[sw]);
}

void SetSensorState(
      tTrainLine train_line,
      tRailPos pos,
      tSensorState state) {
  tRail *rail = &train_line->rails[pos.x][pos.y];

  /* logic problem ??? */
  if(state == NoSensor || (!IsEmptyRail(rail) && !IsCompositeRail(rail) &&
     !IsStop(rail))) {
    rail->sensor = state;
    DrawRail(train_line, pos);
  }
}

tSensorState SensorState(
      tTrainLine train_line,
      tRailPos pos) {
  return(train_line->rails[pos.x][pos.y].sensor);
}

void SetSemaphore(
      tTrainLine train_line,
      tRailPos pos,
      tSemaphoreState state) {
  train_line->rails[pos.x][pos.y].semaphore = state;
  DrawRail(train_line, pos);
}

tSemaphoreState SemaphoreState(
      tTrainLine train_line,
      tRailPos pos) {
  return(train_line->rails[pos.x][pos.y].semaphore);
}

tStatus SetSwitch(
      tTrainLine train_line,
      tRailPos pos,
      tSwitchState state) {
  tRail *rail;
  tTrainNo train_no;
  tStatus status;

  /* double (( )) since it is an assignment within the if */
  if((status = CheckRailPos(train_line, pos))) return status;
  rail = &train_line->rails[pos.x][pos.y];
  if(CarPresent(train_line, WidgetPos(pos), NULL, &train_no)) {
    return(IllegalSwitchingStatus);
  }  else 
    if(IsSwitchRail(rail)) {
      train_line->rails[pos.x][pos.y].sw = state;
      DrawRail(train_line, pos);
    } else
      return(NoSwitchStatus);
  return(SuccessStatus);
}

tSwitchState SwitchState(
      tTrainLine train_line,
      tRailPos pos) {
  return(train_line->rails[pos.x][pos.y].sw);
}

static tStatus Train(train_line, train_no, train)
      tTrainLine train_line;
      tTrainNo train_no;
      tTrain **train;
{
  for(*train = train_line->trains;
      *train && (*train)->no != train_no; *train = (*train)->next);
  if(*train) 
    return(SuccessStatus);
  else
    return(IllegalTrainNoStatus);
}

tStatus DeleteTrain(
      tTrainLine train_line,
      tTrainNo train_no) {
  tTrain *train;
  tTrainPart *car;
  tTrainPart *old_car;
  tStatus status;
  
  if((status = Train(train_line, train_no, &train))) return status;
  car = train->parts;
  while(car) {
    XtDestroyWidget(car->sprite);
    old_car = car;
    car = car->next;
    free(old_car);
  }
  if(train_line->trains == train)
    train_line->trains = train_line->trains->next;
  else {
    tTrain *prev_train;
    for(prev_train = train_line->trains; prev_train->next != train;
        prev_train = prev_train->next) ;
    prev_train->next = train->next;
  }
  free(train);
  return(SuccessStatus);
}

tEvent CreateTrain(
      tTrainLine train_line,
      tRailPos pos,
      tDirection direction,
      Cardinal no_of_cars,
      tTrainNo *train_no ) {
  tTrain *train;

  train = NEW(tTrain);
  train->next = NULL;

  /* Add after last train */
  if(train_line->trains) {
    tTrain *last_train;
    for(last_train = train_line->trains; last_train->next;
        last_train = last_train->next) ;
    last_train->next = train;
  } else
    train_line->trains = train;

  *train_no = train->no = train_line->free_no++;
  train->offset = 0;
  train->wanted_speed = train->speed = train->offset_frac = 0.0;
  train->parts = NULL;
  {
    tTrainPart *car, *previous_car;
    Cardinal i;
    eCarType car_type;
    tRailOffset dummy;
    tEvent event;
    tDirection dummy_dir;
    tRailPos dummy_pos;

    if((event = MovePos(train_line, -RailWidth, 0, direction, pos, 
                         &dummy, &dummy_dir, &dummy_pos))) {
      DeleteTrain(train_line, *train_no);
      return(event);
    }
    for(i = 1; i <= no_of_cars; i++) {
      car = NEW(tTrainPart);
      if(i==1) {
        train->parts = car;
        car_type = Engine;
      } else {
        previous_car->next = car;
        car_type = Car;
      }
      car->pos = pos;
      car->widget_pos = WidgetPos(pos);
      car->direction = direction;
      car->type = car_type;
      car->sprite = NewSprite(train_line, 
                              AdjustCarPos(direction, car->widget_pos), 
                              CarBitmap[car_type][direction]);

      car->next = NULL;
      if(i != no_of_cars &&
         (event = MovePos(train_line, -RailWidth, 0, direction, pos,
                          &dummy, &direction, &pos))) {
        DeleteTrain(train_line, *train_no);
        return(event);
      }
      previous_car = car;
    }
  }
  return(NoEvent);
}

/*
  Sets the wanted speed of the train. Maybe plays a sound.
*/
tStatus SetSpeed(
      tTrainLine train_line,
      tTrainNo train_no,
      tSpeed wanted_speed) {
  tTrain *train;
  tStatus status;

  if( (status = Train(train_line, train_no, &train)) ){
    return status;
  }

  if( train->speed * wanted_speed < 0.0 )
    return IllegalReverseStatus;

  if( train->speed == 0.0 && wanted_speed != 0.0 )
    SoundTrain(train_line);

  if( fabs(wanted_speed) >= 50.0 )
    SoundFastTrain(train_line);

  train->wanted_speed = wanted_speed;

  return SuccessStatus;
}

/* distance < 0 for reverse */
tEvent MoveTrain(
      tTrainLine train_line,
      tTrainNo train_no,
      tTrainDisplacement distance) {
  tTrain *train;
  tStatus status;
  tRailOffset new_offset;
  tTrainPart *car;
  tDirection new_direction;
  tTrainNo dummy_train_no;
  Bool reverse = distance < 0;

  if(distance != 0) {
    if((status = Train(train_line, train_no, &train))) {
      fprintf(stderr, "MoveTrain error: %s\n", StatusToString[status]);
      return(NoEvent);
    }
    for(car = train->parts; car; car = car->next) {
      tRailPos old_pos;
      old_pos = car->pos;
      if((status = MovePos(train_line, distance,
                          train->offset, car->direction, car->pos,
                           &new_offset, &new_direction, &car->pos)))
        return(status);
      car->widget_pos = 
        AddPos(WidgetPos(car->pos), new_direction, new_offset);
      SetSpritePos(car->sprite, 
                   AdjustCarPos(new_direction, car->widget_pos));
      if(new_direction != car->direction) {
        car->direction = new_direction;
        SetSpritePixmap(car->sprite, CarBitmap[car->type][new_direction]);
      }
      /* check sensors... */
      if(car->pos.x != old_pos.x || car->pos.y != old_pos.y) {
#define enter(pos) SendSensorActive(train_line, train_no, (pos), True)
#define leave(pos) SendSensorActive(train_line, train_no, (pos), False)
        if(car == train->parts) { /* engine */
          if(reverse && SensorState(train_line, old_pos) == Sensor)
            leave(old_pos);
          else 
            if(!reverse && SensorState(train_line, car->pos) == Sensor)
              enter(car->pos);
        }
        if(!car->next) { /* last car */
          if(reverse && SensorState(train_line, car->pos) == Sensor)
            enter(car->pos);
          else
            if(!reverse && SensorState(train_line, old_pos) == Sensor)
              leave(old_pos);
        }
      }
    }
    train->offset = new_offset;
    /* check for collision... */
    {
      tTrainPart *first_car = train->parts;
      
      if(reverse) /* find last car */
        while(first_car->next) first_car = first_car->next;
      if(CarPresent(train_line, first_car->widget_pos, first_car,
                    &dummy_train_no)) {
        SoundTrainCrash(train_line);
        return(TrainCollisionEvent);
      }
    }
  }
  return(NoEvent);
}
