/* -*- C -*- 

   @PROLOGUE@

   ----- 

   Jeffrey Vetter vetter@llnl.gov
   Center for Applied Scientific Computing, LLNL
   01 Feb 2001

   aix_local.h -- local aix high res timers

*/

#ifndef _AIX_LOCAL_H
#define _AIX_LOCAL_H

/* read_real_time returns nanoseconds */
#define mpiPi_TIMER int
#define mpiPi_TIMER_INIT(timer_addr) {*(timer_addr) = 0;}
#define mpiPi_TIME timebasestruct_t
#define mpiPi_ASNTIME(lhs,rhs) {bcopy(rhs, lhs, sizeof(mpiPi_TIME));}
#define mpiPi_GETTIME(timer,timeaddr) {read_real_time(timeaddr,TIMEBASE_SZ);}
# if 0
#define mpiPi_GETUSECS(timer,timeaddr) (_aix_cnvt_read_real_time (timeaddr))
static long
_aix_cnvt_read_real_time (mpiPi_TIME * t)
{
  int secs, nsecs;
  time_base_to_time (t, TIMEBASE_SZ);
  secs = t->tb_high;
  nsecs = t->tb_low;
  return /* usecs */ (secs * USECS) + (nsecs / 1000);
}
# endif
static long
_aix_cnvt_read_real_time_diff (mpiPi_TIME * end, mpiPi_TIME * start)
{
  int secs, nsecs;
  time_base_to_time (start, TIMEBASE_SZ);
  time_base_to_time (end, TIMEBASE_SZ);
  secs = end->tb_high - start->tb_high;
  nsecs = end->tb_low - start->tb_low;
  if (nsecs < 0)
    {
      secs--;
      nsecs += 1000000000;
    }
  return /* usecs */ (secs * USECS) + (nsecs / 1000);
}

#define mpiPi_GETTIMEDIFF(timer,end,start) _aix_cnvt_read_real_time_diff(end,start)
#define mpiPi_PRINTTIMEDIFF(timer,end,start) {printf("Time diff is %ld usecs.\n",mpiPi_GETTIMEDIFF(timer,end,start));}

#endif

/* eof */

