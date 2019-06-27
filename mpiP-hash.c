/* -*- C -*- 

   mpiP MPI Profiler ( http://llnl.github.io/mpiP )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   -----

   mpiP-hash.c -- generic hash table

 */

#ifndef lint
static char *svnid = "$Id$";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>

#include "mpiP-hash.h"

#define Abort(str) {printf("HASH: ABORTING (%s:%d): %s\n",__FILE__,__LINE__,str); exit(-1);}

h_t *
h_open (int size, h_HashFunct hf, h_Comparator hc)
{
  h_t *ht;
  ht = (h_t *) malloc (sizeof (h_t));
  if (ht == NULL)
    Abort ("malloc error");
  if (size < 2)
    Abort ("size too small for hash table");
  ht->size = size;
  ht->count = 0;
  if (hf == NULL)
    Abort ("hash key function not defined");
  ht->hf = hf;
  if (hc == NULL)
    Abort ("hash comparator function not defined");
  ht->hc = hc;
  ht->table = (h_entry_t **) malloc (sizeof (h_entry_t *) * size);
  if (ht->table == NULL)
    Abort ("malloc error");
  bzero (ht->table, sizeof (h_entry_t *) * size);
  return ht;
}

void
h_close (h_t * ht)
{
  if (ht == NULL)
    Abort ("hash table uninitialized");
#if 0
  if (ht->count > 0)
    printf ("Hash table not empty (%d elements) on h_close\n", ht->count);
#endif
  free (ht->table);
  free (ht);
}

int
h_count (h_t * ht)
{
  if (ht == NULL)
    Abort ("hash table uninitialized");
  return ht->count;
}

int
h_insert (h_t * ht, void *ptr)
{
  unsigned tableIndex;
  h_entry_t *het = NULL;
  h_entry_t *het_index = NULL;
  if (ht == NULL)
    Abort ("hash table uninitialized");
  het = (h_entry_t *) malloc (sizeof (h_entry_t));
  if (het == NULL)
    Abort ("hash table entry malloc error");
  if (ptr == NULL)
    Abort ("h_insert: ptr == NULL");
  het->ptr = ptr;
  het->next = NULL;
  tableIndex = (ht->hf (ptr));
  tableIndex %= (ht->size);
  if (ht->table[tableIndex] != NULL)
    {
      /* search list for similar key, if not, append it to list */
      /* OPTIMIZATION: remove this and let users deal with it :) */
      for (het_index = ht->table[tableIndex];
           het_index != NULL; het_index = het_index->next)
        {
          if (ht->hc (het_index->ptr, ptr) == 0)
            {
              printf("hash: warning: tried to insert identical entry again\n");
              return 1;
            }
        }
      het->next = ht->table[tableIndex];
      ht->table[tableIndex] = het;
    }
  else
    {
      ht->table[tableIndex] = het;
    }
  ht->count++;
  return 0;
}

void *
h_search (h_t * ht, void *key, void **ptr)
{
  unsigned tableIndex;
  h_entry_t *het_index = NULL;
  if (ht == NULL){
    Abort ("hash table uninitialized");
    }
  if (key == NULL)
    Abort ("h_search: key == NULL");
  if (ptr == NULL)
    Abort ("h_search: ptr == NULL");
  *ptr = NULL;
  tableIndex = (ht->hf (key));
  tableIndex %= (ht->size);
  if (ht->table[tableIndex] != NULL)
    {
      /* search list for similar key  */
      for (het_index = ht->table[tableIndex];
           het_index != NULL; het_index = het_index->next)
        {
          if (ht->hc (het_index->ptr, key) == 0)
            {
              *ptr = het_index->ptr;
              return *ptr;
            }
        }
    }
  return NULL;
}

void *
h_delete (h_t * ht, void *key, void **ptr)
{
  unsigned tableIndex;
  h_entry_t *het_index = NULL;
  h_entry_t **next_addr = NULL;
  if (ht == NULL)
    Abort ("hash table uninitialized");
  if (ptr == NULL)
    Abort ("h_insert: ptr == NULL");
  if (key == NULL)
    Abort ("h_insert: key == NULL");
  *ptr = NULL;
  tableIndex = (ht->hf (key));
  tableIndex %= (ht->size);
  if (ht->table[tableIndex] != NULL)
    {
      /* search list for similar key  */
      for (het_index = ht->table[tableIndex],
           next_addr = &(ht->table[tableIndex]);
           het_index != NULL; het_index = het_index->next)
        {
          if (ht->hc (het_index->ptr, key) == 0)
            {
              /* match */
              *ptr = het_index->ptr;
              *next_addr = het_index->next;
              free (het_index);
              ht->count--;
              return *ptr;
            }

          next_addr = &(het_index->next);
        }
    }
  return NULL;
}

