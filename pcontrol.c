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

static int mpiPi_MPI_Pcontrol(const int flag)
{
  mpiPi_msg_debug("MPI_Pcontrol encountered: flag = %d\n", flag);

  if(flag == 0)
    {
      if(! mpiPi.enabled)
	mpiPi_msg_warn("MPI_Pcontrol trying to disable MPIP while it is already disabled.\n");
      
      mpiPi.enabled = 0;
      mpiPi.endTime = PMPI_Wtime();
      mpiPi.cumulativeTime += mpiPi.endTime - mpiPi.startTime;
    }
  else
    {
      if(mpiPi.enabled)
	mpiPi_msg_warn("MPI_Pcontrol trying to enable MPIP while it is already enabled.\n");

      mpiPi.enabled = 1;
      mpiPi.enabledCount++;
      mpiPi.startTime = PMPI_Wtime ();
    }

    return MPI_SUCCESS;
}

int MPI_Pcontrol(const int flag,... )
{
  return mpiPi_MPI_Pcontrol(flag);
}

int mpi_pcontrol(int *flag,... )
{
  return mpiPi_MPI_Pcontrol(*flag);
}

/* eof */
