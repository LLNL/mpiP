/*
 
   mpiP MPI Profiler ( http://mpip.sourceforge.net/ )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   ----- 

   timer-info.c -- provide timer and traceback overhead information

*/

#include <stdio.h>
#include <stdlib.h>
#include "mpiP-API.h"
#include "mpiPconfig.h"

#define MAX_STRING 256
/*#define TESTING*/

int depth = 0;
int target_depth = 0;

void
thisIsTheTargetFunction ()
{
  void **pc_array = NULL;
  char *file, *func;
  int i, line_no;
  char addr_buf[24];

  pc_array = malloc (sizeof (void *) * target_depth);

  mpiP_record_traceback (pc_array, target_depth);

#ifdef TESTING
  file = malloc (MAX_STRING);
  func = malloc (MAX_STRING);
  for (i = 0; i < target_depth && pc_array[i] != NULL; i++)
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
  free (file);
  free (func);
#endif

  free (pc_array);
}

void
thisIsTheTopLevelFunction ()
{
  if (depth == target_depth)
    thisIsTheTargetFunction ();
  else
    {
      depth++;
      thisIsTheTopLevelFunction ();
    }
}

void
getStackWalkOH (char *infile, int target_depth, long iters)
{
  long idx;
  mpiP_TIMER start, end;

#ifdef TESTING
  if (mpiP_open_executable (infile) != 0)
    {
      fprintf (stderr, "failed to open %s\n", infile);
      exit (1);
    }
#endif

  start = mpiP_gettime ();
  for (idx = 0; idx < iters; idx++)
    {
      thisIsTheTopLevelFunction ();
    }
  end = mpiP_gettime ();

#ifdef TESTING
  mpiP_close_executable ();
#endif

  printf ("\nStackwalk OH testing\n");
  printf ("target_depth is     %d\n", target_depth);
  printf ("start is            %f\n", start);
  printf ("end   is            %f\n", end);
  printf ("time diff is        %.9f sec\n", ((double) end - start) / 1000000);
  printf ("time/stack walk     %.9f sec\n",
	  (((double) end - start) / 1000000) / iters);
  printf ("time/stack level is %.9f sec\n",
	  (((double) end - start) / 1000000) / (target_depth * iters));
}


void
getGranAndOH (long iters)
{
  mpiP_TIMER start, end;
  long idx;

  printf ("\nGranularity Testing\n");
  printf ("\nUsing timer : %s\n", mpiPi_TIMER_NAME);
  while (mpiP_gettime () == (start = mpiP_gettime ()))
    {
      fprintf (stderr, "start is %f\n", start);
    }
  while (start == (end = mpiP_gettime ()));

  printf ("start is %f\n", start);
  printf ("end   is %f\n", end);
  printf ("timer granularity appears to be <%f usec\n\n", end - start);

  while (mpiP_gettime () == (start = mpiP_gettime ()));
  for (idx = 0; idx < iters; idx++)
    end = mpiP_gettime ();

  printf ("\nOH Testing\n");
  printf ("start is %f\n", start);
  printf ("end   is %f\n", end);
  printf ("timer OH appears to be %f usec\n", (end - start) / iters);
}


main (int argc, char *argv[])
{
  char *infile;
  long iters;
  mpiP_TIMER start, end;

  if (argc != 3)
    {
      fprintf (stderr,
	       "arguments are target stack depth and test loop interations.\n");
      exit (1);
    }

  infile = argv[0];
  target_depth = atoi (argv[1]);
  iters = atol (argv[2]);

  getStackWalkOH (infile, target_depth, iters);

  getGranAndOH (iters);
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
