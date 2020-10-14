/* -*- C -*-

   mpiP MPI Profiler ( http://llnl.github.io/mpiP )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   -----

   hash.h -- generic hash table

   $Id$

*/

#include <string.h>
#include "mpiP-callsites.h"
#include "mpiPi.h"


void mpiPi_cs_reset_stat(callsite_stats_t *csp)
{
  csp->maxDur = 0;
  csp->minDur = DBL_MAX;
  csp->maxIO = 0;
  csp->minIO = DBL_MAX;
  csp->maxDataSent = 0;
  csp->minDataSent = DBL_MAX;

  csp->count = 0;
  csp->cumulativeTime = 0;
  csp->cumulativeTimeSquared = 0;
  csp->cumulativeDataSent = 0;
  csp->cumulativeIO = 0;

  csp->arbitraryMessageCount = 0;
}

void mpiPi_cs_init(callsite_stats_t *csp, void *pc[],
                   unsigned op, unsigned rank)
{
  int i;
  csp->op = op;
  csp->rank = rank;
  for (i = 0; i < mpiPi.fullStackDepth; i++)
    {
      csp->pc[i] = pc[i];
    }
  csp->cookie = MPIP_CALLSITE_STATS_COOKIE;
  mpiPi_cs_reset_stat(csp);
}

void mpiPi_cs_update(callsite_stats_t *csp, double dur,
                     double sendSize, double ioSize, double rmaSize,
                     double threshold)
{
  csp->count++;
  csp->cumulativeTime += dur;
  assert (csp->cumulativeTime >= 0);
  csp->cumulativeTimeSquared += (dur * dur);
  assert (csp->cumulativeTimeSquared >= 0);
  csp->maxDur = max (csp->maxDur, dur);
  csp->minDur = min (csp->minDur, dur);
  csp->cumulativeDataSent += sendSize;
  csp->cumulativeIO += ioSize;
  csp->cumulativeRMA += rmaSize;

  csp->maxDataSent = max (csp->maxDataSent, sendSize);
  csp->minDataSent = min (csp->minDataSent, sendSize);

  csp->maxIO = max (csp->maxIO, ioSize);
  csp->minIO = min (csp->minIO, ioSize);

  csp->maxRMA = max (csp->maxRMA, rmaSize);
  csp->minRMA = min (csp->minRMA, rmaSize);

  if (threshold > -1 && (sendSize >= threshold))
    csp->arbitraryMessageCount++;
}


/* Callsite statistics */
void mpiPi_cs_merge(callsite_stats_t *dst, callsite_stats_t *src)
{
  dst->count += src->count;
  dst->cumulativeTime += src->cumulativeTime;
  assert (dst->cumulativeTime >= 0);
  dst->cumulativeTimeSquared += src->cumulativeTimeSquared;
  assert (dst->cumulativeTimeSquared >= 0);
  dst->maxDur = max (dst->maxDur, src->maxDur);
  dst->minDur = min (dst->minDur, src->minDur);
  dst->maxDataSent = max (dst->maxDataSent, src->maxDataSent);
  dst->minDataSent = min (dst->minDataSent, src->minDataSent);
  dst->cumulativeDataSent += src->cumulativeDataSent;
  dst->maxIO = max (dst->maxIO, src->maxIO);
  dst->minIO = min (dst->minIO, src->minIO);
  dst->cumulativeIO += src->cumulativeIO;
  dst->cumulativeRMA += src->cumulativeRMA;
  dst->arbitraryMessageCount += src->arbitraryMessageCount;
}

/*
 * ============================================================================
 *
 * Program counter (PC) to source code id: File/Function/Line
 *
 * ============================================================================
 */

typedef struct callsite_cache_entry_t
{
  void *pc;
  char *filename;
  char *functname;
  int line;
}
callsite_pc_cache_entry_t;

h_t *callsite_pc_cache = NULL;
h_t *callsite_src_id_cache = NULL;
int callsite_src_id_counter = 1;

