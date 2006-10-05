/* -*- C -*- 

   mpiP MPI Profiler ( http://mpip.sourceforge.net/ )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   ----- 

   util.c -- misc util functions

 */

#ifndef lint
static char *svnid =
  "$Id$";
#endif

#include <string.h>
#include <setjmp.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#ifdef OSF1
#include <excpt.h>
#endif

#include "mpiPi.h"
#ifndef ENABLE_API_ONLY
#include "symbols.h"
#endif

#ifdef HAVE_LIBUNWIND
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#endif


static int argc = 0;
static char **argv = NULL;


char *
GetBaseAppName (char *rawName)
{
  char *cp;

  if (rawName == NULL)
    return strdup ("Unknown");

  /* delete directorys and make a copy, if nec */
  if ((cp = rindex (rawName, '/')) == NULL)
    {
      return rawName;
    }
  else
    {
      return cp + 1;		/* skip over the '/' */
    }
}

#ifdef HAVE_LIBUNWIND

int
mpiPi_RecordTraceBack (jmp_buf jb, void *pc_array[], int max_back)
{
  int i, valid_cursor, parent_frame_start, frame_count = 0;
  unw_context_t uc;
  unw_cursor_t cursor;
  unw_word_t pc;

  if (mpiPi.inAPIrtb)		/*  API unwinds fewer frames  */
    parent_frame_start = 1;
  else
    parent_frame_start = 2;

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
	  if (unw_step (&cursor))
	    mpiPi_msg_debug
	      ("unw_step failed to step into mpiPi caller frame.\n");
	}

      for (i = 0, valid_cursor = 1; i < max_back; i++)
	{
	  if (valid_cursor && unw_step (&cursor) > 0)
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

#else /* not LIBUNWIND */

#ifdef OSF1

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

#else /* not OSF1 */

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
#endif
#endif

void
mpiPi_getenv ()
{				/* NEED TO ADD SANITY CHECKS HERE */
  char *cp = NULL;
  char *ep = NULL;

  mpiPi.outputDir = ".";

  ep = getenv ("MPIP");
  mpiPi.envStr = (ep ? strdup (ep) : 0);
  optind = 1;			/* reset to avoid conflicts if getopt already called */
  if (ep != NULL)
    {
      int c;
      extern char *optarg;
      int ac = 0;
      char *av[64];
      char *sep = " \t";

      if (mpiPi.rank == 0)
	mpiPi_msg ("Found MPIP environment variable [%s]\n", ep);
      av[0] = "JUNK";
      for (cp = strtok (ep, sep), ac = 1; (ac < 64) && (NULL != cp); ac++)
	{
	  av[ac] = cp;
	  /*mpiPi_msg("av[%d] = %s\n", ac, av[ac]); */
	  cp = strtok (NULL, sep);
	}

      av[ac] = NULL;

      for (; ((c = getopt (ac, av, "cngf:b:s:k:t:oem:x:dv")) != EOF);)
	{
	  switch (c)
	    {
	    case 'f':
	      mpiPi.outputDir = optarg;
	      if (mpiPi.rank == 0)
		mpiPi_msg
		  ("Set the output directory to [%s].\n", mpiPi.outputDir);
	      break;

	    case 'g':
	      mpiPi_debug = 1;
	      if (mpiPi.rank == 0)
		mpiPi_msg ("Enabled mpiPi debug mode.\n");
	      break;

	    case 's':
	      {
		int defaultSize = mpiPi.tableSize;
		mpiPi.tableSize = atoi (optarg);
		if (mpiPi.tableSize < 2)
		  {
		    if (mpiPi.rank == 0)
		      mpiPi_msg_warn
			("-s tablesize invalid %d. Using default.\n",
			 mpiPi.tableSize);
		    mpiPi.tableSize = defaultSize;
		  }
		if (mpiPi.tableSize < 128)
		  {
		    if (mpiPi.rank == 0)
		      mpiPi_msg_warn
			("tablesize small %d. Consider making it larger w/ -s.\n",
			 mpiPi.tableSize);
		  }
		if (mpiPi.rank == 0)
		  mpiPi_msg
		    ("Set the callsite table size to [%d].\n",
		     mpiPi.tableSize);
	      }
	      break;

	    case 'k':
	      {
		int defaultStackDepth = mpiPi.stackDepth;
		mpiPi.stackDepth = atoi (optarg);
		if (mpiPi.stackDepth < 1)
		  {
		    if (mpiPi.rank == 0)
		      mpiPi_msg_warn
			("-k stack depth invalid %d. Using default.\n",
			 mpiPi.stackDepth);
		    mpiPi.stackDepth = defaultStackDepth;
		  }
		if (mpiPi.stackDepth > 2)
		  {
		    if (mpiPi.rank == 0)
		      mpiPi_msg_warn
			("stackdepth large %d. Consider making it smaller.\n",
			 mpiPi.stackDepth);
		  }
		if (mpiPi.stackDepth > MPIP_CALLSITE_STACK_DEPTH_MAX)
		  {
		    if (mpiPi.rank == 0)
		      mpiPi_msg_warn
			("stackdepth too large %d. Making it smaller %d.\n",
			 mpiPi.stackDepth);
		    mpiPi.stackDepth = MPIP_CALLSITE_STACK_DEPTH_MAX;
		  }
		if (mpiPi.rank == 0)
		  mpiPi_msg
		    ("Set the callsite stack traceback depth to [%d].\n",
		     mpiPi.stackDepth);
	      }
	      break;

	    case 't':
	      {
		int defaultThreshold = mpiPi.reportPrintThreshold;
		mpiPi.reportPrintThreshold = atof (optarg);
		if (mpiPi.reportPrintThreshold < 0)
		  {
		    if (mpiPi.rank == 0)
		      mpiPi_msg_warn
			("-t report print threshold invalid %g. Using default.\n",
			 mpiPi.reportPrintThreshold);
		    mpiPi.reportPrintThreshold = defaultThreshold;
		  }
		if (mpiPi.reportPrintThreshold >= 100)
		  {
		    if (mpiPi.rank == 0)
		      mpiPi_msg_warn
			("report print threshold large %g. Making it default.\n",
			 mpiPi.reportPrintThreshold);
		    mpiPi.reportPrintThreshold = defaultThreshold;
		  }
		if (mpiPi.rank == 0)
		  mpiPi_msg
		    ("Set the report print threshold to [%3.2lf%%].\n",
		     mpiPi.reportPrintThreshold);
	      }
	      break;

	    case 'o':
	      {
		if (mpiPi.rank == 0)
		  mpiPi_msg_warn
		    ("Disabling MPIP at Init. Code must use Pcontrol to enable.\n");
		mpiPi.enabled = 0;
		mpiPi.enabledCount = 0;
	      }
	      break;

	    case 'n':
	      mpiPi.baseNames = 1;
	      break;

	    case 'e':
	      mpiPi.reportFormat = MPIP_REPORT_FLT_FORMAT;
	      break;

	    case 'c':
	      mpiPi.report_style = mpiPi_style_concise;
	      break;

	    case 'v':
	      mpiPi.report_style = mpiPi_style_both;
	      break;

	    case 'm':
	      mpiPi.messageCountThreshold = atoi (optarg);
	      mpiPi_msg_debug ("Set messageCountThreshold to %d\n",
			       mpiPi.messageCountThreshold);
	      break;
	    case 'x':
	      if (optarg != NULL)
		{
		  mpiPi.appFullName = strdup (optarg);
		  mpiPi.av[0] = strdup (optarg);
		  mpiPi.appName = strdup (GetBaseAppName (mpiPi.appFullName));
		  mpiPi_msg_debug ("Set mpiPi.appFullName to %s\n",
				   mpiPi.appFullName);
		}
	      break;
	    case 'd':
	      /*  Suppress printing of call site detail   */
	      mpiPi.print_callsite_detail = 0;
	      break;

	    case 'a':
	    case 'b':
	    case 'h':
	    case 'i':
	    case 'j':
	    case 'l':
	    case 'p':
	    case 'q':
	    case 'r':
	    case 'u':
	    case 'w':
	    case 'y':
	    case 'z':
	    default:
	      if (mpiPi.rank == 0)
		mpiPi_msg_warn
		  ("Option flag (-%c) not recognized. Ignored.\n", c);
	      break;
	    }
	}
    }
  if (mpiPi.rank == 0)
    mpiPi_msg ("\n");
  optind = 1;			/* reset to avoid conflicts if getopt called again */
}


#ifdef Linux
char *
getProcExeLink ()
{
  int pid, exelen, insize = 256;
  char *inbuf = NULL, file[256];

  pid = getpid ();
  snprintf (file, 256, "/proc/%d/exe", pid);
  inbuf = malloc (insize);
  if (inbuf == NULL)
    {
      mpiPi_abort ("unable to allocate space for full executable path.\n");
    }

  exelen = readlink (file, inbuf, 256);
  if (exelen == -1)
    {
      if (errno != ENOENT)
	{
	  while (exelen == -1 && errno == ENAMETOOLONG)
	    {
	      insize += 256;
	      inbuf = realloc (inbuf, insize);
	      exelen = readlink (file, inbuf, insize);
	    }
	  inbuf[exelen] = '\0';
	  return inbuf;
	}
      else
	free (inbuf);
    }
  else
    {
      inbuf[exelen] = '\0';
      return inbuf;
    }
  return NULL;
}

#define MPIP_MAX_ARG_STRING_SIZE 4096
void
getProcCmdLine (int *ac, char **av)
{
  int i = 0, pid;
  char *inbuf, file[256];
  FILE *infile;
  char *arg_ptr;

  *ac = 0;
  *av = NULL;

  pid = getpid ();
  snprintf (file, 256, "/proc/%d/cmdline", pid);
  infile = fopen (file, "r");

  if (infile != NULL)
    {
      while (!feof (infile))
	{
	  inbuf = malloc (MPIP_MAX_ARG_STRING_SIZE);
	  if (fread (inbuf, 1, MPIP_MAX_ARG_STRING_SIZE, infile) > 0)
	    {
	      arg_ptr = inbuf;
	      while (*arg_ptr != '\0')
		{
		  av[i] = strdup (arg_ptr);
		  arg_ptr += strlen (av[i]) + 1;
		  i++;
		}
	    }
	}
      *ac = i;

      free (inbuf);
      fclose (infile);
    }
}

#endif

void
mpiPi_copy_args (int *ac, char **av, int av_len)
{
  assert (ac != NULL);
  assert (av != NULL);

#if defined(AIX)
  {
    /*  Works for C/C++ and Fortran  */
    extern int p_xargc;
    extern char **p_xargv;
    argc = p_xargc;
    argv = p_xargv;
  }
#elif defined(GNU_Fortran)
  {
    extern int f__xargc;
    extern char **f__xargv;
    argc = f__xargc;
    argv = f__xargv;
  }
#elif defined(OSF1)
  {
    /*  Works for C/C++ and Fortran  */
    extern int __Argc;
    extern char **__Argv;
    argc = __Argc;
    argv = __Argv;
  }
#elif defined(PGI)
  {
    extern int __argc_save;
    extern char **__argv_save;
    argc = __argc_save;
    argv = __argv_save;
  }
#endif

  mpiPi_copy_given_args (ac, av, av_len, argc, argv);
}


void
mpiPi_copy_given_args (int *ac, char **av, int av_len, int argc, char **argv)
{
  int i;

  assert (ac != NULL);
  assert (av != NULL);

#if defined(USE_GETARG)

#define EXECUTABLE_LEN 2048
  {
    char buf[EXECUTABLE_LEN];
    int len, mpiPi_argc;
    extern void F77_MPIPI_GET_FORTRAN_ARGC (int *);

    F77_MPIPI_GET_FORTRAN_ARGC (&mpiPi_argc);
    mpiPi_argc++;
    *ac = mpiPi_argc;

    for (i = 0; i < mpiPi_argc; i++)
      {
	int buf_len = EXECUTABLE_LEN;

	extern void F77_MPIPI_GET_FORTRAN_ARG (int *, int *, char *, int *,
					       int);
	F77_MPIPI_GET_FORTRAN_ARG (&i, &buf_len, buf, &len, EXECUTABLE_LEN);

	buf[len < EXECUTABLE_LEN ? len : EXECUTABLE_LEN - 1] = 0;
	av[i] = strdup (buf);
      }
  }

#else /* USE_GETARG */
  *ac = argc;

  for (i = 0; (i < *ac) && (i < av_len); i++)
    av[i] = strdup (argv[i]);
#endif
}


char *
mpiP_format_address (void *pval, char *addr_buf)
{
  static int get_sys_info = 0;
  static int ptr_hex_chars = 0;
  static char hex_prefix[3] = "";
  char test_buf[8] = "";

  if (get_sys_info == 0)
    {
      ptr_hex_chars = sizeof (char *) * 2;
      snprintf (test_buf, 8, "%0p", (void *) 0x1);

      if (strcmp (test_buf, "0x1") != 0)
	strcpy (hex_prefix, "0x");

      get_sys_info = 1;
    }

  sprintf (addr_buf, "%s%0.*p", hex_prefix, ptr_hex_chars, pval);

  return addr_buf;
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


/* eof */
