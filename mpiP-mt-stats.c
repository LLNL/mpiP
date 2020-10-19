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
  mpiPi_mt_stat_tls_t *hndl = (mpiPi_mt_stat_tls_t *)data;
  mpiPi_thread_stat_t *s = hndl->tls_ptr;
  /* Stop the timer and disable this TLS */
  mpiPi_stats_thr_timer_stop(hndl->tls_ptr);
  s->disabled = 1;
  hndl->is_active = 0;
}

/*
 * ============================================================================
 *
 * Multi-threaded layer
 *
 * ============================================================================
 */

static mpiPi_mt_stat_tls_t *_alloc_tls(mpiPi_mt_stat_t *mt_state)
{
  mpiPi_mt_stat_tls_t *hndl = calloc(1, sizeof(*hndl));
  if (NULL == hndl) {
      mpiPi_abort("failed to allocate TLS handler");
    }
  hndl->mt_state = mt_state;
  hndl->tls_ptr = calloc(1, sizeof(*hndl->mt_state));
  if (NULL == hndl->tls_ptr) {
      mpiPi_abort("failed to allocate TLS");
    }
  return hndl;
}

static void _free_tls(mpiPi_mt_stat_tls_t *hndl)
{
  hndl->mt_state = NULL;
  free(hndl->tls_ptr);
  free(hndl);
}

static void _release_all_tls(mpiP_tslist_t *tls_list)
{
  mpiPi_mt_stat_tls_t *hndl = mpiPi_tslist_dequeue(tls_list);
  while(hndl) {
      _free_tls(hndl);
      hndl = mpiPi_tslist_dequeue(tls_list);
    }
}


int mpiPi_stats_mt_init(mpiPi_mt_stat_t *mt_state, mpiPi_thr_mode_t mode)
{
  mt_state->mode = mode;
  mpiPi_stats_thr_init(&mt_state->rank_stats);
  switch(mt_state->mode) {
    case MPIPI_MODE_ST:
      mt_state->st_hndl.mt_state = mt_state;
      mt_state->st_hndl.tls_ptr = &mt_state->rank_stats;
    case MPIPI_MODE_MT:
      mt_state->tls_list = mpiPi_tslist_create();
      pthread_key_create(&mt_state->tls_this, key_destruct);
    default:
      break;
    }

  /* Initialize TLS for the main thread */
  (void)mpiPi_stats_mt_gettls(mt_state);

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
      _release_all_tls(mt_state->tls_list);
      mpiPi_tslist_release(mt_state->tls_list);
      pthread_key_delete(mt_state->tls_this);
      mpiPi_stats_thr_init(&mt_state->rank_stats);
      break;
    }
}


mpiPi_mt_stat_tls_t *
mpiPi_stats_mt_gettls(mpiPi_mt_stat_t *mt_state)
{
  mpiPi_mt_stat_tls_t *hndl = NULL;

  if (MPIPI_MODE_ST == mt_state->mode)
    {
      /* For all non-MPI_THREAD_MULTIPLE modes use
       * embedded data structure
       */
      return &mt_state->st_hndl;
    } else {
      /* For the MPI_THREAD_MULTIPLE mode use TLS
       */
      hndl = pthread_getspecific(mt_state->tls_this);
      if (NULL == hndl)
        {
          hndl = _alloc_tls(mt_state);
          (void) pthread_setspecific(mt_state->tls_this, hndl);
          mpiPi_stats_thr_init(hndl->tls_ptr);
          hndl->is_active = 1;
          if( mpiPi.enabled ) {
              /* NOTE: this is not precise!
               * - The thread could have been started long time ago
               * - In the scope of mpiP we only rely on the MPI API
               *   hich doesn't track thread creation
               * - we only can only start tracking a thread after the first
               *   invocation of any MPI operation tracked by mpiP.
               * - This is still important to track threads at least this way
               *   because we don't want to have MPI operations to span more
               *   than 100% of the application time.
               */
              mpiPi_stats_thr_timer_start(hndl->tls_ptr);
            }
          mpiPi_tslist_append(mt_state->tls_list, hndl);
        }
    }
  return hndl;
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
      mpiPi_mt_stat_tls_t *hndl = curr->ptr;
      mpiPi_thread_stat_t *s = hndl->tls_ptr;
      mpiPi_stats_thr_merge_all(&mt_state->rank_stats, s);
      curr = mpiPi_tslist_next(curr);
    }
}

