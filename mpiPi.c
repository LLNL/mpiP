/* -*- C -*- 

   mpiP MPI Profiler ( http://mpip.sourceforge.net/ )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   ----- 

   mpiPi.c -- main mpiP internal functions

 */

#ifndef lint
static char *svnid = "$Id$";
#endif

#include <string.h>
#include <float.h>
#include <unistd.h>
#include "mpiPi.h"

static int
mpiPi_callsite_stats_pc_hashkey (const void *p)
{
  int res = 0;
  int i;
  callsite_stats_t *csp = (callsite_stats_t *) p;
  MPIP_CALLSITE_STATS_COOKIE_ASSERT (csp);
  for (i = 0; i < MPIP_CALLSITE_STACK_DEPTH; i++)
    {
      res ^= (unsigned) (long) csp->pc[i];
    }
  return 52271 ^ csp->op ^ res ^ csp->rank;
}

static int
mpiPi_callsite_stats_pc_comparator (const void *p1, const void *p2)
{
  int i;
  callsite_stats_t *csp_1 = (callsite_stats_t *) p1;
  callsite_stats_t *csp_2 = (callsite_stats_t *) p2;
  MPIP_CALLSITE_STATS_COOKIE_ASSERT (csp_1);
  MPIP_CALLSITE_STATS_COOKIE_ASSERT (csp_2);

#define express(f) {if ((csp_1->f) > (csp_2->f)) {return 1;} if ((csp_1->f) < (csp_2->f)) {return -1;}}
  express (op);
  express (rank);

  for (i = 0; i < MPIP_CALLSITE_STACK_DEPTH; i++)
    {
      express (pc[i]);
    }
#undef express

  return 0;
}

static int
mpiPi_callsite_stats_src_hashkey (const void *p)
{
  int res = 0;
  callsite_stats_t *csp = (callsite_stats_t *) p;
  MPIP_CALLSITE_STATS_COOKIE_ASSERT (csp);
  return 52271 ^ csp->op ^ res ^ csp->rank ^ csp->csid;
}

static int
mpiPi_callsite_stats_src_comparator (const void *p1, const void *p2)
{
  callsite_stats_t *csp_1 = (callsite_stats_t *) p1;
  callsite_stats_t *csp_2 = (callsite_stats_t *) p2;
  MPIP_CALLSITE_STATS_COOKIE_ASSERT (csp_1);
  MPIP_CALLSITE_STATS_COOKIE_ASSERT (csp_2);

#define express(f) {if ((csp_1->f) > (csp_2->f)) {return 1;} if ((csp_1->f) < (csp_2->f)) {return -1;}}
  express (op);
  express (csid);
  express (rank);
#undef express

  return 0;
}

static int
mpiPi_callsite_stats_src_id_hashkey (const void *p)
{
  int res = 0;
  callsite_stats_t *csp = (callsite_stats_t *) p;
  MPIP_CALLSITE_STATS_COOKIE_ASSERT (csp);
  return 52271 ^ csp->op ^ res ^ csp->csid;
}

static int
mpiPi_callsite_stats_src_id_comparator (const void *p1, const void *p2)
{
  callsite_stats_t *csp_1 = (callsite_stats_t *) p1;
  callsite_stats_t *csp_2 = (callsite_stats_t *) p2;
  MPIP_CALLSITE_STATS_COOKIE_ASSERT (csp_1);
  MPIP_CALLSITE_STATS_COOKIE_ASSERT (csp_2);

#define express(f) {if ((csp_1->f) > (csp_2->f)) {return 1;} if ((csp_1->f) < (csp_2->f)) {return -1;}}
  express (op);
  express (csid);
#undef express

  return 0;
}

#ifndef ENABLE_API_ONLY		/* { */
/* task level init 
   - executed by each MPI task only once immediately after MPI_Init
*/

void init_histogram(mpiPi_histogram_t *h, int first_bin_max, int size, int* intervals)
{
  h->first_bin_max = first_bin_max;
  h->hist_size = size;
  h->bin_intervals = intervals;
}


