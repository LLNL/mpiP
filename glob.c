/* -*- C -*- 

   @PROLOGUE@

   ----- 

   Jeffrey Vetter vetter3@llnl.gov
   Center for Applied Scientific Computing, LLNL
   12 Aug 2001

   glob.c -- globals vars

 */

#ifndef lint
static char *rcsid =
  "$Header$";
#endif

#include "mpiPi.h"

mpiPi_t mpiPi;

int mpiPi_debug = 0;

int mpiPi_vmajor = 2;
int mpiPi_vminor = 7;
int mpiPi_vpatch = 1;
char *mpiPi_vdate = __DATE__;
char *mpiPi_vtime = __TIME__;

/* eof */
