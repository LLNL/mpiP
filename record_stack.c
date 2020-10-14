/* -*- C -*- 

   mpiP MPI Profiler ( http://llnl.github.io/mpiP )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   ----- 

   record_stack.c -- Implementations of mpiPi_RecordTraceBack 
                       stack tracing function.

 */

#ifndef lint
static char *svnid = "$Id$";
#endif

#include <setjmp.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "mpiPi.h"

#ifdef OSF1
#include <excpt.h>
#endif

#ifdef HAVE_LIBUNWIND
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#endif


#ifdef HAVE_LIBUNWIND

int
mpiPi_RecordTraceBack (jmp_buf jb, void *pc_array[], int max_back)
{
  int i, valid_cursor, parent_frame_start, frame_count = 0;
  unw_context_t uc;
  unw_cursor_t cursor;
  unw_word_t pc;

  assert(pc_array != NULL);

  //  If we are unable to get a stack trace, ensure that the first frame PC is NULL
  pc_array[0] = NULL;

  // Inlining / noinlining may affect frames in report
  //   - Tools frames may appear in report
  parent_frame_start = 0;

  if (unw_getcontext (&uc) != 0)
    {
      mpiPi_msg_debug ("Failed unw_getcontext!\n");
      return frame_count;
    }

  if (unw_init_local (&cursor, &uc) != UNW_ESUCCESS)
    {
      mpiPi_msg_debug
          ("Failed to initialize libunwind cursor with unw_init_local\n");
    }
  else
    {
      for (i = 0; i < parent_frame_start; i++)
        {
          if (unw_step (&cursor) < 1)
            mpiPi_msg_debug
                ("unw_step failed to step into mpiPi caller frame.\n");
        }

      for (i = 0, valid_cursor = 1; i < max_back; i++)
        {
          if (valid_cursor && unw_step (&cursor) >= 0)
            {
              frame_count++;
              if (unw_get_reg (&cursor, UNW_REG_IP, &pc) != UNW_ESUCCESS)
                {
                  pc_array[i] = NULL;
                  mpiPi_msg_debug ("unw_get_reg failed.\n");
                }
              else
                {
                  pc_array[i] = (void *) ((char *) pc - 1);
                }
            }
          else
            {
              pc_array[i] = NULL;
              mpiPi_msg_debug ("unw_step failed.\n");
              valid_cursor = 0;
            }
        }
    }

  return frame_count;
}

#elif defined(OSF1)

int
mpiPi_RecordTraceBack (jmp_buf jb, void *pc_array[], int max_back)
{
  int i, parent_frame_start, frame_count = 0;
  void *pc;
  PCONTEXT context;

  context = (PCONTEXT) jb;

  if (mpiPi.inAPIrtb)		/*  API unwinds fewer frames  */
    parent_frame_start = 1;
  else
    parent_frame_start = 2;

  for (i = 1; i < parent_frame_start; i++)
    {
      pc = (void *) context->sc_pc;
      if (pc != NULL)
        unwind (context, 0);
    }

  for (i = 0; i < max_back; i++)
    {
      if (pc != NULL)
        {
          unwind (context, 0);

          /* record this frame's pc and calculate the previous instruction  */
          pc_array[i] = (void *) context->sc_pc - (void *) 0x4;
          pc = (void *) context->sc_pc;
          frame_count++;
        }
      else
        {
          pc_array[i] = NULL;
        }
    }

  return frame_count;
}

#elif defined(mips) && defined(__GNUC__)
  /* on the MIPS we use gcc's predefined macro to get a single level
   * of stack backtrace
   */
int
mpiPi_RecordTraceBack (void *pc, void *pc_array[], int max_back)
{
  pc_array[0] = (void *) ((char *) pc - 1);
  return 1;
}

#elif defined(USE_BACKTRACE)

#include <execinfo.h>

int
mpiPi_RecordTraceBack (jmp_buf jb, void *pc_array[], int max_back)
{
  static void *temp_stack_trace[MPIP_CALLSITE_STACK_DEPTH_MAX];
  int all_frame_count, user_frame_count;
  void **cp;

  /*  backtrace() will provide internal mpiP stack frames,
     including for this function.  

     libunwind functionality provides a stack trace beginning with the
     parent function, so we remove the current function from the
     stack trace for consistent behavior.

   */

  all_frame_count = backtrace (temp_stack_trace, max_back);

  if (all_frame_count <= MPIP_INTERNAL_STACK_DEPTH)
    return 0;

  memcpy(pc_array, temp_stack_trace+1, sizeof(void*)*(all_frame_count-1));
  pc_array[all_frame_count] = NULL;

  /* Subtract 1 pointer size from results to point to the address 
     of the calling instruction rather than the return address */
  for (cp = pc_array; cp != NULL && *cp != NULL; cp++)
    *cp = *cp - sizeof (cp);

  return all_frame_count;
}

#elif defined(USE_SETJMP)

int
mpiPi_RecordTraceBack (jmp_buf jb, void *pc_array[], int max_back)
{
  int i, frame_count = 0;
  void *fp, *lastfp;
  void *pc = (void *) 1;

  /*  
     For standard mpiP, the current stack looks like:

     mpiPi_RecordTraceBack
     mpiPif_MPI_Send
     MPI_Send/F77_MPI_Send  <- setjmp called
     Application function
     ...

     with the jmpbuf set in MPI_Send/F77_MPI_Send.

     For the mpiP-API, the current stack looks like:

     mpiPi_RecordTraceBack
     mpiP_record_traceback  <- setjmp called
     Application function
     ...

     with the jmpbuf set in mpiP_record_traceback
   */


#if !defined(UNICOS_mp)
  /*  This gets the FP for the Application function from
     the setjmp jmp_buf.  */
  fp = ParentFP (jb);
#else
  /*  For UNICOS, we first get the frame pointer for what would
     be the application function from an API call.
     If we are not in the API, we step through an additional frame.
   */
  fp = NextFP (GetFP ());

  if (!mpiPi.inAPIrtb)
    fp = NextFP (fp);

#endif /* defined(UNICOS_mp) */

  for (i = 0; i < max_back; i++)
    {
      if (fp != NULL && pc != NULL)
        {
          /* record this frame's pc */
          pc = FramePC (fp);
          if (pc != NULL)
            {
              frame_count++;
              /*  Get previous instruction  */
              pc_array[i] = (void *) ((char *) pc - 1);
            }
          else
            pc_array[i] = NULL;

          /* update frame ptr */
          lastfp = fp;
          fp = NextFP (fp);
        }
      else
        {
          /* pad the array w/ nulls, if necessary */
          pc_array[i] = NULL;
        }
    }

  return frame_count;
}
#else /* Could not identify stack tracing mechanism */

int
mpiPi_RecordTraceBack (jmp_buf jb, void *pc_array[], int max_back)
{
  mpiPi_abort ("Failed to configure appropriate stack tracing mechanism.");
}
#endif



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
