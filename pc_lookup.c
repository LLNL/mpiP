/* -*- C -*- 

   @PROLOGUE@

   ----- 

   Jeffrey Vetter vetter3@llnl.gov
   Center for Applied Scientific Computing, LLNL
   16 Feb 2001

   pc_lookup.c -- functions that interface to bfd for source code
   lookups

 */

#ifndef lint
static char *rcsid =
  "$Header$";
#endif

#if defined(DEMANGLE_GNU)
#include <ansidecl.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpiPi.h"
#include "mpiPconfig.h"

#ifndef DISABLE_BFD
#ifndef CEXTRACT
#include "bfd.h"
#endif
static asymbol **syms;
static bfd_vma pc;
static const char *filename;
static const char *functionname;
static unsigned int line;
static bfd *abfd = NULL;

/*  BFD boolean and bfd_boolean types have changed through versions.
    It looks like bfd_boolean will be preferred.                     */
#ifdef BFD_TRUE_FALSE
typedef boolean bfd_boolean;
#endif
#ifndef FALSE
#define FALSE 0
#endif
static bfd_boolean found;


#ifdef DEMANGLE_IBM

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

#elif DEMANGLE_Compaq

#include <demangle.h>
#include <string.h>

char* mpiPdemangle(const char* mangledSym)
{
  char out[1024];

  MLD_demangle_string(mangledSym, out, 1024, MLD_SHOW_INFO | MLD_SHOW_DEMANGLED_NAME);

  return strdup(out);
}


#elif DEMANGLE_GNU

#ifdef HAVE_DEMANGLE_H
#include "demangle.h"
#else
#define DMGL_PARAMS    (1 << 0)
#define DMGL_ANSI      (1 << 1)
extern char *cplus_demangle (const char *mangled, int options);
#endif

char* mpiPdemangle(const char* mangledSym)
{
  return cplus_demangle (mangledSym, DMGL_ANSI | DMGL_PARAMS);
}

#endif


void
find_address_in_section (abfd, section, data)
     bfd *abfd;
     asection *section;
     PTR data;
{
  bfd_vma vma;
  bfd_size_type size;
  bfd_vma local_pc = pc;

  assert (abfd);
  if (found)
    return;

#ifdef OSF1
  local_pc = pc | 0x100000000;
#elif defined(UNICOS_mp)
  /* use only least significant 32-bits */
  local_pc = pc & 0xFFFFFFFF;
#else
  local_pc = pc /*& (~0x10000000) */ ;
#endif

  if ((bfd_get_section_flags (abfd, section) & SEC_ALLOC) == 0)
  {
    mpiPi_msg_debug ("failed bfd_get_section_flags\n");
    return;
  }
  vma = bfd_get_section_vma (abfd, section);

  if (local_pc < vma)
  {
    mpiPi_msg_debug ("failed bfd_get_section_vma: local_pc=0x%lx  vma=0x%lx\n",
      local_pc, vma);
    return;
  }

/* The following define addresses binutils modifications between
 * version 2.15 and 2.15.96
 */
#ifdef HAVE_BFD_GET_SECTION_SIZE
  size = bfd_get_section_size (section);
#else      
  if ( section->reloc_done )
     size = bfd_get_section_size_after_reloc (section);
  else
     size = bfd_get_section_size_before_reloc (section);
#endif

  if (local_pc >= vma + size)
  {
    mpiPi_msg_debug ("PC not in section: pc=%lx vma=%lx-%lx\n",
  		     local_pc, vma, vma+size);
    return;
  }


  found = bfd_find_nearest_line (abfd, section, syms, local_pc - vma,
				 &filename, &functionname, &line);

  if ( !found )
  {
    mpiPi_msg_debug ("bfd_find_nearest_line failed for : pc=%x vma=%x-%x\n",
                     local_pc, vma, vma+size);
  }

  mpiPi_msg_debug ("bfd_find_nearest_line for : pc=%x vma=%x-%x\n",
                   local_pc, vma, vma+size);
  mpiPi_msg_debug ("                 returned : %s:%s:%u\n",
                   filename, functionname, line);
}


