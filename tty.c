
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
#ifndef SYSV
#include <sgtty.h>
#else
#include <termio.h>
#endif
#include "x10.h"

void exit();

char x10_tty[50];

int tty = -1;
#ifndef SYSV
struct sgttyb
#else
struct termio
#endif
 oldsb, newsb;

setup_tty()
{
    read_config();
    if (!x10_tty[0])
    	error("no TTY specified in configfile");
    tty = open(x10_tty, 2);
    if (tty < 0)
	error("can't open tty line");

#ifndef SYSV
    (void) ioctl(tty, TIOCFLUSH, (struct sgttyb *) NULL);
    (void) ioctl(tty, TIOCGETP, &oldsb);
    newsb = oldsb;
    newsb.sg_flags |= RAW;
    newsb.sg_flags &= ~(ECHO | EVENP | ODDP);
    hangup();
    newsb.sg_ispeed = newsb.sg_ospeed = B600;	/* raise DTR & set speed */
    (void) ioctl(tty, TIOCSETN, &newsb);
#else
    (void) ioctl(tty, TCGETA, &oldsb);
    newsb = oldsb;
    newsb.c_lflag = 0;
    newsb.c_oflag = 0;
    newsb.c_iflag = IGNBRK | IGNPAR;
    newsb.c_cflag = (CLOCAL | B600 | CS8 | CREAD);
    newsb.c_cc[VMIN] = 1;
    newsb.c_cc[VTIME] = 0;
    newsb.c_cc[VINTR] = 0;
    newsb.c_cc[VQUIT] = 0;
    newsb.c_cc[VSWTCH] = 0;
    newsb.c_cc[VTIME] = 0;
    (void) ioctl(tty, TCSETAF, &newsb);
#endif
}

restore_tty()
{
#ifndef SYSV
    hangup();
    (void) ioctl(tty, TIOCSETN, &oldsb);
#else
    (void) ioctl(tty, TCSETAF, &oldsb);
#endif
}

#ifndef SYSV
hangup()
{
    newsb.sg_ispeed = newsb.sg_ospeed = B0;	/* drop DTR */
    (void) ioctl(tty, TIOCSETN, &newsb);
    sleep(SMALLPAUSE);
}

#endif

quit()
{
    if (tty == -1)
	exit(1);
    restore_tty();
    exit(1);
}
