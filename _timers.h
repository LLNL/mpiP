/* -*- Mode: C; -*- 

   @PROLOGUE@

   ----- 

   Jeffrey Vetter <vetter@llnl.gov>
   Lawrence Livermore National Lab, Center for Applied Scientific Computing
   04 Oct 2000

   mpiTi_timers.h -- timer macros

*/

#ifndef _MPITI_TIMERS_H
#define _MPITI_TIMERS_H

#define MSECS 1000
#define USECS 1000000
#define NSECS 1000000000

#include <sys/time.h>
typedef double mpiP_TIMER;

#if (defined(SunOS) && ! defined(USE_GETTIMEOFDAY))
#include "timers/sunos_local.h"

#elif (defined(AIX) && ! defined(USE_GETTIMEOFDAY))
#include "aix_local.h"

#else
/* gettimeofday returns microseconds */
#define mpiPi_TIMER int
#define mpiPi_TIME struct timeval
#define mpiPi_ASNTIME(lhs,rhs) {bcopy(rhs, lhs, sizeof(mpiPi_TIMER));}
#define mpiPi_GETTIME(timeaddr) gettimeofday(timeaddr,NULL)
#define mpiPi_GETUSECS(timeaddr) ((mpiP_TIMER)(((mpiP_TIMER)(timeaddr)->tv_sec*USECS)+(timeaddr)->tv_usec))
#define mpiPi_PRINTTIME(taddr) printf("Time is %ld sec and %ld usec.\n", (taddr)->tv_sec, (taddr)->tv_usec)
#define mpiPi_GETTIMEDIFF(end,start) ((mpiP_TIMER)(((end)->tv_sec*USECS)+(end)->tv_usec)-(((start)->tv_sec*USECS)+(start)->tv_usec))
#define mpiPi_PRINTTIMEDIFF(end,start) {printf("Time diff is %ld usecs.\n",mpiPi_GETTIMEDIFF(end,start));}

#endif

#endif

/* EOF */
