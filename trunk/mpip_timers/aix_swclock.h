/* -*- C -*- 

   mpiP MPI Profiler ( http://mpip.sourceforge.net/ )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   -----

   aix_swclock.h -- aix global switch clock

   $Id: aix_swclock.h 369 2006-10-05 19:21:12 +0000 (Thu, 05 Oct 2006) chcham $

*/

#ifndef _AIX_SWCLOCK_H
#define _AIX_SWCLOCK_H

/*  PLEASE NOTE: as of release 2.7.1, 
    this functionality is not supported.    */

/* use global switch clock */
#include <swclock.h>

/* swclockRead returns nanoseconds/incr */
typedef struct mpiTi_timer
{
  swclock_handle_t h;
  int incr;
}
mpiTi_timer_t;
#define mpiTi_TIMER mpiTi_timer_t
#define mpiTi_TIMER_INIT(timer_addr) {(timer_addr)->h=swclockInit();if((timer_addr)->h==0){perror("swclockerror");exit(-1);}else{(timer_addr)->incr=swclockGetIncrement((timer_addr)->h);}{long long t1=swclockRead((timer_addr)->h);if(t1==-1){perror("swclockerror");exit(-1);}}}
#define mpiTi_TIME long long

#define mpiTi_ASNTIME(lhs,rhs) {bcopy(rhs, lhs, sizeof(mpiTi_TIME));}
#define mpiTi_GETTIME(timer,timeaddr) {(*(timeaddr))=swclockRead((timer)->h);}
#define mpiTi_GETUSECS(timer,timeaddr) ((*(timeaddr))/(((timer)->incr/1000000)))
#define mpiTi_GETTIMEDIFF(timer,end_addr,start_addr) (((*(end_addr))-(*(start_addr)))/((timer)->incr/1000000))
#define mpiTi_PRINTTIMEDIFF(timer,end_addr,start_addr) {printf("Time diff is %lld usecs.\n",mpiTi_GETTIMEDIFF(timer,end_addr,start_addr));}

#define mpiPi_TIMER_NAME "swclockRead"
#endif


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
