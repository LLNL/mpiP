/* -*- C -*- 

   @PROLOGUE@

   ----- 

   Jeffrey Vetter vetter3@llnl.gov
   Center for Applied Scientific Computing, LLNL
   12 Aug 2001

   util.c -- misc util functions

 */

#ifndef lint
static char *rcsid =
  "$Header$";
#endif

#include <string.h>
#include <setjmp.h>
#include <stdio.h>
#include <errno.h>

#ifdef OSF1
#include <excpt.h>
#endif

#include "mpiPi.h"
#include "symbols.h"

#ifdef HAVE_LIBUNWIND
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#endif


static int argc=0;
static char **argv=0;


char *
GetBaseAppName (char *rawName)
{
  char *cp;

  if ( rawName == NULL )
    return strdup("Unknown");

  /* delete directorys and make a copy, if nec */
  if ((cp = rindex (rawName, '/')) == NULL)
    {
      return rawName;
    }
  else
    {
      return cp + 1;		/* skip over the '/' */
    }
}

#ifdef HAVE_LIBUNWIND

int
mpiPi_RecordTraceBack (jmp_buf jb, void *pc_array[], int max_back, int parent_frame_start)
{
  int i, valid_cursor, frame_count = 0;
  unw_context_t uc;
  unw_cursor_t cursor;
  unw_word_t pc;

  if ( unw_getcontext(&uc) != 0 )
  {
    mpiPi_msg_debug("Failed unw_getcontext!\n");
    return frame_count;
  }

  if ( unw_init_local(&cursor, &uc) != UNW_ESUCCESS )
  {
    mpiPi_msg_debug ("Failed to initialize libunwind cursor with unw_init_local\n");
  }
  else
  {
    for (i = 0; i < parent_frame_start; i++)
    {
      if ( unw_step(&cursor) )
        mpiPi_msg_debug ("unw_step failed to step into mpiPi caller frame.\n");
    }

    for (i = 0, valid_cursor = 1; i < max_back; i++)
    {
      if ( valid_cursor && unw_step(&cursor) > 0 )
      {
        frame_count++;
        if ( unw_get_reg(&cursor, UNW_REG_IP, &pc) != UNW_ESUCCESS )
        {
          pc_array[i] = NULL;
          mpiPi_msg_debug("unw_get_reg failed.\n");
        }
        else
        {
          pc_array[i] = (void*)((char*)pc - 1);
        }
      }
      else
      {
        pc_array[i] = NULL;
        mpiPi_msg_debug("unw_step failed.\n");
        valid_cursor = 0;
      }
    }
  }
  return frame_count;
}

#else  /* not LIBUNWIND */

#ifdef OSF1

int
mpiPi_RecordTraceBack (jmp_buf jb, void *pc_array[], int max_back, int parent_frame_start)
{
  int i, frame_count = 0;
  void *pc;
  PCONTEXT context;

  context = (PCONTEXT)jb;
  pc = (void*)context->sc_pc;

  for (i = 1; i < parent_frame_start; i++)
  {
    if ( pc != NULL )
      unwind(context, 0);
  }

  for (i = 0; i < max_back; i++)
  {
    if ( pc != NULL )
    {
      unwind(context, 0);
      
      /* record this frame's pc and calculate the previous instruction  */
      pc_array[i] = (void*)context->sc_pc - (void*)0x4; 
      pc = (void*)context->sc_pc;
      frame_count++;
    }
    else
    {
      pc_array[i] = NULL;
    }
  }
  return frame_count;
}

#else  /* not OSF1 */

