
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>

#define MESS_BASE_COUNT 256
#define MPI_MESS_TYPE MPI_DOUBLE
#define MESS_TYPE double
#define TOTAL_TEST_OPS 20

int main(int argc, char** argv)
{
  MESS_TYPE *sendbuf;
  void *recvbuf;
  int packsize, typesize, testcount;
  int dest, rank, maxactive, wsize;
  MPI_Status status;
  MPI_Request request;

  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &wsize);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Pack_size(MESS_BASE_COUNT*TOTAL_TEST_OPS, 
                MPI_MESS_TYPE, MPI_COMM_WORLD, &packsize);
  MPI_Type_size(MPI_MESS_TYPE, &typesize);

  if ( wsize % 2 )
    maxactive = wsize - 1;
  else
    maxactive = wsize;

  if ( maxactive < 2 )
  {
    if ( rank == 0 )
      fprintf(stderr, "Test must run with at least 2 MPI processes.\n");
    MPI_Abort(MPI_COMM_WORLD, 0);
  }

  if ( rank % 2 )
    dest = rank - 1;
  else
    dest = rank + 1;

  sendbuf = malloc(typesize * MESS_BASE_COUNT * TOTAL_TEST_OPS);
  recvbuf = malloc(wsize * typesize * MESS_BASE_COUNT * TOTAL_TEST_OPS);

  testcount = MESS_BASE_COUNT;

  if ( rank == 0 )
    fprintf(stderr, "wsize: %d  typesize: %d  MESS_BASE_COUNT: %d\n", 
            wsize, typesize, MESS_BASE_COUNT);

  fprintf(stderr, "rank %d sending to rank %d\n", rank, dest);

  if ( rank == 0 )
    fprintf(stderr, "Allgather\n");

  MPI_Allgather(sendbuf, testcount, MPI_MESS_TYPE, 
                recvbuf, testcount, MPI_MESS_TYPE,
                MPI_COMM_WORLD);

  testcount += MESS_BASE_COUNT;

  if ( rank == 0 )
    fprintf(stderr, "Allgatherv\n");
  {
    int *recvcounts, *displs, i;
    recvcounts = malloc(sizeof(int)*wsize);
    displs = malloc(sizeof(int)*wsize);

    if ( recvcounts && displs )
    {
      for ( i = 0; i < wsize; i++ )
      {
        recvcounts[i] = testcount;
        displs[i] = i*testcount;
      }
      MPI_Allgatherv (sendbuf, testcount, MPI_MESS_TYPE,
                      recvbuf, recvcounts, displs,
                      MPI_MESS_TYPE, MPI_COMM_WORLD);
      free(recvcounts);
      free(displs);
    }
  }

  testcount += MESS_BASE_COUNT;

  if ( rank == 0 )
    fprintf(stderr, "Allreduce\n");
  MPI_Allreduce(sendbuf, recvbuf, testcount, MPI_MESS_TYPE, MPI_SUM,
                MPI_COMM_WORLD);
  testcount += MESS_BASE_COUNT;

  if ( rank == 0 )
    fprintf(stderr, "Alltoall\n");
  MPI_Alltoall(sendbuf, testcount, MPI_MESS_TYPE, 
               recvbuf, testcount, MPI_MESS_TYPE,
               MPI_COMM_WORLD);
  testcount += MESS_BASE_COUNT;

  if ( rank == 0 )
    fprintf(stderr, "Bcast\n");
  MPI_Bcast(sendbuf, testcount, MPI_MESS_TYPE, 0, MPI_COMM_WORLD);
  testcount += MESS_BASE_COUNT;

  if ( rank == 0 )
    fprintf(stderr, "Gather\n");
  MPI_Gather(sendbuf, testcount, MPI_MESS_TYPE, 
             recvbuf, testcount, MPI_MESS_TYPE,
             0, MPI_COMM_WORLD);

  testcount += MESS_BASE_COUNT;

  if ( rank == 0 )
    fprintf(stderr, "Gatherv\n");
  {
    int *recvcounts, *displs, i;
    recvcounts = malloc(sizeof(int)*wsize);
    displs = malloc(sizeof(int)*wsize);

    if ( recvcounts && displs )
    {
      for ( i = 0; i < wsize; i++ )
      {
        recvcounts[i] = testcount;
        displs[i] = i*testcount;
      }
      MPI_Gatherv (sendbuf, testcount, MPI_MESS_TYPE,
                      recvbuf, recvcounts, displs,
                      MPI_MESS_TYPE, 0, MPI_COMM_WORLD);
      free(recvcounts);
      free(displs);
    }
  }
  testcount += MESS_BASE_COUNT;

  if ( rank == 0 )
    fprintf(stderr, "Reduce\n");
  MPI_Reduce(sendbuf, recvbuf, testcount, MPI_MESS_TYPE, MPI_SUM, 0,
             MPI_COMM_WORLD);
  testcount += MESS_BASE_COUNT;

  if ( rank == 0 )
    fprintf(stderr, "Scan\n");
  MPI_Scan(sendbuf, recvbuf, testcount, MPI_MESS_TYPE, MPI_SUM, 
           MPI_COMM_WORLD);
  testcount += MESS_BASE_COUNT;

  if ( rank == 0 )
    fprintf(stderr, "Scatter\n");
  MPI_Scatter(sendbuf, testcount, MPI_MESS_TYPE, 
              recvbuf, testcount, MPI_MESS_TYPE,
              0, MPI_COMM_WORLD);
  testcount += MESS_BASE_COUNT;

  if ( rank == 0 )
    fprintf(stderr, "Sendrecv\n");
  MPI_Sendrecv(sendbuf, testcount, MPI_MESS_TYPE, dest, 0,
               recvbuf, testcount, MPI_MESS_TYPE, dest, 0,
               MPI_COMM_WORLD, &status);
  testcount += MESS_BASE_COUNT;

  MPI_Sendrecv_replace(sendbuf, testcount, MPI_MESS_TYPE, 
                       dest, 0, dest, 0,
                       MPI_COMM_WORLD, &status);
  testcount += MESS_BASE_COUNT;

  free(recvbuf);

  MPI_Finalize();
}
