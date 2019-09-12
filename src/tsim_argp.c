#include <argp.h>
#include <stdlib.h>
#include <unistd.h>  /* nice */
#include <limits.h>

#include "tsim_argp.h"
#include "sound.h"

/*******************************************************************************
  local
*******************************************************************************/

const char *argp_program_version = "Train Simulator, version 0.84";
const char *argp_program_bug_address = "<staffan.bjornesjo@gmail.com>";

/* Program documentation. */
static char doc[] = "Train Simulator";

/* A description of the arguments we accept. */
static char args_doc[] = "mapfile";

/* The options we understand. */
static struct argp_option options[] = {
  {"verbose", 'v', 0, 0, "Show more output on stderr.", 6},
  {"quiet",   'q', 0, 0, "Disable sound effects.", 5},
  {"speed",   's', "integer", 0,
   "Simulator speed. Set the delay between state updates in the simulator "
   "(in milliseconds). Default: 100.", 1},
  {"priority", 'p', "integer", 0,
   "Explicitly sets the process priority instead of getting the default. "
   "This is the nice value of the process. It ranges from 19 (nicest) "
   "to -20 (not nice to other processes), see nice(1).", 2},
  {"width", 'w', "integer", 0,
   "Sets the width of the new map created when no mapfile is specififed.", 3},
  {"height", 'h', "integer", 0,
   "Sets the height of the new map created when no mapfile is specififed.", 4},
  { 0, 0, 0, 0, 0, 0 }
};

/* contains result of argument parsing */
typedef struct _tArguments {
  char *map;     /* optional map filename */
  int verbose;
  int audio;     /* if audio enabled */
  int speed;     /* delay between state updates */
  int priority;  /* process nice value */
  int width;     /* width of new map */
  int height;    /* height of new map */
} tArguments;

/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state) {
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  tArguments *arguments = state->input;

  switch (key)
  {
    case 'q':
      arguments->audio = 0;
      break;
    case 'v':
      arguments->verbose = 1;
      break;
    case 'h':{
      int n;
      if( sscanf(arg, "%d", &n) != 1 ){
        fprintf(stderr, "Height option requires an integer.\n");
        argp_usage (state);
        return 1;
      }
      if( n <= 0 ){
        fprintf(stderr, "Height option requires a positive integer.\n");
        argp_usage (state);
        return 1;
      }
      arguments->height = n;
      break;
    }
    case 'w':{
      int n;
      if( sscanf(arg, "%d", &n) != 1 ){
        fprintf(stderr, "Width option requires an integer.\n");
        argp_usage (state);
        return 1;
      }
      if( n <= 0 ){
        fprintf(stderr, "Width option requires a positive integer.\n");
        argp_usage (state);
        return 1;
      }
      arguments->width = n;
      break;
    }
    case 's':{
      int n;
      if( sscanf(arg, "%d", &n) != 1 ){
        fprintf(stderr, "Speed option requires an integer.\n");
        argp_usage (state);
        return 1;
      }
      if( n < 0 || n > 100 ){
        fprintf(stderr, "Speed option requires an integer within [0..100].\n");
        argp_usage (state);
        return 1;
      }
      arguments->speed = n;
      break;
    }
    case 'p':{
      int n;
      if( sscanf(arg, "%d", &n) != 1 ){
        fprintf(stderr, "Priority option requires an integer.\n");
        argp_usage (state);
        return 1;
      }
      if( n < -20 || n > 19 ){
        fprintf(stderr, "Priority option requires an integer within [-20..19].\n");
        argp_usage (state);
        return 1;
      }
      arguments->priority = n;
      break;
    }
    case ARGP_KEY_ARG:
      if (state->arg_num > 0) {
        fprintf(stderr, "Too many non-option arguments.\n");
        argp_usage (state);
      } else {
/*        fprintf(stderr, "setting map file to: %s\n", arg); */
        arguments->map = arg;
      }
      break;

    case ARGP_KEY_END:
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

/* The argp parser */
static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };

/* these tests should perhaps be done in autoconf instead, but would then
   be static, and it would not be known if audio was enabled later on.
   how should it be done? */
static int audiotest(int want_audio, int verbose){
  /* disable sound if audio==0, /dev/audio is missing, or play missing */
  if( want_audio ) {
    if( system("bash -c \"ls /dev/audio > /dev/null 2>&1\"") ){
     if( verbose )
       fprintf(stderr, "audio error: /dev/audio missing.\n");
      return 0;
    }
    if( system("which play &> /dev/null") ){
      fprintf(stderr, "audio error: command 'play' missing. please install sox (Sound eXchange)\n");
      return 0;
    }
    if( verbose )fprintf(stderr, "audio enabled\n");
    return 1;
  }
  if( verbose )fprintf(stderr, "audio disabled\n");
  return 0;
}

static int setpriority_(int priority, int verbose) {
  int n;
  /* if priority option was not used, it is INT_MAX. */
  if( priority == INT_MAX ) {
    if( verbose )fprintf(stderr, "priority not set. leaving it unchanged.\n");
    return INT_MAX;
  }

  /* assume it is currently 0, and set nice value increment */
  n = nice(priority);
  if( n != priority ){
    n = nice(priority-n);
    if( n != priority ){
      fprintf(stderr, "error: unable to correctly set nice value. it is now: %d\n", n);
    }
  } else if( verbose ){
    /* output that the priority was set correctly if verbose is on. */
    fprintf(stderr, "nice value set to %d\n", n);
  }
  return n;
}

/*******************************************************************************
  exported
*******************************************************************************/

int tsa_speed_value;
int tsa_speed(){
  return tsa_speed_value;
}
int tsa_audio_value;
int tsa_audio(){
  return tsa_audio_value;
}
int tsa_priority_value;
int tsa_priority(){
  return tsa_priority_value;
}
int tsa_verbose_value;
int tsa_verbose(){
  return tsa_verbose_value;
}
int tsa_height_value;
int tsa_height(){
  return tsa_height_value;
}
int tsa_width_value;
int tsa_width(){
  return tsa_width_value;
}

char* parse(
      int argc,
      char **argv) {
  tArguments *arguments = malloc(sizeof(tArguments));

/*  fprintf(stderr, "debug: ARGP_ERR_UNKNOWN: %d\n", ARGP_ERR_UNKNOWN); */

  /* Default values. */
  arguments->map = NULL;
  arguments->verbose = 0;
  arguments->audio = 1;
  arguments->speed = 100;
  arguments->priority = INT_MAX;
  arguments->width = 20;
  arguments->height = 20;

  argp_parse (&argp, argc, argv, 0, 0, arguments);

  arguments->audio = audiotest(arguments->audio, arguments->verbose);
  arguments->priority = setpriority_(arguments->priority, arguments->verbose);

  tsa_width_value = arguments->width;
  tsa_height_value = arguments->height;
  tsa_speed_value = arguments->speed;
  tsa_audio_value = arguments->audio;
  tsa_verbose_value = arguments->verbose;
  setSoundState(tsa_audio_value);

  if( arguments->verbose )fprintf(stderr, "verbose output enabled\n");
  if( arguments->verbose )fprintf(stderr, "simulator speed set to %d\n", arguments->speed);

  return arguments->map;
}

