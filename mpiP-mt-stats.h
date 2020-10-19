#ifndef MPIPMTSTATS_H
#define MPIPMTSTATS_H

#include "mpiP-stats.h"
#include "mpiP-tslist.h"
#include <pthread.h>

typedef enum {
  MPIPI_MODE_ST,
  MPIPI_MODE_MT
} mpiPi_thr_mode_t;

typedef struct mpiPi_mt_stat_s mpiPi_mt_stat_t;

typedef struct {
  mpiPi_mt_stat_t *mt_state;
  int is_active;
  mpiPi_thread_stat_t *tls_ptr;
} mpiPi_mt_stat_tls_t;

/* Per-thread MPI status */
typedef struct mpiPi_mt_stat_s {
  /* generic part */
  mpiPi_thr_mode_t mode;
  mpiPi_mt_stat_tls_t st_hndl;
  mpiPi_thread_stat_t rank_stats;

  /* MT-only part */
  mpiP_tslist_t *tls_list;
  pthread_key_t tls_this;
} mpiPi_mt_stat_t;

int mpiPi_stats_mt_init(mpiPi_mt_stat_t *stat, mpiPi_thr_mode_t mode);
void mpiPi_stats_mt_fini(mpiPi_mt_stat_t *stat);
void mpiPi_stats_mt_merge(mpiPi_mt_stat_t *mt_state);

void mpiPi_stats_mt_timer_stop(mpiPi_mt_stat_t *mt_state);
void mpiPi_stats_mt_timer_start(mpiPi_mt_stat_t *mt_state);
double mpiPi_stats_mt_cum_time(mpiPi_mt_stat_t *mt_state);
mpiPi_mt_stat_tls_t *mpiPi_stats_mt_gettls(mpiPi_mt_stat_t *mt_state);

void mpiPi_stats_mt_cs_gather(mpiPi_mt_stat_t *stat,
                             int *ac, callsite_stats_t ***av );
void mpiPi_stats_mt_cs_upd (mpiPi_mt_stat_tls_t *hndl,
                            unsigned op, unsigned rank, void **pc,
                            double dur, double sendSize, double ioSize,
                            double rmaSize);

void mpiPi_stats_mt_cs_lookup(mpiPi_mt_stat_t *stat,
                              callsite_stats_t *task_stats,
                              callsite_stats_t **task_lookup,
                              callsite_stats_t *dummy_buf,
                              int initMax);

void mpiPi_stats_mt_coll_upd(mpiPi_mt_stat_tls_t *hndl,
                             int op, double dur, double size,
                             MPI_Comm * comm);
void mpiPi_stats_mt_coll_gather(mpiPi_mt_stat_t *stat, double **_outbuf);
void mpiPi_stats_mt_cs_reset(mpiPi_mt_stat_t *stat);
void mpiPi_stats_mt_coll_binstrings(mpiPi_mt_stat_t *stat,
                                     int comm_idx, char *comm_buf,
                                     int size_idx, char *size_buf);

void mpiPi_stats_mt_pt2pt_upd (mpiPi_mt_stat_tls_t *hndl,
                               int op, double dur, double size,
                               MPI_Comm * comm);
void mpiPi_stats_mt_pt2pt_gather(mpiPi_mt_stat_t *stat, double **_outbuf);
void mpiPi_stats_mt_pt2pt_binstrings(mpiPi_mt_stat_t *stat,
                                     int comm_idx, char *comm_buf,
                                     int size_idx, char *size_buf);

void mpiPi_stats_mt_exit(mpiPi_mt_stat_tls_t *hndl);
void mpiPi_stats_mt_enter(mpiPi_mt_stat_tls_t *hndl);
int mpiPi_stats_mt_is_on(mpiPi_mt_stat_tls_t *hndl);

#endif // MPIPMTSTATS_H

/*

  <license>

  Copyright (c) 2019, Mellanox Technologies Inc.
  Written by Artem Polyakov
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
