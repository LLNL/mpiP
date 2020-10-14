/* -*- C -*- 

   mpiP MPI Profiler ( http://llnl.github.io/mpiP )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   ----- 

   report.c -- reporting functions

*/

#ifndef lint
static char *svnid = "$Id$";
#endif

#include <math.h>
#include <float.h>
#include <search.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "mpiPi.h"

#define icmp(a,b) (((a)<(b))?(-1):(((a)>(b))?(1):(0)))

/*  Structure used to track callsite statistics for the detail
 *  sections of the concise report.
 *  */
typedef struct _mpiPi_callsite_stats
{
  char *name;
  int site;
  long long count;
  double max;
  double min;
  double cumulative;
  int max_rnk;
  int min_rnk;
} mpiPi_callsite_summary_t;

static void print_section_heading (FILE * fp, char *str);
static int callsite_src_id_cache_sort_by_id (const void *a, const void *b);
static int callsite_sort_by_cumulative_time (const void *a, const void *b);
static int callsite_sort_by_cumulative_size (const void *a, const void *b);
static int callsite_sort_by_cumulative_io (const void *a, const void *b);
static int callsite_sort_by_cumulative_rma (const void *a, const void *b);
static int callsite_sort_by_name_id_rank (const void *a, const void *b);
static void print_intro_line (FILE * fp, char *name, char *fmt, ...);
static double calc_COV (double *data, int dataSize);
static void mpiPi_print_report_header (FILE * fp);
static void mpiPi_print_task_assignment (FILE * fp);
static void mpiPi_print_verbose_task_info (FILE * fp);
static void mpiPi_print_concise_task_info (FILE * fp);
static void mpiPi_print_callsites (FILE * fp);
static void mpiPi_print_top_time_sites (FILE * fp);
static void mpiPi_print_top_sent_sites (FILE * fp);
static void mpiPi_print_top_collective_sent_sites (FILE * fp);
static void mpiPi_print_top_pt2pt_sent_sites (FILE * fp);
static void mpiPi_print_top_io_sites (FILE * fp);
static void mpiPi_print_top_rma_sites (FILE * fp);
static void mpiPi_print_all_callsite_time_info (FILE * fp);
static int callsite_stats_sort_by_cumulative (mpiPi_callsite_summary_t * cs1,
                                              mpiPi_callsite_summary_t * cs2);
static void mpiPi_print_concise_callsite_time_info (FILE * fp);
static void mpiPi_print_callsite_sent_info (FILE * fp);
static void mpiPi_print_all_callsite_sent_info (FILE * fp);
static void mpiPi_print_concise_callsite_sent_info (FILE * fp);
static void mpiPi_print_all_callsite_io_info (FILE * fp);
static void mpiPi_print_all_callsite_rma_info (FILE * fp);
static void mpiPi_print_concise_callsite_io_info (FILE * fp);
static void mpiPi_print_concise_callsite_rma_info (FILE * fp);
static void mpiPi_coll_print_all_callsite_time_info (FILE * fp);
static void mpiPi_coll_print_concise_callsite_time_info (FILE * fp);
static void mpiPi_coll_print_concise_callsite_sent_info (FILE * fp);
static void mpiPi_coll_print_concise_callsite_io_info (FILE * fp);
static void mpiPi_coll_print_concise_callsite_rma_info (FILE * fp);
static void mpiPi_coll_print_all_callsite_sent_info (FILE * fp);
static void mpiPi_coll_print_all_callsite_io_info (FILE * fp);
static void mpiPi_coll_print_all_callsite_rma_info (FILE * fp);
extern void mpiPi_profile_print (FILE * fp, int report_style);
static void mpiPi_profile_print_concise (FILE * fp);
static void mpiPi_profile_print_verbose (FILE * fp);

static char *mpiP_Report_Formats[][2] = {
  {
   /*  MPIP_MPI_TIME_FMT  */
   "%4d %10.3g %10.3g    %5.2lf\n",
   "%4d %10.3f %10.3f    %5.2lf\n"},
  {
   /*  MPIP_MPI_TIME_SUMMARY_FMT  */
   "   * %10.3g %10.3g    %5.2lf\n",
   "   * %10.3f %10.3f    %5.2lf\n"},
  {
   /*  MPIP_AGGREGATE_TIME_FMT  */
   "%-20s %4d %10.3g  %6.2lf  %6.2lf %10lld\n",
   "%-20s %4d %10.3f  %6.2lf  %6.2lf %10lld\n"},
  {
   /*  MPIP_AGGREGATE_COV_TIME_FMT  */
   "%-20s %4d %10.3g  %6.2lf  %6.2lf %10lld %6.2lf\n",
   "%-20s %4d %10.3f  %6.2lf  %6.2lf %10lld %6.2lf\n",
   },
  {
   /*  MPIP_AGGREGATE_MESS_FMT  */
   "%-20s %4d %10lld %10.3g %10.3g %6.2lf\n",
   "%-20s %4d %10lld %10.3f %10.3f %6.2lf\n"},
  {
   /*  MPIP_AGGREGATE_IO_FMT  */
   "%-20s %4d %10lld %10.3g %10.3g %6.2lf\n",
   "%-20s %4d %10lld %10.3f %10.3f %6.2lf\n"},
  {
   /*  MPIP_CALLSITE_TIME_SUMMARY_FMT  */
   "%-17s %4d %4s %6lld %8.3g %8.3g %8.3g %6.2lf %6.2lf\n",
   "%-17s %4d %4s %6lld %8.3f %8.3f %8.3f %6.2lf %6.2lf\n"},
  {
   /*  MPIP_CALLSITE_TIME_RANK_FMT  */
   "%-17s %4d %4d %6lld %8.3g %8.3g %8.3g %6.2lf %6.2lf\n",
   "%-17s %4d %4d %6lld %8.3f %8.3f %8.3f %6.2lf %6.2lf\n"},
  {
   /*  MPIP_CALLSITE_MESS_SUMMARY_FMT  */
   "%-17s %4d %4s %7lld %9.4g %9.4g %9.4g %9.4g\n",
   "%-17s %4d %4s %7lld %9.4f %9.4f %9.4f %9.4f\n"},
  {
   /*  MPIP_CALLSITE_MESS_RANK_FMT  */
   "%-17s %4d %4d %7lld %9.4g %9.4g %9.4g %9.4g\n",
   "%-17s %4d %4d %7lld %9.4f %9.4f %9.4f %9.4f\n"},
  {
   /*  MPIP_CALLSITE_IO_SUMMARY_FMT  */
   "%-17s %4d %4s %7lld %9.4g %9.4g %9.4g %9.4g\n",
   "%-17s %4d %4s %7lld %9.4f %9.4f %9.4f %9.4f\n"},
  {
   /*  MPIP_CALLSITE_IO_RANK_FMT  */
   "%-17s %4d %4d %7lld %9.4g %9.4g %9.4g %9.4g\n",
   "%-17s %4d %4d %7lld %9.4f %9.4f %9.4f %9.4f\n"},
  {
   /*  MPIP_CALLSITE_TIME_CONCISE_FMT  */
   "%-17s %4d %7lld %9.4g %9.4g %9.4g %6d %6d\n",
   "%-17s %4d %7lld %9.4f %9.4f %9.4f %6d %6d\n"},
  {
   /*  MPIP_CALLSITE_MESS_CONCISE_FMT  */
   "%-17s %4d %7lld %9.4g %9.4g %9.4g %6d %6d\n",
   "%-17s %4d %7lld %9.4f %9.4f %9.4f %6d %6d\n"},
  {
   /*  MPIP_HISTOGRAM_FMT  */
   "%-20s %10.3g  %20s  %20s\n",
   "%-20s %10.3f  %20s  %20s\n"}
};

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
callsite_sort_by_cumulative_size (const void *a, const void *b)
{
  callsite_stats_t **a1 = (callsite_stats_t **) a;
  callsite_stats_t **b1 = (callsite_stats_t **) b;

  /* NOTE: want descending sort, so compares are reveresed */
  if ((*a1)->cumulativeDataSent < (*b1)->cumulativeDataSent)
    {
      return 1;
    }
  if ((*a1)->cumulativeDataSent > (*b1)->cumulativeDataSent)
    {
      return -1;
    }
  return 0;
}

static int
histogram_sort_by_value (const void *a, const void *b)
{
  double **a1 = (double **) a;
  double **b1 = (double **) b;

  /* NOTE: want descending sort, so compares are reveresed */
  if (**a1 < **b1)
    {
      return 1;
    }
  if (**a1 > **b1)
    {
      return -1;
    }
  return 0;
}

static int
callsite_sort_by_cumulative_io (const void *a, const void *b)
{
  callsite_stats_t **a1 = (callsite_stats_t **) a;
  callsite_stats_t **b1 = (callsite_stats_t **) b;

  /* NOTE: want descending sort, so compares are reveresed */
  if ((*a1)->cumulativeIO < (*b1)->cumulativeIO)
    {
      return 1;
    }
  if ((*a1)->cumulativeIO > (*b1)->cumulativeIO)
    {
      return -1;
    }
  return 0;
}

