/* -*- C -*- 

   @PROLOGUE@

   ----- 

   Jeffrey Vetter vetter@llnl.gov
   Center for Applied Scientific Computing, LLNL
   01 Feb 2001

   sunos-timers.h -- sun os timers

*/

#ifndef _SUNOS_TIMERS_H
#define _SUNOS_TIMERS_H

/* gethrtime returns nanoseconds */
#define mpiTi_TIMESTRUCT hrtime_t
#define mpiTi_ASNTIME(lhs,rhs) {*(lhs) = *(rhs);}
#define mpiTi_GETTIME(timeaddr) (*(timeaddr))=gethrtime()
#define mpiTi_GETUSECS(timeaddr) ((*(timeaddr))/1000)
#define mpiTi_GETTIMEDIFF(end,start) (((*(hrtime_t*)(end))-(*(hrtime_t*)(start)))/1000)

#endif

/* eof */

