
#ifndef TS_sound_H_
#define TS_sound_H_

#include "train.h"

void SoundTrain (tTrainLine train_line);
void SoundFastTrain (tTrainLine train_line);
void SoundTrainCrash (tTrainLine train_line);
void SoundDerailCrash (tTrainLine train_line);
void SoundStopCrash (tTrainLine train_line);

void setSoundState(int state);
int getSoundState(void);
#endif /* TS_sound_H_ */

