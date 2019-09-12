
#ifndef TS_tsim_argp_H_
#define TS_tsim_argp_H_

int tsa_speed(void);
int tsa_audio(void);
int tsa_priority(void);
int tsa_verbose(void);
int tsa_height(void);
int tsa_width(void);

char* parse(
      int argc,
      char **argv);

#endif /* TS_tsim_argp_H_ */

