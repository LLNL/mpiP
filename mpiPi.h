/* -*- C -*- 

   @PROLOGUE@

   ----- 

   Jeffrey Vetter vetter3@llnl.gov
   Center for Applied Scientific Computing, LLNL
   12 Aug 2001

   mpiPi.h -- internal mpiP header

*/

#ifndef _MPIPI_H
#define _MPIPI_H

#include <assert.h>
#include <malloc.h>
#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <mpi.h>

#include "hash.h"

#include "mpiPi_def.h"
#include "_timers.h"

#define MPIPI_HOSTNAME_LEN_MAX 128
#define MPIP_CALLSITE_STACK_DEPTH_MAX 8

#define MPIP_CALLSITE_STACK_DEPTH (mpiPi.stackDepth)
#define MPIP_CALLSITE_STATS_COOKIE 518641
#define MPIP_CALLSITE_STATS_COOKIE_ASSERT(f) {assert(MPIP_CALLSITE_STATS_COOKIE==((f)->cookie));}

typedef struct _callsite_stats
{
  unsigned op;
  unsigned rank;
  int csid;
  long long count;
  double cumulativeTime;
  double cumulativeTimeSquared;
  double maxDur;
  double minDur;
  double cumulativeDataSent;
  void *pc[MPIP_CALLSITE_STACK_DEPTH_MAX];
  char *filename[MPIP_CALLSITE_STACK_DEPTH_MAX];
  char *functname[MPIP_CALLSITE_STACK_DEPTH_MAX];
  int lineno[MPIP_CALLSITE_STACK_DEPTH_MAX];
  long cookie;
}
callsite_stats_t;

typedef struct callsite_src_id_cache_entry_t
{
  int id;			/* unique id for this src code/stack location */
  int op;			/* at the lowest level, this is a MPI op */
  char *filename[MPIP_CALLSITE_STACK_DEPTH_MAX];
  char *functname[MPIP_CALLSITE_STACK_DEPTH_MAX];
  int line[MPIP_CALLSITE_STACK_DEPTH_MAX];
}
callsite_src_id_cache_entry_t;

extern h_t *callsite_src_id_cache;


typedef struct _mpiPi_task_info_t
{
  /* this section is received from each task */
  int rank;
  double app_time;		/* time from end of init to start of finalize */
  char hostname[MPIPI_HOSTNAME_LEN_MAX];

  /* this section is calculated at the collector */
  double mpi_time;
}
mpiPi_task_info_t;

typedef struct _mpiPi_lookup_t
{
  int op;
  char *name;
}
mpiPi_lookup_t;

extern mpiPi_lookup_t mpiPi_lookup[];

typedef struct _mpiPi_t
{
  int ac;
  char *av[32];
  char *toolname;
  char *appName;
  char oFilename[256];
  MPI_Comm comm;
  int tag;
  int procID;
  int rank;
  int size;
  int collectorRank;
  char hostname[MPI_MAX_PROCESSOR_NAME];
  int hostnamelen;
  char *outputDir;
  char *envStr;
  FILE *stdout_;
  FILE *stderr_;
  double startTime;
  double endTime;
  double cumulativeTime; /* necessary for pcontrol */
  time_t start_timeofday;
  time_t stop_timeofday;

  /* pcontrol */
  int enabled;
  int enabledCount;

  mpiPi_TIMER timer;
  mpiPi_task_info_t *global_task_info;
  double global_app_time;
  double global_mpi_time;

  int tableSize;
  h_t *task_callsite_stats;
  callsite_stats_t *rawCallsiteData;
  h_t *global_callsite_stats;
  h_t *global_callsite_stats_agg;

  mpiPi_lookup_t *lookup;

  int stackDepth;
  double reportPrintThreshold;
}
mpiPi_t;

extern mpiPi_t mpiPi;

extern int mpiPi_vmajor;
extern int mpiPi_vminor;
extern char *mpiPi_vdate;
extern char *mpiPi_vtime;

extern int mpiPi_debug;



#if defined(AIX)
#define ParentFP(jb) ((void *) *(long *) jb[3])
#define FramePC(fp) ((void *) *(long *) (((long) fp) + (2 * sizeof (void *))))
#define NextFP(fp) ((void *) *(long *) fp)
#elif defined(Linux) && defined(IA32)
/* jb[3] = FP, FP[2] = PC */
#define ParentFP(jb) ((void*) jb[0].__jmpbuf[3])
#define FramePC(fp) ((void*)(((void**)fp)[2]))
#define NextFP(fp) ((void*)((void**)fp)[0])
#if 0
#define ParentFP(jb) ((void*) jb[0].__jmpbuf[JB_SP])
#define FramePC(fp) ((void*)(((void**)fp)[1]))
#define NextFP(fp) ((void*)((void**)fp)[0])
#endif
#else
#define ParentFP(jb) (0)
#define FramePC(fp) (0)
#define NextFP(fp) (0)
#endif
#define GetPPC(jb) FramePC(ParentFP(jb))

#define min(x,y) ((x<y)?(x):(y))
#define max(x,y) ((x>y)?(x):(y))

#include "mpiPi_proto.h"

#endif

/* eof */