int
h_gather_data (h_t * ht, int *ac, void ***ptr)
{
  int i;
  h_entry_t *het_index = NULL;
  if (ht == NULL)
    Abort ("hash table uninitialized");
  if (ptr == NULL)
    Abort ("h_insert: ptr == NULL");

  *ac = 0;
  *ptr = (void **) malloc (sizeof (void *) * ht->count);
  for (i = 0; i < ht->size; i++)
    {
      if (ht->table[i] != NULL)
        {
          for (het_index = ht->table[i];
               het_index != NULL; het_index = het_index->next)
            {
              (*ptr)[*ac] = het_index->ptr;
              (*ac)++;
            }
        }
    }
  return *ac;
}

int
h_drain (h_t * ht, int *ac, void ***ptr)
{
  int i;
  h_entry_t *het_index = NULL;
  h_gather_data (ht, ac, ptr);

  for (i = 0; i < ht->size; i++)
    {
      if (ht->table[i] != NULL)
        {
          for (het_index = ht->table[i];
               het_index != NULL; )
            {
              h_entry_t *to_free = het_index;
              het_index = het_index->next;
              free(to_free);
            }
        }
      ht->table[i] = NULL;
    }
  ht->count = 0;
  return *ac;
}

#define TESTING 0
#if TESTING

typedef struct _SampleRecord_t
{
  int id;
  int size;
  int order;
}
SampleRecord_t;

int
computeKey (void *p)
{
  SampleRecord_t *srp = (SampleRecord_t *) p;
  return 37941685 ^ srp->id ^ (srp->size << 7) ^ (srp->order << 11);
}

int
compareRecs (void *p1, void *p2)
{
  SampleRecord_t *sr1 = (SampleRecord_t *) p1;
  SampleRecord_t *sr2 = (SampleRecord_t *) p2;

#define express(field) {if(sr1->field < sr2->field) return -1; if(sr1->field > sr2->field) return 1;}
  express (id);
  express (size);
  express (order);
#undef express
  return 0;
}


main ()
{
  h_t *ht;
  SampleRecord_t sr[20];
  int i;
  srand (0);
  for (i = 0; i < 20; i++)
    {
      sr[i].id = rand ();
      sr[i].size = i;
      sr[i].order = i * 10;
    }
  ht = h_open (4, computeKey, compareRecs);
  for (i = 0; i < 20; i++)	/* insert some elements */
    {
      if (NULL != h_insert (ht, &sr[i]))
        {
          printf ("%5d: inserted %d\n", i, sr[i].id);
        }
    }
  printf ("-----\n");
  {
    int dc;
    SampleRecord_t **dv;
    h_gather_data (ht, &dc, (void ***) &dv);	/* gather elements and print */
    for (i = 0; i < dc; i++)
      {
        printf ("%5d: %d %d %d\n", i, dv[i]->id, dv[i]->size, dv[i]->order);
      }
    free (dv);			/* remember to release return vector */
  }
  printf ("-----\n");
  for (i = 0; i < 10; i++)	/* delete half elements */
    {
      SampleRecord_t *d;
      if (NULL != h_delete (ht, &sr[i], (void **) &d))
        {
          printf ("%5d: deleted %d\n", i, sr[i].id);
        }
    }

  printf ("-----\n");
  {
    int dc;
    SampleRecord_t **dv;
    h_gather_data (ht, &dc, (void ***) &dv);	/* gather elements and print */
    for (i = 0; i < dc; i++)
      {
        printf ("%5d: %d %d %d\n", i, dv[i]->id, dv[i]->size, dv[i]->order);
      }
    free (dv);			/* remember to release return vector */
  }

  printf ("-----\n");
  for (i = 10; i < 20; i++)	/* delete half elements */
    {
      SampleRecord_t *d;
      if (NULL != h_delete (ht, &sr[i], (void **) &d))
        {
          printf ("%5d: deleted %d\n", i, sr[i].id);
        }
    }

  h_close (ht);
}

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



/* eof */