static int
callsite_sort_by_cumulative_rma (const void *a, const void *b)
{
  callsite_stats_t **a1 = (callsite_stats_t **) a;
  callsite_stats_t **b1 = (callsite_stats_t **) b;

  /* NOTE: want descending sort, so compares are reveresed */
  if ((*a1)->cumulativeRMA < (*b1)->cumulativeRMA)
    {
      return 1;
    }
  if ((*a1)->cumulativeRMA > (*b1)->cumulativeRMA)
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

#include <stdarg.h>

/* proto -- print_intro_line(filep, name, "fmt to print value", args for fmt ) */
static void
print_intro_line (FILE * fp, char *name, char *fmt, ...)
{
  va_list args;
  char *cp = "@";

  va_start (args, fmt);

  fprintf (fp, "%-2s%-25s: ", cp, name);
  vfprintf (fp, fmt, args);
  fprintf (fp, "\n");

  va_end (args);
}


static double
calc_COV (double *data, int dataSize)
{
  int idx;
  double tot, avrg, var, std;

  tot = 0.0;
  avrg = 0.0;
  var = 0.0;
  std = 0.0;

  if (dataSize <= 1)
    return 0;

  for (idx = 0; idx < dataSize; idx++)
    tot += data[idx];

  avrg = tot / dataSize;

  for (idx = 0; idx < dataSize; idx++)
    var += (data[idx] - avrg) * (data[idx] - avrg);

  if (avrg > 0 && dataSize > 1)
    {
      var /= dataSize - 1;
      std = sqrt (var);
      return std / avrg;
    }
  else
    return 0;
}


static void
mpiPi_print_report_header (FILE * fp)
{
  int i;
  fprintf (fp, "@ %s\n", mpiPi.toolname);
  fprintf (fp, "@ Command : ");
  for (i = 0; (i < mpiPi.ac) && (i < MPIP_COPIED_ARGS_MAX); i++)
    {
      fprintf (fp, "%s ", mpiPi.av[i]);
    }
  fprintf (fp, "\n");
  print_intro_line (fp, "Version", "%d.%d.%d", mpiPi_vmajor, mpiPi_vminor,
                    mpiPi_vpatch);
  print_intro_line (fp, "MPIP Build date", "%s, %s", mpiPi_vdate,
                    mpiPi_vtime);
  {
    char nowstr[128];
    const struct tm *nowstruct;
    char *fmtstr = "%Y %m %d %T";

    nowstruct = localtime (&mpiPi.start_timeofday);
    if (strftime (nowstr, 128, fmtstr, nowstruct) == (size_t) 0)
      mpiPi_msg_warn ("Could not get string from strftime()\n");
    print_intro_line (fp, "Start time", "%s", nowstr);

    nowstruct = localtime (&mpiPi.stop_timeofday);
    if (strftime (nowstr, 128, fmtstr, nowstruct) == (size_t) 0)
      mpiPi_msg_warn ("Could not get string from strftime()\n");
    print_intro_line (fp, "Stop time", "%s", nowstr);
  }

  print_intro_line (fp, "Timer Used", "%s", mpiPi_TIMER_NAME);
  print_intro_line (fp, "MPIP env var", "%s",
                    mpiPi.envStr == NULL ? "[null]" : mpiPi.envStr);
  print_intro_line (fp, "Collector Rank", "%d", mpiPi.collectorRank);
  print_intro_line (fp, "Collector PID", "%d", mpiPi.procID);

  print_intro_line (fp, "Final Output Dir", "%s", mpiPi.outputDir);
  print_intro_line (fp, "Report generation", "%s",
                    mpiPi.collective_report ==
                    0 ? "Single collector task" : "Collective");
}

static void
mpiPi_print_task_assignment (FILE * fp)
{
  int i;
  for (i = 0; i < mpiPi.size; i++)
    {
      print_intro_line (fp, "MPI Task Assignment", "%d %s",
                        i, mpiPi.global_task_hostnames[i]);
    }
}

static void
mpiPi_print_verbose_task_info (FILE * fp)
{
  int i;
  /* -- calc the app runtime of each task */
  mpiPi.global_app_time = 0.0;

  for (i = 0; i < mpiPi.size; i++)
    {
      mpiPi_msg_debug ("app runtime for task %d is %g\n", i,
                       mpiPi.global_task_app_time[i]);
      mpiPi.global_app_time += mpiPi.global_task_app_time[i];
      /*mpiPi.global_task_mpi_time[i] = 0; */
    }


  /* -- print the mpi/app times */
  print_section_heading (fp, "MPI Time (seconds)");
  fprintf (fp, "%-4s %10s %10s    %5s\n", "Task", "AppTime", "MPITime",
           "MPI%");
  for (i = 0; i < mpiPi.size; i++)
    {
      double ratio;

      if (mpiPi.global_task_app_time[i] > 0)
        {
          ratio = (100.0 * mpiPi.global_task_mpi_time[i] / 1e6) /
              mpiPi.global_task_app_time[i];
        }
      else
        ratio = 0;

      fprintf (fp,
               mpiP_Report_Formats[MPIP_MPI_TIME_FMT][mpiPi.reportFormat],
               i, mpiPi.global_task_app_time[i],
               mpiPi.global_task_mpi_time[i] / 1e6, ratio);
    }
  fprintf (fp,
           mpiP_Report_Formats[MPIP_MPI_TIME_SUMMARY_FMT][mpiPi.reportFormat],
           mpiPi.global_app_time, mpiPi.global_mpi_time / 1e6,
           mpiPi.global_app_time >
           0 ? (100.0 * mpiPi.global_mpi_time / 1e6) /
           mpiPi.global_app_time : 0);
}

static void
mpiPi_print_concise_task_info (FILE * fp)
{
  double min_app_time = DBL_MAX, min_mpi_time = DBL_MAX;
  double max_app_time = 0.0, max_mpi_time = 0.0;
  double tot_app_time = 0.0, tot_mpi_time = 0.0;
  double mean_app_time = 0.0, mean_mpi_time = 0.0;
  double var_app_time = 0.0, var_mpi_time = 0.0;

  int min_app_rank, min_mpi_rank, max_app_rank, max_mpi_rank;
  int colw = 10;
  int tcolw = 17;
  int pcolw = 6;
  int i;

  for (i = 0; i < mpiPi.size; i++)
    {
      if (mpiPi.global_task_app_time[i] < min_app_time)
        {
          min_app_time = mpiPi.global_task_app_time[i];
          min_app_rank = i;
        }

      if (mpiPi.global_task_app_time[i] > max_app_time)
        {
          max_app_time = mpiPi.global_task_app_time[i];
          max_app_rank = i;
        }

      tot_app_time += mpiPi.global_task_app_time[i];

      if (mpiPi.global_task_mpi_time[i] < min_mpi_time)
        {
          min_mpi_time = mpiPi.global_task_mpi_time[i];
          min_mpi_rank = i;
        }

      if (mpiPi.global_task_mpi_time[i] > max_mpi_time)
        {
          max_mpi_time = mpiPi.global_task_mpi_time[i];
          max_mpi_rank = i;
        }

      tot_mpi_time += mpiPi.global_task_mpi_time[i];
      mpiPi.global_app_time += mpiPi.global_task_app_time[i];
    }

  mean_app_time = tot_app_time / mpiPi.size;
  mean_mpi_time = tot_mpi_time / mpiPi.size;

  for (i = 0; i < mpiPi.size; i++)
    {
      var_app_time += pow (mean_app_time - mpiPi.global_task_app_time[i], 2);
      var_mpi_time += pow (mean_mpi_time - mpiPi.global_task_mpi_time[i], 2);
    }
  var_app_time /= mpiPi.size - 1;
  var_mpi_time /= mpiPi.size - 1;

  print_section_heading (fp, "Task Time Statistics (seconds)");
  fprintf (fp, "%*s %*s %*s %*s %*s %*s\n",
           colw, " ",
           tcolw, "AppTime",
           tcolw, "MPITime", pcolw, "MPI%", colw, "App Task", colw,
           "MPI Task");
  fprintf (fp, "%-*s %*f %*f %*s %*d %*d\n", colw, "Max", tcolw, max_app_time,
           tcolw, max_mpi_time / USECS, pcolw, "", colw, max_app_rank, colw,
           max_mpi_rank);
  fprintf (fp, "%-*s %*f %*f\n", colw, "Mean", tcolw, mean_app_time, tcolw,
           mean_mpi_time / USECS);
  fprintf (fp, "%-*s %*f %*f %*s %*d %*d\n", colw, "Min", tcolw, min_app_time,
           tcolw, min_mpi_time / USECS, pcolw, "", colw, min_app_rank, colw,
           min_mpi_rank);
  fprintf (fp, "%-*s %*f %*f\n", colw, "Stddev", tcolw, sqrt (var_app_time),
           tcolw, sqrt (var_mpi_time) / USECS);
  fprintf (fp, "%-*s %*f %*f %*.2f\n", colw, "Aggregate", tcolw, tot_app_time,
           tcolw, tot_mpi_time / USECS, pcolw,
           tot_mpi_time / USECS / tot_app_time * 100);
}


static void
mpiPi_print_callsites (FILE * fp)
{
  int i, ac;
  char buf[256];
  callsite_src_id_cache_entry_t **av;
  int fileLenMax = 18;
  int funcLenMax = 24;
  int stack_continue_flag;
  char addr_buf[24];

  /*  If stack depth is 0, call sites are really just MPI calls */
  if (mpiPi.reportStackDepth == 0)
    return;

  h_gather_data (callsite_src_id_cache, &ac, (void ***) &av);
  sprintf (buf, "Callsites: %d", ac);
  qsort (av, ac, sizeof (void *), callsite_src_id_cache_sort_by_id);
  print_section_heading (fp, buf);

  /* Find longest file and function names for formatting */
  for (i = 0; i < ac; i++)
    {
      int j, currlen;
      for (j = 0;
           (j < mpiPi.fullStackDepth) && (av[i]->filename[j] != NULL);
           j++)
        {
          currlen = strlen (av[i]->filename[j]);
          fileLenMax = currlen > fileLenMax ? currlen : fileLenMax;
          currlen = strlen (av[i]->functname[j]);
          funcLenMax = currlen > funcLenMax ? currlen : funcLenMax;
        }
    }

  fprintf (fp, "%3s %3s %-*s %5s %-*s %s\n",
           "ID", "Lev", fileLenMax, "File/Address", "Line", funcLenMax,
           "Parent_Funct", "MPI_Call");

  for (i = 0; i < ac; i++)
    {
      int j;
      char * display_op = NULL;
      int frames_printed = 0;

      for (j = 0, stack_continue_flag = 1;
           (frames_printed < mpiPi.reportStackDepth) && (av[i]->filename[j] != NULL) &&
           stack_continue_flag == 1; j++)
        {
            //  May encounter multiple "mpiP-wrappers.c" filename frames
            if ( strcmp(av[i]->filename[j], "mpiP-wrappers.c") == 0 )
                continue;

            if ( NULL == display_op)
                display_op = &(mpiPi.lookup[av[i]->op - mpiPi_BASE].name[4]);


          if (av[i]->line[j] == 0 &&
              (strcmp (av[i]->filename[j], "[unknown]") == 0 ||
               strcmp (av[i]->functname[j], "[unknown]") == 0))
            {
              fprintf (fp, "%3d %3d %-*s %-*s %s\n",
                       av[i]->id,
                       frames_printed,
                       fileLenMax + 6,
                       mpiP_format_address (av[i]->pc[j], addr_buf),
                       funcLenMax,
                       av[i]->functname[j],
                       display_op);
            }
          else
            {
              fprintf (fp, "%3d %3d %-*s %5d %-*s %s\n",
                       av[i]->id,
                       frames_printed,
                       fileLenMax,
                       av[i]->filename[j], av[i]->line[j],
                       funcLenMax,
                       av[i]->functname[j],
                       display_op);
            }
          /*  Do not bother printing stack frames above main   */
          if (strcmp (av[i]->functname[j], "main") == 0
              || strcmp (av[i]->functname[j], ".main") == 0
              || strcmp (av[i]->functname[j], "MAIN__") == 0)
            stack_continue_flag = 0;

          display_op = "";
          ++frames_printed;
        }
    }
  free (av);
}

static void
mpiPi_print_top_time_sites (FILE * fp)
{
  int i, ac;
  callsite_stats_t **av;
  double timeCOV;

  if (mpiPi.reportStackDepth > 0)
    h_gather_data (mpiPi.global_callsite_stats_agg, &ac, (void ***) &av);
  else
    h_gather_data (mpiPi.global_MPI_stats_agg, &ac, (void ***) &av);

  /* -- now that we have all the statistics in a queue, which is
   * pointers to the data, we can sort it however we need to.
   */
  qsort (av, ac, sizeof (void *), callsite_sort_by_cumulative_time);

  print_section_heading (fp,
                         "Aggregate Time (top twenty, descending, milliseconds)");

  if (mpiPi.calcCOV)
    {
      fprintf (fp, "%-20s %4s %12s%6s  %6s     %6s %6s\n", "Call", "Site",
               "Time  ", "App%", "MPI%", "Count", "COV");
    }
  else
    {
      fprintf (fp, "%-20s %4s %12s%6s  %6s     %6s\n", "Call", "Site", "Time  ",
               "App%", "MPI%", "Count");
    }

  for (i = 0; (i < 20) && (i < ac); i++)
    {
      if (av[i]->cumulativeTime > 0)
        {
          if (mpiPi.calcCOV)
            {
              timeCOV = calc_COV (av[i]->siteData, av[i]->siteDataIdx);

              fprintf (fp,
                  mpiP_Report_Formats[MPIP_AGGREGATE_COV_TIME_FMT]
                       [mpiPi.reportFormat],
                  &(mpiPi.lookup[av[i]->op - mpiPi_BASE].name[4]),
                  av[i]->csid, av[i]->cumulativeTime / 1000.0,
                  100.0 * av[i]->cumulativeTime /
                  (mpiPi.global_app_time * 1e6),
                  mpiPi.global_mpi_time >
                  0 ? (100.0 * av[i]->cumulativeTime /
                       mpiPi.global_mpi_time) : 0,
                  av[i]->count,
                  timeCOV);
            }
          else
            {
              fprintf (fp,
                  mpiP_Report_Formats[MPIP_AGGREGATE_TIME_FMT]
                       [mpiPi.reportFormat],
                  &(mpiPi.lookup[av[i]->op - mpiPi_BASE].name[4]),
                  av[i]->csid, av[i]->cumulativeTime / 1000.0,
                  mpiPi.global_app_time >
                  0 ? 100.0 * av[i]->cumulativeTime /
                      (mpiPi.global_app_time * 1e6) : 0,
                  mpiPi.global_mpi_time >
                  0 ? 100.0 * av[i]->cumulativeTime /
                      mpiPi.global_mpi_time : 0,
                  av[i]->count);
            }
        }
    }

  free (av);
}

static void
mpiPi_print_top_sent_sites (FILE * fp)
{
  int i, ac;
  callsite_stats_t **av;

  if (mpiPi.global_mpi_size > 0)
    {
      if (mpiPi.reportStackDepth > 0)
        h_gather_data (mpiPi.global_callsite_stats_agg, &ac, (void ***) &av);
      else
        h_gather_data (mpiPi.global_MPI_stats_agg, &ac, (void ***) &av);

      /* -- now that we have all the statistics in a queue, which is
       * pointers to the data, we can sort it however we need to.
       */
      qsort (av, ac, sizeof (void *), callsite_sort_by_cumulative_size);

      print_section_heading (fp,
                             "Aggregate Sent Message Size (top twenty, descending, bytes)");
      fprintf (fp, "%-20s %4s %10s %10s %10s %6s\n", "Call", "Site",
               "Count", "Total", "Avrg", "Sent%");

      for (i = 0; (i < 20) && (i < ac); i++)
        {
          if (av[i]->cumulativeDataSent > 0)
            {
              fprintf (fp,
                  mpiP_Report_Formats[MPIP_AGGREGATE_MESS_FMT]
                  [mpiPi.reportFormat],
                  &(mpiPi.lookup[av[i]->op - mpiPi_BASE].name[4]),
                  av[i]->csid, av[i]->count, av[i]->cumulativeDataSent,
                  av[i]->cumulativeDataSent / av[i]->count,
                  av[i]->cumulativeDataSent * 100 /
                  mpiPi.global_mpi_size);
            }
        }
      if (mpiPi.messageCountThreshold >= 0)
        {
          fprintf (fp,
                   "\nTotal send/collective operation calls >= %d bytes : %lld of %lld operations\n",
                   mpiPi.messageCountThreshold,
                   mpiPi.global_mpi_msize_threshold_count,
                   mpiPi.global_mpi_sent_count);
        }

      free (av);
    }
}

static void
mpiPi_print_top_collective_sent_sites (FILE * fp)
{
  int result_count, j, i;
  double **result_ptrs;
  int x, y, z;
  int matrix_size;
  char commbinbuf[32];
  char databinbuf[32];

  mpiPi_msg_debug ("In mpiPi_print_top_collective_sent_sites\n");

  matrix_size = MPIP_NFUNC * MPIP_COMM_HISTCNT * MPIP_SIZE_HISTCNT;

  result_ptrs = (double **) malloc (sizeof (double *) * matrix_size);

  result_count = 0;
  for (x = 0; x < MPIP_NFUNC; x++)
    for (y = 0; y < MPIP_COMM_HISTCNT; y++)
      for (z = 0; z < MPIP_SIZE_HISTCNT; z++)
        {
          if (mpiPi.coll_time_stats[x][y][z] > 0)
            {
              result_ptrs[result_count] = &mpiPi.coll_time_stats[x][y][z];
              result_count++;
            }
        }

  qsort (result_ptrs, result_count, sizeof (double *),
         histogram_sort_by_value);

  if (mpiPi.global_mpi_size > 0)
    {
      print_section_heading (fp,
                             "Aggregate Collective Time (top twenty, descending)");
      if (result_count == 0)
        {
          /* there were no collective operations within this phase */
          fprintf (fp, "No collective operations to report\n");
          goto done;
        }

      fprintf (fp, "%-20s %10s %21s %21s\n", "Call", "MPI Time %",
               "Comm Size", "Data Size");

      mpiPi_msg_debug ("Found max time of %6.3f at %p\n", *result_ptrs[0],
          result_ptrs[0]);

      j = 0;
      for (i = 0; (i < 20) && (i < result_count); i++)
        {
          /* Find location in matrix */
          for (x = 0; x < MPIP_NFUNC; x++)
            for (y = 0; y < MPIP_COMM_HISTCNT; y++)
              for (z = 0; z < MPIP_COMM_HISTCNT; z++)
                {
                  if (&mpiPi.coll_time_stats[x][y][z] == result_ptrs[j])
                    {
                      j++;
                      goto print;
                    }
                }

print:
          if (mpiPi.coll_time_stats[x][y][z] == 0)
            goto done;

          mpiPi_stats_mt_coll_binstrings(&mpiPi.task_stats, y, commbinbuf,
                                        z, databinbuf);

          fprintf (fp,
              mpiP_Report_Formats[MPIP_HISTOGRAM_FMT]
              [mpiPi.reportFormat],
              &(mpiPi.lookup[x].name[4]),
              mpiPi.coll_time_stats[x][y][z] / mpiPi.global_mpi_time *
              100, commbinbuf, databinbuf);
        }

    }

done:
  return;
}


static void
mpiPi_print_top_pt2pt_sent_sites (FILE * fp)
{
  int result_count, j, i;
  double **result_ptrs;
  int x, y, z;
  int matrix_size;
  char commbinbuf[32];
  char databinbuf[32];

  mpiPi_msg_debug ("In mpiPi_print_top_pt2pt_sent_sites\n");

  matrix_size = MPIP_NFUNC * MPIP_COMM_HISTCNT * MPIP_SIZE_HISTCNT;
  result_ptrs = (double **) malloc (sizeof (double *) * matrix_size);

  result_count = 0;
  for (x = 0; x < MPIP_NFUNC; x++)
    for (y = 0; y < MPIP_COMM_HISTCNT; y++)
      for (z = 0; z < MPIP_SIZE_HISTCNT; z++)
        {
          if (mpiPi.pt2pt_send_stats[x][y][z] > 0)
            {
              result_ptrs[result_count] = &mpiPi.pt2pt_send_stats[x][y][z];
              result_count++;
            }
        }

  qsort (result_ptrs, result_count, sizeof (double *),
         histogram_sort_by_value);

  if (mpiPi.global_mpi_size > 0)
    {
      print_section_heading (fp,
                             "Aggregate Point-To-Point Sent (top twenty, descending)");
      if (result_count == 0)
        {
          /* there were no point to point messages send in the current phase */
          fprintf (fp, "No point to point operations to report\n");
          goto done;
        }

      fprintf (fp, "%-20s %10s %21s %21s\n", "Call", "MPI Sent %",
               "Comm Size", "Data Size");

      mpiPi_msg_debug ("Found max sent of %6.3f at %p\n", *result_ptrs[0],
          result_ptrs[0]);

      j = 0;
      for (i = 0; (i < 20) && (i < result_count); i++)
        {
          /* Find location in matrix */
          for (x = 0; x < MPIP_NFUNC; x++)
            for (y = 0; y < MPIP_COMM_HISTCNT; y++)
              for (z = 0; z < MPIP_SIZE_HISTCNT; z++)
                {
                  if (&mpiPi.pt2pt_send_stats[x][y][z] == result_ptrs[j])
                    {
                      j++;
                      goto print;
                    }
                }

print:
          if (mpiPi.pt2pt_send_stats[x][y][z] == 0)
            goto done;

          mpiPi_stats_mt_pt2pt_binstrings(&mpiPi.task_stats,
                                           y, commbinbuf,
                                           z, databinbuf);

          fprintf (fp,
                   mpiP_Report_Formats[MPIP_HISTOGRAM_FMT]
                   [mpiPi.reportFormat],
              &(mpiPi.lookup[x].name[4]),
              (mpiPi.pt2pt_send_stats[x][y][z] * 100) /
              mpiPi.global_mpi_size, commbinbuf, databinbuf);
        }

    }

done:
  return;
}

static void
mpiPi_print_top_io_sites (FILE * fp)
{
  int i, ac;
  callsite_stats_t **av;

  /*  This function is only called by the collector, which is the only
   *  process that should have a valid global_mpi_io value.
   *  */
  if (mpiPi.global_mpi_io > 0)
    {
      if (mpiPi.reportStackDepth > 0)
        h_gather_data (mpiPi.global_callsite_stats_agg, &ac, (void ***) &av);
      else
        h_gather_data (mpiPi.global_MPI_stats_agg, &ac, (void ***) &av);

      /* -- now that we have all the statistics in a queue, which is
       * pointers to the data, we can sort it however we need to.
       */
      qsort (av, ac, sizeof (void *), callsite_sort_by_cumulative_io);

      print_section_heading (fp,
                             "Aggregate I/O Size (top twenty, descending, bytes)");
      fprintf (fp, "%-20s %4s %10s %10s %10s %6s\n", "Call", "Site",
               "Count", "Total", "Avrg", "I/O%");

      for (i = 0; (i < 20) && (i < ac); i++)
        {
          if (av[i]->cumulativeIO > 0)
            {
              fprintf (fp,
                       mpiP_Report_Formats[MPIP_AGGREGATE_IO_FMT]
                       [mpiPi.reportFormat],
                  &(mpiPi.lookup[av[i]->op - mpiPi_BASE].name[4]),
                  av[i]->csid, av[i]->count, av[i]->cumulativeIO,
                  av[i]->cumulativeIO / av[i]->count,
                  av[i]->cumulativeIO * 100 / mpiPi.global_mpi_io);
            }
        }

      free (av);
    }
}

static void
mpiPi_print_top_rma_sites (FILE * fp)
{
  int i, ac;
  callsite_stats_t **av;

  /*  This function is only called by the collector, which is the only
   *  process that should have a valid global_mpi_rma value.
   *  */
  if (mpiPi.global_mpi_rma > 0)
    {
      if (mpiPi.reportStackDepth > 0)
        h_gather_data (mpiPi.global_callsite_stats_agg, &ac, (void ***) &av);
      else
        h_gather_data (mpiPi.global_MPI_stats_agg, &ac, (void ***) &av);


      /* -- now that we have all the statistics in a queue, which is
       * pointers to the data, we can sort it however we need to.
       */
      qsort (av, ac, sizeof (void *), callsite_sort_by_cumulative_rma);

      print_section_heading (fp,
                             "Aggregate RMA Origin Size (top twenty, descending, bytes)");
      fprintf (fp, "%-20s %4s %10s %10s %10s %6s\n", "Call", "Site",
               "Count", "Total", "Avrg", "I/O%");

      for (i = 0; (i < 20) && (i < ac); i++)
        {
          if (av[i]->cumulativeRMA > 0)
            {
              fprintf (fp,
                       mpiP_Report_Formats[MPIP_AGGREGATE_IO_FMT]
                       [mpiPi.reportFormat],
                  &(mpiPi.lookup[av[i]->op - mpiPi_BASE].name[4]),
                  av[i]->csid, av[i]->count, av[i]->cumulativeRMA,
                  av[i]->cumulativeRMA / av[i]->count,
                  av[i]->cumulativeRMA * 100 / mpiPi.global_mpi_rma);
            }
        }

      free (av);
    }
}

static void
mpiPi_print_all_callsite_time_info (FILE * fp)
{
  int i, ac;
  char buf[256];
  callsite_stats_t **av;

  h_gather_data (mpiPi.global_callsite_stats, &ac, (void ***) &av);

  /* -- now that we have all the statistics in a queue, which is
   * pointers to the data, we can sort it however we need to.
   */
  qsort (av, ac, sizeof (void *), callsite_sort_by_name_id_rank);

  sprintf (buf, "Callsite Time statistics (all, milliseconds): %d", ac);
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
                     mpiP_Report_Formats[MPIP_CALLSITE_TIME_SUMMARY_FMT]
                     [mpiPi.reportFormat],
                &(mpiPi.lookup[av[i - 1]->op - mpiPi_BASE].name[4]),
                av[i - 1]->csid, "*", sCount, sMax / 1000.0,
                sCumulative / (sCount * 1000.0), sMin / 1000.0,
                mpiPi.global_app_time >
                0 ? 100.0 * sCumulative / (mpiPi.global_app_time *
                                           1e6) : 0,
                mpiPi.global_mpi_time >
                0 ? 100.0 * sCumulative / mpiPi.global_mpi_time : 0);
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

        if (mpiPi.global_task_mpi_time[av[i]->rank] != 0 &&
            (100.0 * av[i]->cumulativeTime /
             mpiPi.global_task_mpi_time[av[i]->rank])
            >= mpiPi.reportPrintThreshold)
          {
            fprintf (fp,
                     mpiP_Report_Formats[MPIP_CALLSITE_TIME_RANK_FMT]
                     [mpiPi.reportFormat],
                &(mpiPi.lookup[av[i]->op - mpiPi_BASE].name[4]),
                av[i]->csid, av[i]->rank, av[i]->count,
                av[i]->maxDur / 1000.0,
                av[i]->cumulativeTime / (av[i]->count * 1000.0),
                av[i]->minDur / 1000.0,
                100.0 * av[i]->cumulativeTime /
                (mpiPi.global_task_app_time[av[i]->rank] * 1e6),
                100.0 * av[i]->cumulativeTime /
                mpiPi.global_task_mpi_time[av[i]->rank]);
          }
      }
    fprintf (fp,
             mpiP_Report_Formats[MPIP_CALLSITE_TIME_SUMMARY_FMT]
             [mpiPi.reportFormat],
        &(mpiPi.lookup[av[i - 1]->op - mpiPi_BASE].name[4]),
        av[i - 1]->csid, "*", sCount, sMax / 1000.0,
        sCumulative / (sCount * 1000.0), sMin / 1000.0,
        mpiPi.global_app_time >
        0 ? 100.0 * sCumulative / (mpiPi.global_app_time * 1e6) : 0,
        mpiPi.global_mpi_time >
        0 ? 100.0 * sCumulative / mpiPi.global_mpi_time : 0);
  }

  free (av);
}

