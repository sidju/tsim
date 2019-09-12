
#ifndef TS_file_H_
#define TS_file_H_

#include "train.h"

#define StringToEnum(s, type, tostring, lower, upper) \
  (type)StringToInt((s), tostring, lower, upper)
#define StringToSwitchState(s) \
  StringToEnum((s), tSwitchState, SwitchStateToString, LeftSwitch, RightSwitch)
#define StringToRailType(s) \
  StringToEnum((s), tRailType, RailTypeToString, NoRail, LeftStopRail)
#define StringToDirection(s) \
  StringToEnum((s), tDirection, DirectionToString, NoDir, Left)
#define StringToSensorState(s) \
  StringToEnum((s), tSensorState, SensorStateToString, NoSensor, \
	       Sensor)

extern String SwitchStateToString[];
extern String DirectionToString[];
extern String RailTypeToString[];
extern String SensorStateToString[];
extern String StatusToString[];
extern String EventToString[];

int StringToInt(
     String s,
     String intToString[],
     int lower,
     int upper);

tTrainLine LoadTrainLine(
     String filename,
     Widget parent);

Boolean SaveTrainLine(
     String filename,
     tTrainLine train_line);

#endif /* TS_file_H_ */

