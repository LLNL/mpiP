#include "mpiPi.h"
#include "mpiP-stats.h"
#include "mpiP-mt-stats.h"
#include "mpiPi_proto.h"

/*
 * ============================================================================
 *
 * Multi-thread management code
 *
 * ============================================================================
 */

static void key_destruct(void *data)
{
  mpiPi_thread_stat_t *stat = (mpiPi_thread_stat_t *)data;
  /* TODO: do we need to do anything here? */
}

static inline mpiPi_thread_stat_t *
_get_local_tls(mpiPi_mt_stat_t *mt_state)
{
  mpiPi_thread_stat_t *st_state;

  if (MPIPI_MODE_ST == mt_state->mode)
    {
      /* For all non-MPI_THREAD_MULTIPLE modes use
       * embedded data structure
       */
      st_state = &mt_state->rank_stats;
    } else {
      /* For the MPI_THREAD_MULTIPLE mode use TLS
       */
      st_state = pthread_getspecific(mt_state->tls_this);
      if (NULL == st_state)
        {
          st_state = malloc(sizeof(mpiPi_thread_stat_t));
          if (NULL == st_state) {
              mpiPi_abort("failed to allocate TLS");
            }
          (void) pthread_setspecific(mt_state->tls_this, st_state);
          mpiPi_stats_thr_init(st_state);
          mpiPi_tslist_append(mt_state->tls_list, st_state);
        }
    }
  return st_state;
}

static void _merge_mt_stat(mpiPi_mt_stat_t *mt_state)
{

}


/*
 * ============================================================================
 *
 * Multi-threaded layer
 *
 * ============================================================================
 */

int mpiPi_stats_mt_init(mpiPi_mt_stat_t *mt_state, mpiPi_thr_mode_t mode)
{
  mt_state->mode = mode;
  mpiPi_stats_thr_init(&mt_state->rank_stats);
  switch(mt_state->mode) {
    case MPIPI_MODE_MT:
      mt_state->tls_list = mpiPi_tslist_create();
      pthread_key_create(&mt_state->tls_this, key_destruct);
    default:
      break;
    }
  return 0;
}

void mpiPi_stats_mt_fini(mpiPi_mt_stat_t *mt_state)
{
  switch(mt_state->mode)
    {
    case MPIPI_MODE_ST:
      mpiPi_stats_thr_fini(&mt_state->rank_stats);
      break;
    case MPIPI_MODE_MT:
      // TODO: Make sure that list is drained
      mpiPi_tslist_release(mt_state->tls_list);
      pthread_key_delete(mt_state->tls_this);
      mpiPi_stats_thr_init(&mt_state->rank_stats);
      break;
    }
}

void mpiPi_stats_mt_merge(mpiPi_mt_stat_t *mt_state)
{
  mpiP_tslist_elem_t *curr = NULL;

  if(MPIPI_MODE_ST == mt_state->mode) {
      /* Nothing to do here */
      return;
    }
  curr = mpiPi_tslist_first(mt_state->tls_list);

  mpiPi_stats_thr_cs_reset(&mt_state->rank_stats);

  /* Go over all TLS structures and merge them into the rank-wide callsite info */
  while (curr)
    {
      mpiPi_stats_thr_merge_all(&mt_state->rank_stats, curr->ptr);
      curr = mpiPi_tslist_next(curr);
    }
}

void mpiPi_stats_mt_cs_gather(mpiPi_mt_stat_t *mt_state,
                             int *ac, callsite_stats_t ***av )
{
  switch(mt_state->mode)
    {
    case MPIPI_MODE_ST:
      mpiPi_stats_thr_cs_gather(&mt_state->rank_stats, ac, av);
      break;
    case MPIPI_MODE_MT:
      mpiPi_stats_thr_cs_gather(&mt_state->rank_stats, ac, av);
      break;
    }
}

void mpiPi_stats_mt_cs_upd (mpiPi_mt_stat_t *mt_state,
                           unsigned op, unsigned rank, void **pc,
                           double dur, double sendSize, double ioSize,
                           double rmaSize)
{
  mpiPi_thread_stat_t *st_state = NULL;
  st_state = _get_local_tls(mt_state);
  mpiPi_stats_thr_cs_upd(st_state, op, rank, pc, dur,
                         sendSize, ioSize, rmaSize);
}

void mpiPi_stats_mt_cs_lookup(mpiPi_mt_stat_t *stat,
                              callsite_stats_t *task_stats,
                              callsite_stats_t **task_lookup,
                              callsite_stats_t *dummy_buf,
                              int initMax)
{
  mpiPi_stats_thr_cs_lookup(&stat->rank_stats, task_stats, task_lookup,
                            dummy_buf, initMax);
}

void mpiPi_stats_mt_coll_upd(mpiPi_mt_stat_t *mt_state,
                                  int op, double dur, double size,
                                  MPI_Comm * comm)
{
  mpiPi_thread_stat_t *st_state;
  st_state = _get_local_tls(mt_state);
  mpiPi_stats_thr_coll_upd(st_state, op, dur, size, comm);
}

void mpiPi_stats_mt_coll_gather(mpiPi_mt_stat_t *stat, double **_outbuf)
{
  mpiPi_stats_thr_coll_gather(&stat->rank_stats, _outbuf);
}

void mpiPi_stats_mt_cs_reset(mpiPi_mt_stat_t *stat)
{
  mpiPi_stats_thr_cs_reset(&stat->rank_stats);
}

void mpiPi_stats_mt_coll_binstrings(mpiPi_mt_stat_t *stat,
                                     int comm_idx, char *comm_buf,
                                     int size_idx, char *size_buf)
{
  mpiPi_stats_thr_coll_binstrings(&stat->rank_stats,
                                  comm_idx, comm_buf,
                                  size_idx, size_buf);
}

void mpiPi_stats_mt_pt2pt_upd (mpiPi_mt_stat_t *mt_state,
                                   int op, double dur, double size,
                                   MPI_Comm * comm)
{
  mpiPi_thread_stat_t *st_state;
  st_state = _get_local_tls(mt_state);
  mpiPi_stats_thr_pt2pt_upd(st_state, op, dur, size, comm);
}

void mpiPi_stats_mt_pt2pt_gather(mpiPi_mt_stat_t *stat, double **_outbuf)
{
  mpiPi_stats_thr_pt2pt_gather(&stat->rank_stats, _outbuf);
}

void mpiPi_stats_mt_pt2pt_binstrings(mpiPi_mt_stat_t *stat,
                                     int comm_idx, char *comm_buf,
                                     int size_idx, char *size_buf)
{
  mpiPi_stats_thr_pt2pt_binstrings(&stat->rank_stats, comm_idx, comm_buf,
                                   size_idx, size_buf);
}

int mpiPi_stats_mt_exit(mpiPi_mt_stat_t *mt_state)
{
  mpiPi_thread_stat_t *st_state;
  st_state = _get_local_tls(mt_state);
  mpiPi_stats_thr_exit(st_state);
}

int mpiPi_stats_mt_enter(mpiPi_mt_stat_t *mt_state)
{
  mpiPi_thread_stat_t *st_state;
  st_state = _get_local_tls(mt_state);
  mpiPi_stats_thr_enter(st_state);
}

int mpiPi_stats_mt_is_on(mpiPi_mt_stat_t *mt_state)
{
  mpiPi_thread_stat_t *st_state;
  st_state = _get_local_tls(mt_state);
  mpiPi_stats_thr_is_on(st_state);
}
