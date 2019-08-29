/* -*- Mode: C; -*- 

   mpiP MPI Profiler ( http://llnl.github.io/mpiP )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   ----- 

   5-medley.c -- a few common mpi functions in one execution 

*/


#ifndef lint
static char *svnid =
  "$Id$";
#endif

#include <stdio.h>
#include <unistd.h>

#include <mpi.h>

int
main (int argc, char **argv)
{
  int nprocs = -1;
  int rank = -1;
  float data = 0.0;
  int tag = 30;
  char processor_name[128];
  int namelen = 128;
  int j;
  const int loopLimit = 10;
  int markerID;

  /* init */
  MPI_Init (&argc, &argv);

  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  printf ("Initializing (%d of %d)\n", rank, nprocs);
  MPI_Get_processor_name (processor_name, &namelen);
  printf ("(%d) is alive on %s\n", rank, processor_name);

  /* phase 1 */
  if (rank == 0)
    {
      printf ("Some Sends and Receives...\n");
    }
  for (j = 0; j < loopLimit; j++)
    {
      MPI_Barrier (MPI_COMM_WORLD);
      if ((rank % 2) == 0)
	{
	  int dest = (rank == nprocs - 1) ? (0) : (rank + 1);
	  int src = (rank == 0) ? (nprocs - 1) : (rank - 1);
	  MPI_Status status;
	  data = rank;
	  MPI_Send (&data, 1, MPI_FLOAT, dest, tag, MPI_COMM_WORLD);
	  printf ("(%d) sent data %f\n", rank, data);
	  fflush (stdout);

	  MPI_Recv (&data, 1, MPI_FLOAT, src, tag, MPI_COMM_WORLD, &status);
	  printf ("(%d) got data %f\n", rank, data);
	  fflush (stdout);
	}
      else
	{
	  int src = (rank == 0) ? (nprocs - 1) : (rank - 1);
	  int dest = (rank == nprocs - 1) ? (0) : (rank + 1);
	  MPI_Status status;
	  MPI_Recv (&data, 1, MPI_FLOAT, src, tag, MPI_COMM_WORLD, &status);
	  printf ("(%d) got data %f\n", rank, data);
	  fflush (stdout);

	  data = rank;
	  MPI_Send (&data, 1, MPI_FLOAT, dest, tag, MPI_COMM_WORLD);
	  printf ("(%d) sent data %f\n", rank, data);
	  fflush (stdout);
	}
    }

  MPI_Barrier (MPI_COMM_WORLD);
  /* phase 2 */
  /* master does a bcast, but everyone calls it to essentially sync up */
  if (rank == 0)
    {
      printf ("Some broadcasts...\n");
    }
  {
    for (j = 0; j < loopLimit; j++)
      {
	int root = 0;
	if (rank == 0)		/* master only */
	  {
	    data = j;
	    printf ("(%d) bcast data %f\n", rank, data);
	    fflush (stdout);
	  }
	MPI_Bcast (&data, 1, MPI_FLOAT, root, MPI_COMM_WORLD);
	/*      sleep (1); */
      }
  }

  MPI_Barrier (MPI_COMM_WORLD);
  /* phase 3 */
  /* do some reductions */
  if (rank == 0)
    {
      printf ("Some reductions...\n");
    }
  {
    float iData = 0.0;
    float oData = 0.0;

    /* master does a bcast, but everyone calls it to essentially sync up */
    for (j = 0; j < loopLimit; j++)
      {
	iData = rank;
	MPI_Reduce ((void *) &iData, (void *) &oData, 1, MPI_FLOAT, MPI_SUM,
		    0, MPI_COMM_WORLD);
	if (rank == 0)
	  {
	    printf ("(%d) reduction total = %f\n", rank, oData);
	  }
	/*      sleep (1); */
      }
  }

  /* phase n */
  MPI_Finalize ();
  printf ("(%d) Finished normally\n", rank);

  return 0;
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

/* EOF */
