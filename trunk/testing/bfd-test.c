/* -*- C -*- 

   @PROLOGUE@

   ----- 

   Jeffrey Vetter vetter@llnl.gov
   Center for Applied Scientific Computing, LLNL
   14 Feb 2001

   bfd-test.c -- is the bfd link successful?

 */

#ifndef lint
static char *rcsid =
  "$Header$";
#endif



main (int ac, char **av)
{
  void *pc;
  char *filename;
  char *functname;
  int lineno;

  if (ac != 3)
    {
      printf ("usage: %s <exe file> <hex pc>\n", av[0]);
      exit (1);
    }

  open_bfd_executable (av[1]);
  sscanf (av[2], "%x", &pc);
  find_src_loc (pc, &filename, &lineno, &functname);
  printf ("\npc = %x (%d) -> %s:%d:%s\n", (long) pc, (long) pc, filename,
	  lineno, functname);
  close_bfd_executable ();
}

/* eof */
