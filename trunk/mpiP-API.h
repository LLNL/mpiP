/* -*- C -*-

   @PROLOGUE@

   -----

   Jeffrey Vetter vetter3@llnl.gov
   Center for Applied Scientific Computing, LLNL
   12 Aug 2001

   mpiP-API.h -- include file for mpiP functionality API

 */

/*
    For stack tracing and address lookup, use the following functions.
    mpiP_record_traceback may be called at any time.
    mpiP_find_src_loc must be called after a successful call to
    mpiP_open_executable and prior to mpiP_close_executable.
*/

#include "mpiPi.h"

#ifndef __CEXTRACT__
#if __STDC__

extern void mpiP_init_api ( void );
extern int mpiP_record_traceback ( void* pc_array[], int max_stack );
extern int mpiP_open_executable ( char* filename );
extern void mpiP_close_executable ( void );
extern mpiP_TIMER mpiP_gettime ( void );
extern char* mpiP_get_executable_name ( void );

#else /* __STDC__ */

extern void mpiP_init_api (/* void */);
extern int mpiP_record_traceback (/* void* pc_array[], int max_stack */);
extern int mpiP_open_executable (/* char* filename */);
extern void mpiP_close_executable (/* void */);
extern mpiP_TIMER mpiP_gettime (/* void */);
extern char* mpiP_get_executable_name (/* void */);

#endif /* __STDC__ */
#endif /* __CEXTRACT__ */
