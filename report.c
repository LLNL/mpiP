/* -*- C -*- 

   @PROLOGUE@

   ----- 

   Jeffrey Vetter vetter3@llnl.gov
   Center for Applied Scientific Computing, LLNL
   18 Oct 2000

   profiler.c -- profiling functions

*/

#ifndef lint
static char *rcsid =
  "$Header$";
#endif

#include <math.h>
#include <float.h>
#include <search.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "mpiPi.h"

#define icmp(a,b) (((a)<(b))?(-1):(((a)>(b))?(1):(0)))

static void
print_section_heading (FILE * fp, char *str)
{
  int slen;
  int maxlen = 75;
  assert (fp);
  assert (str);
  assert (maxlen > 0);

  slen = 0;
  for (; slen < maxlen; slen++)
    {
      fputc ('-', fp);
    }
  fprintf (fp, "\n");
  slen = strlen (str);
  fprintf (fp, "@--- %s ", str);
  slen += 6;
  for (; slen < maxlen; slen++)
    {
      fputc ('-', fp);
    }
  fprintf (fp, "\n");
  slen = 0;
  for (; slen < maxlen; slen++)
    {
      fputc ('-', fp);
    }
  fprintf (fp, "\n");
}

static int
callsite_src_id_cache_sort_by_id (const void *a, const void *b)
{
  callsite_src_id_cache_entry_t **a1 = (callsite_src_id_cache_entry_t **) a;
  callsite_src_id_cache_entry_t **b1 = (callsite_src_id_cache_entry_t **) b;

  if ((*a1)->id > (*b1)->id)
    {
      return 1;
    }
  if ((*a1)->id < (*b1)->id)
    {
      return -1;
    }
  return 0;
}

static int
callsite_sort_by_cumulative_time (const void *a, const void *b)
{
  callsite_stats_t **a1 = (callsite_stats_t **) a;
  callsite_stats_t **b1 = (callsite_stats_t **) b;

  /* NOTE: want descending sort, so compares are reveresed */
  if ((*a1)->cumulativeTime < (*b1)->cumulativeTime)
    {
      return 1;
    }
  if ((*a1)->cumulativeTime > (*b1)->cumulativeTime)
    {
      return -1;
    }
  return 0;
}

static int
callsite_sort_by_name_id_rank (const void *a, const void *b)
{
  int rc;
  callsite_stats_t *csp_1 = *(callsite_stats_t **) a;
  callsite_stats_t *csp_2 = *(callsite_stats_t **) b;

  MPIP_CALLSITE_STATS_COOKIE_ASSERT (csp_1);
  MPIP_CALLSITE_STATS_COOKIE_ASSERT (csp_2);

  if ((rc = strcmp (mpiPi.lookup[csp_1->op - mpiPi_BASE].name,
		    mpiPi.lookup[csp_2->op - mpiPi_BASE].name)) != 0)
    {
      return rc;
    }

#define express(f) {if ((csp_1->f) > (csp_2->f)) {return 1;} if ((csp_1->f) < (csp_2->f)) {return -1;}}
  express (csid);
  express (rank);
#undef express

  return 0;
}

#include <varargs.h>

/* proto -- print_intro_line(filep, name, "fmt to print value", args for fmt ) */
static void
print_intro_line (va_alist)
     va_dcl
{
  va_list args;
  FILE *fp = NULL;
  char *fmt = NULL;
  char *cp = "@";
  char *name = NULL;

  va_start (args);
  fp = va_arg (args, FILE *);
  name = va_arg (args, char *);
  fmt = va_arg (args, char *);

  fprintf (fp, "%-2s%-25s: ", cp, name);
  vfprintf (fp, fmt, args);
  fprintf (fp, "\n");

  va_end (args);
}

