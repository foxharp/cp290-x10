
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
extern char flag;

c_schedule(argc, argv)
char *argv[];
{
    register n;
    int bits, daybits = 0, hcode, dim, mode;
    int eventno, hh, mm, shh, smm;
    unsigned char buf[12];
    char *unitnums;
    char *hhmmp;
    int plus, needhhmm, gotsun;

    if (argc < 6 || argc > 9)
	usage(EM_WNA);

    /* parse the housecode */
    parse_unit(argv[2],&hcode,&unitnums);

    /* parse the unit numbers */
    bits = getunits(unitnums);

    /* parse the mode */
    n = 3;		/* used because argv[4] to argv[8] can vary by one */
    mode = mode2code(argv[n++]);

    /* parse the day if mode requires it */
    if (flag <= 1)		/* first two modes require days */
	daybits = day2bits(argv[n++]);

    /* parse the time */
    hhmmp = argv[n];
    shh = smm = hh = mm = 0;
    plus = 1;
    needhhmm = 1;
    gotsun = 0;
    if (strncasecmp(argv[n],"sunset",6) == 0 ||
	strncasecmp(argv[n],"sunrise",7) == 0) {
	gotsun = 1;
	if (argv[n][3] == 's') {
		sunset(&smm);
		hhmmp = &argv[n][6];
	} else {
		sunrise(&smm);
		hhmmp = &argv[n][7];
	}
	if (*hhmmp == '+' || *hhmmp == '-') {
	    plus = (*hhmmp == '+');
	    hhmmp++;
	}
	if (*hhmmp == '\0')
		needhhmm = 0;
    }
    if (needhhmm && sscanf(hhmmp, "%d:%d", &hh, &mm) != 2)
	error("bad time format");

    if (hh > 23)
	error("bad hours, must be between 0 and 23");
    if (mm > 59)
	error("bad minutes, must be between 0 and 59");

    if (gotsun) {
	    mm += (hh*60);

	    if (plus) {
		smm += mm;
	    } else {
		smm -= mm;
	    }
	    hh  = smm / 60;
	    mm  = smm % 60;
	    if (hh < 0 || hh > 23)
		error("bad sun offset, must be between 0 and 23");

    }

    n++;

    /* parse the state */
	dim = dimstate(argv[n], argc > n + 1 ? argv[n + 1] : "");

    /* no event num on command line? */
    if (argc <= n + 1 || sscanf(argv[argc - 1], "%d", &eventno) != 1)
	    eventno = getslot(GETEVENTS);

    if (eventno < 0 || eventno > 127)
    	error ("event number must be between 0 and 127");
    	

    buf[0] = DATALOAD;
    buf[1] = eventno << 3;
    buf[2] = (eventno >> 5) & 0x3;
    buf[3] = mode;
    buf[4] = daybits;
    buf[5] = hh;
    buf[6] = mm;
    buf[7] = bits >> 8;
    buf[8] = bits & 0xFF;
    buf[9] = hcode;
    buf[10] = dim;
    buf[11] = 0;

    for (n = 3; n <= 10; n++)	/* compute checksum */
	buf[11] += buf[n];

    sendsync();
    (void) write(tty, (char *) buf, 12);
    chkack();

    flag = 0;			/* header wanted */
    pevent(&buf[3], eventno);	/* reassure user */
}
