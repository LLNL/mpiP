
#include "mpiPdemangle.h"
#include "mpiPconfig.h"


#ifdef AIX

#include <demangle.h>
#include <string.h>

char* mpiPdemangle(const char* mangledSym)
{
  Name *ret;
  char *rest;
  unsigned long opts = 0x1;

  ret = demangle((char*)mangledSym, &rest, opts);

  if ( ret != NULL )
    return strdup(text(ret));
  else
    return NULL;
}

#else

#ifdef HAVE_DEMANGLE_H
#include "demangle.h"
#else
extern char *cplus_demangle (const char *mangled, int options);
#endif

char* mpiPdemangle(const char* mangledSym)
{
  return cplus_demangle (mangledSym, DMGL_ANSI | DMGL_PARAMS);
}

#endif

