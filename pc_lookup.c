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
#ifdef HAVE_BFD_BOOLEAN
typedef boolean bfd_boolean;
#endif
#ifndef FALSE
#define FALSE 0
#endif
static bfd_boolean found;


#ifdef DEMANGLE_IBM

#include <demangle.h>
#include <string.h>

char *
mpiPdemangle (const char *mangledSym)
{
  Name *ret;
  char *rest;
  unsigned long opts = 0x1;

  ret = demangle ((char *) mangledSym, &rest, opts);

  if (ret != NULL)
    return strdup (text (ret));
  else
    return NULL;
}

#elif DEMANGLE_Compaq

#include <demangle.h>
#include <string.h>

char *
mpiPdemangle (const char *mangledSym)
{
  char out[1024];

  MLD_demangle_string (mangledSym, out, 1024,
		       MLD_SHOW_INFO | MLD_SHOW_DEMANGLED_NAME);

  return strdup (out);
}


#elif DEMANGLE_GNU

#ifdef HAVE_DEMANGLE_H
#include "demangle.h"
#else
#define DMGL_PARAMS    (1 << 0)
#define DMGL_ANSI      (1 << 1)
extern char *cplus_demangle (const char *mangled, int options);
#endif

char *
mpiPdemangle (const char *mangledSym)
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
  char addr_buf1[24], addr_buf2[24], addr_buf3[24];

  assert (abfd);
  if (found)
    return;

#ifdef OSF1
  local_pc = pc | 0x100000000;
#elif defined(UNICOS_mp)
  /* use only least significant 32-bits */
  local_pc = pc & 0xFFFFFFFF;
#elif defined(AIX)
  local_pc = pc;
  if (mpiPi.obj_mode == 32)
    local_pc -= 0x10000000;
  else
    local_pc &= 0x00000000FFFFFFFF;
  local_pc += mpiPi.text_start;
  mpiPi_msg_debug ("pc is %s, text_start is %s, local_pc is %s\n",
		   mpiP_format_address ((void *) pc, addr_buf1),
		   mpiP_format_address ((void *) mpiPi.text_start, addr_buf2),
		   mpiP_format_address ((void *) local_pc, addr_buf3));
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
      mpiPi_msg_debug
	("failed bfd_get_section_vma: local_pc=%s  vma=%s\n",
	 mpiP_format_address ((void *) local_pc, addr_buf1),
	 mpiP_format_address ((void *) vma, addr_buf2));
      return;
    }

/* The following define addresses binutils modifications between
 * version 2.15 and 2.15.96
 */
#ifdef HAVE_BFD_GET_SECTION_SIZE
  size = bfd_get_section_size (section);
#else
  if (section->reloc_done)
    size = bfd_get_section_size_after_reloc (section);
  else
    size = bfd_get_section_size_before_reloc (section);
#endif

  if (local_pc >= vma + size)
    {
      mpiPi_msg_debug ("PC not in section: pc=%s vma=%s-%s\n",
		       mpiP_format_address ((void *) local_pc, addr_buf1),
		       mpiP_format_address ((void *) vma, addr_buf2),
		       mpiP_format_address ((void *) (vma + size),
					    addr_buf3));
      return;
    }


  found = bfd_find_nearest_line (abfd, section, syms, local_pc - vma,
				 &filename, &functionname, &line);

  if (!found)
    {
      mpiPi_msg_debug ("bfd_find_nearest_line failed for : pc=%s vma=%s-%s\n",
		       mpiP_format_address ((void *) local_pc, addr_buf1),
		       mpiP_format_address ((void *) vma, addr_buf2),
		       mpiP_format_address ((void *) (vma + size),
					    addr_buf3));
    }

  mpiPi_msg_debug ("bfd_find_nearest_line for : pc=%s vma=%s-%s\n",
		   mpiP_format_address ((void *) local_pc, addr_buf1),
		   mpiP_format_address ((void *) vma, addr_buf2),
		   mpiP_format_address ((void *) (vma + size), addr_buf3));

  mpiPi_msg_debug ("                 returned : %s:%s:%u\n",
		   filename, functionname, line);
}


