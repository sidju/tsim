
#ifndef TS_BitmapDirectory_H_
#define TS_BitmapDirectory_H_

#include <X11/X.h>            /* Pixmap */
#include <X11/Intrinsic.h>    /* String, Widget, Boolean */

typedef struct {
  Pixmap bitmap;
  String name;
  unsigned int width, height;
} tBitmapEntry;

typedef struct sBitmapDirectory *pBitmapDirectory;
typedef struct sBitmapDirectory **ppBitmapDirectory;
typedef struct sBitmapDirectory {
  tBitmapEntry bitmap_entry;
  pBitmapDirectory next;
} tBitmapDirectory;


tBitmapDirectory *BDAddDirectory(
      Widget w,
      String directory,
      tBitmapDirectory *bd);

void BDFindBitmap(
      String name,
      tBitmapEntry **bitmap_entry,
      tBitmapDirectory *bd);

void BDBitmaps(
      tBitmapDirectory **entry,
      tBitmapEntry **bitmap_entry,
      tBitmapDirectory *bd);

#endif /* TS_BitmapDirectory_H_ */

