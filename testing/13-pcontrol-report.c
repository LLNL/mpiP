/* 13-pcontrol-report.c  -- test report generation with MPI_Pcontrol(-1) */
/* Chris Chambreau (chcham@llnl.gov)  */
/* LLNL */

#ifndef lint
static char *rcsid =
  "$Header$";
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "mpi.h"

main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  char buf[256];

  MPI_Init (&argc, &argv);	/* use cmd line to disable mpip initially */
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  gethostname (buf, 256);
  printf ("MPI comm size is %d with rank %d executing on %s\n",
	  nprocs, rank, buf);

  MPI_Barrier (MPI_COMM_WORLD);
  sleep(1);
  MPI_Barrier (MPI_COMM_WORLD);
  MPI_Pcontrol (2);		/* generate report */
  MPI_Barrier (MPI_COMM_WORLD);
  exit(1);
#if 0
  sleep(1);
  MPI_Barrier (MPI_COMM_WORLD);
  MPI_Pcontrol (-1);		/* generate report */
  sleep(1);
  MPI_Barrier (MPI_COMM_WORLD);
  sleep(1);
  MPI_Finalize ();
#endif
}
				 /* EOF */
