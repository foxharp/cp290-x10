
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
#ifdef SYSV
#include <string.h>
#else
#include <strings.h>
#endif
#include <time.h>
#include "x10.h"

extern long time();
extern struct tm *localtime();
extern struct nstruct modnames[];
extern int tty;

void sigtimer();
char hc2char();

char
 syncmsg[SYNCN], flag;

struct hstruct			/* table to map housecodes into letters */
 housetab[] =
{
    {HC_A, 'a'},
    {HC_B, 'b'},
    {HC_C, 'c'},
    {HC_D, 'd'},
    {HC_E, 'e'},
    {HC_F, 'f'},
    {HC_G, 'g'},
    {HC_H, 'h'},
    {HC_I, 'i'},
    {HC_J, 'j'},
    {HC_K, 'k'},
    {HC_L, 'l'},
    {HC_M, 'm'},
    {HC_N, 'n'},
    {HC_O, 'o'},
    {HC_P, 'p'}
};

char *wdays[] =
{"Sunday", "Monday", "Tuesday", "Wednesday",
 "Thursday", "Friday", "Saturday", "<day not set>"};

unsigned char	/* table to map unit numbers into unit bit mask */
 maphibyt[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
}, maplobyt[] =
{
    0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

int
 timeout = TIMEOUT, Iloaded, Iminutes, Ihours, Idays;

unsigned char
 Ihcode;

extern int
 c_data(), c_date(), c_delete(), c_diagnostic(), c_dump(), c_fdump(),
 c_finfo(), c_fload(), c_info(), c_monitor(), c_reset(), c_schedule(),
 c_setclock(), c_turn();

struct cmdentry {
    char *cmd_name;
    int (*cmd_routine) ();
} cmdtab[] = {
    "data", c_data,
    "date", c_date,
    "delete", c_delete,
    "diagnostic", c_diagnostic,
    "dump", c_dump,
    "fdump", c_fdump,
    "finfo", c_finfo,
    "fload", c_fload,
    "info", c_info,
    "monitor", c_monitor,
    "reset", c_reset,
    "schedule", c_schedule,
    "setclock", c_setclock,
    "turn", c_turn,
    "", NULL
};

main(argc, argv)
char *argv[];
{
    register i;
    int (*rtn) ();
    struct cmdentry *c;

    if (argc < 2)
	usage(E_NOCMD);

    rtn = NULL;
    for (c = cmdtab; c->cmd_routine != NULL; c++) {
	if (strcmp(argv[1], c->cmd_name) == 0) {
	    rtn = c->cmd_routine;
	    break;
	}
    }
    if (rtn == NULL)
	usage(E_INVCN);
    setup_tty();

#ifdef MINIEXCH
    mxconnect(MINIXPORT);
#endif

    for (i = 0; i < SYNCN; i++)
	syncmsg[i] = i < 11 ? 0xEF : 0xFF;
    init();
    (*rtn) (argc, argv);
    restore_tty();
    return 0;
}

/*
 * Convert X10-style day of week (bit map, bit 0=monday, 6=sunday)
 * to UNIX localtime(3) style day of week (integer, 0=sunday)
 */

dowX2U(b)
register char b;
{
    register n;

    for (n = 1; (!(b & 1)) && n < 8; n++, b = b >> 1);
    if (n == 7)
	n = 0;
    if (n == 8)
	n = 7;
    return (n);
}

dowU2X(d)
register d;
{
    if (d == 0)
	d = 7;
    return (1 << (d - 1));
}

init()
{
    int n;
    unsigned char buf[6];

    sendsync();
    (void) write(tty, "\004", 1);	/* GETINFO command */
    getsync();
    n = xread(tty, buf, 6, timeout);

    if (n != 6)
	error("invalid Clock and Base Housecode message length");
    if (CHKSUM(buf) != buf[5])
	error("checksum error");

    Iloaded = buf[0] & 1;
    Iminutes = buf[1];
    Ihours = buf[2];
    Idays = buf[3];
    Ihcode = buf[4];
}

chkack()
{
    unsigned char buf[7];
    int n;

    n = xread(tty, buf, 7, timeout);
    if (n != 7) {
	int i;

	(void) fprintf(stderr, "chkack dump (%d bytes):\n", n);
	for (i = 0; i < n; i++)
	    (void) fprintf(stderr, "buf[%d] = 0x%x\n", i, buf[i]);
	error("timeout while awaiting ACK message");
    }
}

/*
 * Check command report ("Command Upload", the manual calls it).
 * If argument supplied is non-zero, print the report in human-readable
 * form.
 */

chkrpt(printflag)
{
    static char *statetab[] =
    {"?", "?", "ON", "OFF", "DIM", "DIM", "?", "?"};
    int n;
    unsigned char buf[6];
    long dtime;
    struct tm *tp;

    getsync();
    n = xread(tty, buf, 6, timeout);

    if (n != 6)
	error("chkrpt: invalid event report length");
    if (CHKSUM(buf) != buf[5]) {
	(void) fprintf(stderr,
	"Checksum computed: 0x%x, received: 0x%x\n", CHKSUM(buf), buf[5]);
	error("chkrpt: checksum error");
    }
    if (!printflag)
	return;

    dtime = time((long *) 0);
    tp = localtime(&dtime);

    (void) printf("%2d:%02d:%02d: unit ",
	    tp->tm_hour, tp->tm_min, tp->tm_sec);
    punits(hc2char(buf[1] & 0xF0), (buf[3] << 8) | buf[2]);
    (void) printf(", state %s\n", statetab[buf[1] & 0x07]);
}

getsync()
{
    unsigned char buf[RCVSYNC];

    if (xread(tty, buf, RCVSYNC, timeout) < RCVSYNC)
	error("Failed to get sync characters");
}

sendsync()
{
    (void) write(tty, syncmsg, SYNCN);
}

chksum(buf, size)
unsigned char *buf;
{
    register i, sum;

    for (i = 1, sum = 0; i < (size - 1); i++)
	sum += buf[i];
    return (sum & 0xFF);
}

char hc2char(code)
unsigned code;
{
    register i;

    for (i = 0; i < 16; i++)
	if (housetab[i].h_code == code)
	    return (housetab[i].h_letter);
    return ('?');
}

/*
 * Parse string of comma-separated unit numbers and return bitmap
 * (big-endian) of units specified.  '*' means "all units".
 */

getunits(p)
register char *p;
{

#define DIGBUFN 80

    unsigned lobits, hibits, n, unit, lastunit = 0;
    char digbuf[DIGBUFN];

    lobits = 0;
    hibits = 0;
    while (*p) {
	if (*p == '*') {
	    lobits = 0xFF;
	    hibits = 0xFF;
	    break;
	}
	for (n = 0; n < DIGBUFN && isdigit(*p); n++, p++)
	    digbuf[n] = *p;
	digbuf[n] = '\0';
	if ((unit = atoi(digbuf)) < 1 || unit > 16)
	    error("bad unit code, must be between 1 and 16");
	if (lastunit == 0)
		lastunit = unit;
	else if (lastunit > unit) {
		n = unit; unit = lastunit; lastunit = n;
	}

	for (n = lastunit; n <= unit; n++) {
		lobits |= maplobyt[n - 1];
		hibits |= maphibyt[n - 1];
	}
	if (*p)
	    if (*p == '-') {
		lastunit = unit;
		p++;
	    } else if (*p == ',') {
		lastunit = 0;
		p++;
	    } else {
		error("bad unit separator, use comma or dash please");
	    }
    }
    return ((lobits << 8) | hibits);
}

dimstate(p, level)
register char *p, *level;
{
    unsigned levelnum;

    if (strcmp(p, "on") == 0)
	return (2);
    if (strcmp(p, "off") == 0)
	return (3);
    if (strcmp(p, "dim") != 0)
	error("bad state keyword");
    if (sscanf(level, "%d", &levelnum) == 0)
	error("dim value must be numeric");
    if (levelnum > 15)
	error("dim value out of range, must be between 0 and 15");
    timeout = DTIMEOUT;
    return ((levelnum << 4) | 5);
}

/* names must have first letter capitalized for day2bits() */
struct nstruct
 dtab[] =
{
    "Monday", 0x01,
    "Tuesday", 0x02,
    "Wednesday", 0x04,
    "Thursday", 0x08,
    "Friday", 0x10,
    "Saturday", 0x20,
    "Sunday", 0x40,
    "Everyday", 0x7f,
    "Weekdays", 0x1f,
    "Weekend", 0x60,
    "", 0x00
};

day2bits(p)
char *p;
{
    char c, buf[6];
    int n, mask, length;

    n = 0;
    while (n < 5)
	if (c = *p++) {
	    if (n) {
		if (isupper(c))
		    c = tolower(c);
	    } else if (islower(c))
		c = toupper(c);
	    buf[n++] = c;
	} else
	    break;
    buf[n] = '\0';
    length = strlen(buf);
    mask = 0;
    for (n = 0; dtab[n].n_code != 0; n++) {
	if (strncmp(dtab[n].n_name, buf, length) == 0) {
	    if (mask != 0)
		error("ambiguous day abbreviation");
	    mask = dtab[n].n_code;
	}
    }
    if (mask == 0)
	error("bad day keyword");
    return (mask);
}

mode2code(p)
char *p;
{
    char *np, *sp;
    int n, mode, pos;

    sp = p;
    for (mode = n = 0; *modnames[n].n_name != 0; n++) {
	p = sp;
	np = modnames[n].n_name;/* names have first letter capitalized */
	if ((isupper(*p) ? *p : toupper(*p)) != *np)
	    continue;
	for (p++, np++; *p; p++, np++)
	    if ((isupper(*p) ? tolower(*p) : *p) != *np)
		break;
	if (*p == 0) {
	    if (mode)
		error("ambiguous mode abbreviation");
	    mode = modnames[n].n_code;
	    pos = n;
	}
    }
    if (mode == 0)
	error("bad mode keyword");
    flag = pos;			/* position of function name in table */
    return (mode);
}

#define MODULES 25
#define NAME_LEN 20
#define UNIT_LEN 50

struct x10_mod {
	char name[NAME_LEN+1];
	char hc;
	char un[UNIT_LEN];
} x10_modules[MODULES];


read_config()
{

	char line[256];
	char alias[50];
	char housecode[50];
	char unit[UNIT_LEN];
	char *configfile;
	static FILE *f = NULL;
	int i = 0;
	int n;
	extern char *getenv();

	if (f) return;

	configfile = getenv("X10CONFIG");
	if (!configfile) {
		(void) strcat(strcpy(line, XDIR), ALIASFILE);
		configfile = line;
	}

	f = fopen(configfile,"r");
	if (!f) {
		err("couldn't open configfile %s: X10CONFIG set?",
			configfile);
		exit(1);
	}

	for (;;) {

		line [0] = '\0';
		fgets(line, 255, f);

		if (line[0] == '\0')
			break;

		if (i >= MODULES) {
			err("out of table space for config", NULL);
			break;
		}

		n = sscanf(line, "%s %s %s", alias, housecode, unit);

		if (!n || alias[0] == '#')
			continue;

		switch(n) {
		case 3:
		case 2:
			if (strcmp(alias, "TTY") == 0) {
				extern char x10_tty[];
				/* special case */
				strcpy(x10_tty, housecode);
#ifdef BEFORE
				if (n == 3)
					x10_baud = str_to_baud(unit);
				else
					x10_baud = B600;

				if (x10_baud == 0)
					x10_baud = B600;
#endif
				break;
			}
			housecode[1] = '\0';
			if (isupper(housecode[0]))
				housecode[0] = tolower(housecode[0]);
			if (housecode[0] < 'a' || housecode[0] > 'p') {
				err("bad housecode in config, alias %s",
					alias);
				continue;
			}
#if BEFORE
			if (strcmp(alias, "HOUSECODE") == 0) {
				/* another special case */
				x10_housecode = housecode[0];
				continue;
			}
#endif
			x10_modules[i].hc = housecode[0];
			strncpy(x10_modules[i].un,unit,UNIT_LEN);
#if store_numeric
			if ((x10_modules[i].un = atoi(unit) - 1) < 0) {
				err("bad unit no. in config, alias %s",
					alias);
				continue;
			}
#endif
			strncpy(x10_modules[i].name, alias, NAME_LEN);

			++i;
			break;

		case 1:
			err("warning: short field in line in config:\n	%s",line);
			break;
		default:
			err("warning: too many fields in line in config:\n	%s",line);
			break;
		}

	}

	fclose (f);

}



struct x10_mod *
xmod_lookup(name)
char *name;
{
	int i = 0;
	struct x10_mod *xm;

	read_config();
	xm = x10_modules;
	while (i < MODULES) {
		if (!xm->name[0])
			break;
		if (strcmp(name, xm->name) == 0)
			return xm;
		i++;
		xm++;
	}
	if (!i) {
		err("no config at all", NULL);
	}
	return NULL;
}

char *
xmod_name(hl,unit)
{
	int i = 0;
	struct x10_mod *xm;

	read_config();
	xm = x10_modules;
	if (isupper(hl))
		hl = tolower(hl);
	while (i < MODULES) {
		if (!xm->name[0])
			break;
		if (hl == xm->hc && strcmp(unit,xm->un) == 0)
			return xm->name;
		i++;
		xm++;
	}
	return NULL;
}


parse_unit(name,hcp,unitp)
char *name;
int *hcp;
char **unitp;
{
    struct x10_mod *x;
    int hletter;
    static unitnum[5];

    if (isalpha(name[0]) && (isdigit(name[1]) || name[1] == '*')) {
	hletter = name[0];
	if (isupper(hletter))
	    hletter = tolower(hletter);
	*unitp = &name[1];
    } else {
	x = xmod_lookup(name);
	if (!x) {
		err("bad alias %s",name);
		quit();
	}
	hletter = x->hc;
	*unitp = x->un;
    }
    if ('a' <= hletter && hletter <= 'p')
        *hcp = housetab[hletter - 'a'].h_code;
    else
	error("invalid house code");

#ifdef BEFORE
    for (n = 0, hcode = -1; n < 16; n++) {
	if (housetab[n].h_letter == hletter) {
	    *hcp = housetab[n].h_code;
	    break;
	}
    }
    if (*hcp == -1)
	error("invalid house code");
#endif
}


err(m,s)
char *m, *s;
{
	fprintf(stderr, "x10: ");
	fprintf(stderr, m, s);
	fprintf(stderr, "\n");
}

dump_config()
{
	int i = 0;
	struct x10_mod *xm = x10_modules;
	while (i < MODULES) {
		if (!xm->name[0])
			break;
		printf("%d: name: %s, housecode is %c, unit %s\n",
			i, xm->name, xm->hc, xm->un);
		i++;
		xm++;
	}
	if (!i)
		err("no config at all", NULL);

	if (x10_tty[0])
		printf("tty is %s\n", x10_tty);
#if BEFORE
	if (x10_baud)
		printf("baud is %s\n", baud_to_str(x10_baud));
	printf("default housecode is %c\n", x10_housecode + 'A');
#endif
}

