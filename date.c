
/*
 * Copyright 1986 by Larry Campbell, 73 Concord Street, Maynard MA 01754 USA
 * (maynard!campbell).  You may freely copy, use, and distribute this software
 * subject to the following restrictions:
 *
 *  1)	You may not charge money for it.
 *  2)	You may not remove or alter this copyright notice.
 *  3)	You may not claim you wrote it.
 *  4)	If you make improvements (or other changes), you are requested
 *	to send them to me, so there's a focal point for distributing
 *	improved versions.
 *
 * John Chmielewski (tesla!jlc until 9/1/86, then rogue!jlc) assisted
 * by doing the System V port and adding some nice features.  Thanks!
 */

#include <stdio.h>
#include <time.h>
#ifdef SYSV
#include <sys/types.h>
#endif
/*
 * Copyright 1986 by Larry Campbell, 73 Concord Street, Maynard MA 01754 USA
 * (maynard!campbell).  You may freely copy, use, and distribute this software
 * subject to the following restrictions:
 *
 *  1)	You may not charge money for it.
 *  2)	You may not remove or alter this copyright notice.
 *  3)	You may not claim you wrote it.
 *  4)	If you make improvements (or other changes), you are requested
 *	to send them to me, so there's a focal point for distributing
 *	improved versions.
 *
 * John Chmielewski (tesla!jlc until 9/1/86, then rogue!jlc) assisted
 * by doing the System V port and adding some nice features.  Thanks!
 */

#include <sys/param.h>
#include "x10.h"

extern struct tm *localtime();

extern long lseek();

extern int
 Idays, Ihours, Iminutes;

/* ARGSUSED */

c_date(argc, argv)
char *argv[];
{
    int rf, today;
    struct tm *tp;

    long now;
    if (argc != 2)
	usage(E_2MANY);
    time(&now);
    tp = localtime(&now);
    today = dowX2U(Idays);
    while (tp->tm_wday % 7 != today)
	tp->tm_wday++, tp->tm_mday++;

    (void) printf("%02d%02d%02d%02d%02d\n",
	     tp->tm_mon + 1, tp->tm_mday, Ihours, Iminutes,
	     tp->tm_year & 100);
}
