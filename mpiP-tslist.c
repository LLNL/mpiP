/* -*- C -*-

   mpiP MPI Profiler ( http://llnl.github.io/mpiP )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   -----

   thread_safe_list.c -- Implementation of the thread-safe list based on atomics
   NOTE: only one thread is allowed to extract elements, but many can contribute.
   This fits perfectly fine with the purpose of holding TLS pointers in mpiP.

 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "arch/arch.h"
#include "mpiP-tslist.h"

mpiP_tslist_t *mpiPi_tslist_create()
{
  mpiP_tslist_t *list = calloc(1, sizeof(*list));
  if( NULL == list ) {
      return NULL;
    }
  list->head = calloc(1, sizeof(*list->head));
  list->tail = list->head;
  return list;
}
void mpiPi_tslist_release(mpiP_tslist_t *list)
{
  free(list->head);
  list->head = list->tail = NULL;
  free(list);
}

void mpiPi_tslist_append(mpiP_tslist_t *list, void *data_ptr)
{
  mpiP_tslist_elem_t *elem = calloc(1, sizeof(*elem));
  elem->next = NULL;
  elem->ptr = data_ptr;
  /* Ensure that all chnages to the elem structure are complete */////
  mpiP_atomic_wmb();
  /* Atomically swap the tail of the list to point to this element */
  mpiP_tslist_elem_t *prev = (mpiP_tslist_elem_t*)mpiP_atomic_swap((void**)&list->tail, elem);
  prev->next = elem;
}

void *mpiPi_tslist_dequeue(mpiP_tslist_t *list)
{
  mpiP_tslist_elem_t *elem = NULL, *tmp;
  void *ret = NULL;

  if( list->head == list->tail ){
      // the list is empty
      return NULL;
    }

  if(list->head->next == NULL ){
      // Someone is adding a new element, but it is not yet ready
      return NULL;
    }

  elem = list->head->next;
  if( elem->next ) {
      /* We have more than one elements in the list
         * it is safe to dequeue this elemen as only one thread
         * is allowed to dequeue
         */
      list->head->next = elem->next;
      /* Terminate element to be on the safe side */
      elem->next = NULL;
      goto ret;
    }

  /* There is a possibility of a race condition:
   * elem->next may still be NULL while a new addition has started that
   * already obtained the pointer to elem.
   */
  list->head->next = NULL;
  tmp = elem;
  if( mpiP_atomic_cas((void**)&list->tail, (void**)&tmp, list->head) ){
      /* Replacement was successful, this means that list->head was placed
       * as the very first element of the list
       */
  } else {
      /* Replacement failed, this means that other thread is in the process
       * of adding to the list
       */
      /* Wait for the pointer to appear */
      while(NULL != elem->next) {
          mpiP_atomic_isync();
      }
      /* Hand this element over to the head element */
      list->head->next = elem->next;
  }
ret:
  ret = elem->ptr;
  free(elem);
  return ret;
}

mpiP_tslist_elem_t *mpiPi_tslist_first(mpiP_tslist_t *list)
{
  return list->head->next;
}

mpiP_tslist_elem_t *mpiPi_tslist_next(mpiP_tslist_elem_t *current)
{
  return current->next;
}

/*

<license>

Derived from
https://github.com/artpol84/poc/blob/master/arch/concurrency/thread-safe_list

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
