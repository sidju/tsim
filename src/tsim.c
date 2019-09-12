/* tsim magnus@cs.chalmers.se 1990 */
/* tsim for Linux, algo@chl.chalmers.se 2000 */
/* tsim updates, staffan.bjornesjo 2008 */

#include <stdio.h>
#include <stdlib.h>   /* exit */
#include <time.h>
#include <sys/param.h>   /* PATH_MAX, MAX, MIN */
#include <unistd.h>      /* getcwd */

#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Cardinals.h>

#include "sound.h"
#include "general.h"
#include "tsim_argp.h"
#include "Sprite.h"
#include "train.h"
#include "file.h"
#include "resources.h"
#include "DrawingPalette.h"
#include "pipe.h"
#include "tsim_argp.h"

static String fallback_resources[] = {
  "tsim*illegalNumber.dialog.label: Illegal number or direction missing!",
  "tsim*cannotCreateTrain.dialog.label: Cannot create train!",
  "tsim*noOfCarsAndDirection.dialog.label: Enter number of cars and direction",
  "tsim*noOfCarsAndDirection.dialog.value: 1 Left",
  "tsim*cannotSave.dialog.label: Cannot save train line!",
  "tsim*save.dialog.label: Save train line in:",
  "tsim*save.dialog.value: ",
  "tsim*speed.dialog.label: Enter speed",
  "tsim*speed.dialog.value: 20",
  "tsim*illegalSpeed.dialog.label: Illegal speed!",
  "tsim*bulldozerButton.label: Delete",
  "tsim*speedButton.label: Speed",
  "tsim*switchButton.label: Switch",
  "tsim*quit.label: Quit",
  "tsim*save.label: Save",
  "tsim*tools.geometry: 467x57",
  "tsim*coordinates.label: v 0.84", /* version */
  NULL
  };

static void CheckIfRemoteHost(
      Widget w) {
  char display_host[100];
  char *program_host;
  char *display_name;

  if( ! getSoundState() )return;  /* no purpose of checking if audio is disabled */

  display_name = DisplayString(XtDisplay(w));
  strcpy(display_host, display_name);

  {
    char *cl = strstr(display_host,":");
    if(cl) *cl = '\0';
  }
/*  errprintf("display_host=%s", display_host); */

  program_host = getenv("HOST");
  if( strcmp(display_host,"") == 0 )return;
  if( strcmp(display_host,"localhost") == 0 )return;
  if( strcmp(program_host, display_host) && tsa_audio() ){
    setSoundState(0);
    if( tsa_verbose() )errputs("possibly running on remote host. disabling sound.");
  }
}

int main(
      int argc,
      char **argv ){
  Widget toplevel;
  XtAppContext apc;
  tTrainLine tl;

  char* map = parse(argc,argv);

  #ifdef SOLARIS
    srand((int) time ((time_t *) 0));
  #else
    /* srand is available in Linux, but srandom is better */
    srandom ((int) time (NULL));
  #endif

    toplevel = XtAppInitialize(&apc, "Tsim", NULL, 0, &argc, argv,
                               fallback_resources, NULL, 0);
  #if 0
      (void)XSynchronize(XtDisplay(toplevel),1);
  #endif

  GetResources(toplevel);
  if ( map ) {
    if(!(tl = LoadTrainLine(map, toplevel)))
      exit(1);
  } else { /* if no map, create new with size according to width&height args */
    tl = CreateTrainLine(toplevel, Pos(tsa_width(),tsa_height()));
  }
  /* t1 has been set for all paths */

  AddPipe(apc, tl);
  CreateDrawingPalette("tools", toplevel, tl);
  XtRealizeWidget (toplevel);

  CheckIfRemoteHost(toplevel);

  XtAppMainLoop (apc);

  return EXIT_SUCCESS;
}

