#include <math.h>
#include <stdlib.h>
#include <sched.h>   /* sched_yield */
#include <limits.h>  /* INT_MAX */

#include <X11/Xlib.h>                        /* for Xlib definitions */
#include <X11/StringDefs.h>                /* for useful atom names */
#include <X11/Intrinsic.h>

#include "general.h"
#include "train.h"
#include "resources.h"
#include "pipe.h"
#include "tsim_argp.h"
#include "timer.h"

static XtIntervalId lastIntervalId;

static tSpeed fsign(tSpeed f) {
  if(f > 0.0 )return 1;
  else if(f < 0.0 )return -1;
  else return 0;
}

static void yield(){
  if( sched_yield() ){
    errputs("failed to yield");
    exit(1);
  }
}

static void MoveTrains(tTrainLine train_line, XtIntervalId *timer)
{
  tTrain *train;
  float speed_diff = timeout_interval() * acceleration();
  tEvent event;
  lastIntervalId = *timer;

  for(train = train_line->trains; train; train = train->next) {
    if(train->speed != train->wanted_speed) {
      tSpeed old_dir, new_dir;

      /* calculates if speed should increase or decrease.. */
      old_dir = fsign(train->wanted_speed - train->speed);

      train->speed += old_dir * speed_diff;
      /* if the speed is near wanted speed, then set wanted speed.
         this can cause a slightly faster acceleration in the last step
         though. however, the test needs more information since e.g the
         wanted_speed could be 0, and then it is unknown if the speed has
         passed the wanted speed or not, which it should never do. */
      new_dir = fsign(train->wanted_speed - train->speed);
      if( new_dir != old_dir ){
        train->speed = train->wanted_speed;
      }
/* the old code:
      if(fabs(train->speed - train->wanted_speed) < speed_diff)
        train->speed = train->wanted_speed;
*/
      if( train->speed == train->wanted_speed && tsa_verbose() ){
        errprintf("train %d has reached the wanted speed", train->no);
      }
    }
    train->offset_frac += timeout_interval() * fabs(train->speed);
    if(train->offset_frac >= 1.0) {
      int int_offset = train->offset_frac;
      train->offset_frac -= int_offset;
      event = MoveTrain(train_line, train->no,
                        fsign(train->speed) * int_offset);
      if( event ) {
        /* Collision, stop train! */
        train->speed = 0.0;
        train->wanted_speed = 0.0;
        SendTrainEvent(train_line, train->no, event);
      }
    }
  }
  AddTimeOut(train_line);

  /* both of these are default values. if any are not default, then run yield
     just to decrease the probability of problems */
  if( tsa_speed() != 100 || tsa_priority() != INT_MAX )yield();
}

/* adds a time-out for the next state update */
void AddTimeOut(tTrainLine train_line) {
  XtAppAddTimeOut(XtWidgetToApplicationContext(train_line->top), 
                  /* (unsigned long)(timeout_interval()* */ tsa_speed(),
                  (XtTimerCallbackProc)MoveTrains, (XtPointer)train_line);
}
