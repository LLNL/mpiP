/* 15-pcontrol.c  -- test deactivation of profiling with MPI_Pcontrol(0) */
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
  MPI_Init (&argc, &argv);
  MPI_Barrier (MPI_COMM_WORLD); /*  profiling should initially be disabled  */
  MPI_Pcontrol (1);		/*  enable profiling  */
  MPI_Barrier (MPI_COMM_WORLD);
  MPI_Pcontrol(0); /*  disable profiling  */
  MPI_Barrier (MPI_COMM_WORLD);
  MPI_Pcontrol (1);		/*  enable profiling  */
  MPI_Barrier (MPI_COMM_WORLD);
  MPI_Finalize ();
}