static int
callsite_pc_cache_comparator (const void *p1, const void *p2)
{
  callsite_pc_cache_entry_t *cs1 = (callsite_pc_cache_entry_t *) p1;
  callsite_pc_cache_entry_t *cs2 = (callsite_pc_cache_entry_t *) p2;

  if ((long) cs1->pc > (long) cs2->pc)
    {
      return 1;
    }
  if ((long) cs1->pc < (long) cs2->pc)
    {
      return -1;
    }
  return 0;
}

static int
callsite_pc_cache_hashkey (const void *p1)
{
  callsite_pc_cache_entry_t *cs1 = (callsite_pc_cache_entry_t *) p1;
  return 662917 ^ ((long) cs1->pc);
}

static int
callsite_src_id_cache_comparator (const void *p1, const void *p2)
{
  int i;
  callsite_src_id_cache_entry_t *csp_1 = (callsite_src_id_cache_entry_t *) p1;
  callsite_src_id_cache_entry_t *csp_2 = (callsite_src_id_cache_entry_t *) p2;

#define express(f) {if ((csp_1->f) > (csp_2->f)) {return 1;} if ((csp_1->f) < (csp_2->f)) {return -1;}}
  if (mpiPi.reportStackDepth == 0)
    {
      express (id);		/* In cases where the call stack depth is 0, the only unique info may be the id */
      return 0;
    }
  else
    {
      for (i = 0; i < mpiPi.fullStackDepth; i++)
        {
          if (csp_1->filename[i] != NULL && csp_2->filename[i] != NULL)
            {
              if (strcmp (csp_1->filename[i], csp_2->filename[i]) > 0)
                {
                  return 1;
                }
              if (strcmp (csp_1->filename[i], csp_2->filename[i]) < 0)
                {
                  return -1;
                }
              express (line[i]);
              if (strcmp (csp_1->functname[i], csp_2->functname[i]) > 0)
                {
                  return 1;
                }
              if (strcmp (csp_1->functname[i], csp_2->functname[i]) < 0)
                {
                  return -1;
                }
            }

          express (pc[i]);
        }
    }
#undef express
  return 0;
}

static int
callsite_src_id_cache_hashkey (const void *p1)
{
  int i, j;
  int res = 0;
  callsite_src_id_cache_entry_t *cs1 = (callsite_src_id_cache_entry_t *) p1;
  for (i = 0; i < mpiPi.fullStackDepth; i++)
    {
      if (cs1->filename[i] != NULL)
        {
          for (j = 0; cs1->filename[i][j] != '\0'; j++)
            {
              res ^= (unsigned) cs1->filename[i][j];
            }
          for (j = 0; cs1->functname[i][j] != '\0'; j++)
            {
              res ^= (unsigned) cs1->functname[i][j];
            }
        }
      res ^= cs1->line[i];
    }
  return 662917 ^ res;
}

void mpiPi_cs_cache_init()
{
  if (callsite_pc_cache == NULL)
    {
      callsite_pc_cache = h_open (mpiPi.tableSize,
                                  callsite_pc_cache_hashkey,
                                  callsite_pc_cache_comparator);
    }
  if (callsite_src_id_cache == NULL)
    {
      callsite_src_id_cache = h_open (mpiPi.tableSize,
                                      callsite_src_id_cache_hashkey,
                                      callsite_src_id_cache_comparator);
    }
}

