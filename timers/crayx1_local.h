/* -*- C -*- 

   @PROLOGUE@

   ----- 

   Jeffrey Vetter vetter@llnl.gov
   Center for Applied Scientific Computing, LLNL
   01 Feb 2001

   aix_local.h -- local aix high res timers

*/
#ifndef _CRAYX1_LOCAL_H
#define _CRAYX1_LOCAL_H

#include <sys/time.h>

#define mpiPi_TIMER time_t
#define mpiPi_TIME struct timeval

/* #define mpiPi_ASNTIME(lhs,rhs) {bcopy(rhs, lhs, sizeof(mpiPi_TIMER));} */

/* #define mpiPi_GETTIME(timer,timeaddr) gettimeofday(timeaddr,NULL) */
void mpiPi_GETTIME( mpiPi_TIMER* timer, mpiPi_TIME* timeaddr);

#define mpiPi_GETUSECS(timer,timeaddr) ((((timeaddr)->tv_sec*USECS)+(timeaddr)->tv_usec))
#define mpiPi_PRINTTIME(taddr) printf("Time is %ld sec and %ld usec.\n", (taddr)->tv_sec, (taddr)->tv_usec)

/* #define mpiPi_GETTIMEDIFF(timer,end,start) ((((end)->tv_sec*USECS)+(end)->tv_usec)-(((start)->tv_sec*USECS)+(start)->tv_usec)) */
time_t mpiPi_GETTIMEDIFF( mpiPi_TIMER*, mpiPi_TIME* end, mpiPi_TIME*start);

#define mpiPi_PRINTTIMEDIFF(timer,end,start) {printf("Time diff is %ld usecs.\n",mpiPi_GETTIMEDIFF(timer,end,start));}

#endif /* _CRAYX1_LOCAL_H */

