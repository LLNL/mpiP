/* -*- C -*- 

   @PROLOGUE@

   ----- 

   Jeffrey Vetter vetter3@llnl.gov
   Center for Applied Scientific Computing, LLNL
   12 Aug 2001

   mpiPi.c -- main mpiP internal functions

 */

#ifndef lint
static char *rcsid =
  "$Header$";
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
      res ^= (unsigned) csp->pc[i];
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
  mpiPi.global_mpi_msize_threshold_count = 0.0;
  mpiPi.global_mpi_sent_count = 0.0;


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
  mpiPi_getenv ();

  mpiPi.task_callsite_stats =
    h_open (mpiPi.tableSize, mpiPi_callsite_stats_pc_hashkey,
	    mpiPi_callsite_stats_pc_comparator);

  /* -- welcome msg only collector  */
  if (mpiPi.collectorRank == mpiPi.rank)
    {
      mpiPi_msg ("\n");
      mpiPi_msg ("%s V%d.%d.%d (Build %s/%s)\n", mpiPi.toolname, mpiPi_vmajor,
		 mpiPi_vminor, mpiPi_vpatch, mpiPi_vdate, mpiPi_vtime);
      mpiPi_msg ("Direct questions and errors to mpip-help@llnl.gov>\n");
      mpiPi_msg ("\n");
    }

  mpiPi_msg_debug ("appName is %s\n", appName);
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

int
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

int
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
#if !defined(DISABLE_BFD) || defined(USE_LIBDWARF)
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
	  mpiPi_msg_warn ("Unsuccessful Source lookup for [%s]\n",
			  mpiP_format_address (pc, addr_buf));
	  csp->filename = strdup ("[unknown]");
	  csp->functname = strdup ("[unknown]");
	  csp->line = 0;
	}
#else /* ! DISABLE_BFD */
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

int
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

int
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

      /*  Do not bother recording frames above main  */
      if (p->functname[i] != NULL &&
	  (strcmp (p->functname[i], "MAIN__") == 0 ||
	   strcmp (p->functname[i], "main") == 0 ||
	   strcmp (p->functname[i], ".main") == 0))
	{
	  p->pc[i + 1] = NULL;
	  break;
	}
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
     one src line. We aggregate across rank after this. */
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
      csp->arbitraryMessageCount += p->arbitraryMessageCount;
    }

  /* agg. Don't use rank */
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

      if (mpiPi.calcCOV)
	{
	  csp->siteData[csp->siteDataIdx] = p->cumulativeTime;
	  csp->siteDataIdx += 1;
	}
    }

  return 1;
}


#ifndef ENABLE_API_ONLY		/* { */