int
mpiPi_profile_print (FILE * fp)
{
  int i;
  char buf[256];

  /*
   * RecordIntroMaterial - Record all pertinant information to the final tracefile.
   */

  assert (fp);
  fprintf (fp, "@ %s\n", mpiPi.toolname);
  fprintf (fp, "@ Command : ");
  for (i = 0; i < mpiPi.ac; i++)
    {
      fprintf (fp, "%s ", mpiPi.av[i]);
    }
  fprintf (fp, "\n");
  print_intro_line (fp, "Version", "%d.%d", mpiPi_vmajor, mpiPi_vminor);
  print_intro_line (fp, "MPIP Build date", "%s, %s", mpiPi_vdate,
		    mpiPi_vtime);
  {
    char nowstr[128];
    const struct tm *nowstruct;
    char *fmtstr = "%Y %m %d %T";

    nowstruct = localtime (&mpiPi.start_timeofday);
    if (strftime (nowstr, 128, fmtstr, nowstruct) == (size_t) 0)
      printf ("Could not get string from strftime()\n");
    print_intro_line (fp, "Start time", "%s", nowstr);

    nowstruct = localtime (&mpiPi.stop_timeofday);
    if (strftime (nowstr, 128, fmtstr, nowstruct) == (size_t) 0)
      printf ("Could not get string from strftime()\n");
    print_intro_line (fp, "Stop time", "%s", nowstr);
  }

  print_intro_line (fp, "MPIP env var", "%s",
		    mpiPi.envStr == NULL ? "[null]" : mpiPi.envStr);
  print_intro_line (fp, "Collector Rank", "%d", mpiPi.collectorRank);
  print_intro_line (fp, "Collector PID", "%d", mpiPi.procID);

  print_intro_line (fp, "Final Output Dir", "%s", mpiPi.outputDir);

  for (i = 0; i < mpiPi.size; i++)
    {
      print_intro_line (fp, "MPI Task Assignment", "%d %s",
			mpiPi.global_task_info[i].rank,
			mpiPi.global_task_info[i].hostname);
    }
  fprintf (fp, "\n");


  /*****
   ***** Start the statistics dump
   *****/

  /* -- calc the app runtime of each task */
  mpiPi.global_app_time = 0;
  mpiPi.global_mpi_time = 0;
  for (i = 0; i < mpiPi.size; i++)
    {
      mpiPi_msg_debug ("app runtime for task %d is %g\n", i,
		       mpiPi.global_task_info[i].app_time);
      mpiPi.global_app_time += mpiPi.global_task_info[i].app_time;
      mpiPi.global_task_info[i].mpi_time = 0;
    }

  /* -- sweep across the callsite (task indep) list and accumulate
     each task's mpi time */
  {
    int ac;
    callsite_stats_t **av;
    h_gather_data (mpiPi.global_callsite_stats, &ac, (void ***) &av);
    for (i = 0; i < ac; i++)
      {
	mpiPi.global_task_info[av[i]->rank].mpi_time += av[i]->cumulativeTime;
	mpiPi.global_mpi_time += av[i]->cumulativeTime;
	mpiPi_msg_debug ("Callsite(%d,%d=[%s,%d,%s],%d) %g Cumulative=%g\n",
			 av[i]->op,
			 av[i]->csid,
			 av[i]->filename[0],
			 av[i]->lineno[0],
			 av[i]->functname[0],
			 av[i]->rank,
			 av[i]->cumulativeTime,
			 mpiPi.global_task_info[av[i]->rank].mpi_time);
      }
    free (av);
  }

  /* -- print the mpi/app times */
  {
    print_section_heading (fp, "MPI Time (seconds)");
    fprintf (fp, "%-4s %10s %10s    %5s\n", "Task", "AppTime", "MPITime",
	     "MPI%");
    for (i = 0; i < mpiPi.size; i++)
      {
	double ratio =
	  (100.0 * mpiPi.global_task_info[i].mpi_time / 1e6) /
	  (mpiPi.global_task_info[i].app_time);
	fprintf (fp, "%4d %10.3g %10.3g    %5.2lf\n", i,
		 mpiPi.global_task_info[i].app_time,
		 mpiPi.global_task_info[i].mpi_time / 1e6, ratio);
      }
    fprintf (fp, "   * %10.3g %10.3g    %5.2lf\n",
	     mpiPi.global_app_time,
	     mpiPi.global_mpi_time / 1e6,
	     (100.0 * mpiPi.global_mpi_time / 1e6) / mpiPi.global_app_time);
  }

  /* -- dump the source code locations */
  {
    int ac;
    callsite_src_id_cache_entry_t **av;

    h_gather_data (callsite_src_id_cache, &ac, (void ***) &av);
    sprintf (buf, "Callsites: %d", ac);
    qsort (av, ac, sizeof (void *), callsite_src_id_cache_sort_by_id);
    print_section_heading (fp, buf);
    fprintf (fp, "%3s %3s %-15s %4s %-25s %-25s\n",
	     "ID", "Lev", "File/Address", "Line", "Parent_Funct", "MPI_Call");

    for (i = 0; i < ac; i++)
      {
	int j;
	for (j = 0;
	     (j < MPIP_CALLSITE_STACK_DEPTH) && (av[i]->filename[j] != NULL);
	     j++)
	  {
            if ( av[i]->line[j] == 0 && 
                 (strcmp(av[i]->filename[j], "[unknown]") == 0 ||
                  strcmp(av[i]->functname[j], "[unknown]") == 0) )
              {
                fprintf (fp, "%3d %3d 0x%-18.*lx %-25s %s\n",
                         av[i]->id,
                         j,
                         sizeof(void*)*2,
                         av[i]->pc[j], 
                         av[i]->functname[j],
                         (j ==
                          0) ? &(mpiPi.lookup[av[i]->op -
                                              mpiPi_BASE].name[4]) : "");
              }
            else
              {
                fprintf (fp, "%3d %3d %-15.15s %4d %-25s %s\n",
                         av[i]->id,
                         j,
                         av[i]->filename[j], av[i]->line[j], 
                         av[i]->functname[j],
                         (j ==
                          0) ? &(mpiPi.lookup[av[i]->op -
                                              mpiPi_BASE].name[4]) : "");
              }
          }
      }
    free (av);
  }

  {
    int ac;
    callsite_stats_t **av;

    h_gather_data (mpiPi.global_callsite_stats_agg, &ac, (void ***) &av);

    /* -- now that we have all the statistics in a queue, which is
     * pointers to the data, we can sort it however we need to.
     */
    qsort (av, ac, sizeof (void *), callsite_sort_by_cumulative_time);

    print_section_heading (fp,
			   "Aggregate Time (top twenty, descending, milliseconds)");
    fprintf (fp, "%-20s %4s  %10s %6s  %6s\n", "Call", "Site", "Time  ",
	     "App%", "MPI%");

    for (i = 0; (i < 20) && (i < ac); i++)
      {
	fprintf (fp, "%-20s %4d %10.3g  %6.2lf  %6.2lf\n",
		 &(mpiPi.lookup[av[i]->op - mpiPi_BASE].name[4]), av[i]->csid,
		 av[i]->cumulativeTime / 1000.0,
		 100.0 * av[i]->cumulativeTime / (mpiPi.global_app_time *
						  1e6),
		 mpiPi.global_mpi_time > 0 ? 100.0 * av[i]->cumulativeTime / mpiPi.global_mpi_time : 0);
      }

    free (av);
  }

  {
    int ac;
    callsite_stats_t **av;

    h_gather_data (mpiPi.global_callsite_stats, &ac, (void ***) &av);

    /* -- now that we have all the statistics in a queue, which is
     * pointers to the data, we can sort it however we need to.
     */
    qsort (av, ac, sizeof (void *), callsite_sort_by_name_id_rank);

    sprintf (buf, "Callsite statistics (all, milliseconds): %d", ac);
    print_section_heading (fp, buf);
    fprintf (fp, "%-17s %4s %4s %6s %8s %8s %8s %6s %6s\n", "Name", "Site",
	     "Rank", "Count", "Max", "Mean", "Min", "App%", "MPI%");

    {
      long long sCount = 0;
      double sMin = DBL_MAX;
      double sMax = 0;
      double sCumulative = 0;

      for (i = 0; i < ac; i++)
	{
	  if (i != 0 && (av[i]->csid != av[i - 1]->csid))
	    {
	      fprintf (fp,
		       "%-17s %4d %4s %6lld %8.3g %8.3g %8.3g %6.2lf %6.2lf\n",
		       &(mpiPi.lookup[av[i - 1]->op - mpiPi_BASE].name[4]),
		       av[i - 1]->csid, "*", sCount, sMax / 1000.0,
		       sCumulative / (sCount * 1000.0), sMin / 1000.0,
		       100.0 * sCumulative / (mpiPi.global_app_time * 1e6),
		       100.0 * sCumulative / mpiPi.global_mpi_time);
	      fprintf (fp, "\n");
	      sCount = 0;
	      sMax = 0;
	      sMin = DBL_MAX;
	      sCumulative = 0;
	    }

	  sCount += av[i]->count;
	  sCumulative += av[i]->cumulativeTime;
	  sMax = max (av[i]->maxDur, sMax);
	  sMin = min (av[i]->minDur, sMin);

	  if (mpiPi.global_task_info[av[i]->rank].mpi_time != 0 &&
              (100.0 * av[i]->cumulativeTime /
	       mpiPi.global_task_info[av[i]->rank].mpi_time)
	      >= mpiPi.reportPrintThreshold)
	    {
	      fprintf (fp,
		       "%-17s %4d %4d %6lld %8.3g %8.3g %8.3g %6.2lf %6.2lf\n",
		       &(mpiPi.lookup[av[i]->op - mpiPi_BASE].name[4]),
		       av[i]->csid, av[i]->rank, av[i]->count,
		       av[i]->maxDur / 1000.0,
		       av[i]->cumulativeTime / (av[i]->count * 1000.0),
		       av[i]->minDur / 1000.0,
		       100.0 * av[i]->cumulativeTime /
		       (mpiPi.global_task_info[av[i]->rank].app_time * 1e6),
		       100.0 * av[i]->cumulativeTime /
		       mpiPi.global_task_info[av[i]->rank].mpi_time);
	    }
	}
      fprintf (fp, "%-17s %4d %4s %6lld %8.3g %8.3g %8.3g %6.2lf %6.2lf\n",
	       &(mpiPi.lookup[av[i - 1]->op - mpiPi_BASE].name[4]),
	       av[i - 1]->csid,
	       "*",
	       sCount,
	       sMax / 1000.0,
	       sCumulative / (sCount * 1000.0),
	       sMin / 1000.0,
	       100.0 * sCumulative /
	       (mpiPi.global_app_time * 1e6),
	       100.0 * sCumulative / mpiPi.global_mpi_time);
    }

    free (av);
  }

  print_section_heading (fp, "End of Report");

  return 0;
}

/* eof */
