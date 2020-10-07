/* -*- C -*-

   mpiP MPI Profiler ( http://llnl.github.io/mpiP )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   -----

   arch.h -- code and definitions specific to POWER architecture

 */

#ifndef ARCH_PPC_H
#define ARCH_PPC_H

#include <stdint.h>
#include "mpiPconfig.h"

#define MB()  __asm__ __volatile__ ("sync" : : : "memory")
#define RMB() __asm__ __volatile__ ("lwsync" : : : "memory")
#define WMB() __asm__ __volatile__ ("lwsync" : : : "memory")
#define ISYNC() __asm__ __volatile__ ("isync" : : : "memory")
#if defined(__PGI)
/* work-around for bug in PGI 16.5-16.7 where the compiler fails to
 *  * correctly emit load instructions for 64-bit operands. without this
 *   * it will emit lwz instead of ld to load the 64-bit operand. */
#define OPAL_ASM_VALUE64(x) (void *)(intptr_t) (x)
#else
#define OPAL_ASM_VALUE64(x) x
#endif

static inline
void mpiP_atomic_wmb(void)
{
  WMB();
}

static inline
void mpiP_atomic_isync(void)
{
  ISYNC();
}

#if (SIZEOF_VOIDP == 8)

/* 64-bit system (hacky way) */

static inline void *mpiP_atomic_swap(void **_addr, void *_newval)
{
  int64_t *addr= (int64_t *)_addr;
  int64_t newval = (int64_t)_newval;
  int64_t ret;

  __asm__ __volatile__ ("1: ldarx   %0, 0, %2  \n\t"
                        "   stdcx.  %3, 0, %2  \n\t"
                        "   bne-    1b         \n\t"
                        : "=&r" (ret), "=m" (*addr)
                        : "r" (addr), "r" (OPAL_ASM_VALUE64(newval))
                        : "cc", "memory");

  return (void*)ret;
}

static inline int mpiP_atomic_cas(void **_addr, void **_oldval, void *_newval)
{
  int64_t *addr = (int64_t*)_addr;
  int64_t *oldval = (int64_t*)_oldval;
  int64_t newval = (int64_t)_newval;
  int64_t prev;
  _Bool ret;

  __asm__ __volatile__ (
        "1: ldarx   %0, 0, %2  \n\t"
        "   cmpd    0, %0, %3  \n\t"
        "   bne-    2f         \n\t"
        "   stdcx.  %4, 0, %2  \n\t"
        "   bne-    1b         \n\t"
        "2:"
        : "=&r" (prev), "=m" (*addr)
        : "r" (addr), "r" (OPAL_ASM_VALUE64(*oldval)), "r" (OPAL_ASM_VALUE64(newval)), "m" (*addr)
        : "cc", "memory");

  ret = (prev == *oldval);
  *oldval = prev;
  return ret;
}

#else

static inline void *mpiP_atomic_swap(void **_addr, void *_newval)
{
  int32_t *addr= (int32_t *)_addr;
  int32_t newval = (int32_t)_newval;
  int32_t ret;

    __asm__ __volatile__ ("1: lwarx   %0, 0, %2  \n\t"
                          "   stwcx.  %3, 0, %2  \n\t"
                          "   bne-    1b         \n\t"
                          : "=&r" (ret), "=m" (*addr)
                          : "r" (addr), "r" (newval)
                          : "cc", "memory");

   return (void*)ret;
}

static inline int mpiP_atomic_cas(void **_addr, void **_oldval, void *_newval)
{
  int32_t *addr = (int32_t*)_addr;
  int32_t *oldval = (int32_t*)_oldval;
  int32_t newval = (int32_t)_newval;
  int32_t prev;
  bool ret;

  __asm__ __volatile__ (
        "1: lwarx   %0, 0, %2  \n\t"
        "   cmpw    0, %0, %3  \n\t"
        "   bne-    2f         \n\t"
        "   stwcx.  %4, 0, %2  \n\t"
        "   bne-    1b         \n\t"
        "2:"
        : "=&r" (prev), "=m" (*addr)
        : "r" OPAL_ASM_ADDR(addr), "r" (*oldval), "r" (newval), "m" (*addr)
        : "cc", "memory");

  ret = (prev == *oldval);
  *oldval = prev;
  return ret;
}

#endif

#endif

/*

<license>

This code was derived from Open MPI OPAL layer.

Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
                        University Research and Technology
                        Corporation.  All rights reserved.
Copyright (c) 2004-2005 The University of Tennessee and The University
                        of Tennessee Research Foundation.  All rights
                        reserved.
Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
                        University of Stuttgart.  All rights reserved.
Copyright (c) 2004-2005 The Regents of the University of California.
                        All rights reserved.
Copyright (c) 2010-2017 IBM Corporation.  All rights reserved.
Copyright (c) 2015-2018 Los Alamos National Security, LLC. All rights
                        reserved.
Copyright (c) 2019      Mellanox Technologies Ltd. All rights reserved.

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
