#	$Id: Makefile,v 1.11 1998/11/12 00:37:39 jkh Exp $

.PATH:	${.CURDIR}/../../i386/isa
KMOD	= joy
SRCS	= joy.c joy.h opt_devfs.h
MAN8	= joy.8

CFLAGS		+= -DJOY_MODULE
CLEANFILES	+= joy.h opt_devfs.h

joy.h:
	echo "#define NJOY 1" > joy.h

opt_devfs.h:
	touch opt_devfs.h


afterinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/joy.sh ${DESTDIR}/usr/bin/joy

.include <bsd.kmod.mk>