int
mpiPi_RecordTraceBack (jmp_buf jb, void *pc_array[], int max_back, int parent_frame_start)
{
  int i, frame_count = 0;
  void *fp, *lastfp;
  void* pc;

  /* start w/ the parent frame, not ours (we know who's calling) */
  fp = ParentFP(jb);

  for (i = 1; i < parent_frame_start && fp != NULL; i++)
    fp = NextFP(fp);

  if ( parent_frame_start == 0 )
  {
    pc_array[0] = CurrentPC(jb);
    frame_count++;
  }

  for (i = 0; i < max_back; i++)
    {
      if (fp != NULL)
	{
	  /* record this frame's pc */
	  pc = FramePC(fp);
	  if ( pc != NULL )
	  {
	    frame_count++;
            /*  Get previous instruction  */
	    pc_array[i] = (void*)((char*)pc - 1);
	  }
          else
            pc_array[i] = NULL;
          
	  /* update frame ptr */
          lastfp = fp;
	  fp = NextFP(fp);
	}
      else
	{
	  /* pad the array w/ nulls, if necessary */
	  pc_array[i] = NULL;
	}
    }

  return frame_count;
}
#endif
#endif

void
mpiPi_getenv ()
{				/* NEED TO ADD SANITY CHECKS HERE */
  char *cp = NULL;
  char *ep = NULL;

  mpiPi.outputDir = ".";

  ep = getenv ("MPIP");
  mpiPi.envStr = (ep?strdup (ep):0);
  optind = 1;  /* reset to avoid conflicts if getopt already called */
  if (ep != NULL)
    {
      int c;
      extern char *optarg;
      extern int optind;
      int ac = 0;
      char *av[64];
      char *sep = " \t";

      if (mpiPi.rank == 0)
	mpiPi_msg ("Found MPIP environment variable [%s]\n", ep);
      av[0] = "JUNK";
      for (cp = strtok (ep, sep), ac = 1; (ac < 64) && (NULL != cp); ac++)
	{
	  av[ac] = cp;
	  /*mpiPi_msg("av[%d] = %s\n", ac, av[ac]); */
	  cp = strtok (NULL, sep);
	}

      av[ac] = NULL;

      for (; ((c = getopt (ac, av, "cngf:b:s:k:t:oe")) != EOF);)
	{
	  switch (c) {
	    case 'f':
	      mpiPi.outputDir = optarg;
	      if (mpiPi.rank == 0)
		mpiPi_msg
		  ("Set the final tracfile output directory to [%s].\n",
		   mpiPi.outputDir);
	      break;

	    case 'g':
	      mpiPi_debug = 1;
	      if (mpiPi.rank == 0)
		mpiPi_msg ("Enabled mpiPi debug mode.\n");
	      break;

	    case 's':
	      {
		int defaultSize = mpiPi.tableSize;
		mpiPi.tableSize = atoi (optarg);
		if (mpiPi.tableSize < 2)
		  {
		    if (mpiPi.rank == 0)
		      mpiPi_msg_warn
			("-s tablesize invalid %d. Using default.\n",
			 mpiPi.tableSize);
		    mpiPi.tableSize = defaultSize;
		  }
		if (mpiPi.tableSize < 128)
		  {
		    if (mpiPi.rank == 0)
		      mpiPi_msg_warn
			("tablesize small %d. Consider making it larger w/ -s.\n",
			 mpiPi.tableSize);
		  }
		if (mpiPi.rank == 0)
		  mpiPi_msg
		    ("Set the callsite table size to [%d].\n",
		     mpiPi.tableSize);
	      }
	      break;

	    case 'k':
	      {
		int defaultStackDepth = mpiPi.stackDepth;
		mpiPi.stackDepth = atoi (optarg);
		if (mpiPi.stackDepth < 1)
		  {
		    if (mpiPi.rank == 0)
		      mpiPi_msg_warn
			("-k stack depth invalid %d. Using default.\n",
			 mpiPi.stackDepth);
		    mpiPi.stackDepth = defaultStackDepth;
		  }
		if (mpiPi.stackDepth > 2)
		  {
		    if (mpiPi.rank == 0)
		      mpiPi_msg_warn
			("stackdepth large %d. Consider making it smaller.\n",
			 mpiPi.stackDepth);
		  }
		if (mpiPi.stackDepth > MPIP_CALLSITE_STACK_DEPTH_MAX)
		  {
		    if (mpiPi.rank == 0)
		      mpiPi_msg_warn
			("stackdepth too large %d. Making it smaller %d.\n",
			 mpiPi.stackDepth);
		    mpiPi.stackDepth = MPIP_CALLSITE_STACK_DEPTH_MAX;
		  }
		if (mpiPi.rank == 0)
		  mpiPi_msg
		    ("Set the callsite stack traceback depth to [%d].\n",
		     mpiPi.stackDepth);
	      }
	      break;

	    case 't':
	      {
		int defaultThreshold = mpiPi.reportPrintThreshold;
		mpiPi.reportPrintThreshold = atof (optarg);
		if (mpiPi.reportPrintThreshold < 0)
		  {
		    if (mpiPi.rank == 0)
		      mpiPi_msg_warn
			("-k report print threshold invalid %g. Using default.\n",
			 mpiPi.reportPrintThreshold);
		    mpiPi.reportPrintThreshold = defaultThreshold;
		  }
		if (mpiPi.reportPrintThreshold >= 100)
		  {
		    if (mpiPi.rank == 0)
		      mpiPi_msg_warn
			("report print threshold large %g. Making it default.\n",
			 mpiPi.reportPrintThreshold);
		    mpiPi.reportPrintThreshold = defaultThreshold;
		  }
		if (mpiPi.rank == 0)
		  mpiPi_msg
		    ("Set the report print threshold to [%3.2lf%%].\n",
		     mpiPi.reportPrintThreshold);
	      }
	      break;

	    case 'o':
	      {
		    if (mpiPi.rank == 0)
		      mpiPi_msg_warn
			("Disabling MPIP at Init. Code must use Pcontrol to enable.\n");
		    mpiPi.enabled = 0;
		    mpiPi.enabledCount = 0;
	      }
	      break;
              
	    case 'n':
              mpiPi.baseNames = 1;
              break;
	      
	    case 'e':
	      mpiPi.reportFormat = MPIP_REPORT_FLT_FORMAT;
              break;
	      
	    case 'c':  /* changed to disable COV value */
              mpiPi.calcCOV = 0;
              break;

	    case 'a':
	    case 'b':
	    case 'd':
	    case 'h':
	    case 'i':
	    case 'j':
	    case 'l':
	    case 'm':
	    case 'p':
	    case 'q':
	    case 'r':
	    case 'u':
	    case 'v':
	    case 'w':
	    case 'x':
	    case 'y':
	    case 'z':
	    default:
	      if (mpiPi.rank == 0)
		mpiPi_msg_warn
		  ("Option flag (-%c) not recognized. Ignored.\n", c);
	      break;
	    }
	}
    }
  if (mpiPi.rank == 0)
    mpiPi_msg ("\n");
  optind = 1;  /* reset to avoid conflicts if getopt called again */
}