int
mpiP_find_src_loc (void *i_addr_hex, char **o_file_str, int *o_lineno,
	      char **o_funct_str)
{
  char buf[128];
  assert (i_addr_hex);

  if ( abfd == NULL )
  {
    mpiPi_msg_debug("mpiP_find_src_loc returning failure as abfd == NULL\n");
    return 1;
  }

  sprintf (buf, "0x%lx", (unsigned long) i_addr_hex);
  pc = bfd_scan_vma (buf, NULL, 16);

  /* jsv hack - trim high bit off of address */
  /*  pc &= !0x10000000; */
  found = FALSE;

  bfd_map_over_sections (abfd, find_address_in_section, (PTR) NULL);

  if (!found)
    {
      mpiPi_msg_debug("returning not found in mpiP_find_src_loc\n");
      return 1;
    }
  else
    {
      /* determine the function name */
      if (functionname == NULL || *functionname == '\0')
	{
	  *o_funct_str = strdup ("[unknown]");
	}
      else
	{
	  char *res = NULL;

#if defined(DEMANGLE_IBM) || defined(DEMANGLE_Compaq) || defined(DEMANGLE_GNU)
          res = mpiPdemangle(functionname);
#endif
	  if (res == NULL)
	    {
	      *o_funct_str = strdup (functionname);
	    }
	  else
	    {
	      *o_funct_str = res;
	    }

#if defined(DEMANGLE_IBM) || defined(DEMANGLE_Compaq) || defined(DEMANGLE_GNU)
          mpiPi_msg_debug ("attempted demangle %s->%s\n", functionname, *o_funct_str);
#endif
	}

      /* set the filename and line no */
      if (mpiPi.baseNames == 0 && filename != NULL)
	{
	  char *h;
	  h = strrchr (filename, '/');
	  if (h != NULL)
	    filename = h + 1;
	}

      *o_lineno = line;
      *o_file_str = strdup (filename ? filename : "[unknown]");

      mpiPi_msg_debug ("BFD: %s -> %s:%u:%s\n", buf, *o_file_str, *o_lineno,
		       *o_funct_str);
    }
  return 0;
}


void open_bfd_executable (char *filename)
{
  char *target = NULL;
  char **matching = NULL;
  long storage;
  long symcount;

  if (filename == NULL)
  {
    mpiPi_msg_warn("Executable filename is NULL!\n");
    mpiPi_msg_warn("If this is a Fortran application, you may be using the incorrect mpiP library.\n");
    return;
  }

  bfd_init ();
  /* set_default_bfd_target (); */
  mpiPi_msg_debug ("opening filename %s\n", filename);
  abfd = bfd_openr (filename, target);
  if (abfd == NULL)
    mpiPi_abort ("could not open filename %s", filename);
  if (bfd_check_format (abfd, bfd_archive))
    mpiPi_abort ("can not get addresses from archive");
  if (!bfd_check_format_matches (abfd, bfd_object, &matching))
  {
    char *curr_match;
    if (matching != NULL)
    {
      for ( curr_match = matching[0]; curr_match != NULL; curr_match++ )
        mpiPi_msg_debug ("found matching type %s\n", curr_match);
      free(matching); 
    }
    mpiPi_abort ("matching failed");
  }
  
  if ((bfd_get_file_flags (abfd) & HAS_SYMS) == 0)
    mpiPi_abort ("No symbols in the executable\n");
  /* TODO: move this to the begining of the process so that the user
     knows before the application begins */
  storage = bfd_get_symtab_upper_bound (abfd);
  if (storage < 0)
    mpiPi_abort ("storage < 0");
  syms = (asymbol **) malloc (storage);
  symcount = bfd_canonicalize_symtab (abfd, syms);

  if (symcount < 0)
    mpiPi_abort ("symcount < 0");
  else
    {
      mpiPi_msg_debug ("\n");
      mpiPi_msg_debug ("found %d symbols in file [%s]\n", symcount, filename);
    }
}

void close_bfd_executable ()
{
  assert (abfd);
  bfd_close (abfd);
}

#elif !defined(USE_LIBDWARF)

int
mpiP_find_src_loc (void *i_addr_hex, char **o_file_str, int *o_lineno,
	      char **o_funct_str)
{
  return 1;  /*  failure  */
}

#endif  /* DISABLE_BFD */

/* eof */
