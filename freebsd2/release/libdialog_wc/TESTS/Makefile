# Really quick and evil Makefile for building all the tests.  I wish that bmake was friendlier to
# the concept of multiple progs/libs in the same directory.
# $Id: Makefile,v 1.1 1999/09/13 08:50:19 itojun Exp $

PROGS=	msg yesno prgbox gauge dselect fselect text menu1 menu2 menu3 \
	input1 input2 check1 check2 check3 radio1 radio2 radio3

CFLAGS+= -Wall -Wstrict-prototypes
LDFLAGS += -ldialog -lncurses -lmytinfo

all: ${PROGS}

clean:
	rm -f ${PROGS}
	
.include <bsd.prog.mk>
