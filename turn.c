
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

extern struct hstruct
 housetab[];

c_turn(argc, argv)
char *argv[];
{
    register n;
    int hcode, dim, bits;
    unsigned char buf[6];
    char *unitnums;

    if (argc < 4 || argc > 5)
	usage(E_WNA);

    parse_unit(argv[2],&hcode,&unitnums);

    bits = getunits(unitnums);

    dim = dimstate(argv[3], (argc == 5) ? argv[4] : "");

    buf[0] = DIRCMD;
    buf[1] = dim;
    buf[2] = hcode;
    buf[3] = bits & 0xFF;;
    buf[4] = bits >> 8;
    buf[5] = CHKSUM(buf);

    sendsync();
    (void) write(tty, (char *) buf, 6);
    chkack();
    chkrpt(0);
}
