/* -*- C -*- 

   @PROLOGUE@

   ----- 

   Jeffrey Vetter vetter@llnl.gov
   Center for Applied Scientific Computing, LLNL
   01 Feb 2001

   aix_swclock.h -- aix global switch clock

*/

#ifndef _AIX_SWCLOCK_H
#define _AIX_SWCLOCK_H

/* use global switch clock */
#include <swclock.h>

/* swclockRead returns nanoseconds/incr */
typedef struct mpiTi_timer
{
  swclock_handle_t h;
  int incr;
}
mpiTi_timer_t;
#define mpiTi_TIMER mpiTi_timer_t
#define mpiTi_TIMER_INIT(timer_addr) {(timer_addr)->h=swclockInit();if((timer_addr)->h==0){perror("swclockerror");exit(-1);}else{(timer_addr)->incr=swclockGetIncrement((timer_addr)->h);}{long long t1=swclockRead((timer_addr)->h);if(t1==-1){perror("swclockerror");exit(-1);}}}
#define mpiTi_TIME long long

#define mpiTi_ASNTIME(lhs,rhs) {bcopy(rhs, lhs, sizeof(mpiTi_TIME));}
#define mpiTi_GETTIME(timer,timeaddr) {(*(timeaddr))=swclockRead((timer)->h);}
#define mpiTi_GETUSECS(timer,timeaddr) ((*(timeaddr))/(((timer)->incr/1000000)))
#define mpiTi_GETTIMEDIFF(timer,end_addr,start_addr) (((*(end_addr))-(*(start_addr)))/((timer)->incr/1000000))
#define mpiTi_PRINTTIMEDIFF(timer,end_addr,start_addr) {printf("Time diff is %lld usecs.\n",mpiTi_GETTIMEDIFF(timer,end_addr,start_addr));}

#endif

/* eof */

