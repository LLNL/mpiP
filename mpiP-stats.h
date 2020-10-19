/* -*- C -*- 

   mpiP MPI Profiler ( http://llnl.github.io/mpiP )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   ----- 

   hash.h -- generic hash table

   $Id$

*/

#ifndef _MPIP_STATS_H
#define _MPIP_STATS_H

#include <mpi.h>
#include <float.h>
#include "mpiPconfig.h"
#include "mpiP-hash.h"
#include "mpiPi_def.h"
#include "mpiP-callsites.h"
#include "mpip_timers.h"

/* Histograms */
typedef struct _mpiPi_histogram
{
  int first_bin_max;
  int hist_size;
  int *bin_intervals;
} mpiPi_histogram_t;

/* Message/Communicator statistics */
#define MPIP_NFUNC (mpiPi_DEF_END - mpiPi_BASE)
#define MPIP_COMM_HISTCNT 32
#define MPIP_SIZE_HISTCNT 32
typedef struct {
  /* Collectives statistics */
  mpiPi_histogram_t comm_hist;
  mpiPi_histogram_t size_hist;
  double time_stats[MPIP_NFUNC][MPIP_COMM_HISTCNT][MPIP_SIZE_HISTCNT];
} mpiPi_msg_stat_t;

/* Per-thread MPI status */
typedef struct {
  /* Avoid double-measurement when MPI functions are
   * invoked internally e.g. in collectives
   */
  int disabled;

  /* Usage time */
  mpiPi_TIME ts_start, ts_end;
  double cum_time;

  /* Callsite statistics */
  h_t *cs_stats;
  /* Collectives and point-to-point statistics */
  mpiPi_msg_stat_t coll, pt2pt;
} mpiPi_thread_stat_t;

void mpiPi_stats_thr_init(mpiPi_thread_stat_t *stat);
void mpiPi_stats_thr_fini(mpiPi_thread_stat_t *stat);
void mpiPi_stats_thr_reset_all(mpiPi_thread_stat_t *stat);
void mpiPi_stats_thr_merge_all(mpiPi_thread_stat_t *dst,
                               mpiPi_thread_stat_t *src);

void mpiPi_stats_thr_timer_start(mpiPi_thread_stat_t *s);
void mpiPi_stats_thr_timer_stop(mpiPi_thread_stat_t *s);
double mpiPi_stats_thr_cum_time(mpiPi_thread_stat_t *s);

void mpiPi_stats_thr_cs_gather(mpiPi_thread_stat_t *stat,
                             int *ac, callsite_stats_t ***av );

void mpiPi_stats_thr_cs_upd (mpiPi_thread_stat_t *stat,
                           unsigned op, unsigned rank, void **pc,
                           double dur, double sendSize, double ioSize,
                           double rmaSize);
#define MPIPI_CALLSITE_MIN2MAX 1
#define MPIPI_CALLSITE_MIN2ZERO 0
void mpiPi_stats_thr_cs_lookup(mpiPi_thread_stat_t *stat,
                              callsite_stats_t *task_stats,
                              callsite_stats_t **task_lookup,
                              callsite_stats_t *dummy_buf,
                              int initMax);
void mpiPi_stats_thr_cs_merge(mpiPi_thread_stat_t *dst,
                              mpiPi_thread_stat_t *src);

void mpiPi_stats_thr_coll_upd(mpiPi_thread_stat_t *stat,
                                  int op, double dur, double size,
                                  MPI_Comm * comm);
void mpiPi_stats_thr_coll_gather(mpiPi_thread_stat_t *stat, double **_outbuf);
void mpiPi_stats_thr_cs_reset(mpiPi_thread_stat_t *stat);
void mpiPi_stats_thr_coll_merge(mpiPi_thread_stat_t *dst,
                                mpiPi_thread_stat_t *src);

void mpiPi_stats_thr_coll_binstrings(mpiPi_thread_stat_t *stat,
                                     int comm_idx, char *comm_buf,
                                     int size_idx, char *size_buf);

void mpiPi_stats_thr_pt2pt_upd (mpiPi_thread_stat_t *stat,
                                   int op, double dur, double size,
                                   MPI_Comm * comm);
void mpiPi_stats_thr_pt2pt_gather(mpiPi_thread_stat_t *stat, double **_outbuf);
void mpiPi_stats_thr_pt2pt_merge(mpiPi_thread_stat_t *dst,
                                mpiPi_thread_stat_t *src);
void mpiPi_stats_thr_pt2pt_binstrings(mpiPi_thread_stat_t *stat,
                                     int comm_idx, char *comm_buf,
                                     int size_idx, char *size_buf);

void mpiPi_stats_thr_exit(mpiPi_thread_stat_t *stat);
void mpiPi_stats_thr_enter(mpiPi_thread_stat_t *stat);
int mpiPi_stats_thr_is_on(mpiPi_thread_stat_t *stat);

#endif

/*
  
  <license>
  
  Copyright (c) 2006, The Regents of the University of California. 
  Produced at the Lawrence Livermore National Laboratory 
  Written by Jeffery Vetter and Christopher Chambreau. 
  UCRL-CODE-223450. 
  All rights reserved. 

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

/* EOF */
