# $FreeBSD: src/sys/modules/sio/Makefile,v 1.6 2004/05/26 00:53:08 imp Exp $

.if ${MACHINE} == "pc98"
.PATH: ${.CURDIR}/../../pc98/pc98
.endif
.PATH: ${.CURDIR}/../../dev/sio

KMOD=	sio
SRCS=	bus_if.h card_if.h device_if.h isa_if.h pci_if.h \
	opt_comconsole.h opt_compat.h opt_ddb.h opt_sio.h \
	sio.c sio_pccard.c sio_pci.c sio_puc.c pccarddevs.h
.if ${MACHINE} == "pc98"
SRCS+=	sio_cbus.c
.else
SRCS+=	sio_isa.c
.endif

opt_compat.h:
	echo "#define COMPAT_43 1" > ${.TARGET}

.if ${MACHINE} == "pc98"
opt_sio.h:
	echo "#define COM_MULTIPORT 1" > ${.TARGET}
	echo "#define COM_ESP 1" >> ${.TARGET}
.endif

.include <bsd.kmod.mk>
