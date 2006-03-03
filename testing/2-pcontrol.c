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

  MPI_Init (&argc, &argv);	/* use cmd line to disable mpip initially */
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  gethostname (buf, 256);
  printf ("MPI comm size is %d with rank %d executing on %s\n",
	  nprocs, rank, buf);

  MPI_Barrier (MPI_COMM_WORLD);
  MPI_Pcontrol (1);		/* enable */
  sleep (2);
  MPI_Barrier (MPI_COMM_WORLD);
  MPI_Pcontrol (0);		/* disable */
  sleep (2);
  MPI_Pcontrol (1);		/* enable */
  sleep (2);
  MPI_Barrier (MPI_COMM_WORLD);
  MPI_Pcontrol (0);		/* disable */
  sleep (2);
  MPI_Barrier (MPI_COMM_WORLD);
  sleep (2);
  MPI_Finalize ();
  if (rank == 0)
    {
      printf ("\n\n ---> This program should execute for ~10 secs, \n"
	      " ---> but MPIP should measure only ~4 secs.\n\n\n");
    }
}

				 /* EOF */
