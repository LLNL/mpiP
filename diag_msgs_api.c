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

#include <stdarg.h>

#include "mpiPi.h"

void
mpiPi_msg (char *fmt, ...)
{
  va_list args;
  FILE *fp = mpiPi.stdout_;
  va_start (args, fmt);
  fprintf (fp, "%s: ", mpiPi.toolname);
  vfprintf (fp, fmt, args);
  va_end (args);
  fflush (fp);
}

void
mpiPi_abort (char *fmt, ...)
{
  va_list args;
  FILE *fp = mpiPi.stderr_;
  va_start (args, fmt);
  fprintf (fp, "\n\n%s: ABORTING: ", mpiPi.toolname);
  vfprintf (fp, fmt, args);
  va_end (args);
  fflush (fp);
  abort ();
}

void
mpiPi_msg_debug (char *fmt, ...)
{
  va_list args; FILE *fp = mpiPi.stdout_;

  if (mpiPi_debug <= 0)
    return;

  va_start (args, fmt);
  fprintf (fp, "%s: DBG: ", mpiPi.toolname);
  vfprintf (fp, fmt, args);
  va_end (args);
  fflush (fp);
}

void
mpiPi_msg_debug0 (char *fmt, ...)
{
  va_list args;
  FILE *fp = mpiPi.stdout_;
  if ( mpiPi.rank == 0 )
  {
    va_start (args, fmt);
    fprintf (fp, "%s: ", mpiPi.toolname);
    vfprintf (fp, fmt, args);
    va_end (args);
    fflush (fp);
  }
}

void
mpiPi_msg_warn (char *fmt, ...)
{
  va_list args;
  FILE *fp = mpiPi.stderr_;
  va_start (args, fmt);
  fprintf (fp, "%s: WARNING: ", mpiPi.toolname);
  vfprintf (fp, fmt, args);
  va_end (args);
  fflush (fp);
}

/* eof */


