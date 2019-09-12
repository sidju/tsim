#include <stdio.h>
#include <unistd.h>       /* close */  /* use close or fclose ?! */
#include <strings.h>      /* strcasecmp */

#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include "train.h"
#include "bitmap.h"
#include "file.h"

#define HEADER "TrainLineFile"  /* file header */
#define VERSION 2       /* file format version */
#define RAILCHAR 'R'    /* rail spec */
#define TRAINCHAR 'T'   /* train spec */
#define ENDCHAR '.'     /* termination of trainline description */
#define CUSTOM "Custom"

String SwitchStateToString[] = {
  "LeftSwitch",
  "RightSwitch" };

String DirectionToString[] = {
  "NoDir",
  "Up",
  "Right",
  "Down",
  "Left" };

String RailTypeToString[] = {
  "NoRail",
  "HorizontalRail",
  "VerticalRail", 
  "UpLeftRail",
  "UpRightRail",
  "DownLeftRail",
  "DownRightRail",
  "UpStopRail",
  "RightStopRail",
  "DownStopRail",
  "LeftStopRail" };

String SensorStateToString[] = {
  "NoSensor",
  "Sensor" };

String StatusToString[] = {
  "SuccessStatus",
  "NoSwitchStatus",
  "IllegalReverseStatus",
  "IllegalTrainNoStatus",
  "IllegalRailPosStatus",
  "IllegalSwitchingStatus" };

String EventToString[] = {
  "NoEvent",
  "TrainCollisionEvent",
  "StopCollisionEvent",
  "DerailmentEvent" };


int StringToInt(
     String s,
     String intToString[],
     int lower,
     int upper) {
  int d;
  for(d = lower; d <= upper; d++) {
    if(s && !(strcasecmp(s, intToString[d])))
       return(d);
  }
  fprintf(stderr, "Warning: Cannot recognize \"%s\"\n", s);
  return(lower);
}

static void fprintPos(outfile, pos)
     FILE *outfile;
     tPos pos;
{
  fprintf(outfile, "%d %d", pos.x, pos.y);
}

static tPos fscanPos(infile)
     FILE *infile;
{
  tPos pos;
  fscanf(infile, "%d %d", &pos.x, &pos.y);
  return(pos);
}

static void fprintRail(outfile, rail, x, y)
     FILE *outfile;
     tRail *rail;
     int x,y;
{
  fprintf(outfile, "%c %d %d ", 
          RAILCHAR, x, y);
  if(rail->type[LeftSwitch] != NoRail) {
    int no_of_rails;

    if(rail->type[RightSwitch] == NoRail)
      no_of_rails = 1;
    else
      no_of_rails = 2;
    fprintf(outfile, "%d %s ", no_of_rails,
            RailTypeToString[rail->type[LeftSwitch]]);
    if(no_of_rails == 2)
      fprintf(outfile, "%s ", RailTypeToString[rail->type[RightSwitch]]);
    fprintf(outfile, "%s\n", SensorStateToString[rail->sensor]);
  } else /* custom! */
    fprintf(outfile, "%s %s\n", CUSTOM, rail->bitmap_entry->name);
}

static void fscanRail(infile, train_line)
     FILE *infile;
     tTrainLine train_line;
{
  char token[80], railchar[2];
  tPos pos;

  fscanf(infile, "%1s", railchar);
  pos = fscanPos(infile);
  fscanf(infile, "%s", token);
  if(!(strcasecmp(token, CUSTOM))) {
    tBitmapEntry *bitmap_entry;
    fscanf(infile, "%s", token);
    FindCustomBitmap(token, &bitmap_entry);
    if(bitmap_entry)
      SetRailBitmap(train_line, pos, bitmap_entry);
  } else {
    int i, no_of_rails;
    sscanf(token, "%d", &no_of_rails);
    for(i = 1; i <= no_of_rails; i++) {
      fscanf(infile, "%s", token);
      SetRailType(train_line, pos, StringToRailType(token));
    }
    fscanf(infile, "%s", token);
    SetSensorState(train_line, pos, StringToSensorState(token));
  }
}

