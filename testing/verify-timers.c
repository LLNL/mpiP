#include <stdio.h>
#include "u_timers.h"

main ()
{
  printf ("Verifying timers...\n");

  {
    u_TIMER c;
    u_TIME t1, t2;
    u_TIMER_INIT (&c);

    u_GETTIME (&c, &t1);
    sleep (2);
    u_GETTIME (&c, &t2);

    printf ("u_GETUSECS -> t1 = %lld, t2 = %lld\n",
	    u_GETUSECS (&c, &t1), u_GETUSECS (&c, &t2));

    printf ("u_GETTIMEDIFF -> t2 - t1 = %lld\n",
	    u_GETTIMEDIFF (&c, &t2, &t1));

    u_PRINTTIMEDIFF (&c, &t2, &t1);
  }

  printf ("Verification complete.\n");
}
