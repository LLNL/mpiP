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

#include <ansidecl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpiPconfig.h"

#include "bfd.h"
#if 0
#include "bucomm.h"
#endif

#ifdef HAVE_DEMANGLE_H
#include "demangle.h"
#else
extern char *cplus_demangle (const char *mangled, int options);
#endif

#define boolean int
#define false 0
#define true (!false)

static asymbol **syms;
static bfd_vma pc;
static const char *filename;
static const char *functionname;
static unsigned int line;
static boolean found;
static bfd *abfd = NULL;

static boolean with_functions = 0;	/* -f, show function names.  */
static boolean do_demangle = 0;	/* -C, demangle names.  */
static boolean base_names = 1;	/* -s, strip directory names.  */

void
find_address_in_section (abfd, section, data)
     bfd *abfd;
     asection *section;
     PTR data;
{
  bfd_vma vma;
  bfd_size_type size;
  long local_pc = pc;

  assert (abfd);
  if (found)
    return;

#ifdef OSF1
  local_pc = pc | 0x100000000;
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
    mpiPi_msg_debug ("failed bfd_get_section_vma: local_pc=0x%p  vma=0x%p\n",
      local_pc, vma);
    return;
  }

  if ( section->reloc_done )
     size = bfd_get_section_size_after_reloc (section);
  else
     size = bfd_get_section_size_before_reloc (section);
		   
  if (local_pc >= vma + size)
  {
    mpiPi_msg_debug ("PC not in section: pc=%x vma=%x-%x\n",
  		     (long) local_pc, vma, vma+size);
    return;
  }

  found = bfd_find_nearest_line (abfd, section, syms, local_pc - vma,
				 &filename, &functionname, &line);

  mpiPi_msg_debug ("bfd_find_nearest_line for : pc=%x vma=%x-%x\n",
                   (long) local_pc, vma, vma+size);
  mpiPi_msg_debug ("                 returned : %s:%s:%u\n",
                   filename, functionname, line);
}


int
find_src_loc (void *i_addr_hex, char **o_file_str, int *o_lineno,
	      char **o_funct_str)
{
  char buf[128];
  assert (abfd);
  assert (i_addr_hex);


  sprintf (buf, "0x%x", (long) i_addr_hex);
  pc = bfd_scan_vma (buf, NULL, 16);
  /* jsv hack - trim high bit off of address */
  /*  pc &= !0x10000000; */
  found = false;

  bfd_map_over_sections (abfd, find_address_in_section, (PTR) NULL);

  if (!found)
    {
      return 1;
    }
  else
    {
      /* determine the function name */
      if (functionname == NULL || *functionname == '\0')
	{
	  *o_funct_str = strdup ("??");
	}
      else if (!do_demangle)
	{
	  *o_funct_str = strdup (functionname);
	}
      else
	{
	  char *res = NULL;
#if 0
	  /* jsv - need to figure out how to integrate demangling */
	  res = cplus_demangle (functionname, DMGL_ANSI | DMGL_PARAMS);
#endif
	  if (res == NULL)
	    {
	      *o_funct_str = strdup (functionname);
	    }
	  else
	    {
	      *o_funct_str = res;
	    }
	}

      /* set the filename and line no */
      if (base_names && filename != NULL)
	{
	  char *h;
	  h = strrchr (filename, '/');
	  if (h != NULL)
	    filename = h + 1;
	}

      *o_lineno = line;
      *o_file_str = strdup (filename ? filename : "??");

      mpiPi_msg_debug ("BFD: %s -> %s:%u:%s\n", buf, *o_file_str, *o_lineno,
		       *o_funct_str);
    }
  return 0;
}


open_bfd_executable (char *filename)
{
  char *target = NULL;
  char **matching = NULL;
  long storage;
  long symcount;

  bfd_init ();
  /* set_default_bfd_target (); */
  abfd = bfd_openr (filename, target);
  if (abfd == NULL)
    mpiPi_abort ("could not open filename %s", filename);
  if (bfd_check_format (abfd, bfd_archive))
    mpiPi_abort ("can not get addresses from archive");
  if (!bfd_check_format_matches (abfd, bfd_object, &matching))
    mpiPi_abort ("matching failed");
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
      mpiPi_msg ("\n");
      mpiPi_msg ("found %d symbols in file [%s]\n", symcount, filename);
    }
}

close_bfd_executable ()
{
  assert (abfd);
  bfd_close (abfd);
}

/* eof */
