
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

  /*  TODO:  MPI_Allgatherv  */
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
    fprintf(stderr, "Bsend\n");
  if ( rank % 2 )
  {
    void *buf;
    int bsize;
    bsize = packsize + MPI_BSEND_OVERHEAD; 
    buf = malloc(bsize);
    MPI_Buffer_attach(buf, bsize);
    MPI_Bsend(sendbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
  }
  else
  {
    MPI_Recv(recvbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD, 
             &status);
    MPI_Barrier(MPI_COMM_WORLD);
  }
  testcount += MESS_BASE_COUNT;

  if ( rank == 0 )
    fprintf(stderr, "Gather\n");
  MPI_Gather(sendbuf, testcount, MPI_MESS_TYPE, 
             recvbuf, testcount, MPI_MESS_TYPE,
             0, MPI_COMM_WORLD);

  testcount += MESS_BASE_COUNT;
  /*  TODO:  MPI_Gatherv 
      MPI_Ibsend
      MPI_Irsend
      MPI_Isend
      MPI_Issend  */
  testcount += MESS_BASE_COUNT;
  testcount += MESS_BASE_COUNT;
  testcount += MESS_BASE_COUNT;
  testcount += MESS_BASE_COUNT;
  testcount += MESS_BASE_COUNT;


  if ( rank == 0 )
    fprintf(stderr, "Reduce\n");
  MPI_Reduce(sendbuf, recvbuf, testcount, MPI_MESS_TYPE, MPI_SUM, 0,
             MPI_COMM_WORLD);
  testcount += MESS_BASE_COUNT;

  if ( rank == 0 )
    fprintf(stderr, "Rsend\n");
  if ( rank % 2 )
  {
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Rsend(sendbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD);
  }
  else
  {
    MPI_Irecv(recvbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD, 
             &request);
    MPI_Barrier(MPI_COMM_WORLD);
  }
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
    fprintf(stderr, "Send\n");
  if ( rank % 2 )
    MPI_Send(sendbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD);
  else
    MPI_Recv(recvbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD, 
             &status);
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

  if ( rank == 0 )
    fprintf(stderr, "Ssend\n");
  if ( rank % 2 )
    MPI_Ssend(sendbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD);
  else
    MPI_Recv(recvbuf, testcount, MPI_MESS_TYPE, dest, 0, MPI_COMM_WORLD, 
             &status);

  free(recvbuf);

  MPI_Finalize();
}