static int
callsite_stats_sort_by_cumulative (mpiPi_callsite_summary_t * cs1,
                                   mpiPi_callsite_summary_t * cs2)
{
  if (cs1->cumulative > cs2->cumulative)
    {
      return -1;
    }
  if (cs1->cumulative < cs2->cumulative)
    {
      return 1;
    }
  return 0;
}

static void
mpiPi_print_concise_callsite_time_info (FILE * fp)
{
  int i, ac, csidx = 0;
  char buf[256];
  callsite_stats_t **av;
  mpiPi_callsite_summary_t *callsite_stats = NULL;

  h_gather_data (mpiPi.global_callsite_stats, &ac, (void ***) &av);

  /* -- now that we have all the statistics in a queue, which is
   * pointers to the data, we can sort it however we need to.
   */
  qsort (av, ac, sizeof (void *), callsite_sort_by_name_id_rank);
  callsite_stats =
      (mpiPi_callsite_summary_t *) malloc (sizeof (mpiPi_callsite_summary_t) *
                                           callsite_src_id_cache->count);

  if (callsite_stats == NULL)
    {
      mpiPi_msg_warn
          ("Failed to allocate space for callsite time summary reporting\n");
      free (av);
      return;
    }

  {
    long long sCount = 0;
    double sMin = DBL_MAX;
    double sMax = 0;
    double sCumulative = 0;
    int max_rnk, min_rnk;

    for (i = 0; i < ac; i++)
      {
        if (i != 0 && (av[i]->csid != av[i - 1]->csid))
          {
            if (csidx >= callsite_src_id_cache->count)
              {
                mpiPi_msg_warn
                    ("Concise callsite time report encountered index out of bounds.\n");
                return;
              }
            callsite_stats[csidx].name =
                &(mpiPi.lookup[av[i - 1]->op - mpiPi_BASE].name[4]);
            callsite_stats[csidx].site = av[i - 1]->csid;
            callsite_stats[csidx].count = sCount;
            callsite_stats[csidx].max = sMax;
            callsite_stats[csidx].min = sMin;
            callsite_stats[csidx].cumulative = sCumulative;
            callsite_stats[csidx].max_rnk = max_rnk;
            callsite_stats[csidx].min_rnk = min_rnk;

            sCount = 0;
            sMax = 0;
            sMin = DBL_MAX;
            sCumulative = 0;
            csidx++;
          }

        sCount++;
        sCumulative += av[i]->cumulativeTime;
        if (av[i]->cumulativeTime > sMax)
          {
            sMax = av[i]->cumulativeTime;
            max_rnk = av[i]->rank;
          }
        if (av[i]->cumulativeTime < sMin)
          {
            sMin = av[i]->cumulativeTime;
            min_rnk = av[i]->rank;
          }
      }
    callsite_stats[csidx].name =
        &(mpiPi.lookup[av[i - 1]->op - mpiPi_BASE].name[4]);
    callsite_stats[csidx].site = av[i - 1]->csid;
    callsite_stats[csidx].count = sCount;
    callsite_stats[csidx].max = sMax;
    callsite_stats[csidx].min = sMin;
    callsite_stats[csidx].cumulative = sCumulative;
    callsite_stats[csidx].max_rnk = max_rnk;
    callsite_stats[csidx].min_rnk = min_rnk;

  }

  free (av);
  sprintf (buf, "Callsite Time statistics (all callsites, milliseconds): %d",
           csidx + 1);
  print_section_heading (fp, buf);
  fprintf (fp, "%-17s %4s %7s %9s %9s %9s %6s %6s\n", "Name", "Site",
           "Tasks", "Max", "Mean", "Min", "MaxRnk", "MinRnk");

  qsort (callsite_stats, csidx + 1, sizeof (mpiPi_callsite_summary_t),
         (int (*)(const void *, const void *))
         callsite_stats_sort_by_cumulative);
  for (i = 0; i <= csidx; i++)
    {
      fprintf (fp,
               mpiP_Report_Formats[MPIP_CALLSITE_TIME_CONCISE_FMT]
               [mpiPi.reportFormat], callsite_stats[i].name,
          callsite_stats[i].site, callsite_stats[i].count,
          callsite_stats[i].max / 1000.0,
          callsite_stats[i].cumulative / (callsite_stats[i].count *
                                          1000),
          callsite_stats[i].min / 1000.0, callsite_stats[i].max_rnk,
          callsite_stats[i].min_rnk);
    }
  free (callsite_stats);
}

