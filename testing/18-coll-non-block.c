/* -*- Mode: C; -*- 
   mpiP MPI Profiler ( http://llnl.github.io/mpiP )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   ----- 

  18-coll-non-block.c -- example of collective non-blocking communication

*/


#ifndef lint
static char *svnid =
  "$Id: 18-coll-non-block.c 369 2006-10-05 19:21:12Z chcham $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define MSG_COUNT 4
#define TEST_OP_COUNT 17

int main (int ac, char **av)
{
  int i, j;
  MPI_Request r[TEST_OP_COUNT];
  MPI_Status s[TEST_OP_COUNT];
  int count;
  int tag = 111;
  MPI_Comm comm = MPI_COMM_WORLD;
  int nprocs, rank;
  float *sendX;
  float *recvX;
  int ridx = 0;
  int *sendcounts;
  int *recvcounts;
  int *displs;
  MPI_Datatype *typearr;

  MPI_Init (&ac, &av);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);

  /*  Allocate data relative to size of job  */
  sendX = (float*)malloc(MSG_COUNT * nprocs * sizeof(float));
  recvX = (float*)malloc(MSG_COUNT * nprocs * sizeof(float));
  sendcounts = (int*)malloc(nprocs * sizeof(int));
  recvcounts = (int*)malloc(nprocs * sizeof(int));
  displs = (int*)malloc(nprocs * sizeof(int));
  typearr = (MPI_Datatype*)malloc(nprocs * sizeof(MPI_Datatype));
  if ( sendX == NULL || recvX == NULL || sendcounts == NULL || recvcounts == NULL || displs == NULL ) MPI_Abort(MPI_COMM_WORLD, 1);

  count = nprocs;

  for (i = 0; i < nprocs; i++)
  {
    sendcounts[i] = 2;
    recvcounts[i] = 2;
    displs[i] = 0;
    typearr[i] = MPI_FLOAT;
  }

  for (i = 0; i < 1; i++)
    {

#define MPI_TEST(x,M) if (MPI_SUCCESS != (x) ) \
  { fprintf(stderr, "failed for %s\n", M ); } \
  else if ( rank == 0 ) { fprintf(stderr, "Passed %s\n", M); }

      MPI_TEST(MPI_Iallgather (sendX, count, MPI_FLOAT, recvX, count, MPI_FLOAT, comm, &r[ridx++]), "Iallgather")
      MPI_TEST(MPI_Iallgatherv (sendX, count, MPI_FLOAT, recvX, recvcounts, displs, MPI_FLOAT, comm, &r[ridx++]), "Iallgather")
      MPI_TEST(MPI_Iallreduce (sendX, recvX, count, MPI_FLOAT, MPI_SUM, comm, &r[ridx++]), "Iallreduce")
      MPI_TEST(MPI_Ialltoall (sendX, count, MPI_FLOAT, recvX, count, MPI_FLOAT, comm, &r[ridx++]), "Ialltoall")
      MPI_TEST(MPI_Ialltoallv (sendX, sendcounts, displs, MPI_FLOAT, recvX, recvcounts, displs, MPI_FLOAT, comm, &r[ridx++]), "Ialltoallv")
      MPI_TEST(MPI_Ialltoallw (sendX, sendcounts, displs, typearr, recvX, recvcounts, displs, typearr, comm, &r[ridx++]), "Ialltoallw")
      MPI_TEST(MPI_Ibarrier (comm, &r[ridx++]), "Ibarrier")
      MPI_TEST(MPI_Ibcast (sendX, count, MPI_FLOAT, 0, comm, &r[ridx++]), "Ibcast")
      MPI_TEST(MPI_Iexscan (sendX, recvX, count, MPI_FLOAT, MPI_SUM, comm, &r[ridx++]), "Iexscan")
      MPI_TEST(MPI_Igather (sendX, count, MPI_FLOAT, recvX, count, MPI_FLOAT, 0, comm, &r[ridx++]), "Igather")
      MPI_TEST(MPI_Igatherv (sendX, count, MPI_FLOAT, recvX, recvcounts, displs, MPI_FLOAT, 0, comm, &r[ridx++]), "Igatherv")
      MPI_TEST(MPI_Ireduce (sendX, recvX, count, MPI_FLOAT, MPI_SUM, 0, comm, &r[ridx++]), "Ireduce")
      MPI_TEST(MPI_Ireduce_scatter_block (sendX, recvX, count, MPI_FLOAT, MPI_SUM, comm, &r[ridx++]), "Ireduce_scatter_block")
      MPI_TEST(MPI_Ireduce_scatter (sendX, recvX, recvcounts, MPI_FLOAT, MPI_SUM, comm, &r[ridx++]), "Ireduce_scatter")
      MPI_TEST(MPI_Iscan (sendX, recvX, count, MPI_FLOAT, MPI_SUM, comm, &r[ridx++]), "Iscan")
      MPI_TEST(MPI_Iscatter (sendX, count, MPI_FLOAT, recvX, count, MPI_FLOAT, 0, comm, &r[ridx++]), "Iscatter")
      MPI_TEST(MPI_Iscatterv (sendX, sendcounts, displs, MPI_FLOAT, recvX, count, MPI_FLOAT, 0, comm, &r[ridx++]), "Iscatterv")
      MPI_Waitall (ridx, r, s);
    }

  MPI_Finalize ();

  free(recvcounts);
  free(recvX);

  exit(0);
}

/* eof */

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
