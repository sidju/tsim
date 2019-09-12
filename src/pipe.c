#include <stdio.h>
#include <string.h>
#include <stdlib.h>      /* exit */
#include <unistd.h>      /* read */
#include <sys/param.h>    /* MIN, MAX */

#if defined sun3 || defined sun4
  #include <sys/filio.h>
#endif
#ifdef SOLARIS
  #include <sys/types.h>
  #include <stropts.h>
  #include <sys/conf.h>
#else
  #include <sys/ioctl.h>
#endif

#include <X11/Xlib.h>			/* for Xlib definitions */
#include <X11/StringDefs.h>		/* for useful atom names */
#include <X11/Intrinsic.h>

#include "general.h"
#include "errcheck.h"
#include "train.h"
#include "file.h"
#include "pipe.h"

#define SEPS " \t\n"
static String seps = SEPS;

/*******************************************************************************
  local
*******************************************************************************/

static void Acknowledge(
      tTrainLine train_line,
      tStatus status) {
  fprintf(train_line->out_pipe, "%s\n", StatusToString[status]);
}

static tStatus ParseFloat(
      float *f) {
  String value;
  if(!(sscanf(value = strtok(NULL, seps), "%f", f)))
    fprintf(stderr, "Warning: Illegal float value: \"%s\"\n", value);
  return(SuccessStatus);
}

static tStatus ParseInt(
      int *i) {
  String value = strtok(NULL, seps);
  if(!(value && sscanf(value, "%d", i)))
    fprintf(stderr, "Warning: Illegal integer value: \"%s\"\n", value);
  return(SuccessStatus);
}

static tStatus ParsePos(
      tRailPos *pos) {
  tStatus status;
  if((status = ParseInt(&pos->x))) return(status);
  if((status = ParseInt(&pos->y))) return(status);
  return(SuccessStatus);
}

static tStatus ParseSwitchState(
      tSwitchState *state) {
  *state = StringToSwitchState(strtok(NULL, seps));
  return(SuccessStatus);
}

static tStatus SetSwitchCmd(
      tTrainLine train_line,
      String command_line) {

  // suppress compiler warning about unused callback parameter.
  (void)command_line;

  tRailPos pos;
  tSwitchState state;
  tStatus status;

  if((status = ParsePos(&pos))) return(status);
  if((status = ParseSwitchState(&state))) return(status);
  return(SetSwitch(train_line, pos, state));
}

static tStatus SetSpeedCmd(
      tTrainLine train_line,
      String command_line) {

  // suppress compiler warning about unused callback parameter.
  (void)command_line;

  tTrainNo train_no;
  tSpeed speed;
  tStatus status;

  if((status = ParseInt(&train_no))) return(status);
  if((status = ParseFloat(&speed))) return(status);
  return(SetSpeed(train_line, train_no, speed));
}

static void Dispatch(
      tTrainLine train_line,
      String command_line) {
  typedef struct {
    String name;
    tStatus (*procedure)();
  } tCommandPair;
  static tCommandPair commands[] = {
    { "SetSwitch", SetSwitchCmd },
    { "SetSpeed", SetSpeedCmd } };
  static int no_of_commands = 2;
  String command;
  int cmdno;

  command = strtok(command_line, seps);  /* Remove the command */
  if(!command) return;
  for(cmdno = 0; cmdno < no_of_commands; cmdno++)
    if(!strcasecmp(commands[cmdno].name, command)) {
      Acknowledge(train_line,
		  commands[cmdno].procedure(train_line, command_line));
      return;
    }
  Acknowledge(train_line, SuccessStatus);
  fprintf(stderr, "Warning: Illegal command received: %s\n", command);
}

/*
  should be of type "XtInputCallbackProc", which has type:
*/
static void ReadHandler(
      XtPointer closure,
      int *infiledesc,         /* source */
      XtInputId *input_id) {   /* id */

  // suppress compiler warning about unused callback parameter.
  (void)input_id;

  tTrainLine train_line = (tTrainLine)closure;
#define BUFSIZE 512
  char buf[BUFSIZE];
  int no_of_bytes;

#ifdef SOLARIS
  errcheck(ioctl(*infiledesc, I_NREAD, &no_of_bytes), -1, "ioctl error");
#else
  errcheck(ioctl(*infiledesc, FIONREAD, &no_of_bytes), -1, "ioctl error");
#endif
  if(!no_of_bytes) /* Nothing to read - done! */
    exit(0);
  no_of_bytes = MIN(no_of_bytes, BUFSIZE - 1);
  read(*infiledesc, buf, no_of_bytes);
  buf[no_of_bytes] = '\0';

  /* Assume that a complete line has been received */
  Dispatch(train_line, buf); 
}

/*******************************************************************************
  exported
*******************************************************************************/

void SendSensorActive(
      tTrainLine train_line,
      tTrainNo tno,
      tRailPos pos,
      Bool active) {
  if(train_line->use_pipes) {
    fprintf(train_line->out_pipe, "Sensor %d %d %d ", tno, pos.x, pos.y);
    if(active)
      fprintf(train_line->out_pipe, "active\n");
    else
      fprintf(train_line->out_pipe, "inactive\n");
  }
}

void SendTrainEvent(
      tTrainLine train_line,
      tTrainNo tno,
      tEvent event) {
  if(train_line->use_pipes && event)
    fprintf(train_line->out_pipe, "%s %d\n", EventToString[event], tno);
}

void AddPipe(
      XtAppContext apc,
      tTrainLine train_line) {
  int infiledesc;
#define STDIN_DESC 0

/* Will read form stdin and write to stdout from no on... */
  train_line->out_pipe = stdout; /*(FILE*)errcheck((int)fopen(out_file, "r+"),
					 0, out_file);*/
  infiledesc = STDIN_DESC; /*errcheck(open(in_file, O_RDONLY), -1, in_file);*/
  setbuf(train_line->out_pipe, NULL);
  train_line->use_pipes = True;
  XtAppAddInput(apc, infiledesc, (XtPointer) XtInputReadMask, 
		ReadHandler, train_line);
}

