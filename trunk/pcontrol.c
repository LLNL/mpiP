/* -*- C -*- 

   @PROLOGUE@

   ----- 

   Jeffrey Vetter vetter3@llnl.gov
   Center for Applied Scientific Computing, LLNL
   21 Mar 2002

   pcontrol.c -- implement pcontrol for MPIP

 */

#ifndef lint
static char *rcsid =
  "$Header$";
#endif

#include <float.h>
#include "mpiPi.h"
#include "symbols.h"

void
mpiPi_reset_callsite_data ()
{
  int ac, ndx;
  callsite_stats_t **av;
  callsite_stats_t *csp = NULL;

  /* gather local task data */
  h_gather_data (mpiPi.task_callsite_stats, &ac, (void ***) &av);

  for (ndx = 0; ndx < ac; ndx++)
    {
      csp = av[ndx];

      csp->maxDur = 0;
      csp->minDur = DBL_MAX;
      csp->maxIO = 0;
      csp->minIO = DBL_MAX;
      csp->maxDataSent = 0;
      csp->minDataSent = DBL_MAX;

      csp->count = 0;
      csp->cumulativeTime = 0;
      csp->cumulativeTimeSquared = 0;
      csp->cumulativeDataSent = 0;
      csp->cumulativeIO = 0;

      csp->arbitraryMessageCount = 0;
    }

  if (time (&mpiPi.start_timeofday) == (time_t) - 1)
    {
      mpiPi_msg_warn ("Could not get time of day from time()\n");
    }

  mpiPi_GETTIME (&mpiPi.startTime);
  mpiPi.cumulativeTime = 0;

  mpiPi.global_app_time = 0;
  mpiPi.global_mpi_time = 0;
  mpiPi.global_mpi_size = 0;
  mpiPi.global_mpi_io = 0;
  mpiPi.global_mpi_msize_threshold_count = 0;
  mpiPi.global_mpi_sent_count = 0;
  mpiPi.global_time_callsite_count = 0;

  free (av);
}


static int
mpiPi_MPI_Pcontrol (const int flag)
{
  mpiP_TIMER dur;
  mpiPi_msg_debug ("MPI_Pcontrol encountered: flag = %d\n", flag);

  if (flag == 0)
    {
      if (!mpiPi.enabled)
	mpiPi_msg_warn
	  ("MPI_Pcontrol trying to disable MPIP while it is already disabled.\n");

      mpiPi_GETTIME (&mpiPi.endTime);
      dur =
	(mpiPi_GETTIMEDIFF (&mpiPi.endTime, &mpiPi.startTime) / 1000000.0);
      mpiPi_msg_debug ("In Pcontrol rank %d dur = %g\n", mpiPi.rank, dur);
      mpiPi.cumulativeTime += dur;
      assert (mpiPi.cumulativeTime >= 0);
      mpiPi.enabled = 0;
    }
  else if (flag == 2)
    {
      mpiPi_generateReport ();
    }
  else if (flag == 3)
    {
      mpiPi_reset_callsite_data ();
    }
  else
    {
      if (mpiPi.enabled)
	mpiPi_msg_warn
	  ("MPI_Pcontrol trying to enable MPIP while it is already enabled.\n");

      mpiPi.enabled = 1;
      mpiPi.enabledCount++;
      mpiPi_GETTIME (&mpiPi.startTime);
    }

  return MPI_SUCCESS;
}

int
MPI_Pcontrol (const int flag, ...)
{
  return mpiPi_MPI_Pcontrol (flag);
}

int
F77_MPI_PCONTROL (int *flag, ...)
{
  return mpiPi_MPI_Pcontrol (*flag);
}

/* eof */
