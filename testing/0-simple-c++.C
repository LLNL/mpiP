/* mpi1.c -- simple link test */

/* Originally Created: Jeffrey Vetter (vetter@llnl.gov)  */
/* LLNL */

#ifndef lint
static char *rcsid =
  "$Header$";
#endif

#include "mpi.h"
#include <stdio.h>
#include <unistd.h>

class simplec
{
public:
  simplec()
  {
    int nprocs = -1;
    int rank = -1;
    char buf[256];

    MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    gethostname (buf, 256);
    printf ("MPI comm size is %d with rank %d executing on %s\n",
	    nprocs, rank, buf);
  }
};

int 
main (int argc, char **argv)
{
  simplec * s;

  MPI_Init (&argc, &argv);
  s = new simplec;
  MPI_Finalize ();

  printf("done\n");
}

				 /* EOF */
