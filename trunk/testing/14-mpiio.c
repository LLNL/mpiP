/*
   mpiP MPI Profiler ( http://www.llnl.gov/CASC/mpip/ )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   ----- 

   14-mpiio.c -- MPI I/O test 

*/

#ifndef lint
static char *rcsid =
  "$Header$";
#endif

#include <mpi.h>
#include <string.h>

#define TEST_OPS 20
#define BUF_SIZE 256
#define BLOCK_SIZE 16

int
main (int argc, char **argv)
{
  MPI_Request request;
  MPI_File fh;
  MPI_Datatype ftype;
  MPI_Offset offset;
  MPI_Status status;
  int rank, wsize, fsize, i;
  char file_name[128];
  int buf[BUF_SIZE * TEST_OPS];
  int count;

  MPI_Init (&argc, &argv);

  MPI_Comm_size (MPI_COMM_WORLD, &wsize);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);

  strcpy (file_name, argv[0]);
  strcat (file_name, ".tmp");

  MPI_File_open (MPI_COMM_WORLD, file_name, MPI_MODE_RDWR | MPI_MODE_CREATE,
		 MPI_INFO_NULL, &fh);

  fsize = wsize * BUF_SIZE * TEST_OPS;
  MPI_File_preallocate (fh, fsize);

  memset (buf, 0, BUF_SIZE * TEST_OPS);
  offset = 0;
  count = BLOCK_SIZE;

  for (i = 0; i < TEST_OPS; i++)
    {
      offset = i * BLOCK_SIZE + (rank * BLOCK_SIZE * TEST_OPS);

      MPI_File_seek (fh, offset, MPI_SEEK_SET);
      MPI_File_write (fh, buf, count, MPI_INT, &status);

      MPI_File_seek (fh, offset, MPI_SEEK_SET);
      MPI_File_read (fh, buf, count, MPI_INT, &status);
    }

  for (i = 0; i < TEST_OPS; i++)
    {
      offset = i * BLOCK_SIZE + (rank * BLOCK_SIZE * TEST_OPS);
      MPI_File_write_at (fh, offset, buf, count, MPI_INT, &status);
      MPI_File_read_at (fh, offset, buf, count, MPI_INT, &status);
    }

  MPI_Type_vector (fsize / BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE * wsize,
		   MPI_INT, &ftype);
  MPI_Type_commit (&ftype);

  offset = rank * BLOCK_SIZE * TEST_OPS;
  count = BLOCK_SIZE * TEST_OPS;

  MPI_File_set_view (fh, offset, MPI_INT, ftype, "native", MPI_INFO_NULL);
  MPI_File_write_all (fh, buf, count, MPI_INT, &status);
  MPI_File_read_all (fh, buf, count, MPI_INT, &status);

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
 
This file is part of mpiP.  For details, see http://www.llnl.gov/CASC/mpip/. 
 
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

