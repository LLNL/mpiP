/* -*- C -*- 

   @PROLOGUE@

   ----- 

   Jeffrey Vetter vetter3@llnl.gov
   Center for Applied Scientific Computing, LLNL
   21 Mar 2002

   pcontrol.c -- implement pcontrol for MPIP

 */

#ifndef lint
static char *rcsid = "$Header$";
#endif

#include "mpiPi.h"
#include "symbols.h"

static int mpiPi_MPI_Pcontrol(const int flag)
{
  double dur;
  mpiPi_msg_debug("MPI_Pcontrol encountered: flag = %d\n", flag);

  if(flag == 0)
    {
      if(! mpiPi.enabled)
	mpiPi_msg_warn("MPI_Pcontrol trying to disable MPIP while it is already disabled.\n");
      
      mpiPi_GETTIME (&mpiPi.timer, &mpiPi.endTime);
      dur = (mpiPi_GETTIMEDIFF (&mpiPi.timer, &mpiPi.endTime, &mpiPi.startTime)/1000000.0);
      printf("In Pcontrol rank %d dur = %g\n", mpiPi.rank, dur);
      mpiPi.cumulativeTime += dur;
    }
  else if (flag == -1)
    {
      mpiPi_generateReport();
    }
  else
    {
      if(mpiPi.enabled)
	mpiPi_msg_warn("MPI_Pcontrol trying to enable MPIP while it is already enabled.\n");

      mpiPi.enabled = 1;
      mpiPi.enabledCount++;
      mpiPi_GETTIME (&mpiPi.timer, &mpiPi.startTime);
    }

    return MPI_SUCCESS;
}

int MPI_Pcontrol(const int flag,... )
{
  return mpiPi_MPI_Pcontrol(flag);
}

int F77_MPI_PCONTROL(int *flag,... )
{
  return mpiPi_MPI_Pcontrol(*flag);
}

/* eof */
