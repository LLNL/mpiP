

#include <stdio.h>
#include <setjmp.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "mpiPi.h"
#include "mpiPi_proto.h"
#include "mpiP-API.h"

static int mpiP_api_init = 0;

void mpiP_init_api()
{
  char* mpiP_env;

  mpiP_env = getenv("MPIP");
  if ( mpiP_env != NULL && strstr(mpiP_env, "-g") != NULL )
    mpiPi_debug = 1;
  else
    mpiPi_debug = 0;

  mpiPi.stdout_ = stdout;
  mpiPi.stderr_ = stderr;
  mpiP_api_init = 1;
  mpiPi.toolname = "API";
}


void mpiP_record_traceback(void* pc_array[], int max_stack)
{
  jmp_buf jb;

  if ( mpiP_api_init == 0 )
    mpiP_init_api();

  setjmp(jb);
  mpiPi_RecordTraceBack(jb, pc_array, max_stack);
}

int
mpiP_find_src_loc(void *i_addr_hex, char **o_file_str, int *o_lineno,
              char **o_funct_str);

int mpiP_open_executable(char* filename)
{
  char* cwd = NULL;
  char* workname = NULL;
  int len = 0;

  if ( mpiP_api_init == 0 )
    mpiP_init_api();

  if ( access(filename, R_OK|F_OK) != 0 )
  	return -1;

  /*  open_bfd_executable will fail without full path  */
  if ( filename[0] != '/' )
  {
    cwd = getcwd(NULL, 0);
    len = strlen(cwd) + strlen(filename);
    workname = malloc(len+1);
    if ( workname == NULL )
    	return -1;
    strcpy(workname, cwd);
    strcat(workname, filename);
  }

  open_bfd_executable(filename);

  if ( cwd != NULL )
  	free(cwd);

  if ( workname != NULL )
  	free(workname);

  return 0;
}

void mpiP_close_executable()
{
  close_bfd_executable();
}

mpiP_TIMER mpiP_gettime()
{
  mpiPi_TIME currtime;
  mpiPi_GETTIME(&currtime);
  return mpiPi_GETUSECS(&currtime);
}