void
mpiPi_init (char *appName)
{
  if (time (&mpiPi.start_timeofday) == (time_t) - 1)
    {
      mpiPi_msg_warn ("Could not get time of day from time()\n");
    }

  mpiPi.toolname = "mpiP";
  mpiPi.comm = MPI_COMM_WORLD;
  mpiPi.tag = 9821;
  mpiPi.procID = getpid ();
  mpiPi.appName = strdup (appName);
  PMPI_Comm_rank (mpiPi.comm, &mpiPi.rank);
  PMPI_Comm_size (mpiPi.comm, &mpiPi.size);
  PMPI_Get_processor_name (mpiPi.hostname, &mpiPi.hostnamelen);
  mpiPi.stdout_ = stdout;
  mpiPi.stderr_ = stderr;
  mpiPi.lookup = mpiPi_lookup;

  mpiPi.enabled = 1;
  mpiPi.enabledCount = 1;
  mpiPi.cumulativeTime = 0.0;
  mpiPi.global_app_time = 0.0;
  mpiPi.global_mpi_time = 0.0;
  mpiPi.global_mpi_size = 0.0;
  mpiPi.global_mpi_io = 0.0;
  mpiPi.global_mpi_rma = 0.0;
  mpiPi.global_mpi_msize_threshold_count = 0;
  mpiPi.global_mpi_sent_count = 0;
  mpiPi.global_time_callsite_count = 0;
  mpiPi.global_task_info = NULL;

  /* set some defaults values */
  mpiPi.collectorRank = 0;
  mpiPi.tableSize = 256;
  mpiPi.stackDepth = 1;		/* the value 2 includes parent wrapper function */
  mpiPi.reportPrintThreshold = 0.0;
  mpiPi.baseNames = 0;
  mpiPi.reportFormat = MPIP_REPORT_SCI_FORMAT;
  mpiPi.calcCOV = 1;
  mpiPi.inAPIrtb = 0;
  mpiPi.do_lookup = 1;
  mpiPi.messageCountThreshold = -1;
  mpiPi.report_style = mpiPi_style_verbose;
  mpiPi.print_callsite_detail = 1;
#ifdef COLLECTIVE_REPORT_DEFAULT
  mpiPi.collective_report = 1;
#else
  mpiPi.collective_report = 0;
#endif
  mpiPi.disable_finalize_report = 0;
  mpiPi.do_collective_stats_report = 0;
  mpiPi.do_pt2pt_stats_report = 0;
#ifdef SO_LOOKUP
  mpiPi.so_info = NULL;
#endif
  mpiPi_getenv ();

  mpiPi.task_callsite_stats =
    h_open (mpiPi.tableSize, mpiPi_callsite_stats_pc_hashkey,
	    mpiPi_callsite_stats_pc_comparator);

  if ( mpiPi.do_collective_stats_report == 1 )
    {
      init_histogram(&mpiPi.coll_comm_histogram, 7, 32, NULL);
      init_histogram(&mpiPi.coll_size_histogram, 7, 32, NULL);
    }

  if ( mpiPi.do_pt2pt_stats_report == 1 )
    {
      init_histogram(&mpiPi.pt2pt_comm_histogram, 7, 32, NULL);
      init_histogram(&mpiPi.pt2pt_size_histogram, 7, 32, NULL);
    }

  /* -- welcome msg only collector  */
  if (mpiPi.collectorRank == mpiPi.rank)
    {
      mpiPi_msg ("");
      mpiPi_msg ("%s V%d.%d.%d (Build %s/%s)\n", mpiPi.toolname, mpiPi_vmajor,
		 mpiPi_vminor, mpiPi_vpatch, mpiPi_vdate, mpiPi_vtime);
      mpiPi_msg ("Direct questions and errors to %s\n", MPIP_HELP_LIST);
      mpiPi_msg ("\n");
    }

  mpiPi_msg_debug ("appName is %s\n", appName);
  mpiPi_msg_debug ("sizeof(callsite_stats_t) is %d\n", sizeof(callsite_stats_t));
  mpiPi_msg_debug ("successful init on %d, %s\n", mpiPi.rank, mpiPi.hostname);

  if (mpiPi.enabled)
    {
      mpiPi_GETTIME (&mpiPi.startTime);
    }

  return;
}
#endif /* } ifndef ENABLE_API_ONLY */


typedef struct callsite_cache_entry_t
{
  void *pc;
  char *filename;
  char *functname;
  int line;
}
callsite_pc_cache_entry_t;

h_t *callsite_pc_cache = NULL;

static int
callsite_pc_cache_comparator (const void *p1, const void *p2)
{
  callsite_pc_cache_entry_t *cs1 = (callsite_pc_cache_entry_t *) p1;
  callsite_pc_cache_entry_t *cs2 = (callsite_pc_cache_entry_t *) p2;

  if ((long) cs1->pc > (long) cs2->pc)
    {
      return 1;
    }
  if ((long) cs1->pc < (long) cs2->pc)
    {
      return -1;
    }
  return 0;
}

static int
callsite_pc_cache_hashkey (const void *p1)
{
  callsite_pc_cache_entry_t *cs1 = (callsite_pc_cache_entry_t *) p1;
  return 662917 ^ ((long) cs1->pc);
}

int
mpiPi_query_pc (void *pc, char **filename, char **functname, int *lineno)
{
  int rc = 0;
  callsite_pc_cache_entry_t key;
  callsite_pc_cache_entry_t *csp;
  char addr_buf[24];

  key.pc = pc;
  /* do we have a cache entry for this pc? If so, use entry */
  if (h_search (callsite_pc_cache, &key, (void **) &csp) == NULL)
    {
      /* no cache entry: create, lookup, and insert */
      csp =
	(callsite_pc_cache_entry_t *)
	malloc (sizeof (callsite_pc_cache_entry_t));
      csp->pc = pc;
#if defined(ENABLE_BFD) || defined(USE_LIBDWARF)
      if (mpiP_find_src_loc (pc, filename, lineno, functname) == 0)
	{
	  if (*filename == NULL || strcmp (*filename, "??") == 0)
	    *filename = "[unknown]";

	  if (*functname == NULL)
	    *functname = "[unknown]";

	  mpiPi_msg_debug
	    ("Successful Source lookup for [%s]: %s, %d, %s\n",
	     mpiP_format_address (pc, addr_buf), *filename, *lineno,
	     *functname);

	  csp->filename = strdup (*filename);
	  csp->functname = strdup (*functname);
	  csp->line = *lineno;
	}
      else
	{
	  mpiPi_msg_debug ("Unsuccessful Source lookup for [%s]\n",
			   mpiP_format_address (pc, addr_buf));
	  csp->filename = strdup ("[unknown]");
	  csp->functname = strdup ("[unknown]");
	  csp->line = 0;
	}
#else /* ! ENABLE_BFD || USE_LIBDWARF */
      csp->filename = strdup ("[unknown]");
      csp->functname = strdup ("[unknown]");
      csp->line = 0;
#endif
      h_insert (callsite_pc_cache, csp);
    }

  *filename = csp->filename;
  *functname = csp->functname;
  *lineno = csp->line;

  if (*lineno == 0)
    rc = 1;			/* use this value to indicate a failed lookup */

  return rc;
}


