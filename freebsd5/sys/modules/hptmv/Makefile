#
# Makefile for RR182x FreeBSD driver
# Copyright (c)  2003-2004 HighPoint Technologies, Inc. All rights reserved
#
# $FreeBSD: src/sys/modules/hptmv/Makefile,v 1.2 2004/10/24 08:38:29 scottl Exp $

HPTMV= ${.CURDIR}/../../contrib/dev/hptmv
.PATH: ${.CURDIR}/../../dev/hptmv ${HPTMV}

KMOD= hptmv
SRCS= opt_scsi.h opt_cam.h
SRCS+= bus_if.h device_if.h pci_if.h
SRCS+= mv.c entry.c
SRCS+= ioctl.c hptproc.c gui_lib.c
OBJS+= hptmvraid.o

hptmvraid.o:	${HPTMV}/i386-elf.raid.o.uu
	uudecode -p < ${HPTMV}/i386-elf.raid.o.uu > ${.TARGET}

#
# Debug Options:
#
#   -DDEBUG: include debug code
#   -DDEBUG_LEVEL=x:
#		0 - No debug output message
#		1 - print only error messages
#		2 - print error and warning
#		3 - print all messages (info, warning, error)
#
#DEBUGOPT = -O2

.if defined(DEBUG) && $(DEBUG) == 1
DEBUGOPT += -DDEBUG
.if defined(DEBUG_LEVEL)
DEBUGOPT += -DDEBUG_LEVEL=$(DEBUG_LEVEL)
.else
DEBUGOPT += -DDEBUG_LEVEL=1
.endif
.endif

.if defined(FOR_DEMO) && $(FOR_DEMO) == 1
DEBUGOPT += -DFOR_DEMO
.endif

CFLAGS+= ${DEBUGOPT}

.include <bsd.kmod.mk>