static void
mpiPi_print_callsite_sent_info (FILE * fp)
{
  if (mpiPi.report_style == mpiPi_style_verbose)
    mpiPi_print_all_callsite_sent_info (fp);
  else if (mpiPi.report_style == mpiPi_style_concise)
    mpiPi_print_concise_callsite_sent_info (fp);
}


static void
mpiPi_print_all_callsite_sent_info (FILE * fp)
/*  Print Sent Data Section  */
{
  int i, ac;
  char buf[256];
  callsite_stats_t **av;

  if (mpiPi.global_mpi_size > 0)
    {
      h_gather_data (mpiPi.global_callsite_stats, &ac, (void ***) &av);

      /* -- now that we have all the statistics in a queue, which is
       * pointers to the data, we can sort it however we need to.
       */
      qsort (av, ac, sizeof (void *), callsite_sort_by_name_id_rank);

      sprintf (buf, "Callsite Message Sent statistics (all, sent bytes)");
      print_section_heading (fp, buf);
      fprintf (fp, "%-17s %4s %4s %7s %9s %9s %9s %9s\n", "Name", "Site",
               "Rank", "Count", "Max", "Mean", "Min", "Sum");

      {
        long long sCount = 0;
        double sMin = DBL_MAX;
        double sMax = 0;
        double sCumulative = 0;
        int lastcsid = 0;

        for (i = 0; i < ac; i++)
          {
            if (i != 0 && sCumulative > 0 && (av[i]->csid != av[i - 1]->csid))
              {
                fprintf (fp,
                         mpiP_Report_Formats[MPIP_CALLSITE_MESS_SUMMARY_FMT]
                         [mpiPi.reportFormat],
                    &(mpiPi.lookup[av[i - 1]->op - mpiPi_BASE].name[4]),
                    av[i - 1]->csid, "*", sCount, sMax,
                    sCumulative / sCount, sMin, sCumulative);

                sCount = 0;
                sMax = 0;
                sMin = DBL_MAX;
                sCumulative = 0;
              }

            if (av[i]->cumulativeDataSent > 0)
              {
                sCount += av[i]->count;
                sCumulative += av[i]->cumulativeDataSent;
                sMax = max (av[i]->maxDataSent, sMax);
                sMin = min (av[i]->minDataSent, sMin);

                if (lastcsid != 0 && lastcsid != av[i]->csid)
                  fprintf (fp, "\n");

                fprintf (fp,
                         mpiP_Report_Formats[MPIP_CALLSITE_MESS_RANK_FMT]
                         [mpiPi.reportFormat],
                    &(mpiPi.lookup[av[i]->op - mpiPi_BASE].name[4]),
                    av[i]->csid, av[i]->rank, av[i]->count,
                    av[i]->maxDataSent,
                    av[i]->cumulativeDataSent / av[i]->count,
                    av[i]->minDataSent, av[i]->cumulativeDataSent);

                lastcsid = av[i]->csid;
              }
          }

        if (sCumulative > 0)
          {
            fprintf (fp,
                     mpiP_Report_Formats[MPIP_CALLSITE_MESS_SUMMARY_FMT]
                     [mpiPi.reportFormat],
                &(mpiPi.lookup[av[i - 1]->op - mpiPi_BASE].name[4]),
                av[i - 1]->csid, "*", sCount, sMax,
                sCumulative / sCount, sMin, sCumulative);
          }
      }

      free (av);
    }
}


static void
mpiPi_print_concise_callsite_sent_info (FILE * fp)
{
  int i, ac, csidx = 0;
  char buf[256];
  callsite_stats_t **av;
  mpiPi_callsite_summary_t *callsite_stats = NULL;

  h_gather_data (mpiPi.global_callsite_stats, &ac, (void ***) &av);

  /* -- now that we have all the statistics in a queue, which is
   * pointers to the data, we can sort it however we need to.
   */
  qsort (av, ac, sizeof (void *), callsite_sort_by_name_id_rank);
  callsite_stats =
      (mpiPi_callsite_summary_t *) malloc (sizeof (mpiPi_callsite_summary_t) *
                                           callsite_src_id_cache->count);

  if (callsite_stats == NULL)
    {
      mpiPi_msg_warn
          ("Failed to allocate space for callsite volume summary reporting\n");
      free (av);
      return;
    }

  {
    long long sCount = 0;
    double sMin = DBL_MAX;
    double sMax = 0;
    double sCumulative = 0;
    int max_rnk = -1, min_rnk = -1;

    for (i = 0, csidx = 0; i < ac; i++)
      {
        if (i != 0 && (av[i]->csid != av[i - 1]->csid))
          {
            if (sCumulative > 0)
              {
                if (csidx >= callsite_src_id_cache->count)
                  {
                    mpiPi_msg_warn
                        ("Concise callsite sent report encountered index out of bounds.\n");
                    return;
                  }
                callsite_stats[csidx].name =
                    &(mpiPi.lookup[av[i - 1]->op - mpiPi_BASE].name[4]);
                callsite_stats[csidx].site = av[i - 1]->csid;
                callsite_stats[csidx].count = sCount;
                callsite_stats[csidx].max = sMax;
                callsite_stats[csidx].min = sMin;
                callsite_stats[csidx].cumulative = sCumulative;
                callsite_stats[csidx].max_rnk = max_rnk;
                callsite_stats[csidx].min_rnk = min_rnk;
                csidx++;
              }

            sCount = 0;
            sMax = 0;
            sMin = DBL_MAX;
            sCumulative = 0;
            max_rnk = -1;
            min_rnk = -1;
          }

        sCount++;
        sCumulative += av[i]->cumulativeDataSent;

        if (av[i]->cumulativeDataSent > sMax)
          {
            sMax = av[i]->cumulativeDataSent;
            max_rnk = av[i]->rank;
          }
        if (av[i]->cumulativeDataSent < sMin)
          {
            sMin = av[i]->cumulativeDataSent;
            min_rnk = av[i]->rank;
          }
      }

    if (sCumulative > 0)
      {
        callsite_stats[csidx].name =
            &(mpiPi.lookup[av[i - 1]->op - mpiPi_BASE].name[4]);
        callsite_stats[csidx].site = av[i - 1]->csid;
        callsite_stats[csidx].count = sCount;
        callsite_stats[csidx].max = sMax;
        callsite_stats[csidx].min = sMin;
        callsite_stats[csidx].cumulative = sCumulative;
        callsite_stats[csidx].max_rnk = max_rnk;
        callsite_stats[csidx].min_rnk = min_rnk;
      }
    else
      csidx--;

  }

  free (av);
  if (csidx > 0)
    {
      sprintf (buf,
               "Callsite Message Sent statistics (all callsites, sent bytes): %d",
               csidx + 1);
      print_section_heading (fp, buf);
      fprintf (fp, "%-17s %4s %7s %9s %9s %9s %6s %6s\n", "Name", "Site",
               "Tasks", "Max", "Mean", "Min", "MaxRnk", "MinRnk");

      qsort (callsite_stats, csidx + 1, sizeof (mpiPi_callsite_summary_t),
             (int (*)(const void *, const void *))
             callsite_stats_sort_by_cumulative);
      for (i = 0; i <= csidx; i++)
        {
          fprintf (fp,
                   mpiP_Report_Formats[MPIP_CALLSITE_MESS_CONCISE_FMT]
                   [mpiPi.reportFormat], callsite_stats[i].name,
              callsite_stats[i].site, callsite_stats[i].count,
              callsite_stats[i].max,
              callsite_stats[i].cumulative / callsite_stats[i].count,
              callsite_stats[i].min, callsite_stats[i].max_rnk,
              callsite_stats[i].min_rnk);
        }
    }
  free (callsite_stats);
}

static void
mpiPi_print_all_callsite_io_info (FILE * fp)
{
  int i, ac;
  char buf[256];
  callsite_stats_t **av;

  /*  This function is only called by the collector, which is the only
   *  process that should have a valid global_mpi_io value.
   *  */
  if (mpiPi.global_mpi_io > 0)
    {
      h_gather_data (mpiPi.global_callsite_stats, &ac, (void ***) &av);

      /* -- now that we have all the statistics in a queue, which is
       * pointers to the data, we can sort it however we need to.
       */
      qsort (av, ac, sizeof (void *), callsite_sort_by_name_id_rank);

      sprintf (buf, "Callsite I/O statistics (all, I/O bytes)");
      print_section_heading (fp, buf);
      fprintf (fp, "%-17s %4s %4s %7s %9s %9s %9s %9s\n", "Name", "Site",
               "Rank", "Count", "Max", "Mean", "Min", "Sum");

      {
        long long sCount = 0;
        double sMin = DBL_MAX;
        double sMax = 0;
        double sCumulative = 0;
        int lastcsid = 0;

        for (i = 0; i < ac; i++)
          {
            if (i != 0 && sCumulative > 0 && (av[i]->csid != av[i - 1]->csid))
              {
                fprintf (fp,
                         mpiP_Report_Formats[MPIP_CALLSITE_IO_SUMMARY_FMT]
                         [mpiPi.reportFormat],
                    &(mpiPi.lookup[av[i - 1]->op - mpiPi_BASE].name[4]),
                    av[i - 1]->csid, "*", sCount, sMax,
                    sCumulative / sCount, sMin, sCumulative);

                sCount = 0;
                sMax = 0;
                sMin = DBL_MAX;
                sCumulative = 0;
              }

            if (av[i]->cumulativeIO > 0)
              {
                sCount += av[i]->count;
                sCumulative += av[i]->cumulativeIO;
                sMax = max (av[i]->maxIO, sMax);
                sMin = min (av[i]->minIO, sMin);

                if (lastcsid != 0 && lastcsid != av[i]->csid)
                  fprintf (fp, "\n");

                fprintf (fp,
                    mpiP_Report_Formats[MPIP_CALLSITE_IO_RANK_FMT]
                    [mpiPi.reportFormat],
                    &(mpiPi.lookup[av[i]->op - mpiPi_BASE].name[4]),
                    av[i]->csid, av[i]->rank, av[i]->count,
                    av[i]->maxIO, av[i]->cumulativeIO / av[i]->count,
                    av[i]->minIO, av[i]->cumulativeIO);

                lastcsid = av[i]->csid;
              }
          }

        if (sCumulative > 0)
          {
            fprintf (fp,
                mpiP_Report_Formats[MPIP_CALLSITE_IO_SUMMARY_FMT]
                [mpiPi.reportFormat],
                &(mpiPi.lookup[av[i - 1]->op - mpiPi_BASE].name[4]),
                av[i - 1]->csid, "*", sCount, sMax,
                sCumulative / sCount, sMin, sCumulative);
          }
      }

      free (av);
    }
}