void mpiPi_stats_mt_timer_start(mpiPi_mt_stat_t *mt_state)
{
  mpiP_tslist_elem_t *curr = NULL;

  if(MPIPI_MODE_ST == mt_state->mode) {
      /* Only update the cumulative time */
      mpiPi_stats_thr_timer_start(&mt_state->rank_stats);
      return;
    }

  curr = mpiPi_tslist_first(mt_state->tls_list);
  /* Go over all TLS structures and update the timers */
  while (curr)
    {
      mpiPi_mt_stat_tls_t *hndl = curr->ptr;
      if( hndl->is_active )
        {
          mpiPi_stats_thr_timer_start(hndl->tls_ptr);
        }
      curr = mpiPi_tslist_next(curr);
    }
}

void mpiPi_stats_mt_timer_stop(mpiPi_mt_stat_t *mt_state)
{
  mpiP_tslist_elem_t *curr = NULL;

  if(MPIPI_MODE_ST == mt_state->mode) {
      /* Only update the cumulative time */
      mpiPi_stats_thr_timer_stop(&mt_state->rank_stats);
      return;
    }

  curr = mpiPi_tslist_first(mt_state->tls_list);
  /* Go over all TLS structures and update the timers */
  while (curr)
    {
      mpiPi_mt_stat_tls_t *hndl = curr->ptr;
      if( hndl->is_active )
        {
          mpiPi_stats_thr_timer_stop(hndl->tls_ptr);
        }
      curr = mpiPi_tslist_next(curr);
    }
}

double mpiPi_stats_mt_cum_time(mpiPi_mt_stat_t *mt_state)
{
    return mpiPi_stats_thr_cum_time(&mt_state->rank_stats);
}

void mpiPi_stats_mt_cs_gather(mpiPi_mt_stat_t *mt_state,
                             int *ac, callsite_stats_t ***av)
{
  mpiPi_stats_thr_cs_gather(&mt_state->rank_stats, ac, av);
}

void mpiPi_stats_mt_cs_upd (mpiPi_mt_stat_tls_t *hndl,
                            unsigned op, unsigned rank, void **pc,
                            double dur, double sendSize, double ioSize,
                            double rmaSize)
{
  mpiPi_stats_thr_cs_upd(hndl->tls_ptr, op, rank, pc, dur,
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

void mpiPi_stats_mt_coll_upd(mpiPi_mt_stat_tls_t *hndl,
                             int op, double dur, double size,
                             MPI_Comm * comm)
{
  mpiPi_stats_thr_coll_upd(hndl->tls_ptr, op, dur, size, comm);
}

void mpiPi_stats_mt_coll_gather(mpiPi_mt_stat_t *stat, double **_outbuf)
{
  mpiPi_stats_thr_coll_gather(&stat->rank_stats, _outbuf);
}

void mpiPi_stats_mt_cs_reset(mpiPi_mt_stat_t *mt_state)
{
  mpiP_tslist_elem_t *curr = NULL;

  mpiPi_stats_thr_cs_reset(&mt_state->rank_stats);

  if(MPIPI_MODE_ST == mt_state->mode) {
      /* Only update the cumulative time */
      return;
    }

  curr = mpiPi_tslist_first(mt_state->tls_list);

  /* Go over all TLS structures and merge them into the rank-wide callsite info */
  while (curr)
    {
      mpiPi_mt_stat_tls_t *hndl = curr->ptr;
      mpiPi_stats_thr_cs_reset(hndl->tls_ptr);
      curr = mpiPi_tslist_next(curr);
    }
}

void mpiPi_stats_mt_coll_binstrings(mpiPi_mt_stat_t *stat,
                                     int comm_idx, char *comm_buf,
                                     int size_idx, char *size_buf)
{
  mpiPi_stats_thr_coll_binstrings(&stat->rank_stats,
                                  comm_idx, comm_buf,
                                  size_idx, size_buf);
}

void mpiPi_stats_mt_pt2pt_upd (mpiPi_mt_stat_tls_t *hndl,
                               int op, double dur, double size,
                               MPI_Comm * comm)
{
  mpiPi_stats_thr_pt2pt_upd(hndl->tls_ptr, op, dur, size, comm);
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

void mpiPi_stats_mt_exit(mpiPi_mt_stat_tls_t *hndl)
{
  mpiPi_stats_thr_exit(hndl->tls_ptr);
}

void mpiPi_stats_mt_enter(mpiPi_mt_stat_tls_t *hndl)
{
  mpiPi_stats_thr_enter(hndl->tls_ptr);
}

int mpiPi_stats_mt_is_on(mpiPi_mt_stat_tls_t *hndl)
{
  return mpiPi_stats_thr_is_on(hndl->tls_ptr);
}