h_t *callsite_src_id_cache = NULL;
int callsite_src_id_counter = 1;

static int
callsite_src_id_cache_comparator (const void *p1, const void *p2)
{
  int i;
  callsite_src_id_cache_entry_t *csp_1 = (callsite_src_id_cache_entry_t *) p1;
  callsite_src_id_cache_entry_t *csp_2 = (callsite_src_id_cache_entry_t *) p2;

#define express(f) {if ((csp_1->f) > (csp_2->f)) {return 1;} if ((csp_1->f) < (csp_2->f)) {return -1;}}
  for (i = 0; i < MPIP_CALLSITE_STACK_DEPTH; i++)
    {
      if (csp_1->filename[i] != NULL && csp_2->filename[i] != NULL)
	{
	  if (strcmp (csp_1->filename[i], csp_2->filename[i]) > 0)
	    {
	      return 1;
	    }
	  if (strcmp (csp_1->filename[i], csp_2->filename[i]) < 0)
	    {
	      return -1;
	    }
	  express (line[i]);
	  if (strcmp (csp_1->functname[i], csp_2->functname[i]) > 0)
	    {
	      return 1;
	    }
	  if (strcmp (csp_1->functname[i], csp_2->functname[i]) < 0)
	    {
	      return -1;
	    }
	}

      express (pc[i]);
    }
#undef express
  return 0;
}

static int
callsite_src_id_cache_hashkey (const void *p1)
{
  int i, j;
  int res = 0;
  callsite_src_id_cache_entry_t *cs1 = (callsite_src_id_cache_entry_t *) p1;
  for (i = 0; i < MPIP_CALLSITE_STACK_DEPTH; i++)
    {
      if (cs1->filename[i] != NULL)
	{
	  for (j = 0; cs1->filename[i][j] != '\0'; j++)
	    {
	      res ^= (unsigned) cs1->filename[i][j];
	    }
	  for (j = 0; cs1->functname[i][j] != '\0'; j++)
	    {
	      res ^= (unsigned) cs1->functname[i][j];
	    }
	}
      res ^= cs1->line[i];
    }
  return 662917 ^ res;
}

/* take a callstats record (the pc) and determine src file, line, if
   possible and assign a callsite id.
 */
int
mpiPi_query_src (callsite_stats_t * p)
{
  int i;
  callsite_src_id_cache_entry_t key;
  callsite_src_id_cache_entry_t *csp;
  assert (p);

  /* Because multiple pcs can map to the same source line, we must
     check that mapping here. If we got unknown, then we assign
     different ids */
  bzero (&key, sizeof (callsite_src_id_cache_entry_t));

  for (i = 0; (i < MPIP_CALLSITE_STACK_DEPTH) && (p->pc[i] != NULL); i++)
    {
      if (mpiPi.do_lookup == 1)
	mpiPi_query_pc (p->pc[i], &(p->filename[i]), &(p->functname[i]),
			&(p->lineno[i]));
      else
	{
	  p->filename[i] = strdup ("[unknown]");
	  p->functname[i] = strdup ("[unknown]");
	  p->lineno[i] = 0;
	}

      key.filename[i] = p->filename[i];
      key.functname[i] = p->functname[i];
      key.line[i] = p->lineno[i];
      key.pc[i] = p->pc[i];
    }

  /* lookup/generate an ID based on the callstack, not just the callsite pc */
  if (h_search (callsite_src_id_cache, &key, (void **) &csp) == NULL)
    {
      /* create a new entry, and assign an id based on callstack */
      csp =
	(callsite_src_id_cache_entry_t *)
	malloc (sizeof (callsite_src_id_cache_entry_t));
      bzero (csp, sizeof (callsite_src_id_cache_entry_t));

      for (i = 0; (i < MPIP_CALLSITE_STACK_DEPTH) && (p->pc[i] != NULL); i++)
	{
	  csp->filename[i] = strdup (key.filename[i]);
	  csp->functname[i] = strdup (key.functname[i]);
	  csp->line[i] = key.line[i];
	  csp->pc[i] = p->pc[i];
	}
      csp->id = callsite_src_id_counter++;
      csp->op = p->op;
      h_insert (callsite_src_id_cache, csp);
    }

  /* assign ID to this record */
  p->csid = csp->id;
  return p->csid;
}


