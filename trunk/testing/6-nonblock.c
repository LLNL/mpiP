/* -*- Mode: C; -*- 

   @PROLOGUE@

   ----- 

   Jeffrey Vetter vetter@llnl.gov
   Lawrence Livermore Nat'l Laboratory, Center for Applied Scientific Computing
   10 Oct 2000

   6-nonblock.c -- example of simple non-blocking communication

 */

#ifndef lint
static char *rcsid =
  "$Header$";
#endif

#include <stdio.h>
#include <mpi.h>

#define MSG_CNT 4

main (int ac, char **av)
{
  int i, j;
  MPI_Request r1, r2;
  MPI_Status s1, s2;
  int cnt = MSG_CNT;
  int tag = 111;
  int comm = MPI_COMM_WORLD;
  int nprocs, rank;
  float sendX[MSG_CNT] = { 1, 2, 3, 4 };
  float recvX[MSG_CNT];

  MPI_Init (&ac, &av);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  for (i = 0; i < 10; i++)
    {
      int src = (rank == 0) ? (nprocs - 1) : (rank - 1);
      int dest = (rank == nprocs - 1) ? (0) : (rank + 1);
      MPI_Irecv (recvX, cnt, MPI_FLOAT, src, tag, comm, &r1);
      MPI_Isend (sendX, cnt, MPI_FLOAT, dest, tag, comm, &r2);
      MPI_Wait (&r1, &s1);
      MPI_Wait (&r2, &s2);
    }
  MPI_Finalize ();
}

/* eof */
