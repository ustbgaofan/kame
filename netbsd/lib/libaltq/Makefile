LIB=	altq
SRCS=	parser.c qop.c qop_blue.c qop_cbq.c qop_cdnr.c \
	qop_conf.c qop_dummy.c qop_errlist.c qop_fifoq.c qop_hfsc.c \
	qop_jobs.c qop_priq.c qop_red.c qop_rio.c qop_wfq.c quip_server.c
CPPFLAGS+=-DALTQ -DINET6

.PATH:	${.CURDIR}/../../kame/libaltq

MKPIC=no

.include <bsd.lib.mk>
