/*
   mpiP MPI Profiler ( http://mpip.sourceforge.net/ )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   ----- 

   11-p2p-mess-size -- hot potato test 
*/

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

/* 

<license>

Copyright (c) 2006, The Regents of the University of California. 
Produced at the Lawrence Livermore National Laboratory 
Written by Jeffery Vetter and Christopher Chambreau. 
UCRL-CODE-223450. 
All rights reserved. 
 
This file is part of mpiP.  For details, see http://mpip.sourceforge.net/. 
 
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

/* EOF */