int
mpiPi_query_pc (void *pc, char **filename, char **functname, int *lineno)
{
  int rc = 0;
  callsite_pc_cache_entry_t key;
  callsite_pc_cache_entry_t *csp;
  char addr_buf[24];

  key.pc = pc;
  /* do we have a cache entry for this pc? If so, use entry */
  if (h_search (callsite_pc_cache, &key, (void **) &csp) == NULL)
    {
      /* no cache entry: create, lookup, and insert */
      csp =
          (callsite_pc_cache_entry_t *)
          malloc (sizeof (callsite_pc_cache_entry_t));
      csp->pc = pc;
#if defined(ENABLE_BFD) || defined(USE_LIBDWARF)
      if (mpiP_find_src_loc (pc, filename, lineno, functname) == 0)
        {
          if (*filename == NULL || strcmp (*filename, "??") == 0)
            *filename = "[unknown]";

          if (*functname == NULL)
            *functname = "[unknown]";

          mpiPi_msg_debug
              ("Successful Source lookup for [%s]: %s, %d, %s\n",
               mpiP_format_address (pc, addr_buf), *filename, *lineno,
               *functname);

          csp->filename = strdup (*filename);
          csp->functname = strdup (*functname);
          csp->line = *lineno;
        }
      else
        {
          mpiPi_msg_debug ("Unsuccessful Source lookup for [%s]\n",
                           mpiP_format_address (pc, addr_buf));
          csp->filename = strdup ("[unknown]");
          csp->functname = strdup ("[unknown]");
          csp->line = 0;
        }
#else /* ! ENABLE_BFD || USE_LIBDWARF */
      csp->filename = strdup ("[unknown]");
      csp->functname = strdup ("[unknown]");
      csp->line = 0;
#endif
      h_insert (callsite_pc_cache, csp);
    }

  *filename = csp->filename;
  *functname = csp->functname;
  *lineno = csp->line;

  if (*lineno == 0)
    rc = 1;			/* use this value to indicate a failed lookup */

  return rc;
}

/* take a callstats record (the pc) and determine src file, line, if
   possible and assign a callsite id.
 */
int
mpiPi_query_src (callsite_stats_t * p)
{
  int i;
  callsite_src_id_cache_entry_t key;
  callsite_src_id_cache_entry_t *csp;
  assert (p);

  /* Because multiple pcs can map to the same source line, we must
     check that mapping here. If we got unknown, then we assign
     different ids */
  bzero (&key, sizeof (callsite_src_id_cache_entry_t));

  for (i = 0; (i < mpiPi.fullStackDepth) && (p->pc[i] != NULL); i++)
    {
      if (mpiPi.do_lookup == 1)
        mpiPi_query_pc (p->pc[i], &(p->filename[i]), &(p->functname[i]),
                        &(p->lineno[i]));
      else
        {
          p->filename[i] = strdup ("[unknown]");
          p->functname[i] = strdup ("[unknown]");
          p->lineno[i] = 0;
        }

      key.filename[i] = p->filename[i];
      key.functname[i] = p->functname[i];
      key.line[i] = p->lineno[i];
      key.pc[i] = p->pc[i];
    }

  /* MPI ID is compared when stack depth is 0 */
  key.id = p->op - mpiPi_BASE;

  /* lookup/generate an ID based on the callstack, not just the callsite pc */
  if (h_search (callsite_src_id_cache, &key, (void **) &csp) == NULL)
    {
      /* create a new entry, and assign an id based on callstack */
      csp =
          (callsite_src_id_cache_entry_t *)
          malloc (sizeof (callsite_src_id_cache_entry_t));
      bzero (csp, sizeof (callsite_src_id_cache_entry_t));

      for (i = 0; (i < mpiPi.fullStackDepth) && (p->pc[i] != NULL); i++)
        {
          csp->filename[i] = strdup (key.filename[i]);
          csp->functname[i] = strdup (key.functname[i]);
          csp->line[i] = key.line[i];
          csp->pc[i] = p->pc[i];
        }
      csp->op = p->op;
      if (mpiPi.reportStackDepth == 0)
        csp->id = csp->op - mpiPi_BASE;
      else
        csp->id = callsite_src_id_counter++;
      h_insert (callsite_src_id_cache, csp);
    }

  /* assign ID to this record */
  p->csid = csp->id;

  return p->csid;
}

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