#ifdef Linux
char* getProcExeLink()
{
  int pid, fh, exelen, insize = 256;
  char *inbuf = NULL, file[256];

  pid = getpid();
  sprintf(file, "/proc/%d/exe", pid);
  inbuf = malloc(insize);

  exelen = readlink(file, inbuf, 256);
  if ( exelen != ENOENT )
  {
    while ( exelen == ENAMETOOLONG )
    {
      insize += 256;
      inbuf = realloc(inbuf, insize);
      exelen = readlink(file,inbuf,insize);
    }
    inbuf[exelen] = 0;
    return inbuf;
  }
  else
    free(inbuf);

  return NULL;
}

#define MPIP_MAX_ARG_STRING_SIZE 4096
void getProcCmdLine(int* ac, char** av, int max_args)
{
  int i = 0, pid, exelen, insize = 256;
  char *inbuf, file[256];
  FILE* infile;
  char* arg_ptr;
  long fsize;

  *ac = 0;
  *av = NULL;

  pid = getpid();
  sprintf(file, "/proc/%d/cmdline", pid);
  infile = fopen(file, "r");
  
  if ( infile != NULL )
  {
    while ( !feof(infile) )
    {
      inbuf = malloc(MPIP_MAX_ARG_STRING_SIZE);
      if ( fread(inbuf, 1, MPIP_MAX_ARG_STRING_SIZE, infile) > 0 )
      {
        arg_ptr = inbuf;
	while ( *arg_ptr != 0 )
	{
          av[i] = strdup(arg_ptr);
	  arg_ptr += strlen(av[i]) + 1;
	  i++;
        }
      }
    }
    *ac = i;

    free(inbuf);
    fclose(infile);
  }
}