static int
mpiPi_insert_callsite_records (callsite_stats_t * p)
{
  callsite_stats_t *csp = NULL;

  mpiPi_query_src (p);		/* sets the file/line in p */

  /* If exists, accumulate, otherwise insert. This is
     specifically for optimizations that have multiple PCs for
     one src line. We aggregate across rank after this. 

     The collective_report reporting approach does not aggregate individual 
     process callsite information at the collector process.
   */
  if (mpiPi.collective_report == 0)
    {
      if (NULL == h_search (mpiPi.global_callsite_stats, p, (void **) &csp))
	{
	  int j;
	  callsite_stats_t *newp = NULL;
	  newp = (callsite_stats_t *) malloc (sizeof (callsite_stats_t));
	  bzero (newp, sizeof (callsite_stats_t));
	  newp->op = p->op;
	  newp->rank = p->rank;
	  for (j = 0; j < MPIP_CALLSITE_STACK_DEPTH; j++)
	    {
	      newp->pc[j] = p->pc[j];
	      newp->filename[j] = p->filename[j];
	      newp->functname[j] = p->functname[j];
	      newp->lineno[j] = p->lineno[j];
	    }
	  newp->csid = p->csid;
	  newp->count = p->count;
	  newp->cumulativeTime = p->cumulativeTime;
	  newp->cumulativeTimeSquared = p->cumulativeTimeSquared;
	  newp->maxDur = p->maxDur;
	  newp->minDur = p->minDur;
	  newp->maxDataSent = p->maxDataSent;
	  newp->minDataSent = p->minDataSent;
	  newp->cumulativeDataSent = p->cumulativeDataSent;
	  newp->maxIO = p->maxIO;
	  newp->minIO = p->minIO;
	  newp->cumulativeIO = p->cumulativeIO;
	  newp->cumulativeRMA = p->cumulativeRMA;
	  newp->arbitraryMessageCount = p->arbitraryMessageCount;
	  newp->cookie = MPIP_CALLSITE_STATS_COOKIE;

	  /* insert new record into global */
	  h_insert (mpiPi.global_callsite_stats, newp);
	}
      else
	{
	  csp->count += p->count;
	  csp->cumulativeTime += p->cumulativeTime;
	  assert (csp->cumulativeTime >= 0);
	  csp->cumulativeTimeSquared += p->cumulativeTimeSquared;
	  assert (csp->cumulativeTimeSquared >= 0);
	  csp->maxDur = max (csp->maxDur, p->maxDur);
	  csp->minDur = min (csp->minDur, p->minDur);
	  csp->maxDataSent = max (csp->maxDataSent, p->maxDataSent);
	  csp->minDataSent = min (csp->minDataSent, p->minDataSent);
	  csp->cumulativeDataSent += p->cumulativeDataSent;
	  csp->maxIO = max (csp->maxIO, p->maxIO);
	  csp->minIO = min (csp->minIO, p->minIO);
	  csp->cumulativeIO += p->cumulativeIO;
	  csp->cumulativeRMA += p->cumulativeRMA;
	  csp->arbitraryMessageCount += p->arbitraryMessageCount;
	}
    }

  /* Collect aggregate callsite summary information indpendent of rank. */
  if (NULL == h_search (mpiPi.global_callsite_stats_agg, p, (void **) &csp))
    {
      int j;
      callsite_stats_t *newp = NULL;
      newp = (callsite_stats_t *) malloc (sizeof (callsite_stats_t));
      bzero (newp, sizeof (callsite_stats_t));
      newp->op = p->op;
      newp->rank = -1;
      for (j = 0; j < MPIP_CALLSITE_STACK_DEPTH; j++)
	{
	  newp->pc[j] = p->pc[j];
	  newp->filename[j] = p->filename[j];
	  newp->functname[j] = p->functname[j];
	  newp->lineno[j] = p->lineno[j];
	}
      newp->csid = p->csid;
      newp->count = p->count;
      newp->cumulativeTime = p->cumulativeTime;
      newp->cumulativeTimeSquared = p->cumulativeTimeSquared;
      newp->maxDur = p->maxDur;
      newp->minDur = p->minDur;
      newp->maxDataSent = p->maxDataSent;
      newp->minDataSent = p->minDataSent;
      newp->cumulativeDataSent = p->cumulativeDataSent;
      newp->cumulativeIO = p->cumulativeIO;
      newp->cumulativeRMA = p->cumulativeRMA;
      newp->maxIO = p->maxIO;
      newp->minIO = p->minIO;
      newp->cookie = MPIP_CALLSITE_STATS_COOKIE;

      if (mpiPi.calcCOV)
	{
	  newp->siteData = (double *) malloc (mpiPi.size * sizeof (double));
	  newp->siteData[0] = p->cumulativeTime;
	  newp->siteDataIdx = 1;
	}

      /* insert new record into global */
      h_insert (mpiPi.global_callsite_stats_agg, newp);
    }
  else
    {
      csp->count += p->count;
      csp->cumulativeTime += p->cumulativeTime;
      assert (csp->cumulativeTime >= 0);
      csp->cumulativeTimeSquared += p->cumulativeTimeSquared;
      assert (csp->cumulativeTimeSquared >= 0);
      csp->maxDur = max (csp->maxDur, p->maxDur);
      csp->minDur = min (csp->minDur, p->minDur);
      csp->maxDataSent = max (csp->maxDataSent, p->maxDataSent);
      csp->minDataSent = min (csp->minDataSent, p->minDataSent);
      csp->cumulativeDataSent += p->cumulativeDataSent;
      csp->maxIO = max (csp->maxIO, p->maxIO);
      csp->minIO = min (csp->minIO, p->minIO);
      csp->cumulativeIO += p->cumulativeIO;
      csp->cumulativeRMA += p->cumulativeRMA;

      if (mpiPi.calcCOV)
	{
	  csp->siteData[csp->siteDataIdx] = p->cumulativeTime;
	  csp->siteDataIdx += 1;
	}
    }

  /* Do global accumulation while we are iterating through individual callsites */
  mpiPi.global_task_info[p->rank].mpi_time += p->cumulativeTime;

  mpiPi.global_mpi_time += p->cumulativeTime;
  assert (mpiPi.global_mpi_time >= 0);
  mpiPi.global_mpi_size += p->cumulativeDataSent;
  mpiPi.global_mpi_io += p->cumulativeIO;
  mpiPi.global_mpi_rma += p->cumulativeRMA;
  if (p->cumulativeTime > 0)
    mpiPi.global_time_callsite_count++;

  if (p->cumulativeDataSent > 0)
    {
      mpiPi.global_mpi_msize_threshold_count += p->arbitraryMessageCount;
      mpiPi.global_mpi_sent_count += p->count;
    }

  return 1;
}


