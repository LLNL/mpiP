#include "mpiP-stats.h"
#include "mpiP-mt-stats.h"

int mpiPi_stats_mt_init(mpiPi_mt_stat_t *stat)
{
  mpiPi_stats_thr_init(&stat->st_stat);
}
void mpiPi_stats_mt_fini(mpiPi_mt_stat_t *stat)
{
  mpiPi_stats_thr_fini(&stat->st_stat);
}

void mpiPi_stats_mt_cs_gather(mpiPi_mt_stat_t *stat,
                             int *ac, callsite_stats_t ***av )
{
  mpiPi_stats_thr_cs_gather(&stat->st_stat, ac, av);
}

void mpiPi_stats_mt_cs_upd (mpiPi_mt_stat_t *stat,
                           unsigned op, unsigned rank, void **pc,
                           double dur, double sendSize, double ioSize,
                           double rmaSize)
{
  mpiPi_stats_thr_cs_upd(&stat->st_stat,
                         op, rank, pc, dur,
                         sendSize, ioSize, rmaSize);
}

void mpiPi_stats_mt_cs_lookup(mpiPi_mt_stat_t *stat,
                              callsite_stats_t *task_stats,
                              callsite_stats_t **task_lookup,
                              callsite_stats_t *dummy_buf,
                              int initMax)
{
  mpiPi_stats_thr_cs_lookup(&stat->st_stat, task_stats, task_lookup,
                            dummy_buf, initMax);
}

void mpiPi_stats_mt_coll_upd(mpiPi_mt_stat_t *stat,
                                  int op, double dur, double size,
                                  MPI_Comm * comm)
{
  mpiPi_stats_thr_coll_upd(&stat->st_stat, op, dur, size, comm);
}

void mpiPi_stats_mt_coll_gather(mpiPi_mt_stat_t *stat, double **_outbuf)
{
  mpiPi_stats_thr_coll_gather(&stat->st_stat, _outbuf);
}

void mpiPi_stats_mt_cs_reset(mpiPi_mt_stat_t *stat)
{
  mpiPi_stats_thr_cs_reset(&stat->st_stat);
}

void mpiPi_stats_mt_coll_binstrings(mpiPi_mt_stat_t *stat,
                                     int comm_idx, char *comm_buf,
                                     int size_idx, char *size_buf)
{
  mpiPi_stats_thr_coll_binstrings(&stat->st_stat,
                                  comm_idx, comm_buf,
                                  size_idx, size_buf);
}

void mpiPi_stats_mt_pt2pt_upd (mpiPi_mt_stat_t *stat,
                                   int op, double dur, double size,
                                   MPI_Comm * comm)
{
  mpiPi_stats_thr_pt2pt_upd(&stat->st_stat, op, dur, size, comm);
}

void mpiPi_stats_mt_pt2pt_gather(mpiPi_mt_stat_t *stat, double **_outbuf)
{
  mpiPi_stats_thr_pt2pt_gather(&stat->st_stat, _outbuf);
}

void mpiPi_stats_mt_pt2pt_binstrings(mpiPi_mt_stat_t *stat,
                                     int comm_idx, char *comm_buf,
                                     int size_idx, char *size_buf)
{
  mpiPi_stats_thr_pt2pt_binstrings(&stat->st_stat, comm_idx, comm_buf,
                                   size_idx, size_buf);
}

int mpiPi_stats_mt_exit(mpiPi_mt_stat_t *stat)
{
  mpiPi_stats_thr_exit(&stat->st_stat);
}

int mpiPi_stats_mt_enter(mpiPi_mt_stat_t *stat)
{
  mpiPi_stats_thr_enter(&stat->st_stat);
}

int mpiPi_stats_mt_is_on(mpiPi_mt_stat_t *stat)
{
  mpiPi_stats_thr_is_on(&stat->st_stat);
}
