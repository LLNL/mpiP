/* mpi1.c -- simple link test */

/* Originally Created: Jeffrey Vetter (vetter@llnl.gov)  */
/* LLNL */

#ifndef lint
static char *rcsid =
  "$Header$";
#endif

#include <stdio.h>
#include <unistd.h>
#include "mpi.h"

main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  char buf[256];

  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  gethostname (buf, 256);
  printf ("MPI comm size is %d with rank %d executing on %s\n",
	  nprocs, rank, buf);
  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);
}

				 /* EOF */
