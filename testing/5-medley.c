/* -*- Mode: C; -*- */
/* Creator: Jeffrey Vetter (j-vetter@llnl.gov) Fri Jul 23 1999 */
/* 5-medley.c -- a few common mpi functions in one execution */

#ifndef lint
static char *rcsid =
  "$Header$";
#endif

#include <stdio.h>
#include <unistd.h>

#include "mpi.h"

int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  float data = 0.0;
  int tag = 30;
  char processor_name[128];
  int namelen = 128;
  int j;
  const int loopLimit = 10;
  int markerID;

  /* init */
  MPI_Init (&argc, &argv);

  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  printf ("Initializing (%d of %d)\n", rank, nprocs);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);

  /* phase 1 */
  if (rank == 0)
    {
      printf ("Some Sends and Receives...\n");
    }
  for (j = 0; j < loopLimit; j++)
    {
      MPI_Barrier (MPI_COMM_WORLD);
      if ((rank % 2) == 0)
	{
	  int dest = (rank == nprocs - 1) ? (0) : (rank + 1);
	  int src = (rank == 0) ? (nprocs - 1) : (rank - 1);
	  MPI_Status status;
	  data = rank;
	  MPI_Send (&data, 1, MPI_FLOAT, dest, tag, MPI_COMM_WORLD);
	  printf ("(%d) sent data %f\n", rank, data);
	  fflush (stdout);

	  MPI_Recv (&data, 1, MPI_FLOAT, src, tag, MPI_COMM_WORLD, &status);
	  printf ("(%d) got data %f\n", rank, data);
	  fflush (stdout);
	}
      else
	{
	  int src = (rank == 0) ? (nprocs - 1) : (rank - 1);
	  int dest = (rank == nprocs - 1) ? (0) : (rank + 1);
	  MPI_Status status;
	  MPI_Recv (&data, 1, MPI_FLOAT, src, tag, MPI_COMM_WORLD, &status);
	  printf ("(%d) got data %f\n", rank, data);
	  fflush (stdout);

	  data = rank;
	  MPI_Send (&data, 1, MPI_FLOAT, dest, tag, MPI_COMM_WORLD);
	  printf ("(%d) sent data %f\n", rank, data);
	  fflush (stdout);
	}
    }

  MPI_Barrier (MPI_COMM_WORLD);
  /* phase 2 */
  /* master does a bcast, but everyone calls it to essentially sync up */
  if (rank == 0)
    {
      printf ("Some broadcasts...\n");
    }
  {
    for (j = 0; j < loopLimit; j++)
      {
	int root = 0;
	if (rank == 0)		/* master only */
	  {
	    data = j;
	    printf ("(%d) bcast data %f\n", rank, data);
	    fflush (stdout);
	  }
	MPI_Bcast (&data, 1, MPI_FLOAT, root, MPI_COMM_WORLD);
	/*      sleep (1); */
      }
  }

  MPI_Barrier (MPI_COMM_WORLD);
  /* phase 3 */
  /* do some reductions */
  if (rank == 0)
    {
      printf ("Some reductions...\n");
    }
  {
    float iData = 0.0;
    float oData = 0.0;

    /* master does a bcast, but everyone calls it to essentially sync up */
    for (j = 0; j < loopLimit; j++)
      {
	iData = rank;
	MPI_Reduce ((void *) &iData, (void *) &oData, 1, MPI_FLOAT, MPI_SUM,
		    0, MPI_COMM_WORLD);
	if (rank == 0)
	  {
	    printf ("(%d) reduction total = %f\n", rank, oData);
	  }
	/*      sleep (1); */
      }
  }

  /* phase n */
  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);

  return 0;
}

/* EOF */
