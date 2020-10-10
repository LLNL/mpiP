/* -*- C -*- 

   mpiP MPI Profiler ( http://llnl.github.io/mpiP )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   ----- 

   wrappers_special.c -- wrappers that demand special attention

 */

#ifndef lint
static char *svnid =
  "$Id$";
#endif

#include "mpiPconfig.h"
#include "mpiPi.h"
#include "symbols.h"
#ifdef ENABLE_FORTRAN_WEAK_SYMS
#include "weak-symbols-special.h"
#endif
#include <string.h>


/* ----- INIT -------------------------------------------------- */

static int
_MPI_Init (int *argc, char ***argv)
{
  int rc = 0;
  int enabledStatus;

  enabledStatus = mpiPi.enabled;
  mpiPi.enabled = 0;

  rc = PMPI_Init (argc, argv);

  mpiPi.enabled = enabledStatus;

#if defined(Linux) && ! defined(ppc64)
  mpiPi.appFullName = getProcExeLink ();
  mpiPi_msg_debug ("appFullName is %s\n", mpiPi.appFullName);
  mpiPi_init (GetBaseAppName (mpiPi.appFullName), MPIPI_MODE_ST);
#else
  if (argv != NULL && *argv != NULL && **argv != NULL)
    {
      mpiPi_init (GetBaseAppName (**argv), MPIPI_MODE_ST);
      mpiPi.appFullName = strdup (**argv);
    }
  else
    {
      mpiPi_init ("Unknown", MPIPI_MODE_ST);
      mpiPi_msg_debug ("argv is NULL\n");
    }
#endif

  return rc;
}

extern int
MPI_Init (int *argc, char ***argv)
{
  int rc = 0;

  mpiPi.toolname = "mpiP";

  rc = _MPI_Init (argc, argv);

  if (argc != NULL && argv != NULL)
    mpiPi_copy_given_args (&(mpiPi.ac), mpiPi.av,
            MPIP_COPIED_ARGS_MAX, *argc, *argv);
  else
    {
#ifdef Linux
      getProcCmdLine (&(mpiPi.ac), mpiPi.av);
#else
      mpiPi.ac = 0;
#endif
    }

  return rc;
}

extern void
F77_MPI_INIT (int *ierr)
{
  int rc = 0;
  char **tmp_argv;

  mpiPi.toolname = "mpiP";
#ifdef Linux
  getProcCmdLine (&(mpiPi.ac), mpiPi.av);
#else
  mpiPi_copy_args (&(mpiPi.ac), mpiPi.av, MPIP_COPIED_ARGS_MAX);
#endif

  tmp_argv = mpiPi.av;
  rc = _MPI_Init (&(mpiPi.ac), (char ***) &tmp_argv);
  *ierr = rc;

  return;
}


/* ----- INIT_thread -------------------------------------------------- */

static int
_MPI_Init_thread (int *argc, char ***argv, int required, int *provided)
{
  int rc = 0;
  int enabledStatus;
  mpiPi_thr_mode_t mode = MPIPI_MODE_ST;

  enabledStatus = mpiPi.enabled;
  mpiPi.enabled = 0;

  rc = PMPI_Init_thread (argc, argv, required, provided);
  if (MPI_THREAD_MULTIPLE == *provided) {
      mode = MPIPI_MODE_MT;
  }

  mpiPi.enabled = enabledStatus;

#if defined(Linux) && ! defined(ppc64)
  mpiPi.appFullName = getProcExeLink ();
  mpiPi_msg_debug ("appFullName is %s\n", mpiPi.appFullName);
  mpiPi_init (GetBaseAppName (mpiPi.appFullName), mode);
#else
  if (argv != NULL && *argv != NULL && **argv != NULL)
    {
      mpiPi_init (GetBaseAppName (**argv), mode);
      mpiPi.appFullName = strdup (**argv);
    }
  else
    {
      mpiPi_init ("Unknown", MPIPI_MODE_MT);
      mpiPi_msg_debug ("argv is NULL\n");
    }
#endif

  return rc;
}

extern int
MPI_Init_thread (int *argc, char ***argv, int required, int *provided)
{
  int rc = 0;

  mpiPi.toolname = "mpiP";

  rc = _MPI_Init_thread (argc, argv, required, provided);

  if (argc != NULL && argv != NULL)
    mpiPi_copy_given_args (&(mpiPi.ac), mpiPi.av,
            MPIP_COPIED_ARGS_MAX, *argc, *argv);
  else
    {
#ifdef Linux
      getProcCmdLine (&(mpiPi.ac), mpiPi.av);
#else
      mpiPi.ac = 0;
#endif
    }

  return rc;
}

extern void
F77_MPI_INIT_THREAD (int *required, int *provided, int *ierr)
{
  int rc = 0;
  char **tmp_argv;

  mpiPi.toolname = "mpiP";
#ifdef Linux
  getProcCmdLine (&(mpiPi.ac), mpiPi.av);
#else
  mpiPi_copy_args (&(mpiPi.ac), mpiPi.av, MPIP_COPIED_ARGS_MAX);
#endif

  tmp_argv = mpiPi.av;
  rc =
    _MPI_Init_thread (&(mpiPi.ac), (char ***) &tmp_argv, *required, provided);
  *ierr = rc;

  return;
}


/* ----- FINALIZE -------------------------------------------------- */

static int
_MPI_Finalize ()
{
  int rc = 0;

  mpiPi_finalize ();
  mpiPi.enabled = 0;
  mpiPi_msg_debug ("calling PMPI_Finalize\n");
  rc = PMPI_Finalize ();
  mpiPi_msg_debug ("returning from PMPI_Finalize\n");

  return rc;
}

extern int
MPI_Finalize (void)
{
  int rc = 0;

  rc = _MPI_Finalize ();

  return rc;
}

extern void
F77_MPI_FINALIZE (int *ierr)
{
  int rc = 0;

  rc = _MPI_Finalize ();
  *ierr = rc;

  return;
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
