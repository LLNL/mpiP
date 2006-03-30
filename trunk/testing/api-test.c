
#include <stdio.h>
#include <stdlib.h>
#include "mpiP-API.h"
#include "mpiPconfig.h"

#define MAX_STACK 16
#define MAX_STRING 256

void
thisIsTheTargetFunction ()
{
  void *pc_array[MAX_STACK] = { NULL };
  char *file, *func;
  int i, line_no, frame_count;
  char addr_buf[24];

  file = malloc (MAX_STRING);
  func = malloc (MAX_STRING);

  frame_count = mpiP_record_traceback (pc_array, MAX_STACK);
  printf ("frame_count is %d\n", frame_count);
  for (i = 0; i < MAX_STACK && pc_array[i] != NULL; i++)
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
}

void
thisIsTheNextLevelFunction ()
{
  thisIsTheTargetFunction ();
}

void
thisIsTheTopLevelFunction ()
{
  thisIsTheNextLevelFunction ();
}

main (int argc, char *argv[])
{
  char *infile;
  mpiP_TIMER start, end;

  start = mpiP_gettime ();
  infile = mpiP_get_executable_name ();

  if (infile == NULL || mpiP_open_executable (infile) != 0)
    {
      fprintf (stderr, "failed to open %s\n", infile);
      exit (1);
    }

  fprintf (stderr, "opened executable file\n");

  thisIsTheTopLevelFunction ();
  fprintf (stderr, "finished function test\n");

  mpiP_close_executable ();
  end = mpiP_gettime ();
  fprintf (stderr, "start is     %11.9f\n", start);
  fprintf (stderr, "end   is     %11.9f\n", end);
  fprintf (stderr, "time diff is %11.9f\n", (end - start) / 1000000);
}
