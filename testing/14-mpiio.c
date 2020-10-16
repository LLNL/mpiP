/*
   mpiP MPI Profiler ( http://llnl.github.io/mpiP )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   ----- 

   14-mpiio.c -- MPI I/O test 

*/

#ifndef lint
static char *svnid =
  "$Id$";
#endif

#include <mpi.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define TEST_OPS 20
#define BLOCK_SIZE 16

//#define DEBUG
#ifdef DEBUG
#define fprintf(...) fprintf(__VA_ARGS__)
#else
#define fprintf(...)
#endif

int
main (int argc, char **argv)
{
  MPI_Request request;
  MPI_File fh;
  MPI_Datatype ftype;
  MPI_Status status;
  MPI_Offset offset;

  int rank, wsize, i;
  char file_name[128];
  size_t data_size;
  int *buf;
  int count;
  int ret;
  char *pfs_path;

  MPI_Init (&argc, &argv);

  MPI_Comm_size (MPI_COMM_WORLD, &wsize);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);

  data_size = BLOCK_SIZE * TEST_OPS * wsize * sizeof(int);
  fprintf(stderr, "data_size is %ld\n", data_size);

  pfs_path = getenv("MPIP_TEST_PFS_DIR");
  if ( NULL == pfs_path)
      strcpy (file_name, "./testing/");
  else
      strcpy (file_name, pfs_path);
  strcat (file_name, "14-mpiio.tmp");
  fprintf(stderr, "file_name is %s\n", file_name);

  ret = MPI_File_open (MPI_COMM_WORLD, file_name, MPI_MODE_RDWR | MPI_MODE_CREATE | MPI_MODE_DELETE_ON_CLOSE,
		 MPI_INFO_NULL, &fh);
  if ( MPI_SUCCESS != ret ) fprintf(stderr, " Rank %d failed MPI_File_open\n", rank);
  fprintf(stderr, "After MPI_File_open\n");

  buf = (int*)malloc(data_size);
  if ( NULL == buf ) fprintf(stderr, " Rank %d failed to allocate %zd bytes\n", rank, data_size);

  ret = MPI_File_preallocate (fh, data_size);
  if ( MPI_SUCCESS != ret ) fprintf(stderr, " Rank %d failed MPI_File_preallocate\n", rank);
  fprintf(stderr, "After MPI_File_preallocate\n");

  memset (buf, 0, data_size);
  offset = 0;
  count = BLOCK_SIZE;

  for (i = 0; i < TEST_OPS; i++)
    {
      offset = (i * BLOCK_SIZE * sizeof(int)) + (rank * BLOCK_SIZE * TEST_OPS * sizeof(int));
      fprintf(stderr, "Rank %d offset 1:%d is %lld, count %d\n", rank, i, offset, count);

      ret = MPI_File_seek (fh, offset, MPI_SEEK_SET);
      if ( MPI_SUCCESS != ret ) fprintf(stderr, " Rank %d failed MPI_File_seek\n", rank);
      fprintf(stderr, "After MPI_File_seek\n");

      ret = MPI_File_write (fh, buf, count, MPI_INT, &status);
      if ( MPI_SUCCESS != ret ) fprintf(stderr, " Rank %d failed MPI_File_write\n", rank);

      ret = MPI_File_seek (fh, offset, MPI_SEEK_SET);
      if ( MPI_SUCCESS != ret ) fprintf(stderr, " Rank %d failed MPI_File_seek 2\n", rank);

      ret = MPI_File_read (fh, buf, count, MPI_INT, &status);
      if ( MPI_SUCCESS != ret ) fprintf(stderr, " Rank %d failed MPI_File_read\n", rank);
    }

  for (i = 0; i < TEST_OPS; i++)
    {
      offset = (i * BLOCK_SIZE * sizeof(int)) + (rank * BLOCK_SIZE * TEST_OPS * sizeof(int));
      fprintf(stderr, "Rank %d offset 2:%d is %lld, count %d\n", rank, i, offset, count);

      ret = MPI_File_write_at (fh, offset, buf, count, MPI_INT, &status);
      if ( MPI_SUCCESS != ret ) fprintf(stderr, " Rank %d failed MPI_File_write_at\n", rank);

      ret = MPI_File_read_at (fh, offset, buf, count, MPI_INT, &status);
      if ( MPI_SUCCESS != ret ) fprintf(stderr, " Rank %d failed MPI_File_read_at\n", rank);
    }

  ret = MPI_Type_vector (data_size / BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE * wsize,
		   MPI_INT, &ftype);
  if ( MPI_SUCCESS != ret ) fprintf(stderr, " Rank %d failed MPI_Type_vector\n", rank);

  ret = MPI_Type_commit (&ftype);
  if ( MPI_SUCCESS != ret ) fprintf(stderr, " Rank %d failed MPI_Type_commit\n", rank);

  offset = rank * BLOCK_SIZE * TEST_OPS;
  count = BLOCK_SIZE * TEST_OPS;

  ret = MPI_File_set_view (fh, offset, MPI_INT, ftype, "native", MPI_INFO_NULL);
  if ( MPI_SUCCESS != ret ) fprintf(stderr, " Rank %d failed MPI_File_set_view\n", rank);

  ret = MPI_File_write_all (fh, buf, count, MPI_INT, &status);
  if ( MPI_SUCCESS != ret ) fprintf(stderr, " Rank %d failed MPI_File_write_all\n", rank);

  ret = MPI_File_read_all (fh, buf, count, MPI_INT, &status);
  if ( MPI_SUCCESS != ret ) fprintf(stderr, " Rank %d failed MPI_File_read_all\n", rank);

  free(buf);

  MPI_File_close (&fh);

  MPI_Finalize ();
}

/* 

<license>

Copyright (c) 2006, The Regents of the University of California. 
Produced at the Lawrence Livermore National Laboratory 
Written by Jeffery Vetter and Christopher Chambreau. 
UCRL-CODE-223450. 
All rights reserved. 
 
This file is part of mpiP.  For details, see http://llnl.github.io/mpiP. 
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
 
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the disclaimer below.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the disclaimer (as noted below) in
the documentation and/or other materials provided with the
distribution.

* Neither the name of the UC/LLNL nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OF
THE UNIVERSITY OF CALIFORNIA, THE U.S. DEPARTMENT OF ENERGY OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 
Additional BSD Notice 
 
1. This notice is required to be provided under our contract with the
U.S. Department of Energy (DOE).  This work was produced at the
University of California, Lawrence Livermore National Laboratory under
Contract No. W-7405-ENG-48 with the DOE.
 
2. Neither the United States Government nor the University of
California nor any of their employees, makes any warranty, express or
implied, or assumes any liability or responsibility for the accuracy,
completeness, or usefulness of any information, apparatus, product, or
process disclosed, or represents that its use would not infringe
privately-owned rights.
 
3.  Also, reference herein to any specific commercial products,
process, or services by trade name, trademark, manufacturer or
otherwise does not necessarily constitute or imply its endorsement,
recommendation, or favoring by the United States Government or the
University of California.  The views and opinions of authors expressed
herein do not necessarily state or reflect those of the United States
Government or the University of California, and shall not be used for
advertising or product endorsement purposes.

</license>

*/
