/*
  
     mpiP MPI Profiler ( http://mpip.sourceforge.net/ )
  
     Please see COPYRIGHT AND LICENSE information at the end of this file.
  
     ----- 
  
     mpiPi_proto.h
  
     $Id$

*/

extern int open_dwarf_executable (char *fileName);
extern void close_dwarf_executable (void);
extern void mpiPi_msg (char *fmt, ...);
extern void mpiPi_abort (char *fmt, ...);
extern void mpiPi_msg_debug (char *fmt, ...);
extern void mpiPi_msg_warn (char *fmt, ...);
extern void mpiPi_msg_debug0 (char *fmt, ...);

extern void mpiP_init_api (void);
extern int mpiP_record_traceback (void *pc_array[], int max_stack);
extern int mpiP_open_executable (char *filename);
extern void mpiP_close_executable (void);
extern mpiP_TIMER mpiP_gettime (void);
extern char *mpiP_get_executable_name (void);
extern char *mpiP_format_address (void *pval, char *addr_buf);
extern h_t *h_open (int size, h_HashFunct hf, h_Comparator hc);
extern void h_close (h_t * ht);
extern int h_count (h_t * ht);
extern int h_insert (h_t * ht, void *ptr);
extern void *h_search (h_t * ht, void *key, void **ptr);
extern void *h_delete (h_t * ht, void *key, void **ptr);
extern int h_gather_data (h_t * ht, int *ac, void ***ptr);
extern void mpiPi_init (char *appName);
extern int callsite_pc_cache_comparator (const void *p1, const void *p2);
extern int callsite_pc_cache_hashkey (const void *p1);
extern int mpiPi_query_pc (void *pc, char **filename, char **functname,
			   int *lineno);
extern int callsite_src_id_cache_comparator (const void *p1, const void *p2);
extern int callsite_src_id_cache_hashkey (const void *p1);
extern int mpiPi_query_src (callsite_stats_t * p);
extern int mpiPi_mergeResults (void);
extern void mpiPi_publishResults (int report_style);
extern void mpiPi_collect_basics (void);
extern void mpiPi_generateReport (int report_style);
extern void mpiPi_finalize (void);
extern void mpiPi_update_callsite_stats (unsigned op, unsigned rank,
					 void **pc, double dur,
					 double sendSize, double ioSize);
extern char *mpiPdemangle (const char *mangledSym);
extern int mpiP_find_src_loc (void *i_addr_hex, char **o_file_str,
			      int *o_lineno, char **o_funct_str);
extern int open_bfd_executable (char *filename);
extern void close_bfd_executable (void);
extern int MPI_Pcontrol (const int flag, ...);
extern int mpi_pcontrol_ (int *flag, ...);
extern double calc_COV (double *data, int dataSize);
extern void mpiPi_profile_print (FILE * fp, int report_style);
extern void mpiPi_profile_print_verbose (FILE * fp);
extern void mpiPi_profile_print_concise (FILE * fp);
extern char *GetBaseAppName (char *rawName);
extern int mpiPi_RecordTraceBack (jmp_buf jb, void *pc_array[], int max_back);
extern void mpiPi_getenv (void);
extern char *getProcExeLink (void);
extern void getProcCmdLine (int *ac, char **av);
extern void mpiPi_copy_args (int *ac, char **av, int av_len);
extern void mpiPi_copy_given_args (int *ac, char **av, int av_len, int argc,
				   char **argv);
extern unsigned long long mpiPi_get_text_start (char *filename);
extern void mpiPi_print_callsite_time_info (FILE * fp);
extern void mpiPi_print_all_callsite_time_info (FILE * fp);
extern void mpiPi_print_concise_callsite_time_info (FILE * fp);
extern void mpiPi_print_callsite_sent_info (FILE * fp);
extern void mpiPi_print_all_callsite_sent_info (FILE * fp);
extern void mpiPi_print_concise_callsite_sent_info (FILE * fp);
extern void mpiPi_print_task_info (FILE * fp, int report_style);
extern void mpiPi_print_verbose_task_info (FILE * fp);
extern void mpiPi_print_concise_task_info (FILE * fp);


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
