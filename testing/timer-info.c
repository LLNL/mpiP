
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