static void
mpiPi_print_all_callsite_rma_info (FILE * fp)
{
  int i, ac;
  char buf[256];
  callsite_stats_t **av;

  /*  This function is only called by the collector, which is the only
   *  process that should have a valid global_mpi_io value.
   *  */
  if (mpiPi.global_mpi_rma > 0)
    {
      h_gather_data (mpiPi.global_callsite_stats, &ac, (void ***) &av);

      /* -- now that we have all the statistics in a queue, which is
       * pointers to the data, we can sort it however we need to.
       */
      qsort (av, ac, sizeof (void *), callsite_sort_by_name_id_rank);

      sprintf (buf, "Callsite RMA statistics (all, origin bytes)");
      print_section_heading (fp, buf);
      fprintf (fp, "%-17s %4s %4s %7s %9s %9s %9s %9s\n", "Name", "Site",
               "Rank", "Count", "Max", "Mean", "Min", "Sum");

      {
        long long sCount = 0;
        double sMin = DBL_MAX;
        double sMax = 0;
        double sCumulative = 0;
        int lastcsid = 0;

        for (i = 0; i < ac; i++)
          {
            if (i != 0 && sCumulative > 0 && (av[i]->csid != av[i - 1]->csid))
              {
                fprintf (fp,
                    mpiP_Report_Formats[MPIP_CALLSITE_IO_SUMMARY_FMT]
                    [mpiPi.reportFormat],
                    &(mpiPi.lookup[av[i - 1]->op - mpiPi_BASE].name[4]),
                    av[i - 1]->csid, "*", sCount, sMax,
                    sCumulative / sCount, sMin, sCumulative);

                sCount = 0;
                sMax = 0;
                sMin = DBL_MAX;
                sCumulative = 0;
              }

            if (av[i]->cumulativeRMA > 0)
              {
                sCount += av[i]->count;
                sCumulative += av[i]->cumulativeRMA;
                sMax = max (av[i]->maxRMA, sMax);
                sMin = min (av[i]->minRMA, sMin);

                if (lastcsid != 0 && lastcsid != av[i]->csid)
                  fprintf (fp, "\n");

                fprintf (fp,
                    mpiP_Report_Formats[MPIP_CALLSITE_IO_RANK_FMT]
                    [mpiPi.reportFormat],
                    &(mpiPi.lookup[av[i]->op - mpiPi_BASE].name[4]),
                    av[i]->csid, av[i]->rank, av[i]->count,
                    av[i]->maxRMA, av[i]->cumulativeRMA / av[i]->count,
                    av[i]->minRMA, av[i]->cumulativeRMA);

                lastcsid = av[i]->csid;
              }
          }

        if (sCumulative > 0)
          {
            fprintf (fp,
                mpiP_Report_Formats[MPIP_CALLSITE_IO_SUMMARY_FMT]
                [mpiPi.reportFormat],
                &(mpiPi.lookup[av[i - 1]->op - mpiPi_BASE].name[4]),
                av[i - 1]->csid, "*", sCount, sMax,
                sCumulative / sCount, sMin, sCumulative);
          }
      }

      free (av);
    }
}


static void
mpiPi_print_concise_callsite_io_info (FILE * fp)
{
  int i, ac, csidx = 0;
  char buf[256];
  callsite_stats_t **av;
  mpiPi_callsite_summary_t *callsite_stats = NULL;

  /*  This function is only called by the collector, which is the only
   *  process that should have a valid global_mpi_io value.
   *  */
  if (mpiPi.global_mpi_io > 0)
    {
      h_gather_data (mpiPi.global_callsite_stats, &ac, (void ***) &av);

      /* -- now that we have all the statistics in a queue, which is
       * pointers to the data, we can sort it however we need to.
       */
      qsort (av, ac, sizeof (void *), callsite_sort_by_name_id_rank);
      callsite_stats =
          (mpiPi_callsite_summary_t *) malloc (sizeof (mpiPi_callsite_summary_t)
                                               * callsite_src_id_cache->count);

      if (callsite_stats == NULL)
        {
          mpiPi_msg_warn
              ("Failed to allocate space for callsite volume summary reporting\n");
          free (av);
          return;
        }

      {
        long long sCount = 0;
        double sMin = DBL_MAX;
        double sMax = 0;
        double sCumulative = 0;
        int max_rnk = -1, min_rnk = -1;

        for (i = 0, csidx = 0; i < ac; i++)
          {
            if (i != 0 && (av[i]->csid != av[i - 1]->csid))
              {
                if (sCumulative > 0)
                  {
                    if (csidx >= callsite_src_id_cache->count)
                      {
                        mpiPi_msg_warn
                            ("Concise callsite i/o report encountered index out of bounds.\n");
                        return;
                      }
                    callsite_stats[csidx].name =
                        &(mpiPi.lookup[av[i - 1]->op - mpiPi_BASE].name[4]);
                    callsite_stats[csidx].site = av[i - 1]->csid;
                    callsite_stats[csidx].count = sCount;
                    callsite_stats[csidx].max = sMax;
                    callsite_stats[csidx].min = sMin;
                    callsite_stats[csidx].cumulative = sCumulative;
                    callsite_stats[csidx].max_rnk = max_rnk;
                    callsite_stats[csidx].min_rnk = min_rnk;
                    csidx++;
                  }

                sCount = 0;
                sMax = 0;
                sMin = DBL_MAX;
                sCumulative = 0;
                max_rnk = -1;
                min_rnk = -1;
              }

            sCount++;
            sCumulative += av[i]->cumulativeIO;

            if (av[i]->cumulativeIO > sMax)
              {
                sMax = av[i]->cumulativeIO;
                max_rnk = av[i]->rank;
              }
            if (av[i]->cumulativeIO < sMin)
              {
                sMin = av[i]->cumulativeIO;
                min_rnk = av[i]->rank;
              }
          }

        if (sCumulative > 0)
          {
            callsite_stats[csidx].name =
                &(mpiPi.lookup[av[i - 1]->op - mpiPi_BASE].name[4]);
            callsite_stats[csidx].site = av[i - 1]->csid;
            callsite_stats[csidx].count = sCount;
            callsite_stats[csidx].max = sMax;
            callsite_stats[csidx].min = sMin;
            callsite_stats[csidx].cumulative = sCumulative;
            callsite_stats[csidx].max_rnk = max_rnk;
            callsite_stats[csidx].min_rnk = min_rnk;
          }
        else
          csidx--;

      }

      free (av);

      if (csidx > 0)
        {
          snprintf (buf, 256,
                    "Callsite I/O statistics (all callsites, bytes): %d",
                    csidx + 1);
          print_section_heading (fp, buf);
          fprintf (fp, "%-17s %4s %7s %9s %9s %9s %6s %6s\n", "Name", "Site",
                   "Tasks", "Max", "Mean", "Min", "MaxRnk", "MinRnk");

          qsort (callsite_stats, csidx + 1, sizeof (mpiPi_callsite_summary_t),
                 (int (*)(const void *, const void *))
                 callsite_stats_sort_by_cumulative);
          for (i = 0; i <= csidx; i++)
            {
              fprintf (fp,
                  mpiP_Report_Formats[MPIP_CALLSITE_MESS_CONCISE_FMT]
                  [mpiPi.reportFormat], callsite_stats[i].name,
                  callsite_stats[i].site, callsite_stats[i].count,
                  callsite_stats[i].max,
                  callsite_stats[i].cumulative / callsite_stats[i].count,
                  callsite_stats[i].min, callsite_stats[i].max_rnk,
                  callsite_stats[i].min_rnk);
            }
        }
      free (callsite_stats);
    }
}

static void
mpiPi_print_concise_callsite_rma_info (FILE * fp)
{
  int i, ac, csidx = 0;
  char buf[256];
  callsite_stats_t **av;
  mpiPi_callsite_summary_t *callsite_stats = NULL;

  /*  This function is only called by the collector, which is the only
   *  process that should have a valid global_mpi_rma value.
   *  */
  if (mpiPi.global_mpi_rma > 0)
    {
      h_gather_data (mpiPi.global_callsite_stats, &ac, (void ***) &av);

      /* -- now that we have all the statistics in a queue, which is
       * pointers to the data, we can sort it however we need to.
       */
      qsort (av, ac, sizeof (void *), callsite_sort_by_name_id_rank);
      callsite_stats =
          (mpiPi_callsite_summary_t *) malloc (sizeof (mpiPi_callsite_summary_t)
                                               * callsite_src_id_cache->count);

      if (callsite_stats == NULL)
        {
          mpiPi_msg_warn
              ("Failed to allocate space for callsite RMA volume summary reporting\n");
          free (av);
          return;
        }

      {
        long long sCount = 0;
        double sMin = DBL_MAX;
        double sMax = 0;
        double sCumulative = 0;
        int max_rnk = -1, min_rnk = -1;

        for (i = 0, csidx = 0; i < ac; i++)
          {
            if (i != 0 && (av[i]->csid != av[i - 1]->csid))
              {
                if (sCumulative > 0)
                  {
                    if (csidx >= callsite_src_id_cache->count)
                      {
                        mpiPi_msg_warn
                            ("Concise callsite i/o report encountered index out of bounds.\n");
                        return;
                      }
                    callsite_stats[csidx].name =
                        &(mpiPi.lookup[av[i - 1]->op - mpiPi_BASE].name[4]);
                    callsite_stats[csidx].site = av[i - 1]->csid;
                    callsite_stats[csidx].count = sCount;
                    callsite_stats[csidx].max = sMax;
                    callsite_stats[csidx].min = sMin;
                    callsite_stats[csidx].cumulative = sCumulative;
                    callsite_stats[csidx].max_rnk = max_rnk;
                    callsite_stats[csidx].min_rnk = min_rnk;
                    csidx++;
                  }

                sCount = 0;
                sMax = 0;
                sMin = DBL_MAX;
                sCumulative = 0;
                max_rnk = -1;
                min_rnk = -1;
              }

            sCount++;
            sCumulative += av[i]->cumulativeRMA;

            if (av[i]->cumulativeRMA > sMax)
              {
                sMax = av[i]->cumulativeRMA;
                max_rnk = av[i]->rank;
              }
            if (av[i]->cumulativeRMA < sMin)
              {
                sMin = av[i]->cumulativeRMA;
                min_rnk = av[i]->rank;
              }
          }

        if (sCumulative > 0)
          {
            callsite_stats[csidx].name =
                &(mpiPi.lookup[av[i - 1]->op - mpiPi_BASE].name[4]);
            callsite_stats[csidx].site = av[i - 1]->csid;
            callsite_stats[csidx].count = sCount;
            callsite_stats[csidx].max = sMax;
            callsite_stats[csidx].min = sMin;
            callsite_stats[csidx].cumulative = sCumulative;
            callsite_stats[csidx].max_rnk = max_rnk;
            callsite_stats[csidx].min_rnk = min_rnk;
          }
        else
          csidx--;

      }

      free (av);

      if (csidx > 0)
        {
          snprintf (buf, 256,
                    "Callsite RMA Target statistics (all callsites, bytes): %d",
                    csidx + 1);
          print_section_heading (fp, buf);
          fprintf (fp, "%-17s %4s %7s %9s %9s %9s %6s %6s\n", "Name", "Site",
                   "Tasks", "Max", "Mean", "Min", "MaxRnk", "MinRnk");

          qsort (callsite_stats, csidx + 1, sizeof (mpiPi_callsite_summary_t),
                 (int (*)(const void *, const void *))
                 callsite_stats_sort_by_cumulative);
          for (i = 0; i <= csidx; i++)
            {
              fprintf (fp,
                  mpiP_Report_Formats[MPIP_CALLSITE_MESS_CONCISE_FMT]
                  [mpiPi.reportFormat], callsite_stats[i].name,
                  callsite_stats[i].site, callsite_stats[i].count,
                  callsite_stats[i].max,
                  callsite_stats[i].cumulative / callsite_stats[i].count,
                  callsite_stats[i].min, callsite_stats[i].max_rnk,
                  callsite_stats[i].min_rnk);
            }
        }
      free (callsite_stats);
    }
}

