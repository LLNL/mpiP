/* -*- Mode: C; -*- 

   mpiP MPI Profiler ( http://llnl.github.io/mpiP )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   ----- 

   mpip_timers.h -- timer macros

   $Id$

*/

#ifndef _MPITI_TIMERS_H
#define _MPITI_TIMERS_H

#define MSECS 1000
#define USECS 1000000
#define NSECS 1000000000

#include <unistd.h>
#include <sys/time.h>
typedef double mpiP_TIMER;

#if (defined(SunOS) && ! defined(USE_GETTIMEOFDAY))
#include "mpip_timers/sunos_local.h"

#elif (defined(AIX) && ! defined(USE_GETTIMEOFDAY))
#include "mpip_timers/aix_local.h"

#elif (defined(UNICOS_mp) && ! defined(USE_GETTIMEOFDAY))
#include "mpip_timers/crayx1_hw.h"

#elif (defined(Catamount) && defined(USE_DCLOCK))
#include "mpip_timers/crayxt.h"

#elif defined(USE_RTS_GET_TIMEBASE)
#include "mpip_timers/bgl_local.h"

#elif (defined(Linux) && defined(USE_CLOCK_GETTIME) && defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0))
#include "mpip_timers/linux_posix.h"

#elif defined(USE_WTIME)
#define mpiPi_TIMER double
#define mpiPi_TIME double
#define mpiPi_TIMER_NAME "PMPI_Wtime"
#define mpiPi_GETTIME(timeaddr) (*(timeaddr) = (PMPI_Wtime()*USECS))
#define mpiPi_GETUSECS(timeaddr) (*(timeaddr))
#define mpiPi_GETTIMEDIFF(end,start) ((*end)-(*start))
#else

/* gettimeofday returns microseconds */
#define mpiPi_TIMER double
#define mpiPi_TIME struct timeval
#define mpiPi_TIMER_NAME "gettimeofday"
#define mpiPi_ASNTIME(lhs,rhs) {bcopy(rhs, lhs, sizeof(mpiPi_TIMER));}
#define mpiPi_GETTIME(timeaddr) gettimeofday(timeaddr,NULL)
#define mpiPi_GETUSECS(timeaddr) (((mpiPi_TIMER)(timeaddr)->tv_sec)*USECS+((mpiPi_TIMER)(timeaddr)->tv_usec))
#define mpiPi_PRINTTIME(taddr) printf("Time is %ld sec and %ld usec.\n", (taddr)->tv_sec, (taddr)->tv_usec)
#define mpiPi_GETTIMEDIFF(end,start) ((mpiP_TIMER)((((mpiPi_TIMER)(end)->tv_sec)*USECS)+(end)->tv_usec)-((((mpiPi_TIMER)(start)->tv_sec)*USECS)+(start)->tv_usec))
#define mpiPi_PRINTTIMEDIFF(end,start) {printf("Time diff is %ld usecs.\n",mpiPi_GETTIMEDIFF(end,start));}

#endif

#endif


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

/* EOF */
