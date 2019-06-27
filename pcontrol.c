/* -*- C -*- 

   mpiP MPI Profiler ( http://llnl.github.io/mpiP )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   ----- 

   pcontrol.c -- implement pcontrol for MPIP

 */

#ifndef lint
static char *svnid = "$Id$";
#endif

#include <float.h>
#include "mpiPi.h"
#include "symbols.h"
#ifdef ENABLE_FORTRAN_WEAK_SYMS
#include "weak-symbols-pcontrol.h"
#endif

void
mpiPi_reset_callsite_data ()
{
  mpiPi_stats_mt_cs_reset(&mpiPi.task_stats);
  /* Drop the content of the src ID cache so the old callsites won't appear in
   * the callsites unless they are invoked again
   */
  if (NULL != callsite_src_id_cache) {
      int ac, ndx;
      callsite_stats_t **av;

      h_drain(callsite_src_id_cache, &ac, (void ***)&av);
      for (ndx = 0; ndx < ac; ndx++)
        {
          free(av[ndx]);
        }
      free(av);
    }

  if (time (&mpiPi.start_timeofday) == (time_t) - 1)
    {
      mpiPi_msg_warn ("Could not get time of day from time()\n");
    }

  mpiPi_stats_mt_timer_start(&mpiPi.task_stats);
  mpiPi.cumulativeTime = 0;

  mpiPi.global_app_time = 0;
  mpiPi.global_mpi_time = 0;
  mpiPi.global_mpi_size = 0;
  mpiPi.global_mpi_io = 0;
  mpiPi.global_mpi_rma = 0;
  mpiPi.global_mpi_msize_threshold_count = 0;
  mpiPi.global_mpi_sent_count = 0;
  mpiPi.global_time_callsite_count = 0;
}


static int
mpiPi_MPI_Pcontrol (const int flag)
{
  mpiP_TIMER dur;
  mpiPi_msg_debug ("MPI_Pcontrol encountered: flag = %d\n", flag);

  if (flag == 0)
    {
      if (!mpiPi.enabled)
        mpiPi_msg_warn
            ("MPI_Pcontrol trying to disable MPIP while it is already disabled.\n");

      mpiPi_stats_mt_timer_stop(&mpiPi.task_stats);
      mpiPi.enabled = 0;
    }
  else if (flag == 2)
    {
      mpiPi_reset_callsite_data ();
    }
  else if (flag == 3)
    {
      mpiPi_generateReport (mpiPi_style_verbose);
      mpiPi_stats_mt_timer_start(&mpiPi.task_stats);
    }
  else if (flag == 4)
    {
      mpiPi_generateReport (mpiPi_style_concise);
      mpiPi_stats_mt_timer_start(&mpiPi.task_stats);
    }
  else
    {
      if (mpiPi.enabled)
        mpiPi_msg_warn
            ("MPI_Pcontrol trying to enable MPIP while it is already enabled.\n");

      mpiPi.enabled = 1;
      mpiPi.enabledCount++;
      mpiPi_stats_mt_timer_start(&mpiPi.task_stats);
    }

  return MPI_SUCCESS;
}

int
MPI_Pcontrol (const int flag, ...)
{
  return mpiPi_MPI_Pcontrol (flag);
}

int
F77_MPI_PCONTROL (int *flag, ...)
{
  return mpiPi_MPI_Pcontrol (*flag);
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


/* eof */