static void
mpiPi_coll_print_all_callsite_time_info (FILE * fp)
{
  int i, j, ac;
  char buf[256];
  callsite_stats_t **av;
  callsite_stats_t *task_stats, *task_lookup, *task_data;
  callsite_stats_t cs_buf;
  long long sCount = 0;
  double sMin = DBL_MAX;
  double sMax = 0;
  double sCumulative = 0;
  int malloc_check = 1;

  /* Gather global callsite information at collectorRank and print header */
  if (mpiPi.rank == mpiPi.collectorRank)
    {
      h_gather_data (mpiPi.global_callsite_stats_agg, &ac, (void ***) &av);
      qsort (av, ac, sizeof (void *), callsite_sort_by_name_id_rank);

      task_data = malloc (sizeof (callsite_stats_t) * mpiPi.size);
      if (task_data == NULL)
        {
          mpiPi_msg_warn ("Failed to allocate space for task time data\n");
          malloc_check = 0;
          free (av);
        }
      else
        {
          sprintf (buf, "Callsite Time statistics (all, milliseconds): %lld",
                   mpiPi.global_time_callsite_count);
          print_section_heading (fp, buf);
          fprintf (fp, "%-17s %4s %4s %6s %8s %8s %8s %6s %6s\n", "Name",
                   "Site", "Rank", "Count", "Max", "Mean", "Min", "App%",
                   "MPI%");
        }
    }

  /*  Check whether collector malloc succeeded.   */
  PMPI_Bcast (&malloc_check, 1, MPI_INT, mpiPi.collectorRank, mpiPi.comm);
  if (malloc_check == 0)
    return;

  PMPI_Bcast (&ac, 1, MPI_INT, mpiPi.collectorRank, mpiPi.comm);

  /* For each global callsite:
   *   Broadcast callsite info to remote tasks
   *   Lookup local callsite info in local hashes
   *   Gather all task info at collectorRank
   */
  for (i = 0; i < ac; i++)
    {
      if (mpiPi.rank == mpiPi.collectorRank)
        task_stats = av[i];
      else
        task_stats = &cs_buf;

      PMPI_Bcast (task_stats, sizeof (callsite_stats_t),
                  MPI_CHAR, mpiPi.collectorRank, mpiPi.comm);

      task_stats->rank = mpiPi.rank;

      mpiPi_stats_mt_cs_lookup(&mpiPi.task_stats,
                               task_stats, &task_lookup, &cs_buf,
                               MPIPI_CALLSITE_MIN2ZERO);

      PMPI_Gather (task_lookup, sizeof (callsite_stats_t),
                   MPI_CHAR, task_data,
                   sizeof (callsite_stats_t), MPI_CHAR,
                   mpiPi.collectorRank, mpiPi.comm);

      if (mpiPi.rank == mpiPi.collectorRank)
        {
          sCount = 0;
          sMax = 0;
          sMin = DBL_MAX;
          sCumulative = 0;

          for (j = 0; j < mpiPi.size; j++)
            {
              sCount += task_data[j].count;
              sCumulative += task_data[j].cumulativeTime;
              sMax = max (task_data[j].maxDur, sMax);
              sMin = min (task_data[j].minDur, sMin);

              if (task_data[j].count > 0 &&
                  (100.0 * task_data[j].cumulativeTime /
                   mpiPi.global_task_mpi_time[task_data[j].rank])
                  >= mpiPi.reportPrintThreshold)
                {
                  fprintf (fp,
                      mpiP_Report_Formats[MPIP_CALLSITE_TIME_RANK_FMT]
                      [mpiPi.reportFormat],
                      &(mpiPi.lookup[task_stats->op - mpiPi_BASE].
                      name[4]), av[i]->csid, task_data[j].rank,
                      task_data[j].count, task_data[j].maxDur / 1000.0,
                      task_data[j].cumulativeTime / (task_data[j].count *
                                                     1000.0),
                      task_data[j].minDur / 1000.0,
                      100.0 * task_data[j].cumulativeTime /
                      (mpiPi.global_task_app_time[task_data[j].rank] *
                      1e6),
                      100.0 * task_data[j].cumulativeTime /
                      mpiPi.global_task_mpi_time[task_data[j].rank]);
                }
            }
          if (sCount > 0)
            {
              fprintf (fp,
                  mpiP_Report_Formats[MPIP_CALLSITE_TIME_SUMMARY_FMT]
                  [mpiPi.reportFormat],
                  &(mpiPi.lookup[task_data[j - 1].op - mpiPi_BASE].
                  name[4]), av[i]->csid, "*", sCount, sMax / 1000.0,
                  sCumulative / (sCount * 1000.0), sMin / 1000.0,
                  mpiPi.global_app_time >
                  0 ? 100.0 * sCumulative / (mpiPi.global_app_time *
                                             1e6) : 0,
                  mpiPi.global_mpi_time >
                  0 ? 100.0 * sCumulative / mpiPi.global_mpi_time : 0);
              fprintf (fp, "\n");
            }
        }
    }


  if (mpiPi.rank == mpiPi.collectorRank)
    {
      free (av);
      free (task_data);
    }
}


static void
mpiPi_coll_print_concise_callsite_time_info (FILE * fp)
{
  int i, ac;
  char buf[256];
  callsite_stats_t **av;
  callsite_stats_t *task_stats, *task_lookup;
  callsite_stats_t cs_buf;
  double tot_time;
  long long task_flag, tot_tasks;
  struct
  {
    double val;
    int rank;
  } min_time, max_time, local_min_time, local_max_time;

  if (mpiPi.rank == mpiPi.collectorRank)
    {
      /*  Sort aggregate callsite stats by descending cumulative time   */
      h_gather_data (mpiPi.global_callsite_stats_agg, &ac, (void ***) &av);
      qsort (av, ac, sizeof (void *), callsite_sort_by_cumulative_time);

      snprintf (buf, 256,
                "Callsite Time statistics (all callsites, milliseconds): %d",
                ac);
      print_section_heading (fp, buf);
      fprintf (fp, "%-17s %4s %7s %9s %9s %9s %6s %6s\n", "Name", "Site",
               "Tasks", "Max", "Mean", "Min", "MaxRnk", "MinRnk");
    }

  PMPI_Bcast (&ac, 1, MPI_INT, mpiPi.collectorRank, mpiPi.comm);

  /* For each global callsite:
   *   Broadcast callsite info to remote tasks
   *   Lookup local callsite info in local hashes
   *   Use Reduce to get task statistics at collectorRank
   */
  for (i = 0; i < ac; i++)
    {
      if (mpiPi.rank == mpiPi.collectorRank)
        task_stats = av[i];
      else
        task_stats = &cs_buf;

      /*  Broadcast current call site to all tasks   */
      PMPI_Bcast (task_stats, sizeof (callsite_stats_t),
                  MPI_CHAR, mpiPi.collectorRank, mpiPi.comm);


      /*  Search for task local entry for the current call site   */
      task_stats->rank = mpiPi.rank;
      mpiPi_stats_mt_cs_lookup(&mpiPi.task_stats,
                               task_stats, &task_lookup, &cs_buf,
                               MPIPI_CALLSITE_MIN2MAX);
      tot_tasks = 0;
      task_flag = task_lookup->count > 0 ? 1 : 0;

      /*  Get minimum aggregate time and rank for this call site   */
      if (task_lookup->cumulativeTime > 0)
        local_min_time.val = task_lookup->cumulativeTime;
      else
        local_min_time.val = DBL_MAX;
      local_min_time.rank = mpiPi.rank;
      PMPI_Reduce (&local_min_time, &min_time, 1, MPI_DOUBLE_INT, MPI_MINLOC,
                   mpiPi.collectorRank, mpiPi.comm);

      /*  Get maximum aggregate time and rank for this call site   */
      local_max_time.val = task_lookup->cumulativeTime;
      local_max_time.rank = mpiPi.rank;
      PMPI_Reduce (&local_max_time, &max_time, 1, MPI_DOUBLE_INT, MPI_MAXLOC,
                   mpiPi.collectorRank, mpiPi.comm);


      /*  Get sum of aggregate time for all tasks for this call site   */
      PMPI_Reduce (&(task_lookup->cumulativeTime), &tot_time, 1, MPI_DOUBLE,
                   MPI_SUM, mpiPi.collectorRank, mpiPi.comm);


      /*  Get the number of tasks with non-zero values for this call site   */
      PMPI_Reduce (&task_flag, &tot_tasks, 1, MPI_LONG_LONG, MPI_SUM,
                   mpiPi.collectorRank, mpiPi.comm);

      /*  Print summary statistics for this call site  */
      if (mpiPi.rank == mpiPi.collectorRank)
        {
          fprintf (fp,
              mpiP_Report_Formats[MPIP_CALLSITE_TIME_CONCISE_FMT]
              [mpiPi.reportFormat],
              &(mpiPi.lookup[av[i]->op - mpiPi_BASE].name[4]),
              av[i]->csid, tot_tasks, max_time.val / 1000.0,
              tot_time / (tot_tasks * 1000), min_time.val / 1000.0,
              max_time.rank, min_time.rank);
        }
    }

  if (mpiPi.rank == mpiPi.collectorRank)
    free (av);
}


static void
mpiPi_coll_print_concise_callsite_sent_info (FILE * fp)
{
  int ci, i, ac, callsite_count;
  char buf[256];
  callsite_stats_t **av;
  callsite_stats_t *task_stats, *task_lookup;
  callsite_stats_t cs_buf;
  double tot_sent;
  long long task_flag, tot_tasks;
  struct
  {
    double val;
    int rank;
  } min_sent, max_sent, local_min_sent, local_max_sent;

  if (mpiPi.rank == mpiPi.collectorRank)
    {
      /*  Sort aggregate callsite stats by descending cumulative sent   */
      h_gather_data (mpiPi.global_callsite_stats_agg, &ac, (void ***) &av);
      qsort (av, ac, sizeof (void *), callsite_sort_by_cumulative_size);
      for (i = 0, callsite_count = 0; i < ac; i++)
        {
          if (av[i]->cumulativeDataSent > 0)
            callsite_count++;
        }
      if (callsite_count > 0)
        {

          snprintf (buf, 256,
                    "Callsite Message Sent statistics (all callsites, bytes sent): %d",
                    callsite_count);
          print_section_heading (fp, buf);
          fprintf (fp, "%-17s %4s %7s %9s %9s %9s %6s %6s\n", "Name", "Site",
                   "Tasks", "Max", "Mean", "Min", "MaxRnk", "MinRnk");
        }
    }

  PMPI_Bcast (&callsite_count, 1, MPI_INT, mpiPi.collectorRank, mpiPi.comm);

  /* For each global callsite with cumulativeDataSent > 0:
   *   Broadcast callsite info to remote tasks
   *   Lookup local callsite info in local hashes
   *   Use Reduce to get task statistics at collectorRank
   */
  for (i = 0, ci = 0; i < callsite_count; i++, ci++)
    {
      if (mpiPi.rank == mpiPi.collectorRank)
        {
          task_stats = av[ci];
          while (task_stats->cumulativeDataSent == 0)
            {
              ci++;
              task_stats = av[ci];
            }
        }
      else
        task_stats = &cs_buf;

      /*  Broadcast current call site to all tasks   */
      PMPI_Bcast (task_stats, sizeof (callsite_stats_t),
                  MPI_CHAR, mpiPi.collectorRank, mpiPi.comm);


      task_stats->rank = mpiPi.rank;
      /*  Search for task local entry for the current call site   */
      mpiPi_stats_mt_cs_lookup(&mpiPi.task_stats,
                               task_stats, &task_lookup, &cs_buf,
                               MPIPI_CALLSITE_MIN2MAX);
      tot_tasks = 0;
      task_flag = task_lookup->cumulativeDataSent > 0 ? 1 : 0;

      /*  Get minimum aggregate sent and rank for this call site   */
      if (task_lookup->cumulativeDataSent > 0)
        local_min_sent.val = task_lookup->cumulativeDataSent;
      else
        local_min_sent.val = DBL_MAX;
      local_min_sent.rank = mpiPi.rank;
      PMPI_Reduce (&local_min_sent, &min_sent, 1, MPI_DOUBLE_INT, MPI_MINLOC,
                   mpiPi.collectorRank, mpiPi.comm);

      /*  Get maximum aggregate sent and rank for this call site   */
      local_max_sent.val = task_lookup->cumulativeDataSent;
      local_max_sent.rank = mpiPi.rank;
      PMPI_Reduce (&local_max_sent, &max_sent, 1, MPI_DOUBLE_INT, MPI_MAXLOC,
                   mpiPi.collectorRank, mpiPi.comm);


      /*  Get sum of aggregate sent for all tasks for this call site   */
      PMPI_Reduce (&(task_lookup->cumulativeDataSent), &tot_sent, 1,
                   MPI_DOUBLE, MPI_SUM, mpiPi.collectorRank, mpiPi.comm);


      /*  Get the number of tasks with non-zero values for this call site   */
      PMPI_Reduce (&task_flag, &tot_tasks, 1, MPI_LONG_LONG, MPI_SUM,
                   mpiPi.collectorRank, mpiPi.comm);

      /*  Print summary statistics for this call site  */
      if (mpiPi.rank == mpiPi.collectorRank)
        {
          fprintf (fp,
              mpiP_Report_Formats[MPIP_CALLSITE_MESS_CONCISE_FMT]
              [mpiPi.reportFormat],
              &(mpiPi.lookup[av[ci]->op - mpiPi_BASE].name[4]),
              av[ci]->csid, tot_tasks, max_sent.val,
              tot_sent / tot_tasks, min_sent.val, max_sent.rank,
              min_sent.rank);
        }
    }

  if (mpiPi.rank == mpiPi.collectorRank)
    free (av);
}