#ifndef ENABLE_API_ONLY		/* { */

static int
mpiPi_mergeResults ()
{
  int ac;
  callsite_stats_t **av;
  int totalCount = 0;
  int maxCount = 0;
  int retval = 1, sendval;

  /* gather local task data */
  h_gather_data (mpiPi.task_callsite_stats, &ac, (void ***) &av);

  /* determine size of space necessary on collector */
  PMPI_Allreduce (&ac, &totalCount, 1, MPI_INT, MPI_SUM, mpiPi.comm);
  PMPI_Reduce (&ac, &maxCount, 1, MPI_INT, MPI_MAX, mpiPi.collectorRank,
	       mpiPi.comm);

  if (totalCount < 1)
    {
      mpiPi_msg_warn
	("Collector found no records to merge. Omitting report.\n");
      return 0;
    }

  /* gather global data at collector */
  if (mpiPi.rank == mpiPi.collectorRank)
    {
      int i;
      int ndx = 0;

#ifdef ENABLE_BFD
      if (mpiPi.appFullName != NULL)
	{
	  if (open_bfd_executable (mpiPi.appFullName) == 0)
	    mpiPi.do_lookup = 0;
	}
#elif defined(USE_LIBDWARF)
      if (mpiPi.appFullName != NULL)
	{
	  if (open_dwarf_executable (mpiPi.appFullName) == 0)
	    mpiPi.do_lookup = 0;
	}
#endif
#if defined(ENABLE_BFD) || defined(USE_LIBDWARF)
      else
	{
	  mpiPi_msg_warn
	    ("Failed to open executable.  The mpiP -x runtime flag may address this issue.\n");
	  mpiPi.do_lookup = 0;
	}
#endif
      /* convert data to src line; merge, if nec */
      mpiPi.global_callsite_stats = h_open (mpiPi.tableSize,
					    mpiPi_callsite_stats_src_hashkey,
					    mpiPi_callsite_stats_src_comparator);
      mpiPi.global_callsite_stats_agg = h_open (mpiPi.tableSize,
						mpiPi_callsite_stats_src_id_hashkey,
						mpiPi_callsite_stats_src_id_comparator);
      if (callsite_pc_cache == NULL)
	{
	  callsite_pc_cache = h_open (mpiPi.tableSize,
				      callsite_pc_cache_hashkey,
				      callsite_pc_cache_comparator);
	}
      if (callsite_src_id_cache == NULL)
	{
	  callsite_src_id_cache = h_open (mpiPi.tableSize,
					  callsite_src_id_cache_hashkey,
					  callsite_src_id_cache_comparator);
	}
      /* Try to allocate space for max count of callsite info from all tasks  */
      mpiPi.rawCallsiteData =
	(callsite_stats_t *) calloc (maxCount, sizeof (callsite_stats_t));
      if (mpiPi.rawCallsiteData == NULL)
	{
	  mpiPi_msg_warn
	    ("Failed to allocate memory to collect callsite info");
	  retval = 0;
	}

      /* Clear global_mpi_time and global_mpi_size before accumulation in mpiPi_insert_callsite_records */
      mpiPi.global_mpi_time = 0.0;
      mpiPi.global_mpi_size = 0.0;

      if (retval == 1)
	{
	  /* Insert collector callsite data into global and task-specific hash tables */
	  for (ndx = 0; ndx < ac; ndx++)
	    {
	      mpiPi_insert_callsite_records (av[ndx]);
	    }
	  ndx = 0;
	  for (i = 1; i < mpiPi.size; i++)	/* n-1 */
	    {
	      MPI_Status status;
	      int count;
	      int j;

	      /* okay in any order */
	      PMPI_Probe (MPI_ANY_SOURCE, mpiPi.tag, mpiPi.comm, &status);
	      PMPI_Get_count (&status, MPI_CHAR, &count);
	      PMPI_Recv (&(mpiPi.rawCallsiteData[ndx]), count, MPI_CHAR,
			 status.MPI_SOURCE, mpiPi.tag, mpiPi.comm, &status);
	      count /= sizeof (callsite_stats_t);


	      for (j = 0; j < count; j++)
		{
		  mpiPi_insert_callsite_records (&(mpiPi.rawCallsiteData[j]));
		}
	    }
	  free (mpiPi.rawCallsiteData);
	}
    }
  else
    {
      int ndx;
      char *sbuf = (char *) malloc (ac * sizeof (callsite_stats_t));
      for (ndx = 0; ndx < ac; ndx++)
	{
	  bcopy (av[ndx],
		 &(sbuf[ndx * sizeof (callsite_stats_t)]),
		 sizeof (callsite_stats_t));
	}
      PMPI_Send (sbuf, ac * sizeof (callsite_stats_t),
		 MPI_CHAR, mpiPi.collectorRank, mpiPi.tag, mpiPi.comm);
      free (sbuf);
    }
  if (mpiPi.rank == mpiPi.collectorRank && retval == 1)
    {
      if (mpiPi.collective_report == 0)
	mpiPi_msg_debug
	  ("MEMORY : Allocated for global_callsite_stats     : %13ld\n",
	   h_count (mpiPi.global_callsite_stats) * sizeof (callsite_stats_t));
      mpiPi_msg_debug
	("MEMORY : Allocated for global_callsite_stats_agg : %13ld\n",
	 h_count (mpiPi.global_callsite_stats_agg) *
	 sizeof (callsite_stats_t));
    }

  /* TODO: need to free all these pointers as well. */
  free (av);

  if (mpiPi.rank == mpiPi.collectorRank)
    {
      if (mpiPi.do_lookup == 1)
	{
#ifdef ENABLE_BFD
	  /* clean up */
	  close_bfd_executable ();
#elif defined(USE_LIBDWARF)
	  close_dwarf_executable ();
#endif
	}
    }

  /*  Quadrics MPI does not appear to support MPI_IN_PLACE   */
  sendval = retval;
  PMPI_Allreduce (&sendval, &retval, 1, MPI_INT, MPI_MIN, mpiPi.comm);
  return retval;
}


