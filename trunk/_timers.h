/* -*- Mode: C; -*- 

   @PROLOGUE@

   ----- 

   Jeffrey Vetter <vetter@llnl.gov>
   Lawrence Livermore National Lab, Center for Applied Scientific Computing
   04 Oct 2000

   _timers.h -- timer macros

*/

#ifndef _MPITI_TIMERS_H
#define _MPITI_TIMERS_H

#define MSECS 1000
#define USECS 1000000
#define NSECS 1000000000

#include <unistd.h>
#include <sys/time.h>
typedef double mpiP_TIMER;

#if (defined(SunOS) && ! defined(USE_GETTIMEOFDAY))
#include "timers/sunos_local.h"

#elif (defined(AIX) && ! defined(USE_GETTIMEOFDAY))
#include "timers/aix_local.h"

#elif (defined(UNICOS_mp) && ! defined(USE_GETTIMEOFDAY))
#include "timers/crayx1_hw.h"

#elif (defined(Linux) && ! defined(USE_GETTIMEOFDAY) && defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0))
#include "timers/linux_posix.h"

#else
/* gettimeofday returns microseconds */
#define mpiPi_TIMER double
#define mpiPi_TIME struct timeval
#define mpiPi_TIMER_NAME "gettimeofday"
#define mpiPi_ASNTIME(lhs,rhs) {bcopy(rhs, lhs, sizeof(mpiPi_TIMER));}
#define mpiPi_GETTIME(timeaddr) gettimeofday(timeaddr,NULL)
#define mpiPi_GETUSECS(timeaddr) (((mpiPi_TIMER)(timeaddr)->tv_sec)*USECS+((mpiPi_TIMER)(timeaddr)->tv_usec))
#define mpiPi_PRINTTIME(taddr) printf("Time is %ld sec and %ld usec.\n", (taddr)->tv_sec, (taddr)->tv_usec)
#define mpiPi_GETTIMEDIFF(end,start) ((mpiP_TIMER)((((mpiPi_TIMER)(end)->tv_sec)*USECS)+(end)->tv_usec)-((((mpiPi_TIMER)(start)->tv_sec)*USECS)+(start)->tv_usec))
#define mpiPi_PRINTTIMEDIFF(end,start) {printf("Time diff is %ld usecs.\n",mpiPi_GETTIMEDIFF(end,start));}

#endif

#endif

/* EOF */