int
mpiP_find_src_loc (void *i_addr_hex, char **o_file_str, int *o_lineno,
		   char **o_funct_str)
{
  char buf[128];
  char addr_buf[24];

  if (i_addr_hex == NULL)
    {
      mpiPi_msg_debug
	("mpiP_find_src_loc returning failure as i_addr_hex == NULL\n");
      return 1;
    }

  if (abfd == NULL)
    {
      mpiPi_msg_debug
	("mpiP_find_src_loc returning failure as abfd == NULL\n");
      return 1;
    }

  sprintf (buf, "%s", mpiP_format_address (i_addr_hex, addr_buf));
  pc = bfd_scan_vma (buf, NULL, 16);

  /* jsv hack - trim high bit off of address */
  /*  pc &= !0x10000000; */
  found = FALSE;

  bfd_map_over_sections (abfd, find_address_in_section, (PTR) NULL);

  if (!found)
    {
      mpiPi_msg_debug ("returning not found in mpiP_find_src_loc\n");
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
	  res = mpiPdemangle (functionname);
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
	  mpiPi_msg_debug ("attempted demangle %s->%s\n", functionname,
			   *o_funct_str);
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


int
open_bfd_executable (char *filename)
{
  char *target = NULL;
  char **matching = NULL;
  long storage;
  long symcount;

  if (filename == NULL)
    {
      mpiPi_msg_warn ("Executable filename is NULL!\n");
      mpiPi_msg_warn
	("If this is a Fortran application, you may be using the incorrect mpiP library.\n");
      return 0;
    }

#ifdef AIX

  /*  Kludge to get XCOFF text_start value to feed an approriate address
     value to bfd for looking up source info
   */
  mpiPi.text_start = mpiPi_get_text_start (filename);
#endif

  bfd_init ();
  /* set_default_bfd_target (); */
  mpiPi_msg_debug ("opening filename %s\n", filename);
  abfd = bfd_openr (filename, target);
  if (abfd == NULL)
    {
      mpiPi_msg_warn ("could not open filename %s", filename);
      return 0;
    }
  if (bfd_check_format (abfd, bfd_archive))
    {
      mpiPi_msg_warn ("can not get addresses from archive");
      bfd_close (abfd);
      return 0;
    }
  if (!bfd_check_format_matches (abfd, bfd_object, &matching))
    {
      char *curr_match;
      if (matching != NULL)
	{
	  for (curr_match = matching[0]; curr_match != NULL; curr_match++)
	    mpiPi_msg_debug ("found matching type %s\n", curr_match);
	  free (matching);
	}
      mpiPi_msg_warn ("matching failed");
      bfd_close (abfd);
      return 0;
    }

#if 0
  if ((bfd_get_file_flags (abfd) & HAS_SYMS) == 0)
    {
      mpiPi_msg_warn ("No symbols in the executable\n");
      bfd_close (abfd);
      return 0;
    }
#endif
  /* TODO: move this to the begining of the process so that the user
     knows before the application begins */
  storage = bfd_get_symtab_upper_bound (abfd);
  if (storage < 0)
    {
      mpiPi_msg_warn ("storage < 0");
      bfd_close (abfd);
      return 0;
    }
  syms = (asymbol **) malloc (storage);
  symcount = bfd_canonicalize_symtab (abfd, syms);

  if (symcount < 0)
    {
      mpiPi_msg_warn ("symcount < 0");
      bfd_close (abfd);
      return 0;
    }
  else
    {
      mpiPi_msg_debug ("\n");
      mpiPi_msg_debug ("found %d symbols in file [%s]\n", symcount, filename);
    }

  return 1;
}

void
close_bfd_executable ()
{
  assert (abfd);
  bfd_close (abfd);
}

#elif !defined(USE_LIBDWARF)

int
mpiP_find_src_loc (void *i_addr_hex, char **o_file_str, int *o_lineno,
		   char **o_funct_str)
{
  return 1;			/*  failure  */
}

#endif /* DISABLE_BFD */

#ifdef AIX
/*  Get the text_start value from the auxiliary header
    to provide BFD with pc values that match source information.
*/
#define __XCOFF32__
#define __XCOFF64__
#include <fcntl.h>
#include <filehdr.h>
#include <aouthdr.h>
#include <scnhdr.h>

unsigned long long
mpiPi_get_text_start (char *filename)
{
  int fh;
  short magic;
  FILHDR FileHeader32;
  FILHDR_64 FileHeader64;
  AOUTHDR AoutHeader32;
  AOUTHDR_64 AoutHeader64;
  SCNHDR SectHeader32;
  SCNHDR_64 SectHeader64;

  unsigned long long text_start = 0;
  int count = 0;

  fh = open (filename, O_RDONLY);
  if (fh == -1)
    return 0;

  read (fh, &magic, 2);
  mpiPi_msg_debug ("magic is 0x%x\n", magic);
  lseek (fh, 0, 0);

  if (magic == 0x01DF)		/* 32-bit  */
    {
      mpiPi.obj_mode = 32;
      read (fh, &FileHeader32, sizeof (FILHDR));
      mpiPi_msg_debug ("aout size is %d\n", FileHeader32.f_opthdr);
      read (fh, &AoutHeader32, FileHeader32.f_opthdr);
      mpiPi_msg_debug ("text start is 0x%0x\n", AoutHeader32.o_text_start);

      while (count++ < FileHeader32.f_nscns)
	{
	  read (fh, &SectHeader32, sizeof (SCNHDR));
	  mpiPi_msg_debug ("found header name %s\n", SectHeader32.s_name);
	  mpiPi_msg_debug ("found header raw ptr 0x%0x\n",
			   SectHeader32.s_scnptr);

	  if (SectHeader32.s_flags & STYP_TEXT)
	    text_start = AoutHeader32.o_text_start - SectHeader32.s_scnptr;
	}
    }
  else if (magic == 0x01EF || magic == 0x01F7)	/*  64-bit  */
    {
      mpiPi.obj_mode = 64;
      read (fh, &FileHeader64, sizeof (FILHDR_64));
      mpiPi_msg_debug ("aout size is %d\n", FileHeader64.f_opthdr);
      read (fh, &AoutHeader64, FileHeader64.f_opthdr);
      mpiPi_msg_debug ("text start is 0x%0llx\n", AoutHeader64.o_text_start);

      while (count++ < FileHeader64.f_nscns)
	{
	  read (fh, &SectHeader64, sizeof (SCNHDR_64));
	  mpiPi_msg_debug ("found header name %s\n", SectHeader64.s_name);
	  mpiPi_msg_debug ("found header raw ptr 0x%0llx\n",
			   SectHeader64.s_scnptr);

	  if (SectHeader64.s_flags & STYP_TEXT)
	    text_start = AoutHeader64.o_text_start - SectHeader64.s_scnptr;
	}
    }
  else
    {
      mpiPi_msg_debug ("invalid magic number.\n");
      return 0;
    }

  mpiPi_msg_debug ("text_start is 0x%0llx\n", text_start);
  close (fh);

  return text_start;
}

#endif

/* eof */
