/* -*- C -*- 

   @PROLOGUE@

   ----- 


   linux_posix.h -- POSIX high-resolution timers on Linux

*/

#ifndef _LINUX_POSIX_H
#define _LINUX_POSIX_H

/* read_real_time returns nanoseconds */
#define mpiPi_TIMER double
#define mpiPi_TIME  struct timespec
#define mpiPi_TIMER_NAME    "clock_gettime"

#define mpiPi_TIMER_INIT(timer_addr) {*(timer_addr) = 0;}
#define mpiPi_ASNTIME(lhs,rhs) {bcopy(rhs, lhs, sizeof(mpiPi_TIME));}
#define mpiPi_GETTIME(timeaddr) {clock_gettime(CLOCK_PROCESS_CPUTIME_ID, timeaddr);}

#define mpiPi_GETUSECS(timeaddr) ((((mpiPi_TIMER)((timeaddr)->tv_sec))*USECS) + ((mpiPi_TIMER)((timeaddr)->tv_nsec))/1000)
#define mpiPi_GETTIMEDIFF(end,start) (mpiPi_GETUSECS(end)-mpiPi_GETUSECS(start))

#endif

/* eof */