static int
mpiPi_mergeCollectiveStats ()
{
  int matrix_size;
  int all_call_count;
  double* coll_time_results, *coll_time_local;
  int i, x, y, z;

  if ( mpiPi.do_collective_stats_report )
    {
      all_call_count = mpiPi_DEF_END - mpiPi_BASE + 1;
      matrix_size = all_call_count * mpiPi.coll_comm_histogram.hist_size * mpiPi.coll_size_histogram.hist_size;
      mpiPi_msg_debug("matrix_size is %d, histogram data is %d\n", matrix_size, sizeof(mpiPi.coll_time_stats));
      coll_time_local = (double*)malloc(matrix_size * sizeof(double));
      coll_time_results = (double*)malloc(matrix_size * sizeof(double));

      i = 0;
      for ( x = 0; x < mpiPi_DEF_END - mpiPi_BASE; x++ )
        for ( y = 0; y < 32; y++ )
          for ( z = 0; z < 32; z++ )
            coll_time_local[i++] = mpiPi.coll_time_stats[x][y][z];

      /*  Collect Collective statistic data were used  */
      PMPI_Reduce (coll_time_local, coll_time_results, matrix_size, MPI_DOUBLE, MPI_SUM, mpiPi.collectorRank,
                   mpiPi.comm);

      i = 0;
      for ( x = 0; x < mpiPi_DEF_END - mpiPi_BASE; x++ )
        for ( y = 0; y < 32; y++ )
          for ( z = 0; z < 32; z++ )
            mpiPi.coll_time_stats[x][y][z] = coll_time_results[i++];

      free(coll_time_local);
      free(coll_time_results);
    }

  return 1;
}


static int
mpiPi_mergept2ptStats ()
{
  int matrix_size;
  int all_call_count;
  double* pt2pt_send_results, *pt2pt_send_local;
  int i, x, y, z;

  if ( mpiPi.do_pt2pt_stats_report )
    {
      all_call_count = mpiPi_DEF_END - mpiPi_BASE + 1;
      matrix_size = all_call_count * mpiPi.pt2pt_comm_histogram.hist_size * mpiPi.pt2pt_size_histogram.hist_size;
      mpiPi_msg_debug("matrix_size is %d, histogram data is %d\n", matrix_size, sizeof(mpiPi.pt2pt_send_stats));
      pt2pt_send_local = (double*)malloc(matrix_size * sizeof(double));
      pt2pt_send_results = (double*)malloc(matrix_size * sizeof(double));

      i = 0;
      for ( x = 0; x < mpiPi_DEF_END - mpiPi_BASE; x++ )
        for ( y = 0; y < 32; y++ )
          for ( z = 0; z < 32; z++ )
            pt2pt_send_local[i++] = mpiPi.pt2pt_send_stats[x][y][z];

      /*  Collect Collective statistic data were used  */
      PMPI_Reduce (pt2pt_send_local, pt2pt_send_results, matrix_size, MPI_DOUBLE, MPI_SUM, mpiPi.collectorRank,
                   mpiPi.comm);

      i = 0;
      for ( x = 0; x < mpiPi_DEF_END - mpiPi_BASE; x++ )
        for ( y = 0; y < 32; y++ )
          for ( z = 0; z < 32; z++ )
            mpiPi.pt2pt_send_stats[x][y][z] = pt2pt_send_results[i++];

      free(pt2pt_send_local);
      free(pt2pt_send_results);
    }

  return 1;
}


static void
mpiPi_publishResults (int report_style)
{
  FILE *fp = NULL;
  static int printCount = 0;

  if (mpiPi.collectorRank == mpiPi.rank)
    {

      /* Generate output filename, and open */
      do
	{
	  printCount++;
	  snprintf (mpiPi.oFilename, 256, "%s/%s.%d.%d.%d.mpiP",
		    mpiPi.outputDir, mpiPi.appName, mpiPi.size, mpiPi.procID,
		    printCount);
	}
      while (access (mpiPi.oFilename, F_OK) == 0);

      fp = fopen (mpiPi.oFilename, "w");

      if (fp == NULL)
	{
	  mpiPi_msg_warn ("Could not open [%s], writing to stdout\n",
			  mpiPi.oFilename);
	  fp = stdout;
	}
      else
	{
	  mpiPi_msg ("\n");
	  mpiPi_msg ("Storing mpiP output in [%s].\n", mpiPi.oFilename);
	  mpiPi_msg ("\n");
	}
    }
  mpiPi_profile_print (fp, report_style);
  PMPI_Barrier (MPI_COMM_WORLD);
  if (fp != stdout && fp != NULL)
    {
      fclose (fp);
    }
}

/*
 * mpiPi_collect_basics() - all tasks send their basic info to the
 * collectorRank.
 */
