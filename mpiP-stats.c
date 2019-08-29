#include <stdlib.h>
#include <string.h>
#include "mpiP-stats.h"
#include "mpiPi.h"

/*
 * ============================================================================
 *
 * Histogram code
 *
 * ============================================================================
 */

static int
get_histogram_bin (mpiPi_histogram_t * h, int val)
{
  int wv = val;
  int bin;

  bin = 0;

  if (h->bin_intervals == NULL)
    {
      while (wv > h->first_bin_max && bin < h->hist_size)
        {
          wv >>= 1;
          bin++;
        }
    }
  else   /* Add code for custom intervals later */
    {
    }

  return bin;
}


void
init_histogram (mpiPi_histogram_t * h, int first_bin_max, int size,
                int *intervals)
{
  h->first_bin_max = first_bin_max;
  h->hist_size = size;
  h->bin_intervals = intervals;
}

void
get_histogram_bin_str (mpiPi_histogram_t * h, int v, char *s)
{
  int min = 0, max = 0;

  if (v == 0)
    {
      min = 0;
      max = h->first_bin_max;
    }
  else
    {
      min = h->first_bin_max + 1;
      min <<= (v - 1);
      max = (min << 1) - 1;
    }

  sprintf (s, "%8d - %8d", min, max);
}

/*
 * ============================================================================
 *
 * Per-thread statistics
 *
 * ============================================================================
 */


static int
_thrd_pc_hashkey (const void *p)
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
trd_pc_comparator (const void *p1, const void *p2)
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

int mpiPi_stats_thr_init(mpiPi_thread_stat_t *stat)
{
  stat->task_callsite_stats = h_open (mpiPi.tableSize, _thrd_pc_hashkey,
                                      trd_pc_comparator);

  if (mpiPi.do_collective_stats_report == 1)
    {
      init_histogram (&stat->coll.comm_hist, 7, MPIP_COMM_HISTCNT, NULL);
      init_histogram (&stat->coll.size_hist, 7, MPIP_SIZE_HISTCNT, NULL);
    }

  if (mpiPi.do_pt2pt_stats_report == 1)
    {
      init_histogram (&stat->pt2pt.comm_hist, 7, MPIP_COMM_HISTCNT, NULL);
      init_histogram (&stat->pt2pt.size_hist, 7, MPIP_SIZE_HISTCNT, NULL);
    }
}

void mpiPi_stats_thr_fini(mpiPi_thread_stat_t *stat)
{
  h_close (stat->task_callsite_stats);
}

int mpiPi_stats_thr_exit(mpiPi_thread_stat_t *stat)
{
  stat->disabled++;
}

int mpiPi_stats_thr_enter(mpiPi_thread_stat_t *stat)
{
  stat->disabled--;
}

int mpiPi_stats_thr_is_on(mpiPi_thread_stat_t *stat)
{
  return !(stat->disabled) && mpiPi.enabled;
}