static void
mpiPi_coll_print_concise_callsite_io_info (FILE * fp)
{
  int ci, i, ac, callsite_count;
  char buf[256];
  callsite_stats_t **av;
  callsite_stats_t *task_stats, *task_lookup;
  callsite_stats_t cs_buf;
  double tot_io;
  long long task_flag, tot_tasks;
  struct
  {
    double val;
    int rank;
  } min_io, max_io, local_min_io, local_max_io;

#ifndef HAVE_MPI_IO
  return;
#endif

  if (mpiPi.rank == mpiPi.collectorRank)
    {
      /*  Sort aggregate callsite stats by descending cumulative io   */
      h_gather_data (mpiPi.global_callsite_stats_agg, &ac, (void ***) &av);
      qsort (av, ac, sizeof (void *), callsite_sort_by_cumulative_io);
      for (i = 0, callsite_count = 0; i < ac; i++)
        {
          if (av[i]->cumulativeIO > 0)
            callsite_count++;
        }
      if (callsite_count > 0)
        {

          snprintf (buf, 256,
                    "Callsite I/O statistics (all callsites, bytes): %d",
                    callsite_count);
          print_section_heading (fp, buf);
          fprintf (fp, "%-17s %4s %7s %9s %9s %9s %6s %6s\n", "Name", "Site",
                   "Tasks", "Max", "Mean", "Min", "MaxRnk", "MinRnk");
        }
    }

  PMPI_Bcast (&callsite_count, 1, MPI_INT, mpiPi.collectorRank, mpiPi.comm);

  /* For each global callsite with cumulativeIO > 0:
   *   Broadcast callsite info to remote tasks
   *   Lookup local callsite info in local hashes
   *   Use Reduce to get task statistics at collectorRank
   */
  for (i = 0, ci = 0; i < callsite_count; i++, ci++)
    {
      if (mpiPi.rank == mpiPi.collectorRank)
        {
          /*  Find next call site with IO activity   */
          task_stats = av[ci];
          while (task_stats->cumulativeIO == 0)
            {
              ci++;
              task_stats = av[ci];
            }
        }
      else
        task_stats = &cs_buf;

      /*  Broadcast current call site to all tasks   */
      PMPI_Bcast (task_stats, sizeof (callsite_stats_t),
                  MPI_CHAR, mpiPi.collectorRank, mpiPi.comm);


      /*  Search for task local entry for the current call site   */
      task_stats->rank = mpiPi.rank;
      mpiPi_stats_mt_cs_lookup(&mpiPi.task_stats,
                               task_stats, &task_lookup, &cs_buf,
                               MPIPI_CALLSITE_MIN2MAX);
      tot_tasks = 0;
      task_flag = task_lookup->cumulativeIO > 0 ? 1 : 0;

      /*  Get minimum aggregate io and rank for this call site   */
      if (task_lookup->cumulativeIO > 0)
        local_min_io.val = task_lookup->cumulativeIO;
      else
        local_min_io.val = DBL_MAX;
      local_min_io.rank = mpiPi.rank;
      PMPI_Reduce (&local_min_io, &min_io, 1, MPI_DOUBLE_INT, MPI_MINLOC,
                   mpiPi.collectorRank, mpiPi.comm);

      /*  Get maximum aggregate io and rank for this call site   */
      local_max_io.val = task_lookup->cumulativeIO;
      local_max_io.rank = mpiPi.rank;
      PMPI_Reduce (&local_max_io, &max_io, 1, MPI_DOUBLE_INT, MPI_MAXLOC,
                   mpiPi.collectorRank, mpiPi.comm);


      /*  Get sum of aggregate io for all tasks for this call site   */
      PMPI_Reduce (&(task_lookup->cumulativeIO), &tot_io, 1, MPI_DOUBLE,
                   MPI_SUM, mpiPi.collectorRank, mpiPi.comm);


      /*  Get the number of tasks with non-zero values for this call site   */
      PMPI_Reduce (&task_flag, &tot_tasks, 1, MPI_LONG_LONG, MPI_SUM,
                   mpiPi.collectorRank, mpiPi.comm);

      /*  Print summary statistics for this call site  */
      if (mpiPi.rank == mpiPi.collectorRank)
        {
          fprintf (fp,
              mpiP_Report_Formats[MPIP_CALLSITE_MESS_CONCISE_FMT]
              [mpiPi.reportFormat],
              &(mpiPi.lookup[av[ci]->op - mpiPi_BASE].name[4]),
              av[ci]->csid, tot_tasks, max_io.val, tot_io / tot_tasks,
              min_io.val, max_io.rank, min_io.rank);
        }
    }

  if (mpiPi.rank == mpiPi.collectorRank)
    free (av);
}


static void
mpiPi_coll_print_concise_callsite_rma_info (FILE * fp)
{
  int ci, i, ac, callsite_count;
  char buf[256];
  callsite_stats_t **av;
  callsite_stats_t *task_stats, *task_lookup;
  callsite_stats_t cs_buf;
  double tot_sent;
  long long task_flag, tot_tasks;
  struct
  {
    double val;
    int rank;
  } min_sent, max_sent, local_min_sent, local_max_sent;

  if (mpiPi.rank == mpiPi.collectorRank)
    {
      /*  Sort aggregate callsite stats by descending cumulative sent   */
      h_gather_data (mpiPi.global_callsite_stats_agg, &ac, (void ***) &av);
      qsort (av, ac, sizeof (void *), callsite_sort_by_cumulative_rma);
      for (i = 0, callsite_count = 0; i < ac; i++)
        {
          if (av[i]->cumulativeRMA > 0)
            callsite_count++;
        }
      if (callsite_count > 0)
        {

          snprintf (buf, 256,
                    "Callsite RMA Target statistics (all callsites, bytes): %d",
                    callsite_count);
          print_section_heading (fp, buf);
          fprintf (fp, "%-17s %4s %7s %9s %9s %9s %6s %6s\n", "Name", "Site",
                   "Tasks", "Max", "Mean", "Min", "MaxRnk", "MinRnk");
        }
    }

  PMPI_Bcast (&callsite_count, 1, MPI_INT, mpiPi.collectorRank, mpiPi.comm);

  /* For each global callsite with cumulativeDataSent > 0:
   *   Broadcast callsite info to remote tasks
   *   Lookup local callsite info in local hashes
   *   Use Reduce to get task statistics at collectorRank
   */
  for (i = 0, ci = 0; i < callsite_count; i++, ci++)
    {
      if (mpiPi.rank == mpiPi.collectorRank)
        {
          task_stats = av[ci];
          while (task_stats->cumulativeRMA == 0)
            {
              ci++;
              task_stats = av[ci];
            }
        }
      else
        task_stats = &cs_buf;

      /*  Broadcast current call site to all tasks   */
      PMPI_Bcast (task_stats, sizeof (callsite_stats_t),
                  MPI_CHAR, mpiPi.collectorRank, mpiPi.comm);


      /*  Search for task local entry for the current call site   */
      task_stats->rank = mpiPi.rank;
      mpiPi_stats_mt_cs_lookup(&mpiPi.task_stats,
                               task_stats, &task_lookup, &cs_buf,
                               MPIPI_CALLSITE_MIN2MAX);

      tot_tasks = 0;
      task_flag = task_lookup->cumulativeRMA > 0 ? 1 : 0;

      /*  Get minimum aggregate sent and rank for this call site   */
      if (task_lookup->cumulativeRMA > 0)
        local_min_sent.val = task_lookup->cumulativeRMA;
      else
        local_min_sent.val = DBL_MAX;
      local_min_sent.rank = mpiPi.rank;
      PMPI_Reduce (&local_min_sent, &min_sent, 1, MPI_DOUBLE_INT, MPI_MINLOC,
                   mpiPi.collectorRank, mpiPi.comm);

      /*  Get maximum aggregate sent and rank for this call site   */
      local_max_sent.val = task_lookup->cumulativeRMA;
      local_max_sent.rank = mpiPi.rank;
      PMPI_Reduce (&local_max_sent, &max_sent, 1, MPI_DOUBLE_INT, MPI_MAXLOC,
                   mpiPi.collectorRank, mpiPi.comm);


      /*  Get sum of aggregate sent for all tasks for this call site   */
      PMPI_Reduce (&(task_lookup->cumulativeRMA), &tot_sent, 1,
                   MPI_DOUBLE, MPI_SUM, mpiPi.collectorRank, mpiPi.comm);


      /*  Get the number of tasks with non-zero values for this call site   */
      PMPI_Reduce (&task_flag, &tot_tasks, 1, MPI_LONG_LONG, MPI_SUM,
                   mpiPi.collectorRank, mpiPi.comm);

      /*  Print summary statistics for this call site  */
      if (mpiPi.rank == mpiPi.collectorRank)
        {
          fprintf (fp,
              mpiP_Report_Formats[MPIP_CALLSITE_MESS_CONCISE_FMT]
              [mpiPi.reportFormat],
              &(mpiPi.lookup[av[ci]->op - mpiPi_BASE].name[4]),
              av[ci]->csid, tot_tasks, max_sent.val,
              tot_sent / tot_tasks, min_sent.val, max_sent.rank,
              min_sent.rank);
        }
    }

  if (mpiPi.rank == mpiPi.collectorRank)
    free (av);
}



static void
mpiPi_coll_print_all_callsite_sent_info (FILE * fp)
{
  int i, j, ac;
  char buf[256];
  callsite_stats_t **av;
  callsite_stats_t *task_stats, *task_lookup, *task_data;
  callsite_stats_t cs_buf;
  long long sCount = 0;
  double sMin = DBL_MAX;
  double sMax = 0;
  double sCumulative = 0;
  double tot_data_sent = 0;
  int malloc_check = 1;

  PMPI_Bcast (&mpiPi.global_mpi_sent_count, 1, MPI_LONG_LONG,
              mpiPi.collectorRank, mpiPi.comm);
  if (mpiPi.global_mpi_sent_count > 0)
    {
      /* Gather global callsite information at collectorRank and print header */
      if (mpiPi.rank == mpiPi.collectorRank)
        {
          h_gather_data (mpiPi.global_callsite_stats_agg, &ac,
                         (void ***) &av);
          qsort (av, ac, sizeof (void *), callsite_sort_by_name_id_rank);
          task_data = malloc (sizeof (callsite_stats_t) * mpiPi.size);

          if (task_data == NULL)
            {
              mpiPi_msg_warn
                  ("Failed to allocate space for task volume data\n");
              malloc_check = 0;
              free (av);
            }
          else
            {
              sprintf (buf,
                       "Callsite Message Sent statistics (all, sent bytes)");
              print_section_heading (fp, buf);
              fprintf (fp, "%-17s %4s %4s %7s %9s %9s %9s %9s\n", "Name",
                       "Site", "Rank", "Count", "Max", "Mean", "Min", "Sum");
            }
        }

      /*  Check whether collector malloc succeeded.   */
      PMPI_Bcast (&malloc_check, 1, MPI_INT, mpiPi.collectorRank, mpiPi.comm);
      if (malloc_check == 0)
        return;


      PMPI_Bcast (&ac, 1, MPI_INT, mpiPi.collectorRank, mpiPi.comm);

      /* For each global callsite:
       *   Broadcast callsite info to remote tasks
       *   Lookup local callsite info in local hashes
       *   Gather all task info at collectorRank
       */
      for (i = 0; i < ac; i++)
        {
          if (mpiPi.rank == mpiPi.collectorRank)
            task_stats = av[i];
          else
            task_stats = &cs_buf;

          tot_data_sent = task_stats->cumulativeDataSent;
          PMPI_Bcast (&tot_data_sent, 1, MPI_DOUBLE, mpiPi.collectorRank,
                      mpiPi.comm);

          if (tot_data_sent > 0)
            {
              PMPI_Bcast (task_stats, sizeof (callsite_stats_t),
                          MPI_CHAR, mpiPi.collectorRank, mpiPi.comm);

              task_stats->rank = mpiPi.rank;
              mpiPi_stats_mt_cs_lookup(&mpiPi.task_stats,
                                       task_stats, &task_lookup, &cs_buf,
                                       MPIPI_CALLSITE_MIN2ZERO);

              PMPI_Gather (task_lookup, sizeof (callsite_stats_t),
                           MPI_CHAR, task_data,
                           sizeof (callsite_stats_t), MPI_CHAR,
                           mpiPi.collectorRank, mpiPi.comm);

              if (mpiPi.rank == mpiPi.collectorRank)
                {
                  sCount = 0;
                  sMax = 0;
                  sMin = DBL_MAX;
                  sCumulative = 0;

                  for (j = 0; j < mpiPi.size; j++)
                    {
                      if (task_data[j].cumulativeDataSent > 0)
                        {
                          sCount += task_data[j].count;
                          sCumulative += task_data[j].cumulativeDataSent;
                          sMax = max (task_data[j].maxDataSent, sMax);
                          sMin = min (task_data[j].minDataSent, sMin);

                          fprintf (fp,
                              mpiP_Report_Formats
                              [MPIP_CALLSITE_MESS_RANK_FMT]
                              [mpiPi.reportFormat],
                              &(mpiPi.lookup[av[i]->op - mpiPi_BASE].
                              name[4]), av[i]->csid, task_data[j].rank,
                              task_data[j].count,
                              task_data[j].maxDataSent,
                              task_data[j].cumulativeDataSent /
                              task_data[j].count,
                              task_data[j].minDataSent,
                              task_data[j].cumulativeDataSent);
                        }

                    }
                  if (sCumulative > 0)
                    {
                      fprintf (fp,
                          mpiP_Report_Formats
                          [MPIP_CALLSITE_MESS_SUMMARY_FMT]
                          [mpiPi.reportFormat],
                          &(mpiPi.lookup[av[i]->op - mpiPi_BASE].
                          name[4]), av[i]->csid, "*", sCount, sMax,
                          sCumulative / sCount, sMin, sCumulative);
                    }
                  fprintf (fp, "\n");
                }
            }
        }

      if (mpiPi.rank == mpiPi.collectorRank)
        {
          free (av);
          free (task_data);
        }
    }
}

