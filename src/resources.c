#include <stdio.h>
#include <stdlib.h>   /* char *getenv(const char *name); */

#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>

#include "resources.h"

typedef struct {
  String bitmap_dir;
  String sound_dir;
  float timeout_interval;
  float acceleration;
  String play_cmd;
} tData, *tData_ptr;

#define XtNbitmapDir "bitmapDir"
#define XtCBitmapDir "BitmapDir"
#define XtNsoundDir "soundDir"
#define XtCSoundDir "SoundDir"
#define XtNtimeoutInterval "timeoutInterval"
#define XtCTimeoutInterval "TimeoutInterval"
#define XtNacceleration "acceleration"
#define XtCAcceleration "Acceleration"
#define XtNplayCmd "playCmd"
#define XtCPlayCmd "PlayCmd"

/* Change these to point to your local bitmap and sound directories */
#define BITMAPDIR (RESDIR "/bitmaps")
#define SOUNDDIR  (RESDIR "/sounds")

#define offset(field) XtOffset(tData_ptr, field)

static XtResource resources[] = {
  { XtNbitmapDir, XtCBitmapDir, XtRString, sizeof(String), 
      offset(bitmap_dir), XtRString, BITMAPDIR },
  { XtNsoundDir, XtCSoundDir, XtRString, sizeof(String), 
      offset(sound_dir), XtRString, SOUNDDIR },
  { XtNtimeoutInterval, XtCTimeoutInterval, XtRFloat, sizeof(float),
      offset(timeout_interval), XtRString, "0.1" }, /* seconds */
  { XtNacceleration, XtCAcceleration, XtRFloat, sizeof(float),
      offset(acceleration), XtRString, "5.0", }, /* pixels / second**2 */
  { XtNplayCmd, XtCPlayCmd, XtRString, sizeof(String),
      offset(play_cmd), XtRString, "cat \"%s\" > /dev/audio", }, 
};

static tData data;

void GetResources(Widget toplevel) {
  XtGetApplicationResources(toplevel, &data, resources, XtNumber(resources),
			    NULL, 0);
}

/* set by calling initResourcePath from main when program starts up */
static char* appdir;
/* could, and perhaps should be deallocated when terminating the program.. */

String app_dir(){
  if( appdir == NULL ){
    /* find the installation location */
/*    fprintf(stderr, "{%s}\n", getenv("PATH")); */
    appdir = getenv("TSIM_HOME");
    /* if missing, use root '/' as installation directory */
    if( appdir == NULL ){
      appdir = malloc(1);
      appdir[0] = '\0';
    }
/*    fprintf(stderr, "[%s]\n", appdir); */
  }
  return appdir;
}

String bitmap_dir() {
  return data.bitmap_dir;
}

String sound_dir() {
  return data.sound_dir;
}

float timeout_interval() {
  return data.timeout_interval;
}

float acceleration() {
  return data.acceleration;
}

String play_cmd() {
  return data.play_cmd;
}

