/* -*- C -*- 

   @PROLOGUE@

   ----- 

   Jeffrey Vetter vetter@llnl.gov
   Center for Applied Scientific Computing, LLNL
   12 Feb 2001

   9-test-mpip-time.c -- concoct a simple scheme that we can test mpiP
numbers with

 */

#ifndef lint
static char *rcsid =
  "$Header$";
#endif

#include <stdio.h>
#include "mpi.h"


int sleeptime = (10);

main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  char buf[256];
  MPI_Comm comm = MPI_COMM_WORLD;

  /*  Call MPI_Init before checking args for MPICH  */
  MPI_Init (&argc, &argv);

  if (argc > 2)
    {
      printf ("usage: %s <sleep seconds>\n", argv[0]);
      MPI_Finalize ();
      exit (1);
    }
  if (argc == 2)
    {
      sleeptime = atoi (argv[1]);
    }

  MPI_Comm_size (comm, &nprocs);
  MPI_Comm_rank (comm, &rank);
  MPI_Barrier (comm);
  switch (rank)
    {
    case 0:
      sleep (sleeptime);	/* slacker! holding everyone else up */
      break;
    default:
      break;
    }
  MPI_Barrier (comm);
  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

/* eof */