static void fprintTrain(outfile, train)
     FILE *outfile;
     tTrain *train;
{
  int no_of_cars;
  tTrainPart *car, *engine;

  no_of_cars = 0;
  for(car = train->parts; car; car = car->next)
    no_of_cars++;
  engine = train->parts;
  fprintf(outfile, "%c %d %d %d %s\n",
          TRAINCHAR,
          engine->pos.x, engine->pos.y,
          no_of_cars,
          DirectionToString[engine->direction]);
}

static void fscanTrain(infile, train_line)
     FILE *infile;
     tTrainLine train_line;
{
  char token[80], trainchar[2];
  tTrainNo train_no;
  int x,y, no_of_cars;
  tDirection direction;

  fscanf(infile, "%1s %d %d %d", trainchar, &x, &y, &no_of_cars);
  fscanf(infile, "%s", token);
  direction = StringToDirection(token);
  CreateTrain(train_line, Pos(x, y), direction, no_of_cars, &train_no);
}
          
Boolean SaveTrainLine(
     String filename,
     tTrainLine train_line) {
  FILE *outfile;
  
  if(!(outfile = fopen(filename, "w"))) {
    fprintf(stderr, "Error creating file \"%s\"\n", filename);
    return(True);
  }
  fprintf(outfile, "%s %d\n", HEADER, VERSION);
  fprintPos(outfile, train_line->space);
  fprintf(outfile,"\n");
  {
    int x,y;

    for(x = 0; x < train_line->space.x; x++)
      for(y = 0; y < train_line->space.y; y++) {
        tRail *rail = &train_line->rails[x][y];
        if(rail->type[LeftSwitch] != NoRail || IsCustom(rail))
          fprintRail(outfile, rail, x, y);
      }
  }
  {
    tTrain *train;

    for(train = train_line->trains; train; train = train->next)
      fprintTrain(outfile, train);
  }
  fprintf(outfile, "%c\n", ENDCHAR);
  fclose(outfile);
  train_line->file_name = filename;
  return(False);
}

tTrainLine LoadTrainLine(
     String filename,
     Widget parent) {
  FILE *infile;
  char token[80], lookahead[2];
  tTrainLine train_line;
  int version;

  /* Since infile is opened with 'fopen' here, I believe it should be closed
     with 'fclose' instead of 'close'. Changed 5 times. alGo-000830 */
  if(!(infile = fopen(filename, "r"))) {
    fprintf(stderr, "Error reading file \"%s\"\n", filename);
    return(NULL);
  }
  fscanf(infile, "%s", token);
  if(strcasecmp(token, HEADER)) {
    fprintf(stderr, "Illegal header: \"%s\" in \"%s\"\n", token, filename);
    fclose(infile);
    return(NULL);
  }
  fscanf(infile, "%d", &version);
  if(version > VERSION) {
    fprintf(stderr, "Illegal version: \"%d\" in \"%s\"\n", version, filename);
    fclose(infile);
    return(NULL);
  }
  if(!(train_line = CreateTrainLine(parent, fscanPos(infile)))) {
    fprintf(stderr, "Error creating train line\n");
    fclose(infile);
    DeleteTrainLine(train_line);
    return(NULL);
  }
  while(True) {
    fscanf(infile, "%1s", lookahead);
    switch(lookahead[0]) {
      case RAILCHAR:
        ungetc(RAILCHAR, infile);
        fscanRail(infile, train_line);
        break;
      case TRAINCHAR:
        ungetc(TRAINCHAR, infile);
        fscanTrain(infile, train_line);
        break;
      case ENDCHAR:
        fclose(infile);
        train_line->file_name = filename;
        return(train_line);
      default:
        fprintf(stderr, "Unexpected character '%s' in \"%s\"\n", lookahead,
                filename);
        DeleteTrainLine(train_line);
        fclose(infile);
        return(NULL);
    }
  }
}
