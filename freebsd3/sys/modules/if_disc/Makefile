# $FreeBSD: src/sys/modules/if_disc/Makefile,v 1.6.2.1 1999/08/29 16:27:15 peter Exp $

.PATH:  ${.CURDIR}/../../net
KMOD=   if_disc
SRCS=   if_disc.c bpfilter.h	opt_inet.h	opt_inet6.h
NOMAN=

NBPFILTER?=	0

CFLAGS+= ${PROTOS}
CLEANFILES+=	bpfilter.h	opt_inet.h	opt_inet6.h

bpfilter.h:
	echo "#define NBPFILTER ${NBPFILTER}" > bpfilter.h

opt_inet.h:
	echo "#define INET 1" > opt_inet.h

opt_inet6.h:
	echo "#define INET6 1" > opt_inet6.h

.include <bsd.kmod.mk>