/* Callsite statistics */
void
mpiPi_stats_thr_cs_upd (mpiPi_thread_stat_t *stat,
                           unsigned op, unsigned rank, void **pc,
                           double dur, double sendSize, double ioSize,
                           double rmaSize)
{
  int i;
  callsite_stats_t *csp = NULL;
  callsite_stats_t key;

  assert (dur >= 0);

  key.op = op;
  key.rank = rank;
  key.cookie = MPIP_CALLSITE_STATS_COOKIE;
  for (i = 0; i < MPIP_CALLSITE_STACK_DEPTH; i++)
    {
      key.pc[i] = pc[i];
    }

  if (NULL == h_search (stat->task_callsite_stats, &key, (void **) &csp))
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
      csp->cumulativeTime = 0;
      csp->minDur = DBL_MAX;
      csp->minDataSent = DBL_MAX;
      csp->minIO = DBL_MAX;
      csp->arbitraryMessageCount = 0;
      h_insert (stat->task_callsite_stats, csp);
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

void mpiPi_stats_thr_cs_gather(mpiPi_thread_stat_t *stat,
                             int *ac, callsite_stats_t ***av )
{
  h_gather_data (stat->task_callsite_stats, ac, (void ***)av);
}

void mpiPi_stats_thr_cs_reset(mpiPi_thread_stat_t *stat)
{
  int ac, ndx;
  callsite_stats_t **av;
  callsite_stats_t *csp = NULL;

  /* gather local task data */
  mpiPi_stats_thr_cs_gather(stat, &ac, &av );

  for (ndx = 0; ndx < ac; ndx++)
    {
      csp = av[ndx];

      csp->maxDur = 0;
      csp->minDur = DBL_MAX;
      csp->maxIO = 0;
      csp->minIO = DBL_MAX;
      csp->maxDataSent = 0;
      csp->minDataSent = DBL_MAX;

      csp->count = 0;
      csp->cumulativeTime = 0;
      csp->cumulativeTimeSquared = 0;
      csp->cumulativeDataSent = 0;
      csp->cumulativeIO = 0;

      csp->arbitraryMessageCount = 0;
    }
  free(av);

}

void mpiPi_stats_thr_cs_lookup(mpiPi_thread_stat_t *stat,
                              callsite_stats_t *task_stats,
                              callsite_stats_t **task_lookup,
                              callsite_stats_t *dummy_buf,
                              int initMax)
{
  callsite_stats_t *record = NULL;
  if (NULL == h_search(stat->task_callsite_stats,
                       task_stats,(void **)&record))
    {
      record = dummy_buf;
      record->count = 0;
      record->cumulativeTime = 0;
      record->cumulativeTimeSquared = 0;
      record->maxDur = 0;
      if (initMax) {
          record->minDur = DBL_MAX;
        } else {
          record->minDur = 0;
        }
      record->cumulativeDataSent = 0;
      record->cumulativeIO = 0;
      record->maxDataSent = 0;
      if (initMax) {
          record->minDataSent = DBL_MAX;
        } else {
          record->minDataSent = 0;
        }
      record->maxIO = 0;
      if (initMax){
          record->minIO = DBL_MAX;
        } else {
          record->minIO = 0;
        }
      record->arbitraryMessageCount = 0;
      record->rank = mpiPi.rank;
    }
  *task_lookup = record;
}

/* Message size statistics */
static void _gather_msize_stat(mpiPi_msg_stat_t *stat,
                               double **_outbuf)
{
  double *outbuf = malloc(sizeof(stat->time_stats));
  memcpy(outbuf, stat->time_stats, sizeof(stat->time_stats));
  *_outbuf = outbuf;
}

static void _update_msize_stat (mpiPi_msg_stat_t *stat,
                                int op, double dur, double size,
                                MPI_Comm * comm,
                                char *dbg_prefix)
{
  int op_idx, comm_size, comm_bin, size_bin;

  PMPI_Comm_size (*comm, &comm_size);

  op_idx = op - mpiPi_BASE;

  comm_bin = get_histogram_bin (&stat->comm_hist, comm_size);
  size_bin = get_histogram_bin (&stat->size_hist, size);

  mpiPi_msg_debug
      ("Adding %.0f send size to entry %s[%d][%d][%d] value of %.0f\n",
       size, dbg_prefix,
       op_idx, comm_bin, size_bin,
       stat->time_stats[op_idx][comm_bin][size_bin]);

  stat->time_stats[op_idx][comm_bin][size_bin] += size;
}

static void _get_binstrings (mpiPi_msg_stat_t *stat,
                             int comm_idx, char *comm_buf,
                             int size_idx, char *size_buf)
{
  get_histogram_bin_str (&stat->comm_hist, comm_idx, comm_buf);
  get_histogram_bin_str (&stat->size_hist, size_idx, size_buf);
}


/* Collectives msg size stat */
void mpiPi_stats_thr_coll_upd (mpiPi_thread_stat_t *stat,
                                  int op, double dur, double size,
                                  MPI_Comm * comm)
{
  _update_msize_stat(&stat->coll, op, dur, size, comm, "collectives");
}

void mpiPi_stats_thr_coll_gather(mpiPi_thread_stat_t *stat, double **_outbuf)
{
  _gather_msize_stat(&stat->coll, _outbuf);
}

void mpiPi_stats_thr_coll_binstrings(mpiPi_thread_stat_t *stat,
                                     int comm_idx, char *comm_buf,
                                     int size_idx, char *size_buf)
{
  _get_binstrings(&stat->coll, comm_idx, comm_buf, size_idx, size_buf);
}


/* Point-to-point statistics */
void mpiPi_stats_thr_pt2pt_upd (mpiPi_thread_stat_t *stat,
                                   int op, double dur, double size,
                                   MPI_Comm * comm)
{
  _update_msize_stat(&stat->pt2pt, op, dur, size, comm, "point-to-point");
}

void mpiPi_stats_thr_pt2pt_gather(mpiPi_thread_stat_t *stat, double **_outbuf)
{
  _gather_msize_stat(&stat->pt2pt, _outbuf);
}

void mpiPi_stats_thr_pt2pt_binstrings(mpiPi_thread_stat_t *stat,
                                     int comm_idx, char *comm_buf,
                                     int size_idx, char *size_buf)
{
  _get_binstrings(&stat->pt2pt, comm_idx, comm_buf, size_idx, size_buf);
}

