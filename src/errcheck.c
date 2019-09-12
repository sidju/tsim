#include <stdio.h>
#include <stdlib.h>      /* for function "exit" */

#include "errcheck.h"

int errcheck(
      int i,
      int ec,
      char *s) {
  if(i != ec) return(i);
  perror(s);
  exit(1);
}

void* errcheck_ptr(
      void *i,
      void *ec,
      char *s) {
  if(i != ec) return(i);
  perror(s);
  exit(1);
}

