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
extern void mpiP_record_traceback ( void* pc_array[], int max_stack );
extern void mpiP_close_executable ( void );
extern int mpiP_open_executable ( char* filename );
extern int mpiP_find_src_loc ( void *i_addr_hex, char **o_file_str, 
                               int *o_lineno, char **o_funct_str );

/*  Returns current time in microseconds  */
unsigned long mpiP_gettime();

typedef unsigned long mpiP_TIMER;