static void
mpiPi_collect_basics ()
{
  int i = 0;
  double app_time = mpiPi.cumulativeTime;
  int cnt;
  mpiPi_task_info_t mti;
  int blockcounts[4] = { 1, 1, 1, MPIPI_HOSTNAME_LEN_MAX };
  MPI_Datatype types[4] = { MPI_DOUBLE, MPI_DOUBLE, MPI_INT, MPI_CHAR };
  MPI_Aint displs[4];
  MPI_Datatype mti_type;
  MPI_Request *recv_req_arr;

  mpiPi_msg_debug ("Collect Basics\n");

  cnt = 0;

  PMPI_Address (&mti.mpi_time, &displs[cnt++]);
  PMPI_Address (&mti.app_time, &displs[cnt++]);
  PMPI_Address (&mti.rank, &displs[cnt++]);
  PMPI_Address (&mti.hostname, &displs[cnt++]);

  for (i = (cnt - 1); i >= 0; i--)
    {
      displs[i] -= displs[0];
    }
  PMPI_Type_struct (cnt, blockcounts, displs, types, &mti_type);
  PMPI_Type_commit (&mti_type);

  if (mpiPi.rank == mpiPi.collectorRank)
    {
      /* In the case where multiple reports are generated per run,
         only allocate memory for global_task_info once */
      if (mpiPi.global_task_info == NULL)
	{
	  mpiPi.global_task_info =
	    (mpiPi_task_info_t *) calloc (mpiPi.size,
					  sizeof (mpiPi_task_info_t));
	  if (mpiPi.global_task_info == NULL)
	    mpiPi_abort ("Failed to allocate memory for global_task_info");

	  mpiPi_msg_debug
	    ("MEMORY : Allocated for global_task_info :          %13ld\n",
	     mpiPi.size * sizeof (mpiPi_task_info_t));
	}

      bzero (mpiPi.global_task_info, mpiPi.size * sizeof (mpiPi_task_info_t));

      recv_req_arr =
	(MPI_Request *) malloc (sizeof (MPI_Request) * mpiPi.size);
      for (i = 0; i < mpiPi.size; i++)
	{
	  mpiPi_task_info_t *p = &mpiPi.global_task_info[i];
	  if (i != mpiPi.collectorRank)
	    {
	      PMPI_Irecv (p, 1, mti_type, i, mpiPi.tag,
			  mpiPi.comm, &(recv_req_arr[i]));
	    }
	  else
	    {
	      strcpy (p->hostname, mpiPi.hostname);
	      p->app_time = app_time;
	      p->rank = mpiPi.rank;
	      recv_req_arr[i] = MPI_REQUEST_NULL;
	    }
	}
      PMPI_Waitall (mpiPi.size, recv_req_arr, MPI_STATUSES_IGNORE);
      free (recv_req_arr);
      /* task MPI time is calculated from callsites data 
         in mpiPi_insert_callsite_records.
       */
      for (i = 0; i < mpiPi.size; i++)
	mpiPi.global_task_info[i].mpi_time = 0.0;
    }
  else
    {
      strcpy (mti.hostname, mpiPi.hostname);
      mti.app_time = app_time;
      mti.rank = mpiPi.rank;
      PMPI_Send (&mti, 1, mti_type, mpiPi.collectorRank, mpiPi.tag,
		 mpiPi.comm);
    }

  PMPI_Type_free (&mti_type);

  return;
}


void
mpiPi_generateReport (int report_style)
{
  mpiP_TIMER dur;
  mpiPi_TIME timer_start, timer_end;
  int mergeResult;

  mpiPi_GETTIME (&mpiPi.endTime);

  if (mpiPi.enabled)
    {
      dur =
	(mpiPi_GETTIMEDIFF (&mpiPi.endTime, &mpiPi.startTime) / 1000000.0);
      mpiPi.cumulativeTime += dur;
      assert (mpiPi.cumulativeTime >= 0);
      mpiPi_GETTIME (&mpiPi.startTime);
    }

  if (time (&mpiPi.stop_timeofday) == (time_t) - 1)
    {
      mpiPi_msg_warn ("Could not get time of day from time()\n");
    }

  /* collect results and publish */
  mpiPi_msg_debug0 ("starting collect_basics\n");

  mpiPi_GETTIME (&timer_start);
  mpiPi_collect_basics ();
  mpiPi_GETTIME (&timer_end);
  dur = (mpiPi_GETTIMEDIFF (&timer_end, &timer_start) / 1000000.0);

  mpiPi_msg_debug0 ("TIMING : collect_basics_time is %12.6f\n", dur);

  mpiPi_msg_debug0 ("starting mergeResults\n");

  mpiPi_GETTIME (&timer_start);
  mergeResult = mpiPi_mergeResults ();
  mergeResult = mpiPi_mergeCollectiveStats ();
  mergeResult = mpiPi_mergept2ptStats ();
  mpiPi_GETTIME (&timer_end);
  dur = (mpiPi_GETTIMEDIFF (&timer_end, &timer_start) / 1000000.0);

  mpiPi_msg_debug0 ("TIMING : merge time is          %12.6f\n", dur);
  mpiPi_msg_debug0 ("starting publishResults\n");

  if (mergeResult == 1)
    {
      mpiPi_GETTIME (&timer_start);
      if (mpiPi.report_style == mpiPi_style_both)
	{
	  mpiPi_publishResults (mpiPi_style_concise);
	  mpiPi_publishResults (mpiPi_style_verbose);
	}
      else
	mpiPi_publishResults (report_style);
      mpiPi_GETTIME (&timer_end);
      dur = (mpiPi_GETTIMEDIFF (&timer_end, &timer_start) / 1000000.0);
      mpiPi_msg_debug0 ("TIMING : publish time is        %12.6f\n", dur);
    }
}


