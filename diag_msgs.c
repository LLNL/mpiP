/* -*- C -*- 

   @PROLOGUE@

   ----- 

   Jeffrey Vetter vetter3@llnl.gov
   Center for Applied Scientific Computing, LLNL
   07 Feb 2001

   diag.c -- diagnostic routines, error logging, warning, etc

 */

#ifndef lint
static char *rcsid =
  "$Header$";
#endif

#include <varargs.h>

#include "mpiPi.h"


void
mpiPi_msg (va_alist)
     va_dcl
{
  va_list args;
  FILE *fp = mpiPi.stdout_;
  char *fmt = NULL;
  va_start (args);
  fmt = va_arg (args, char *);
  fprintf (fp, "%s: ", mpiPi.toolname);
  vfprintf (fp, fmt, args);
  va_end (args);
  fflush (fp);
}

void
mpiPi_abort (va_alist)
     va_dcl
{
  va_list args;
  FILE *fp = mpiPi.stderr_;
  char *fmt = NULL;
  va_start (args);
  fprintf (fp, "\n\n%s: ABORTING: ", mpiPi.toolname);
  fmt = va_arg (args, char *);
  vfprintf (fp, fmt, args);
  va_end (args);
  fflush (fp);
  PMPI_Abort (mpiPi.comm, -1);
}

void
mpiPi_msg_debug (va_alist)
     va_dcl
{
  va_list args;
  FILE *fp = mpiPi.stdout_;
  char *fmt = NULL;

  if (mpiPi_debug <= 0)
    return;

  va_start (args);
  fmt = va_arg (args, char *);
  fprintf (fp, "%s: DBG: ", mpiPi.toolname);
  vfprintf (fp, fmt, args);
  va_end (args);
  fflush (fp);
}

void
mpiPi_msg_warn (va_alist)
     va_dcl
{
  va_list args;
  FILE *fp = mpiPi.stderr_;
  char *fmt = NULL;
  va_start (args);
  fmt = va_arg (args, char *);
  fprintf (fp, "%s: WARNING: ", mpiPi.toolname);
  vfprintf (fp, fmt, args);
  va_end (args);
  fflush (fp);
}

/* eof */