static void
mpiPi_coll_print_all_callsite_io_info (FILE * fp)
{
  int i, j, ac;
  char buf[256];
  callsite_stats_t **av;
  callsite_stats_t *task_stats, *task_lookup, *task_data;
  callsite_stats_t cs_buf;
  long long sCount = 0;
  double sMin = DBL_MAX;
  double sMax = 0;
  double sCumulative = 0;
  double tot_data_sent = 0;
  int malloc_check = 1;

#ifndef HAVE_MPI_IO
  return;
#endif

  PMPI_Bcast (&mpiPi.global_mpi_io, 1, MPI_DOUBLE, mpiPi.collectorRank,
              mpiPi.comm);

  if (mpiPi.global_mpi_io > 0)
    {
      /* Gather global callsite information at collectorRank and print header */
      if (mpiPi.rank == mpiPi.collectorRank)
        {
          h_gather_data (mpiPi.global_callsite_stats_agg, &ac,
                         (void ***) &av);
          qsort (av, ac, sizeof (void *), callsite_sort_by_name_id_rank);
          task_data = malloc (sizeof (callsite_stats_t) * mpiPi.size);

          if (task_data == NULL)
            {
              mpiPi_msg_warn ("Failed to allocate space for task I/O data\n");
              malloc_check = 0;
              free (av);
            }
          else
            {
              sprintf (buf, "Callsite I/O statistics (all, I/O bytes)");
              print_section_heading (fp, buf);
              fprintf (fp, "%-17s %4s %4s %7s %9s %9s %9s %9s\n", "Name",
                       "Site", "Rank", "Count", "Max", "Mean", "Min", "Sum");
            }
        }

      /*  Check whether collector malloc succeeded.   */
      PMPI_Bcast (&malloc_check, 1, MPI_INT, mpiPi.collectorRank, mpiPi.comm);
      if (malloc_check == 0)
        return;

      PMPI_Bcast (&ac, 1, MPI_INT, mpiPi.collectorRank, mpiPi.comm);

      /* For each global callsite:
       *   Broadcast callsite info to remote tasks
       *   Lookup local callsite info in local hashes
       *   Gather all task info at collectorRank
       */
      for (i = 0; i < ac; i++)
        {
          if (mpiPi.rank == mpiPi.collectorRank)
            task_stats = av[i];
          else
            task_stats = &cs_buf;

          tot_data_sent = task_stats->cumulativeIO;
          PMPI_Bcast (&tot_data_sent, 1, MPI_DOUBLE, mpiPi.collectorRank,
                      mpiPi.comm);

          if (tot_data_sent > 0)
            {
              PMPI_Bcast (task_stats, sizeof (callsite_stats_t),
                          MPI_CHAR, mpiPi.collectorRank, mpiPi.comm);

              task_stats->rank = mpiPi.rank;
              mpiPi_stats_mt_cs_lookup(&mpiPi.task_stats,
                                       task_stats, &task_lookup, &cs_buf,
                                       MPIPI_CALLSITE_MIN2ZERO);

              PMPI_Gather (task_lookup, sizeof (callsite_stats_t),
                           MPI_CHAR, task_data,
                           sizeof (callsite_stats_t), MPI_CHAR,
                           mpiPi.collectorRank, mpiPi.comm);

              if (mpiPi.rank == mpiPi.collectorRank)
                {
                  sCount = 0;
                  sMax = 0;
                  sMin = DBL_MAX;
                  sCumulative = 0;

                  for (j = 0; j < mpiPi.size; j++)
                    {
                      if (task_data[j].cumulativeIO > 0)
                        {
                          sCount += task_data[j].count;
                          sCumulative += task_data[j].cumulativeIO;
                          sMax = max (task_data[j].maxIO, sMax);
                          sMin = min (task_data[j].minIO, sMin);

                          fprintf (fp,
                              mpiP_Report_Formats
                              [MPIP_CALLSITE_IO_RANK_FMT]
                              [mpiPi.reportFormat],
                              &(mpiPi.lookup
                                [task_data[j].op - mpiPi_BASE].name[4]),
                              av[i]->csid, task_data[j].rank,
                              task_data[j].count, task_data[j].maxIO,
                              task_data[j].cumulativeIO /
                              task_data[j].count, task_data[j].minIO,
                              task_data[j].cumulativeIO);
                        }
                    }
                  if (sCumulative > 0)
                    {
                      fprintf (fp,
                          mpiP_Report_Formats
                          [MPIP_CALLSITE_IO_SUMMARY_FMT]
                          [mpiPi.reportFormat],
                          &(mpiPi.lookup
                            [task_data[j - 1].op - mpiPi_BASE].name[4]),
                          av[i]->csid, "*", sCount, sMax,
                          sCumulative / sCount, sMin, sCumulative);
                    }
                  fprintf (fp, "\n");
                }
            }
        }

      if (mpiPi.rank == mpiPi.collectorRank)
        {
          free (av);
          free (task_data);
        }
    }
}


static void
mpiPi_coll_print_all_callsite_rma_info (FILE * fp)
{
  int i, j, ac;
  char buf[256];
  callsite_stats_t **av;
  callsite_stats_t *task_stats, *task_lookup, *task_data;
  callsite_stats_t cs_buf;
  long long sCount = 0;
  double sMin = DBL_MAX;
  double sMax = 0;
  double sCumulative = 0;
  double tot_data_sent = 0;
  int malloc_check = 1;

  PMPI_Bcast (&mpiPi.global_mpi_rma, 1, MPI_DOUBLE, mpiPi.collectorRank,
              mpiPi.comm);

  if (mpiPi.global_mpi_rma > 0)
    {
      /* Gather global callsite information at collectorRank and print header */
      if (mpiPi.rank == mpiPi.collectorRank)
        {
          h_gather_data (mpiPi.global_callsite_stats_agg, &ac,
                         (void ***) &av);
          qsort (av, ac, sizeof (void *), callsite_sort_by_name_id_rank);
          task_data = malloc (sizeof (callsite_stats_t) * mpiPi.size);

          if (task_data == NULL)
            {
              mpiPi_msg_warn ("Failed to allocate space for task RMA data\n");
              malloc_check = 0;
              free (av);
            }
          else
            {
              sprintf (buf, "Callsite RMA statistics (all, origin bytes)");
              print_section_heading (fp, buf);
              fprintf (fp, "%-17s %4s %4s %7s %9s %9s %9s %9s\n", "Name",
                       "Site", "Rank", "Count", "Max", "Mean", "Min", "Sum");
            }
        }

      /*  Check whether collector malloc succeeded.   */
      PMPI_Bcast (&malloc_check, 1, MPI_INT, mpiPi.collectorRank, mpiPi.comm);
      if (malloc_check == 0)
        return;

      PMPI_Bcast (&ac, 1, MPI_INT, mpiPi.collectorRank, mpiPi.comm);

      /* For each global callsite:
       *   Broadcast callsite info to remote tasks
       *   Lookup local callsite info in local hashes
       *   Gather all task info at collectorRank
       */
      for (i = 0; i < ac; i++)
        {
          if (mpiPi.rank == mpiPi.collectorRank)
            task_stats = av[i];
          else
            task_stats = &cs_buf;

          tot_data_sent = task_stats->cumulativeRMA;
          PMPI_Bcast (&tot_data_sent, 1, MPI_DOUBLE, mpiPi.collectorRank,
                      mpiPi.comm);

          if (tot_data_sent > 0)
            {
              PMPI_Bcast (task_stats, sizeof (callsite_stats_t),
                          MPI_CHAR, mpiPi.collectorRank, mpiPi.comm);

              task_stats->rank = mpiPi.rank;
              mpiPi_stats_mt_cs_lookup(&mpiPi.task_stats,
                                       task_stats, &task_lookup, &cs_buf,
                                       MPIPI_CALLSITE_MIN2ZERO);

              PMPI_Gather (task_lookup, sizeof (callsite_stats_t),
                           MPI_CHAR, task_data,
                           sizeof (callsite_stats_t), MPI_CHAR,
                           mpiPi.collectorRank, mpiPi.comm);

              if (mpiPi.rank == mpiPi.collectorRank)
                {
                  sCount = 0;
                  sMax = 0;
                  sMin = DBL_MAX;
                  sCumulative = 0;

                  for (j = 0; j < mpiPi.size; j++)
                    {
                      if (task_data[j].cumulativeRMA > 0)
                        {
                          sCount += task_data[j].count;
                          sCumulative += task_data[j].cumulativeRMA;
                          sMax = max (task_data[j].maxRMA, sMax);
                          sMin = min (task_data[j].minRMA, sMin);

                          fprintf (fp,
                              mpiP_Report_Formats
                              [MPIP_CALLSITE_IO_RANK_FMT]
                              [mpiPi.reportFormat],
                              &(mpiPi.lookup
                                [task_data[j].op - mpiPi_BASE].name[4]),
                              av[i]->csid, task_data[j].rank,
                              task_data[j].count, task_data[j].maxRMA,
                              task_data[j].cumulativeRMA /
                              task_data[j].count, task_data[j].minRMA,
                              task_data[j].cumulativeRMA);
                        }
                    }
                  if (sCumulative > 0)
                    {
                      fprintf (fp,
                          mpiP_Report_Formats
                          [MPIP_CALLSITE_IO_SUMMARY_FMT]
                          [mpiPi.reportFormat],
                          &(mpiPi.lookup
                            [task_data[j - 1].op - mpiPi_BASE].name[4]),
                          av[i]->csid, "*", sCount, sMax,
                          sCumulative / sCount, sMin, sCumulative);
                    }
                  fprintf (fp, "\n");
                }
            }
        }

      if (mpiPi.rank == mpiPi.collectorRank)
        {
          free (av);
          free (task_data);
        }
    }
}


void
mpiPi_profile_print (FILE * fp, int report_style)
{
  if (mpiPi.collectorRank == mpiPi.rank)
    {
      assert (fp);

      mpiPi_print_report_header (fp);
    }

  if (report_style == mpiPi_style_verbose)
    mpiPi_profile_print_verbose (fp);
  else if (report_style == mpiPi_style_concise)
    mpiPi_profile_print_concise (fp);

  if (mpiPi.collectorRank == mpiPi.rank)
    print_section_heading (fp, "End of Report");
}


static void
mpiPi_profile_print_concise (FILE * fp)
{
  if (mpiPi.collectorRank == mpiPi.rank)
    {
      fprintf (fp, "\n");
      mpiPi_print_concise_task_info (fp);

      mpiPi_print_top_time_sites (fp);
      mpiPi_print_top_sent_sites (fp);
      if (mpiPi.do_collective_stats_report)
        mpiPi_print_top_collective_sent_sites (fp);
      if (mpiPi.do_pt2pt_stats_report)
        mpiPi_print_top_pt2pt_sent_sites (fp);
      mpiPi_print_top_io_sites (fp);
      mpiPi_print_top_rma_sites (fp);


      if (mpiPi.collective_report == 0)
        {
          if (mpiPi.print_callsite_detail)
            {
              mpiPi_print_callsites (fp);
              mpiPi_print_concise_callsite_time_info (fp);
              mpiPi_print_concise_callsite_sent_info (fp);
              mpiPi_print_concise_callsite_io_info (fp);
              mpiPi_print_concise_callsite_rma_info (fp);
            }
        }
    }
  if (mpiPi.collective_report == 1)
    {
      if (mpiPi.print_callsite_detail)
        {
          if (mpiPi.collectorRank == mpiPi.rank)
            mpiPi_print_callsites (fp);

          mpiPi_coll_print_concise_callsite_time_info (fp);
          mpiPi_coll_print_concise_callsite_sent_info (fp);
          mpiPi_coll_print_concise_callsite_io_info (fp);
          mpiPi_coll_print_concise_callsite_rma_info (fp);
        }
    }
}

static void
mpiPi_profile_print_verbose (FILE * fp)
{
  if (mpiPi.collectorRank == mpiPi.rank)
    {
      mpiPi_print_task_assignment (fp);
      fprintf (fp, "\n");

      mpiPi_print_verbose_task_info (fp);

      if (mpiPi.print_callsite_detail)
        mpiPi_print_callsites (fp);

      mpiPi_print_top_time_sites (fp);
      mpiPi_print_top_sent_sites (fp);
      if (mpiPi.do_collective_stats_report)
        mpiPi_print_top_collective_sent_sites (fp);
      if (mpiPi.do_pt2pt_stats_report)
        mpiPi_print_top_pt2pt_sent_sites (fp);
      mpiPi_print_top_io_sites (fp);
      mpiPi_print_top_rma_sites (fp);
    }

  if (mpiPi.print_callsite_detail)
    {
      if (mpiPi.collective_report == 1)
        {
          mpiPi_msg_debug0 ("Using collective process reporting routines\n");
          mpiPi_msg_debug0
              ("MEMORY : collective reporting memory allocation :        %13ld\n",
               sizeof (callsite_stats_t) * mpiPi.size);

          mpiPi_coll_print_all_callsite_time_info (fp);
          mpiPi_coll_print_all_callsite_sent_info (fp);
          mpiPi_coll_print_all_callsite_io_info (fp);
          mpiPi_coll_print_all_callsite_rma_info (fp);
        }
      else
        {


          if (mpiPi.collectorRank == mpiPi.rank)
            {
              mpiPi_msg_debug
                  ("Using standard process reporting routines aggregating data at process rank %d\n",
                   mpiPi.collectorRank);

              mpiPi_print_all_callsite_time_info (fp);
              mpiPi_print_all_callsite_sent_info (fp);
              mpiPi_print_all_callsite_io_info (fp);
              mpiPi_print_all_callsite_rma_info (fp);
            }
        }
    }

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
