/* -*- C -*-

   @PROLOGUE@

   -----

   Chris Chambreau chcham@llnl.gov
   Integrated Computing and Communications, LLNL
   22 OCT 2004

   mpiP-API.c -- mpiP API functions

 */


#include <stdio.h>
#include <setjmp.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "mpiPi.h"
#include "mpiPi_proto.h"
#include "mpiP-API.h"

static int mpiP_api_init = 0;

void
mpiP_init_api ()
{
  char *mpiP_env;

  mpiP_env = getenv ("MPIP");
  if (mpiP_env != NULL && strstr (mpiP_env, "-g") != NULL)
    mpiPi_debug = 1;
  else
    mpiPi_debug = 0;

  mpiPi.stdout_ = stdout;
  mpiPi.stderr_ = stderr;
  mpiP_api_init = 1;
  mpiPi.toolname = "mpiP-API";
  mpiPi.inAPIrtb = 0;
}


int
mpiP_record_traceback (void *pc_array[], int max_stack)
{
  jmp_buf jb;
  int retval;

  if (mpiP_api_init == 0)
    mpiP_init_api ();

  setjmp (jb);

  mpiPi.inAPIrtb = 1;		/*  Used to correctly identify caller FP  */

  retval = mpiPi_RecordTraceBack (jb, pc_array, max_stack);

  mpiPi.inAPIrtb = 0;

  return retval;
}

extern int
mpiP_find_src_loc (void *i_addr_hex, char **o_file_str, int *o_lineno,
		   char **o_funct_str);

int
mpiP_open_executable (char *filename)
{
  if (mpiP_api_init == 0)
    mpiP_init_api ();

  if (access (filename, R_OK | F_OK) != 0)
    return -1;

#ifndef DISABLE_BFD

  open_bfd_executable (filename);

#elif defined(USE_LIBDWARF)

  open_dwarf_executable (filename);

#endif

  return 0;
}


void
mpiP_close_executable ()
{
#ifndef DISABLE_BFD
  close_bfd_executable ();
#elif defined(USE_LIBDWARF)
  close_dwarf_executable ();
#endif
}

/*  Returns current time in usec  */
mpiP_TIMER
mpiP_gettime ()
{
  mpiPi_TIME currtime;

  mpiPi_GETTIME (&currtime);

  return mpiPi_GETUSECS (&currtime);
}

char *
mpiP_get_executable_name ()
{
  int ac;
  char *av[1];

#ifdef Linux
  return getProcExeLink ();
#else
  mpiPi_copy_args (&ac, av, 1);
  if (av[0] != NULL)
    return av[0];
#endif
  return NULL;
}
