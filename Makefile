
HOME = /usr2/foxharp
BIN = $(HOME)/bin
GROUP = sys
OWNER = bin

REMOTE=gutso!foxharp

#	set DFLAGS equal to:
#	   -DVENIX	if using VENIX
#	   -DSYSV	if using SYSTEM V
#	   -DVOID	if compiler doesn't understand 'void'
#	   -DMINIEXCH	if using the DEC mini-exchange
#	   -DXDIR=\"fullpath_name/x10\" if not using default of "."
DFLAGS = -DSYSV -DPOSIX -DXDIR=\"$(HOME)/X10\" -DPGF

CFLAGS = -g $(DFLAGS)
#LDFLAGS = -z -i
#LIBS = -lc_s		# uncomment if using shared libraries

SRCS =	data.c date.c delete.c diagnstc.c dump.c fdump.c \
	finfo.c fload.c info.c getslot.c message.c miniexch.c \
	monitor.c prints.c readid.c reset.c schedule.c setclock.c \
	tty.c turn.c x10.c xread.c

OBJS =	data.o date.o delete.o diagnstc.o dump.o fdump.o \
	finfo.o fload.o info.o getslot.o message.o miniexch.o \
	monitor.o prints.o readid.o reset.o schedule.o setclock.o \
	tty.o turn.o x10.o xread.o

OTHERSRC = README REVIEW Makefile x10config sched x10.1 x10.h

EVERYTHING = $(OTHERSRC) $(SRCS)

x10:	$(OBJS)
	cc $(LDFLAGS) -o x10 $(OBJS) $(LIBS)
#	chgrp $(GROUP) x10
#	chmod 2755 x10
#	chown $(OWNER) x10

$(OBJS): x10.h

install: x10
	mv x10 $(BIN)

lint:
	lint $(DFLAGS) $(SRCS)

shar:	x10.shar.1 x10.shar.2

x10.shar.1:
	shar $(OTHERSRC) >x10.shar.1

x10.shar.2:
	shar $(SRCS) > x10.shar.2

bigshar:
	shar $(EVERYTHING) > x10.shar

touch:
	touch $(OTHERSRC)
	touch $(SRCS)

clean:
	rm -f *.o

clobber: clean
	rm -f x10

uurw:
	uuto `make rw` $(REMOTE)

rcsdiffrw:
	@-for x in `$(MAKE) rw`	;\
	do	\
		echo 		;\
		echo $$x	;\
		echo =========	;\
		rcsdiff $$x	;\
	done 2>&1		;\
	echo			;\
	echo all done

list:
	@ls $(EVERYTHING) | more

rw:
	@ls -l $(EVERYTHING) | \
		egrep '^[^l].w' | \
		sed 's;.* ;;'   # strip to last space

update:
	nupdatefile.pl -r $(EVERYTHING)

populate: $(EVERYTHING)

$(EVERYTHING):
	co -r$(revision) $@

