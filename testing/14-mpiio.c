/* 14-mpiio.c -- MPI I/O test */

#ifndef lint
static char *rcsid =
  "$Header$";
#endif

#include <mpi.h>
#include <string.h>

#define TEST_OPS 20
#define BUF_SIZE 256
#define BLOCK_SIZE 16

int main(int argc, char** argv)
{
  MPI_Request request;
  MPI_File fh;
  MPI_Datatype ftype;
  MPI_Offset offset;
  MPI_Status status;
  int rank, wsize, fsize, i;
  char file_name[128];
  int buf[BUF_SIZE*TEST_OPS];
  int count;

  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &wsize);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  strcpy(file_name, argv[0]);
  strcat(file_name, ".tmp");

  MPI_File_open(MPI_COMM_WORLD, file_name, MPI_MODE_RDWR | MPI_MODE_CREATE,
      MPI_INFO_NULL, &fh);

  fsize = wsize*BUF_SIZE*TEST_OPS;
  MPI_File_preallocate(fh, fsize);

  memset(buf, 0, BUF_SIZE*TEST_OPS);
  offset = 0;
  count  = BLOCK_SIZE;

  for ( i = 0; i < TEST_OPS; i++ )
  {
    offset = i*BLOCK_SIZE+(rank*BLOCK_SIZE*TEST_OPS);

    MPI_File_seek (fh, offset, MPI_SEEK_SET);
    MPI_File_write(fh, buf, count, MPI_INT, &status);

    MPI_File_seek (fh, offset, MPI_SEEK_SET);
    MPI_File_read (fh, buf, count, MPI_INT, &status);
  }

  for ( i = 0; i < TEST_OPS; i++ )
  {
    offset = i*BLOCK_SIZE+(rank*BLOCK_SIZE*TEST_OPS);
    MPI_File_write_at(fh, offset, buf, count, MPI_INT, &status);
    MPI_File_read_at (fh, offset, buf, count, MPI_INT, &status);
  }

  MPI_Type_vector(fsize/BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE*wsize, 
                  MPI_INT, &ftype);
  MPI_Type_commit(&ftype);

  offset = rank*BLOCK_SIZE*TEST_OPS;
  count  = BLOCK_SIZE*TEST_OPS;

  MPI_File_set_view(fh, offset, MPI_INT, ftype, "native", MPI_INFO_NULL);
  MPI_File_write_all(fh, buf, count, MPI_INT, &status);
  MPI_File_read_all (fh, buf, count, MPI_INT, &status);

  MPI_File_close(&fh);

  MPI_Finalize();
}
