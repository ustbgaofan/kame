# $FreeBSD: src/sys/modules/3dfx/Makefile,v 1.11 2001/03/27 19:04:09 cokane Exp $

MAINTAINER=	cokane@FreeBSD.org

.PATH: $(.CURDIR)/../../dev/tdfx

KMOD=	3dfx
SRCS=	opt_tdfx.h  bus_if.h pci_if.h device_if.h tdfx_pci.h tdfx_io.h\
	tdfx_vars.h tdfx_pci.c
INCSRC=	../../sys

.if ${MACHINE_ARCH} == "i386"
# This line enables linux ioctl handling by default
# comment out if you don't want it
TDFX_OPTS=	"\#define TDFX_LINUX"
.endif

# Uncomment this for debugging messages
#CFLAGS+=	-DDEBUG

opt_tdfx.h:
	echo $(TDFX_OPTS) > opt_tdfx.h

.include <bsd.kmod.mk>
