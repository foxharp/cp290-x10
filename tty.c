
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
#include <string.h>
#include "x10.h"

#ifdef ALLOW_SOCKET
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#define	RS232PORT	87	/* the NCD RS-232 daemon TCP port */
#endif

void exit();

char x10_tty[50];

int x10_tty_is_host = 0;

int tty = -1;
#ifndef SYSV
#include <sgtty.h>
struct sgttyb oldsb, newsb;
#else
#ifndef POSIX
#include <termio.h>
struct termio oldsb, newsb;
#else
#include <termios.h>
struct termios oldsb, newsb;
#endif

#endif

setup_tty()
{
    if (!x10_tty[0])
    	error("no TTY specified in configfile");

#ifdef ALLOW_SOCKET
    if (x10_tty[0] != '/') {	/* then it's a hostname */
	struct sockaddr_in inaddr;
	struct hostent *hp;
	unsigned int port = RS232PORT;
	char *colon;
	int r, i;

	x10_tty_is_host = 1;

	/* hostname may include a port number, e.g. "annex:7002" */
	colon = strchr(x10_tty, ':');
	if (colon && colon[1]) {
	    port = atoi(&colon[1]);
	    *colon = '\0';
	}

	/* resolve network address of host */
	inaddr.sin_family = AF_INET;
	inaddr.sin_port = htons (port);
	if ((inaddr.sin_addr.s_addr = inet_addr (x10_tty)) == -1) {
	    if ((hp = gethostbyname (x10_tty)) == NULL
		    || hp->h_addrtype != AF_INET) {
		fprintf (stderr, "x10: can't resolve name %s\n", x10_tty);
		exit (1);
	    }
	    bcopy (hp->h_addr, &inaddr.sin_addr.s_addr, hp->h_length);
	}

	/* the xylogics microannex won't accept multiple connections
	 * quickly enough.  so retry.
	 */
#define CONNECT_RETRIES 10
	for (i = 0; i < CONNECT_RETRIES; i++) {
	    /* create a socket */
	    if ((tty = socket (AF_INET, SOCK_STREAM, 0)) < 0)
		perror ("can't open socket");

	    /* connect to serial port daemon */
	    r = connect (tty, (struct sockaddr *)&inaddr, sizeof (inaddr));
	    if (r == 0)
		return;

	    close(tty);

	    /* could use usleep(), but i _think_ this is more portable.
	     * but just use "sleep(1);" otherwise
	     */
	    {
		fd_set readbits;
		struct timeval tim;
		select(1,&readbits,0,0,&tim);
		FD_ZERO(&readbits);
		tim.tv_usec = 500000;
		tim.tv_sec = 0;
	    }
	    
	}
	perror ("x10");
	close(tty);
	exit(1);
    }
#endif

    tty = open(x10_tty, 2);
    if (tty < 0) {
	perror(x10_tty);
	exit(1);
    } 

#ifndef SYSV
    (void) ioctl(tty, TIOCFLUSH, (struct sgttyb *) NULL);
    (void) ioctl(tty, TIOCGETP, &oldsb);
    ewsb = oldsb;
    newsb.sg_flags |= RAW;
    newsb.sg_flags &= ~(ECHO | EVENP | ODDP);
    hangup();
    newsb.sg_ispeed = newsb.sg_ospeed = B600;	/* raise DTR & set speed */
    (void) ioctl(tty, TIOCSETN, &newsb);
#else
#ifndef POSIX
    if (ioctl(tty, TCGETA, &oldsb) < 0) {
    	perror("ioctl get");
	close(tty);
	exit(1);
    }
    newsb = oldsb;
    newsb.c_lflag = 0;
    newsb.c_oflag = 0;
    newsb.c_iflag = IGNBRK | IGNPAR;
    newsb.c_cflag = (CLOCAL | B600 | CS8 | CREAD);
    newsb.c_cc[VMIN] = 1;
    newsb.c_cc[VTIME] = 0;
    newsb.c_cc[VINTR] = 0;
    newsb.c_cc[VQUIT] = 0;
#ifdef VSWTCH
    newsb.c_cc[VSWTCH] = 0;
#endif
    newsb.c_cc[VTIME] = 0;
    if (ioctl(tty, TCSETAF, &newsb) < 0) {
    	perror("ioctl set");
	close(tty);
	exit(1);
    }
#else
    {
	int s;
	s = tcgetattr(tty, &oldsb);
	if (s < 0) {
		perror("ttopen tcgetattr");
		close(tty);
		exit(1);
	}

	newsb = oldsb;

#ifndef VDISABLE
# ifdef _POSIX_VDISABLE
#  define VDISABLE _POSIX_VDISABLE
# else
#  define VDISABLE '\0'
# endif
#endif
	/* printf("vdisable is %d\n", VDISABLE); */
	newsb.c_oflag = 0;	/* no output flags at all */

	newsb.c_lflag = 0;	/* no line flags at all */

	newsb.c_cflag &= ~PARENB;	/* disable parity, both in and out */
	newsb.c_cflag |= CSTOPB|CLOCAL|CS8|CREAD;   /* two stop bits on transmit */
						/* no modem control, 8bit chars, */
						/* receiver enabled, */

	newsb.c_iflag = IGNBRK | IGNPAR;    /* ignore break, ignore parity errors */

	newsb.c_cc[VMIN] = 1;
	newsb.c_cc[VTIME] = 0;
#ifdef  VSWTCH
	newsb.c_cc[VSWTCH] = VDISABLE;
#endif
	newsb.c_cc[VSUSP]  = VDISABLE;
#if defined (VDSUSP) && defined(NCCS) && VDSUSP < NCCS
	newsb.c_cc[VDSUSP]  = VDISABLE;
#endif
	newsb.c_cc[VSTART] = VDISABLE;
	newsb.c_cc[VSTOP]  = VDISABLE;

	s = cfsetospeed (&newsb, B600);
	if (s < 0) {
		perror("ttopen cfsetospeed");
		close(tty);
		exit(1);
	}
	s = cfsetispeed (&newsb, B600);
	if (s < 0) {
		perror("ttopen cfsetispeed");
		close(tty);
		exit(1);
	}
	s = tcsetattr(tty, TCSAFLUSH, &newsb);
	if (s < 0) {
		perror("ttopen tcsetattr");
		close(tty);
		exit(1);
	}
    }
#endif
#endif
}

restore_tty()
{
    if (x10_tty_is_host)
	return;

#ifndef SYSV
    hangup();
    (void) ioctl(tty, TIOCSETN, &oldsb);
#else
#ifndef POSIX
    (void) ioctl(tty, TCSETAF, &oldsb);
#else
    tcsetattr(tty, TCSADRAIN, &oldsb);
#endif
#endif
}

#ifndef SYSV
hangup()
{
    if (x10_tty_is_host) {
	close(tty);
    }
    newsb.sg_ispeed = newsb.sg_ospeed = B0;	/* drop DTR */
    (void) ioctl(tty, TIOCSETN, &newsb);
    sleep(SMALLPAUSE);
}

#endif

close_tty()
{
    close(tty);
}

quit()
{
    if (tty == -1 || x10_tty_is_host)
	exit(1);

    restore_tty();
    close_tty();
    exit(1);
}
