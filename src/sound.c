#include <sys/types.h>   /* pid_t */
#include <unistd.h>      /* fork */ 
#ifdef SOLARIS
#include <string.h>
#else
#include <strings.h>
#endif
#include <sys/wait.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#include "general.h"
#include "tsim_argp.h"
#include "train.h"
#include "resources.h"
#include "sound.h"

/* LINUX defines random in <stdlib.h> as: long int random(void); */
#ifdef SOLARIS
extern long random();
#endif

extern int bingo;
int bingo;

/*******************************************************************************
  local
*******************************************************************************/

typedef struct {
  char *fname;
  double p;
} SoundTable;

/* sound categories with their probability distribution over
   the possible sounds in that category. */
static SoundTable startsound[] = {
  {"train.ul", 1.0}
};
static SoundTable faststartsound[] = {
  {"ftrain.ul", 1.0}
};
static SoundTable traincrashsound[] = { 
  {"ayacrash.ul", 0.5},
  {"boom_oh.ul", 0.5}
};
static SoundTable derailcrashsound[] = { 
  {"dummare.ul", 1.0}
};
static SoundTable stopcrashsound[] = { 
  {"gameover.ul", 1.0}
};


/* global variable that keeps track of if audio is enabled or disabled.
   can be set via setSoundState() */
static int sound_state;


/*
  Plays a sound file.
*/
static void play(
      char *sound) {

  #define PATH_SIZE 200
  #define PLAY_CMD_SIZE (PATH_SIZE + 8)
  char path[PATH_SIZE];
  char playc[PLAY_CMD_SIZE];

/* Give peace to defunct processes */
/*
#ifndef SOLARIS
  while(wait3(NULL, WNOHANG, NULL) > 0);
#endif
*/

  switch ( wait3(NULL, WNOHANG, NULL) ){
    case -1: /* no child processes, or some error.. so try playing */
      break;
    case 0: /* sound is currently playing, so skip playing new sound */
      return;
    default: /* last played sound has ended, so play now */
      break;
  }

/*  fprintf(stderr, "<playing>\n"); */

  if(!fork()) {  /* run in child thread */
    /* By closing stdin and stdout we will not interfere with the parent
       process.
       We can call system() or fork() another thread, and it has no effect. */
    fclose(stdin);
    fclose(stdout);
    #ifndef DEBUG
      fclose(stderr);
      /* Let stderr be available so that errors can be shown by calls to
         system() or any library used to play the sound */
    #endif

    snprintf(path, PATH_SIZE, "%s/%s/%s", app_dir(), sound_dir(), sound);
/*    snprintf(playc, PLAY_CMD_SIZE, play_cmd(), path); */
    snprintf(playc, PLAY_CMD_SIZE, "play \"%s\"", path);

    /* Find out if we are running on host */
/*    if(host && strlen(host) && strcmp(getenv("HOST"), host))
      sprintf(command,"rsh %s -n '(%s)' >&- 2>&-",host,playc);
    else
      sprintf(command,"csh -c '(%s)' >&- 2>&-",playc);
*/

/*    f = fopen("sound_cmd.txt", "w");
    fprintf(f, "%s\n", playc);
    fclose(f); */

    if( system(playc) ) {
      if( tsa_verbose() )
        errputs("failed to play audio. disabling audio..");
      setSoundState(0);
    }
    exit(0); /* terminate child thread */
  }
}


static void Sound(
      tTrainLine train_line,
      char *sound) {

  // suppress compiler warning about unused callback parameter.
  (void)train_line;

  if( getSoundState() )play(sound);
}

/*
  given a SoundTable, picks a sound randomly and plays it.
*/
static void Sounds(
      tTrainLine train_line,
      SoundTable table[],
      int n) {
  int i;

  /* randomize which sound to play */
#define DIGS 0xffffff
#ifdef SOLARIS
  double s = rand() % DIGS * 1.0 /DIGS;
#else
  double s = random() % DIGS * 1.0 / DIGS;
#endif

/*  printf("#Sounds(n=%d)\n", n); */

  /* go through all possible sounds */
  for(i=0; i<n; i++) {
    /* the probabilities sum to one, so this will randomly pick a sound to
       play depending on the probability distribution */
/*    printf("s%d:%f\n", i, table[i].p); */
    if(s < table[i].p) {
      Sound(train_line,table[i].fname);
      return;
    }
    s -= table[i].p;
  }
}

#define SOUNDS(n,t) void n(train_line) \
   tTrainLine train_line; \
{ \
  Sounds(train_line,(t),XtNumber(t)); \
}

/*******************************************************************************
  exported
*******************************************************************************/

/* functions for playing a certain sound */
SOUNDS(SoundTrain,startsound)
SOUNDS(SoundFastTrain,faststartsound)
SOUNDS(SoundTrainCrash,traincrashsound)
SOUNDS(SoundDerailCrash,derailcrashsound)
SOUNDS(SoundStopCrash,stopcrashsound)

/* used to enable/disable sound */
void setSoundState(int state){
  sound_state = state;
}
int getSoundState(){
  return sound_state;
}

