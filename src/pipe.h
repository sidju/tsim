
#ifndef TS_pipe_H_
#define TS_pipe_H_

void AddPipe(
      XtAppContext apc,
      tTrainLine train_line);

void SendTrainEvent(
      tTrainLine train_line,
      tTrainNo tno,
      tEvent event);

void SendSensorActive(
      tTrainLine train_line,
      tTrainNo tno,
      tRailPos pos,
      Bool active);

#endif /* TS_pipe_H_ */

