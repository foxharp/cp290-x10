
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
#include <ctype.h>
#include "x10.h"

extern int tty;

/* ARGSUSED */

c_reset(argc, argv)
char *argv[];
{
    int hcode, hletter, n;
    char buf[20];
    extern int x10_housecode;

    if (argc > 3)
	usage(E_WNA);

    buf[0] = SETHCODE;
    buf[1] = x10_housecode;		/* default house code */

    if (argc == 3) {
	hletter = argv[2][0];
	buf[1] = hcode = char2hc(hletter);
    }
    sendsync();
    (void) write(tty, buf, 2);
    chkack();
}
