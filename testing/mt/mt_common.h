#ifndef MT_COMMON_H
#define MT_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>  //getopt
#include <string.h>
#include <pthread.h>

#define GET_TS() ({                         \
    struct timespec ts;                     \
    double ret = 0;                         \
    clock_gettime(CLOCK_MONOTONIC, &ts);    \
    ret = ts.tv_sec + 1E-9*ts.tv_nsec;      \
    ret;                                    \
})

typedef enum {
    TEST_MPI_SEND,
    TEST_MPI_RECV,
    TEST_MPI_ISEND,
    TEST_MPI_IRECV,
    TEST_MPI_WAITALL,
    TEST_MPI_BARRIER,
    TEST_MPI_COUNT
} test_mpi_call_ids_t;

int mt_common_dbg();
#define MT_COMMON_DBG(tid,fmt,args...)          \
    if(mt_common_dbg())                                   \
        printf("%d:%d:%s: " fmt " \n",          \
            mt_common_rank(), tid, __FUNCTION__,        \
            ## args);                           \

void mt_common_init(int *argc, char ***argv);
void mt_common_fini();

int mt_common_nthreads();
int mt_common_rank();
int mt_common_size();
int mt_common_nbr();
int mt_common_iter();

typedef void (*mt_common_thrptr_t)(int tid);
void mt_common_exec(mt_common_thrptr_t *workers);

void mt_common_thr_enter(int tid);
void mt_common_thr_exit(int tid);
void mt_common_stat_append(int tid, test_mpi_call_ids_t id,
                           int count, double duration);
void mt_common_sync();
MPI_Comm mt_common_comm(int tid);

#endif // MT_COMMON_H

/*

<license>

Copyright (c) 2019      Mellanox Technologies Ltd.
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
