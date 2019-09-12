
#ifndef TS_train_H_
#define TS_train_H_

#include <stdio.h>            /* FILE */
#include "BitmapDirectory.h"

#define TRAINLINENAME "trainLine"
#define BACKGROUNDNAME "railBackground"
#define CARNAME "car"

/* do not re-order these */
typedef enum {
  NoRail,
  HorizontalRail,
  VerticalRail, 
  UpLeftRail,
  UpRightRail,
  DownLeftRail,
  DownRightRail,
  UpStopRail,
  RightStopRail,
  DownStopRail,
  LeftStopRail
} tRailType;

typedef struct { 
  int x,y;
} tPos;

typedef tPos tRailPos;

typedef enum {
  NoDir,
  Up,
  Right,
  Down,
  Left 
} tDirection;

typedef struct _tTrainLine *tTrainLine;
typedef int tTrainNo;

typedef enum {
  NoSensor,
  Sensor
} tSensorState;

typedef enum {
  NoSemaphore,
  GoSemaphore,
  StopSemaphore
} tSemaphoreState;

typedef enum {
  LeftSwitch,
  RightSwitch
} tSwitchState;

typedef enum {
  SuccessStatus,        /* 0 */
  NoSwitchStatus,
  IllegalReverseStatus,
  IllegalTrainNoStatus,
  IllegalRailPosStatus,
  IllegalSwitchingStatus
} tStatus;

typedef enum {
  NoEvent,
  TrainCollisionEvent,
  StopCollisionEvent,
  DerailmentEvent
} tEvent;

/******************************************************************************/

typedef enum {
  NoCar,
  Car,
  Engine
} eCarType;
#define eCarType_min NoCar
#define eCarType_max Engine
#define eCarType_size (Engine+1)

#define MAXRAILS 200

typedef struct {
  tRailType type[RightSwitch+1];
  tSensorState sensor;
  tSwitchState sw;
  tSemaphoreState semaphore;
  Boolean custom;
  tBitmapEntry *bitmap_entry;
  tRailPos bitmap_pos;
} tRail;

typedef struct _tTrainPart {
  Widget sprite;
  tRailPos pos;
  tPos widget_pos;
  tDirection direction;
  eCarType type;
  struct _tTrainPart *next;
} tTrainPart;

typedef int tRailOffset;

typedef float tSpeed;


typedef struct _tTrain {
  tTrainNo no;
  tRailOffset offset;
  tSpeed speed, wanted_speed;
  float offset_frac;
  tTrainPart *parts;
  struct _tTrain *next;
} tTrain;

typedef enum {
  ForegroundBitmap,
  MaskBitmap } tBitmapType;

typedef tRail tRailArray[MAXRAILS][MAXRAILS];

typedef struct _tTrainLine {
  Widget top;
  tRailPos space;
  Widget back_widget;
  tRailArray rails;
  tTrain *trains;
  tTrainNo free_no;
  String file_name;    /* the file from which the description was read */
  Boolean use_pipes;
  FILE *out_pipe;
} sTrainLine;

extern unsigned int RailWidth;
extern int WheelOffset;
extern Pixmap 
  CarBitmap[eCarType_size][Left+1][MaskBitmap+1],
  RailBitmap[LeftStopRail+1],
  SwitchBitmap[Left+1][RightSwitch+1],
  SensorBitmap[LeftStopRail+1];

/******************************************************************************/

typedef int tTrainDisplacement;

tPos WidgetPos(tRailPos rail_pos);
tRailPos RailPos(tPos widget_pos);
tTrainLine CreateTrainLine(Widget parent, tRailPos space);
void DeleteTrainLine(tTrainLine train_line);
tRailPos Space(tTrainLine train_line);
tStatus SetSpeed(
      tTrainLine train_line,
      tTrainNo train_no,
      tSpeed wanted_speed);
void SetRailType(
      tTrainLine train_line,
      tRailPos pos,
      tRailType new);
void SetRailBitmap(
      tTrainLine train_line,
      tRailPos pos,
      tBitmapEntry *bitmap_entry);
tRailType RailType(
      tTrainLine train_line,
      tRailPos pos,
      tSwitchState sw);
void SetSensorState(
      tTrainLine train_line,
      tRailPos pos,
      tSensorState state);
tSensorState SensorState(
      tTrainLine train_line,
      tRailPos pos);
void SetSemaphore(
      tTrainLine train_line,
      tRailPos pos,
      tSemaphoreState state);
tSemaphoreState SemaphoreState(
      tTrainLine train_line,
      tRailPos pos);
tStatus SetSwitch(
      tTrainLine train_line,
      tRailPos pos,
      tSwitchState state);
tSwitchState SwitchState(
      tTrainLine train_line,
      tRailPos pos);
tStatus DeleteTrain(
      tTrainLine train_line,
      tTrainNo train_no);
tEvent CreateTrain(
      tTrainLine train_line,
      tRailPos pos,
      tDirection direction,
      Cardinal no_of_cars,
      tTrainNo *train_no );
tEvent MoveTrain(
      tTrainLine train_line,
      tTrainNo train_no,
      tTrainDisplacement distance);

tPos Pos(int x, int y);

Boolean IsCustom(tRail *rail);

Boolean CarPresent(
      tTrainLine train_line,
      tPos pos,
      tTrainPart *not_this_car,
      tTrainNo *train);

#endif /* TS_train_H_ */

