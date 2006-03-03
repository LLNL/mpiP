
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>

#define MESS_BASE_COUNT 256
#define MPI_MESS_TYPE MPI_DOUBLE
#define MESS_TYPE double
#define TOTAL_TEST_OPS 20

int
main (int argc, char **argv)
{
  MESS_TYPE *sendbuf;
  void *recvbuf, *buf;
  int packsize, typesize, testcount;
  int dest, rank, maxactive, wsize, bsize;
  MPI_Status status;
  MPI_Request request;

  MPI_Init (&argc, &argv);

  MPI_Comm_size (MPI_COMM_WORLD, &wsize);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Pack_size (MESS_BASE_COUNT * TOTAL_TEST_OPS,
		 MPI_MESS_TYPE, MPI_COMM_WORLD, &packsize);
  MPI_Type_size (MPI_MESS_TYPE, &typesize);

  if (wsize % 2)
    maxactive = wsize - 1;
  else
    maxactive = wsize;

  if (maxactive < 2)
    {
      if (rank == 0)
	fprintf (stderr, "Test must run with at least 2 MPI processes.\n");
      MPI_Abort (MPI_COMM_WORLD, 0);
    }

  if (rank % 2)
    dest = rank - 1;
  else
    dest = rank + 1;

  sendbuf = malloc (typesize * MESS_BASE_COUNT * TOTAL_TEST_OPS);
  recvbuf = malloc (wsize * typesize * MESS_BASE_COUNT * TOTAL_TEST_OPS);

  testcount = MESS_BASE_COUNT;

  if (rank == 0)
    fprintf (stderr, "wsize: %d  typesize: %d  MESS_BASE_COUNT: %d\n",
	     wsize, typesize, MESS_BASE_COUNT);

  fprintf (stderr, "rank %d sending to rank %d\n", rank, dest);

  if (rank == 0)
    fprintf (stderr, "Bsend\n");

  if (rank % 2)
    {
      bsize = packsize + MPI_BSEND_OVERHEAD;
      buf = malloc (bsize);
      MPI_Buffer_attach (buf, bsize);
      MPI_Bsend (sendbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD);
      MPI_Barrier (MPI_COMM_WORLD);
      MPI_Buffer_detach (buf, &bsize);
      free (buf);
    }
  else
    {
      MPI_Recv (recvbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD,
		&status);
      MPI_Barrier (MPI_COMM_WORLD);
    }

  testcount += MESS_BASE_COUNT;

  if (rank == 0)
    fprintf (stderr, "Ibsend\n");

  if (rank % 2)
    {
      bsize = packsize + MPI_BSEND_OVERHEAD;
      buf = malloc (bsize);
      MPI_Buffer_attach (buf, bsize);

      MPI_Ibsend (sendbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD,
		  &request);
      MPI_Barrier (MPI_COMM_WORLD);
      MPI_Wait (&request, &status);
      MPI_Buffer_detach (buf, &bsize);
      free (buf);
    }
  else
    {
      MPI_Recv (recvbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD,
		&status);
      MPI_Barrier (MPI_COMM_WORLD);
    }

  testcount += MESS_BASE_COUNT;

  if (rank == 0)
    fprintf (stderr, "Irsend\n");
  if (rank % 2)
    {
      MPI_Barrier (MPI_COMM_WORLD);
      MPI_Irsend (sendbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD,
		  &request);
    }
  else
    {
      MPI_Irecv (recvbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD,
		 &request);
      MPI_Barrier (MPI_COMM_WORLD);
    }
  testcount += MESS_BASE_COUNT;

  if (rank == 0)
    fprintf (stderr, "Isend\n");
  if (rank % 2)
    MPI_Isend (sendbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD,
	       &request);
  else
    MPI_Recv (recvbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD,
	      &status);
  testcount += MESS_BASE_COUNT;

  if (rank == 0)
    fprintf (stderr, "Issend\n");
  if (rank % 2)
    MPI_Issend (sendbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD,
		&request);
  else
    MPI_Recv (recvbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD,
	      &status);
  testcount += MESS_BASE_COUNT;

  if (rank == 0)
    fprintf (stderr, "Rsend\n");
  if (rank % 2)
    {
      MPI_Barrier (MPI_COMM_WORLD);
      MPI_Rsend (sendbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD);
    }
  else
    {
      MPI_Irecv (recvbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD,
		 &request);
      MPI_Barrier (MPI_COMM_WORLD);
    }
  testcount += MESS_BASE_COUNT;

  if (rank == 0)
    fprintf (stderr, "Send\n");
  if (rank % 2)
    MPI_Send (sendbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD);
  else
    MPI_Recv (recvbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD,
	      &status);
  testcount += MESS_BASE_COUNT;

  if (rank == 0)
    fprintf (stderr, "Ssend\n");
  if (rank % 2)
    MPI_Ssend (sendbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD);
  else
    MPI_Recv (recvbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD,
	      &status);

  free (recvbuf);

  MPI_Finalize ();
}