#endif

void
mpiPi_copy_args (int *ac, char **av, int av_len)
{
  int i;
  extern int mpiPi_debug;

  assert( ac != NULL );
  assert( av != NULL );

#if defined(AIX)
  {
    /*  Works for C/C++ and Fortran  */
    extern int p_xargc;
    extern char **p_xargv;
    argc = p_xargc;
    argv = p_xargv;
  }
#elif defined(Intel_Fortran) && !defined(Linux) && !defined(USE_GETARG)
  {
    extern int xargc;
    extern char **xargv;
    argc = xargc;
    argv = xargv;
  }
#elif defined(GNU_Fortran)
  {
    extern int f__xargc;
    extern char **f__xargv;
    argc = f__xargc;
    argv = f__xargv;
  }
#elif defined(OSF1)
  {
    /*  Works for C/C++ and Fortran  */
    extern int __Argc;
    extern char **__Argv;
    argc = __Argc;
    argv = __Argv;
  }
#elif defined(PGI)
  {
    extern int __argc_save;
    extern char **__argv_save;
    argc = __argc_save;
    argv = __argv_save;
  }
#endif  

   mpiPi_copy_given_args(ac, av, av_len, argc, argv);
}


void
mpiPi_copy_given_args (int *ac, char **av, int av_len, int argc, char **argv)
{
  int i;

  assert( ac != NULL );
  assert( av != NULL );

#if defined(USE_GETARG) && (defined(Intel_Fortran) || defined(Cray_Fortran))

#define EXECUTABLE_LEN 2048
{
  char buf[EXECUTABLE_LEN];
  int len, mpiPi_argc;
  extern void F77_MPIPI_GET_FORTRAN_ARGC(int*);

  F77_MPIPI_GET_FORTRAN_ARGC(&mpiPi_argc);
  *ac = mpiPi_argc;
/*  mpiPi_msg_debug("ac is %d\n", *ac); */

  for ( i = 0; i <= mpiPi_argc; i++ )
  {
    int buf_len = EXECUTABLE_LEN;

#if defined(Intel_Fortran)
    extern void F77_MPIPI_GET_FORTRAN_ARG(int, int, char*, int*);
    F77_MPIPI_GET_FORTRAN_ARG(i,EXECUTABLE_LEN,buf,&len);
#elif defined(Cray_Fortran)
    /* As far as I can tell from the Cray C and C++ Reference Manual,
     * parameter passing to Cray Fortran subroutines is always by
     * reference, so we need to pass pointers for all arguments even
     * if they are input-only parameters.
     */
    extern void F77_MPIPI_GET_FORTRAN_ARG(int*, int*, char*, int*);
    F77_MPIPI_GET_FORTRAN_ARG( &i, &buf_len, buf, &len);
#endif

    buf[len<EXECUTABLE_LEN ? len : EXECUTABLE_LEN-1] = 0;
    av[i] = strdup(buf);
  }
}

#else   /* USE_GETARG */
  *ac = argc;

  for (i = 0; (i < *ac) && (i < av_len); i++)
      av[i] = strdup(argv[i]);
#endif
}

/* eof */

