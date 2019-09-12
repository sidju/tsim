
#ifndef TS_bitmap_H_
#define TS_bitmap_H_

#include "BitmapDirectory.h"

#define ReadCar(f,c,d,t) \
  ReadBitmap(parent, (f), &CarBitmap[c][d][t], &dummy4, &dummy5)
#define ReadRail(f,r,width,hot_y) \
  ReadBitmap(parent, (f), &RailBitmap[r], (width), hot_y)
#define ReadSemaphore(f,s,t) \
  ReadBitmap(parent, (f), &SemaphoreBitmap[s][t], &dummy4, &dummy5)
#define ReadSensor(f,r) \
  ReadBitmap(parent, (f), &SensorBitmap[r], &dummy4, &dummy5)
#define ReadSwitch(f,ce,d) \
  ReadBitmap(parent, (f), &SwitchBitmap[ce][d], &dummy4, &dummy5)

void ReadBitmap(
      Widget w,
      String name,
      Pixmap *dest,
      unsigned int *width_return,
      int *hot_y_return);

void WriteBitmap(
      Widget w,
      String name,
      Pixmap bitmap );

void CustomBitmaps(
      void **entry,
      tBitmapEntry **bitmap_entry);

void ReadCustomBitmaps(
     Widget w);

void FindCustomBitmap(
     String name,
     tBitmapEntry **bitmap_entry);

#endif /* TS_bitmap_H_ */