int
mpiPi_mergeResults ()
{
  int ac;
  callsite_stats_t **av;
  int totalCount = 0;
  int maxCount = 0;
  mpiPi_TIME tstart, tend;
  mpiP_TIMER dur;

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

#ifndef DISABLE_BFD
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
#if !defined(DISABLE_BFD) || defined(USE_LIBDWARF)
      else
	{
	  mpiPi_msg_warn ("Failed to open executable\n");
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
      callsite_pc_cache = h_open (mpiPi.tableSize,
				  callsite_pc_cache_hashkey,
				  callsite_pc_cache_comparator);
      callsite_src_id_cache = h_open (mpiPi.tableSize,
				      callsite_src_id_cache_hashkey,
				      callsite_src_id_cache_comparator);
      /* Try to allocate space for max count of callsite info from all tasks  */
      mpiPi.rawCallsiteData =
	(callsite_stats_t *) calloc (maxCount, sizeof (callsite_stats_t));
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

  /* TODO: need to free all these pointers as well. */
  free (av);

  if (mpiPi.rank == mpiPi.collectorRank)
    {
      if (mpiPi.do_lookup == 1)
	{
#ifndef DISABLE_BFD
	  /* clean up */
	  close_bfd_executable ();
#elif defined(USE_LIBDWARF)
	  close_dwarf_executable ();
#endif
	}
    }

  return 1;
}


void
mpiPi_publishResults ()
{
  int ac;
  callsite_stats_t **av;
  int i;
  FILE *fp;
  static int printCount = 0;

  if (mpiPi.collectorRank != mpiPi.rank)
    {
      return;
    }

  /* take the final data from merge and display in a nice format */
  if (0 == h_gather_data (mpiPi.global_callsite_stats, &ac, (void ***) &av))
    {
      mpiPi_msg_warn ("Global callsite table empty! Aborting report!\n");
      return;
    }

  mpiPi_msg_debug ("Found %d entries in global callsite table.\n", ac);
  mpiPi_msg_debug ("\n");

  /* Generate output filename, and open */
  {
    char nowstr[128];
    const struct tm *nowstruct;
    char *fmtstr = "%Y %m %d %H %M %S";
    nowstruct = localtime (&mpiPi.start_timeofday);
    if (strftime (nowstr, 128, fmtstr, nowstruct) == (size_t) 0)
      mpiPi_msg_warn ("Could not get string from strftime()\n");
    for (i = 0; nowstr[i] != '\0'; i++)
      {
	if (nowstr[i] == ' ')
	  {
	    nowstr[i] = '-';
	  }
      }

    do
      {
	printCount++;
	sprintf (mpiPi.oFilename, "%s/%s.%d.%d.%d.mpiP", mpiPi.outputDir,
		 mpiPi.appName, mpiPi.size, mpiPi.procID, printCount);
      }
    while (access (mpiPi.oFilename, F_OK) == 0);

    fp = fopen (mpiPi.oFilename, "w");
  }
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
  mpiPi_profile_print (fp);
  if (fp != stdout)
    {
      fclose (fp);
    }
}

/*
 * mpiPi_collect_basics() - all tasks send their basic info to the
 * collectorRank.
 */
void
mpiPi_collect_basics ()
{
  int i = 0;
  double app_time = mpiPi.cumulativeTime;

  mpiPi_msg_debug ("Collect Basics\n");

  /* 
   * -- sweep across all tasks, collecting task information about them 
   */
  {
    int cnt;
    MPI_Status status;
    mpiPi_task_info_t mti;
    int blockcounts[5] = { 1, 1, 1, MPIPI_HOSTNAME_LEN_MAX };
    MPI_Datatype types[5] = { MPI_DOUBLE, MPI_DOUBLE, MPI_INT, MPI_CHAR };
    MPI_Aint displs[5];
    MPI_Datatype mti_type;
    MPI_Request *recv_req_arr;

    cnt = 0;
    PMPI_Address (&mti.mpi_time, &displs[cnt++]);
    PMPI_Address (&mti.app_time, &displs[cnt++]);
    PMPI_Address (&mti.rank, &displs[cnt++]);
    PMPI_Address (&mti.hostname, &displs[cnt++]);

    for (i = cnt; i >= 0; i--)
      {
	displs[i] -= displs[0];
      }
    PMPI_Type_struct (cnt, blockcounts, displs, types, &mti_type);
    PMPI_Type_commit (&mti_type);

    if (mpiPi.rank == mpiPi.collectorRank)
      {
	mpiPi.global_task_info =
	  (mpiPi_task_info_t *) calloc (mpiPi.size,
					sizeof (mpiPi_task_info_t));
	if (mpiPi.global_task_info == NULL)
	  mpiPi_abort ("Failed to allocate memory for global task_info");

	bzero (mpiPi.global_task_info,
	       mpiPi.size * sizeof (mpiPi_task_info_t));

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
      }
    else
      {
	strcpy (mti.hostname, mpiPi.hostname);
	mti.app_time = app_time;
	mti.rank = mpiPi.rank;
	PMPI_Send (&mti, 1, mti_type, mpiPi.collectorRank,
		   mpiPi.tag, mpiPi.comm);
      }

    PMPI_Type_free (&mti_type);
  }

  return;
}


void
mpiPi_generateReport ()
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

  mpiPi_msg_debug0 ("TIMING : collect_basics_time is %f\n", dur);

  mpiPi_msg_debug0 ("starting mergeResults\n");

  mpiPi_GETTIME (&timer_start);
  mergeResult = mpiPi_mergeResults ();
  mpiPi_GETTIME (&timer_end);
  dur = (mpiPi_GETTIMEDIFF (&timer_end, &timer_start) / 1000000.0);

  mpiPi_msg_debug0 ("TIMING : merge time is %f\n", dur);
  mpiPi_msg_debug0 ("starting publishResults\n");

  if (mergeResult == 1)
    {
      mpiPi_GETTIME (&timer_start);
      mpiPi_publishResults ();
      mpiPi_GETTIME (&timer_end);
      dur = (mpiPi_GETTIMEDIFF (&timer_end, &timer_start) / 1000000.0);
      mpiPi_msg_debug0 ("TIMING : publish time is %f\n", dur);
    }

}


void
mpiPi_finalize ()
{
  mpiPi_generateReport ();

  /* clean up data structures, etc */
  h_close (mpiPi.task_callsite_stats);

  return;
}


void
mpiPi_update_callsite_stats (unsigned op, unsigned rank, void **pc,
			     double dur, double sendSize, double ioSize)
{
  int i;
  callsite_stats_t *csp = NULL;
  callsite_stats_t key;

  if (!mpiPi.enabled)
    return;

/*  fprintf(stderr, "received sendSize of %f\n", sendSize); */

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

  csp->maxDataSent = max (csp->maxDataSent, sendSize);
  csp->minDataSent = min (csp->minDataSent, sendSize);

  csp->maxIO = max (csp->maxIO, ioSize);
  csp->minIO = min (csp->minIO, ioSize);


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

#endif /* } ifndef ENABLE_API_ONLY */

/* eof */
