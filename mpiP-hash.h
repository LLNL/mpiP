/* -*- C -*- 

   @PROLOGUE@

   ----- 

   Jeffrey Vetter vetter3@llnl.gov
   Center for Applied Scientific Computing, LLNL
   31 Jul 2001

   hash.h -- generic hash table

*/

#ifndef _HASH_H
#define _HASH_H

typedef int (*h_HashFunct) (const void *);
typedef int (*h_Comparator) (const void *, const void *);

typedef struct _h_entry_t
{
  void *ptr;
  struct _h_entry_t *next;
}
h_entry_t;

typedef struct _h_t
{
  int size;
  int count;
  h_HashFunct hf;		/* given an element compute the hash number */
  h_Comparator hc;		/* given two elements, do they match? 0 for yes */
  h_entry_t **table;		/* table[size] of pointers to lists of entries */
}
h_t;

#include "hash_proto.h"

#endif

/* eof */