void
mpiPi_finalize ()
{
  if (mpiPi.disable_finalize_report == 0)
    mpiPi_generateReport (mpiPi.report_style);

  /* clean up data structures, etc */
  h_close (mpiPi.task_callsite_stats);

  /*  Could do a lot of housekeeping before calling PMPI_Finalize()
   *  but is it worth the additional work?
   *  For instance:
   h_gather_data (mpiPi.global_callsite_stats_agg, &ac, (void ***) &av);
   for (i = 0; (i < 20) && (i < ac); i++)
   {
   if (av[i]->siteData != NULL )
   free (av[i]->siteData);
   }
   */

  return;
}


void
mpiPi_update_callsite_stats (unsigned op, unsigned rank, void **pc,
			     double dur, double sendSize, double ioSize,
			     double rmaSize)
{
  int i;
  callsite_stats_t *csp = NULL;
  callsite_stats_t key;

  if (!mpiPi.enabled)
    return;

  assert (mpiPi.task_callsite_stats != NULL);
  assert (dur >= 0);


  key.op = op;
  key.rank = rank;
  key.cookie = MPIP_CALLSITE_STATS_COOKIE;
  for (i = 0; i < MPIP_CALLSITE_STACK_DEPTH; i++)
    {
      key.pc[i] = pc[i];
    }

  if (NULL == h_search (mpiPi.task_callsite_stats, &key, (void **) &csp))
    {
      /* create and insert */
      csp = (callsite_stats_t *) malloc (sizeof (callsite_stats_t));
      bzero (csp, sizeof (callsite_stats_t));
      csp->op = op;
      csp->rank = rank;
      for (i = 0; i < MPIP_CALLSITE_STACK_DEPTH; i++)
	{
	  csp->pc[i] = pc[i];
	}
      csp->cookie = MPIP_CALLSITE_STATS_COOKIE;
      csp->minDur = DBL_MAX;
      csp->minDataSent = DBL_MAX;
      csp->minIO = DBL_MAX;
      csp->arbitraryMessageCount = 0;
      h_insert (mpiPi.task_callsite_stats, csp);
    }
  /* ASSUME: csp cannot be deleted from list */
  csp->count++;
  csp->cumulativeTime += dur;
  assert (csp->cumulativeTime >= 0);
  csp->cumulativeTimeSquared += (dur * dur);
  assert (csp->cumulativeTimeSquared >= 0);
  csp->maxDur = max (csp->maxDur, dur);
  csp->minDur = min (csp->minDur, dur);
  csp->cumulativeDataSent += sendSize;
  csp->cumulativeIO += ioSize;
  csp->cumulativeRMA += rmaSize;

  csp->maxDataSent = max (csp->maxDataSent, sendSize);
  csp->minDataSent = min (csp->minDataSent, sendSize);

  csp->maxIO = max (csp->maxIO, ioSize);
  csp->minIO = min (csp->minIO, ioSize);

  csp->maxRMA = max (csp->maxRMA, rmaSize);
  csp->minRMA = min (csp->minRMA, rmaSize);

  if (mpiPi.messageCountThreshold > -1
      && sendSize >= (double) mpiPi.messageCountThreshold)
    csp->arbitraryMessageCount++;

#if 0
  mpiPi_msg_debug ("mpiPi.messageCountThreshold is %d\n",
		   mpiPi.messageCountThreshold);
  mpiPi_msg_debug ("sendSize is %f\n", sendSize);
  mpiPi_msg_debug ("csp->arbitraryMessageCount is %lld\n",
		   csp->arbitraryMessageCount);
#endif

  return;
}


static int get_histogram_bin(mpiPi_histogram_t* h, int val)
{
  int wv = val;
  int bin;

  bin = 0;

  if ( h->bin_intervals == NULL )
    {
      while ( wv > h->first_bin_max && bin < h->hist_size )
        {
          wv >>= 1;
          bin++;
        }
    }
  else  /* Add code for custom intervals later */
    {
    }

  return bin;
}


void mpiPi_update_collective_stats(int op, double dur, double size, MPI_Comm *comm)
{
  int op_idx, comm_size, comm_bin, size_bin;

  PMPI_Comm_size(*comm, &comm_size);

  op_idx = op - mpiPi_BASE;

  comm_bin = get_histogram_bin(&mpiPi.coll_comm_histogram, comm_size);
  size_bin = get_histogram_bin(&mpiPi.coll_size_histogram, size);

  mpiPi_msg_debug("Adding %.0f time to entry mpiPi.collective_stats[%d][%d][%d] value of %.0f\n",
    dur, op_idx, comm_bin, size_bin, mpiPi.coll_time_stats[op_idx][comm_bin][size_bin]);

  mpiPi.coll_time_stats[op_idx][comm_bin][size_bin] += dur;
}


void mpiPi_update_pt2pt_stats(int op, double dur, double size, MPI_Comm *comm)
{
  int op_idx, comm_size, comm_bin, size_bin;

  PMPI_Comm_size(*comm, &comm_size);

  op_idx = op - mpiPi_BASE;

  comm_bin = get_histogram_bin(&mpiPi.pt2pt_comm_histogram, comm_size);
  size_bin = get_histogram_bin(&mpiPi.pt2pt_size_histogram, size);

  mpiPi_msg_debug("Adding %.0f send size to entry mpiPi.pt2pt_stats[%d][%d][%d] value of %.0f\n",
    size, op_idx, comm_bin, size_bin, mpiPi.pt2pt_send_stats[op_idx][comm_bin][size_bin]);

  mpiPi.pt2pt_send_stats[op_idx][comm_bin][size_bin] += size;
}


#endif /* } ifndef ENABLE_API_ONLY */



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
