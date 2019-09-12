
#ifndef TS_Sprite_H_
#define TS_Sprite_H_

/*
 * Sprite widget; window shape from one bitmap, content from another.
 */

/* resource names used by sprite widget that aren't defined in StringDefs.h */

/* Pixmap defining window's shape */
#define XtNwindowPixmapMask "windowPixmapMask"
#define XtNcontentPixmap "contentPixmap"

#define XtCPixmapMask "PixmapMask"

/* abstract structures */
typedef struct _SpriteRec *SpriteWidget;
typedef struct _SpriteClassRec *SpriteWidgetClass;

/* global */
extern WidgetClass spriteWidgetClass;

#endif /* TS_Sprite_H_ */

