/* 3-stacktrace.c -- hot potato stack trace test */

#ifndef lint
static char *rcsid =
  "$Header$";
#endif

#include <stdio.h>
#include "mpi.h"

int nprocs = -1;
int rank = -1;
int tag = 30;
int dest, src;
float data = 0.0;


void
sendData ()
{
  dest = (rank == nprocs - 1) ? (0) : (rank + 1);
  data = rank;
  MPI_Send (&data, 1, MPI_FLOAT, dest, tag, MPI_COMM_WORLD);
  printf ("(%d) sent data %f\n", rank, data);
  fflush (stdout);
}


void
recvData ()
{
  int src = (rank == 0) ? (nprocs - 1) : (rank - 1);
  MPI_Status status;
  MPI_Recv (&data, 1, MPI_FLOAT, src, tag, MPI_COMM_WORLD, &status);
  printf ("(%d) got data %f\n", rank, data);
  fflush (stdout);
}


void
exchangeData ()
{
  MPI_Barrier (MPI_COMM_WORLD);

  if (rank % 2)
    {
      sendData ();
      recvData ();
    }
  else
    {
      recvData ();
      sendData ();
    }
}


main (int argc, char **argv)
{
  char processor_name[128];
  int namelen = 128;

  /* init */
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  printf ("Initializing (%d of %d)\n", rank, nprocs);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);
  fflush (stdout);

  exchangeData ();

  MPI_Barrier (MPI_COMM_WORLD);

  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

				 /* EOF */
