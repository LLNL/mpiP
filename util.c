/* -*- C -*- 

   mpiP MPI Profiler ( http://llnl.github.io/mpiP )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   -----

   util.c -- misc util functions

 */

#ifndef lint
static char *svnid = "$Id$";
#endif

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "mpiPi.h"
#ifndef ENABLE_API_ONLY
#include "symbols.h"
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


void
mpiPi_getenv ()
{  /* NEED TO ADD SANITY CHECKS HERE */
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
      char *sep = " \t,";

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

      for (; ((c = getopt (ac, av, "cdef:gk:lm:noprs:t:vx:yz")) != EOF);)
        {
          switch (c)
            {
            case 'f':
              mpiPi.outputDir = optarg;
              if (mpiPi.rank == 0)
                mpiPi_msg ("Set the output directory to [%s].\n",
                           mpiPi.outputDir);
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
                mpiPi.reportStackDepth = atoi (optarg);
                if (mpiPi.reportStackDepth < 0)
                  {
                    if (mpiPi.rank == 0)
                      mpiPi_msg_warn
                          ("-k stackdepth invalid %d. Using 0.\n",
                           mpiPi.reportStackDepth);
                    mpiPi.reportStackDepth = 0;
                    mpiPi.print_callsite_detail = 0;
                  }
                if (mpiPi.reportStackDepth > MPIP_CALLSITE_REPORT_STACK_DEPTH_MAX)
                  {
                    if (mpiPi.rank == 0)
                      mpiPi_msg_warn
                          ("stackdepth of %d too large. Using %d.\n",
                           mpiPi.reportStackDepth, MPIP_CALLSITE_REPORT_STACK_DEPTH_MAX);
                    mpiPi.reportStackDepth = MPIP_CALLSITE_REPORT_STACK_DEPTH_MAX;
                  }
                else if (mpiPi.reportStackDepth > 4)
                  {
                    if (mpiPi.rank == 0)
                      mpiPi_msg_warn
                          ("stackdepth of %d is large. Consider making it smaller.\n",
                           mpiPi.reportStackDepth);
                  }

                //  If the stack depth is 0, we are accumulating data
                //  for each MPI op (i.e. potentially multiple callsites),
                //  resulting in data that would not be useful for calculating COV.
                if (mpiPi.reportStackDepth == 0)
                  mpiPi.calcCOV = 0;

                if (mpiPi.rank == 0)
                  mpiPi_msg
                      ("Set the callsite stack traceback depth to [%d].\n",
                       mpiPi.reportStackDepth);
              }
              mpiPi.fullStackDepth = mpiPi.reportStackDepth + mpiPi.internalStackDepth;

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
              /*  Suppress/Activate printing of call site detail based on default.  */
              mpiPi.print_callsite_detail ^= 1;
              break;

            case 'l':
              /*  Use low-memory use approach using MPI collectives
             for report generation   */
              mpiPi.collective_report = 1;
              break;

            case 'r':
              /*  Use collector task to aggregate all task data and
             generate report   */
              mpiPi.collective_report = 0;
              break;

            case 'z':
              mpiPi.disable_finalize_report = 1;
              break;

            case 'y':
              mpiPi.do_collective_stats_report = 1;
              break;

            case 'p':
              mpiPi.do_pt2pt_stats_report = 1;
              break;

            case 'a':
            case 'b':
            case 'h':
            case 'i':
            case 'j':
            case 'q':
            case 'u':
            case 'w':
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

#if !defined(USE_GETARG)
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
#endif /* USE_GETARG */

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
#ifdef Linux
      snprintf (test_buf, 8, "%p", (void *) 0x1);
#else
      snprintf (test_buf, 8, "%0p", (void *) 0x1);
#endif

      if (strcmp (test_buf, "0x1") != 0)
        strcpy (hex_prefix, "0x");

      get_sys_info = 1;
    }

#ifdef Linux
  sprintf (addr_buf, "%s%p", hex_prefix, pval);
#else
  sprintf (addr_buf, "%s%0.*p", hex_prefix, ptr_hex_chars, pval);
#endif

  return addr_buf;
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
