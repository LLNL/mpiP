/*

   mpiP MPI Profiler ( http://mpip.sourceforge.net/ )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   ----- 

   api-test.c -- stand-alone test to check API functionality

*/

#include <stdio.h>
#include <stdlib.h>
#include "mpiP-API.h"
#include "mpiPconfig.h"

#define MAX_STACK 16
#define MAX_STRING 256

void
thisIsTheTargetFunction ()
{
  void *pc_array[MAX_STACK] = { NULL };
  char *file, *func;
  int i, line_no, frame_count;
  char addr_buf[24];

  file = malloc (MAX_STRING);
  func = malloc (MAX_STRING);

  frame_count = mpiP_record_traceback (pc_array, MAX_STACK);
  printf ("frame_count is %d\n", frame_count);
  for (i = 0; i < MAX_STACK && pc_array[i] != NULL; i++)
    {
#if ! defined(DISABLE_BFD) || defined(USE_LIBDWARF)
      if (mpiP_find_src_loc (pc_array[i], &file, &line_no, &func) == 0)
	{
	  printf ("for %s, file: %-10s line:%-2d func:%s\n",
		  mpiP_format_address (pc_array[i], addr_buf), file, line_no,
		  func);
	}
      else
	printf ("lookup failed for %s\n",
		mpiP_format_address (pc_array[i], addr_buf));
#else
      printf ("  %s\n", mpiP_format_address (pc_array[i], addr_buf));
#endif
    }
}

void
thisIsTheNextLevelFunction ()
{
  thisIsTheTargetFunction ();
}

void
thisIsTheTopLevelFunction ()
{
  thisIsTheNextLevelFunction ();
}

main (int argc, char *argv[])
{
  char *infile;
  mpiP_TIMER start, end;

  start = mpiP_gettime ();
  infile = mpiP_get_executable_name ();

  if (infile == NULL || mpiP_open_executable (infile) != 0)
    {
      fprintf (stderr, "failed to open %s\n", infile);
      exit (1);
    }

  fprintf (stderr, "opened executable file\n");

  thisIsTheTopLevelFunction ();
  fprintf (stderr, "finished function test\n");

  mpiP_close_executable ();
  end = mpiP_gettime ();
  fprintf (stderr, "start is     %11.9f\n", start);
  fprintf (stderr, "end   is     %11.9f\n", end);
  fprintf (stderr, "time diff is %11.9f\n", (end - start) / 1000000);
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
