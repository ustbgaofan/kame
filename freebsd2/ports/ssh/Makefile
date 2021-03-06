# New ports collection makefile for:	ssh
# Version required:	1.2.27
# Date created:		30 Jul 1995
# Whom:			torstenb@FreeBSD.ORG
#
# $Id: Makefile,v 1.2 2001/02/09 06:00:59 itojun Exp $
#
# Maximal ssh package requires YES values for
# USE_PERL, USE_TCPWRAP
#

DISTNAME=	ssh-1.2.27
PKGNAME=	ssh6-1.2.27
CATEGORIES=	security net
MASTER_SITES=   ftp://ftp.funet.fi/pub/unix/security/login/ssh/ \
		ftp://ftp.kyoto.wide.ad.jp/pub/security/ssh/

PATCH_SITES=	ftp://ftp.kyoto.wide.ad.jp/IPv6/ssh/
PATCHFILES=	ssh-1.2.27-IPv6-1.5-patch.gz
PATCH_DIST_STRIP=	-p1

MAINTAINER=	itojun@itojun.org

BROKEN=		"obsolete, security issues"

# You can set USA_RESIDENT appropriately in /etc/make.conf if this bugs you..

.if defined(USA_RESIDENT) && ${USA_RESIDENT} == YES
DISTFILES=	${DISTNAME}.tar.gz rsaref2.tar.gz
MASTER_SITES=	\
	ftp://ftp.funet.fi/pub/unix/security/login/ssh/ \
	ftp://nic.funet.fi/pub/crypt/mirrors/ftp.dsi.unimi.it/applied-crypto/ \
	ftp://rzsun2.informatik.uni-hamburg.de/pub/virus/crypt/ripem/ \
	ftp://idea.sec.dsi.unimi.it/pub/security/crypt/math/ \
	ftp://ftp.univie.ac.at/security/crypt/cryptography/asymmetric/rsa/ \
	ftp://isdec.vc.cvut.cz/pub/security/unimi/crypt/applied-crypto/
.endif

RESTRICTED=	"Crypto; export-controlled"
IS_INTERACTIVE=	YES

GNU_CONFIGURE=	YES

CONFIGURE_ARGS= --prefix=${PREFIX} --with-etcdir=${PREFIX}/etc

#Uncomment if all your users are in their own group and their homedir
#is writeable by that group.  Beware the security implications!
#CONFIGURE_ARGS+= --enable-group-writeability

#Uncomment if you want to allow ssh to emulate an unencrypted rsh connection
#over a secure medium.  This is normally dangerous since it can lead to the
#disclosure keys and passwords.
#CONFIGURE_ARGS+= --with-none

.if defined(USA_RESIDENT) && ${USA_RESIDENT} == YES
CONFIGURE_ARGS+= --with-rsaref
.endif

# Include support for the SecureID card
# Warning: untested !
.if defined(USE_SECUREID) && ${USE_SECUREID} == YES
CONFIGURE_ARGS+= --with-secureid
.endif

# Don't use IDEA. IDEA can be freely used for non-commercial use. However,
# commercial use may require a licence in a number of countries
# Warning: untested !
.if defined(DONT_USE_IDEA) && ${DONT_USE_IDEA} == YES
CONFIGURE_ARGS+= --without-idea
.endif

MAN1=		scp1.1 ssh-add1.1 ssh-agent1.1 ssh-keygen1.1 ssh1.1 \
		make-ssh-known-hosts1.1
MAN8=		sshd1.8

pre-patch:
	@${MV} -f ${WRKSRC}/make-ssh-known-hosts.pl \
	    ${WRKSRC}/make-ssh-known-hosts.pl.in

fetch-depends:
.if !defined(USA_RESIDENT) || ${USA_RESIDENT} != YES && ${USA_RESIDENT} != NO
	@${ECHO}
	@${ECHO} You must set variable USA_RESIDENT to YES if you are a USA
	@${ECHO} resident or NO otherwise.
	@${ECHO} If you are a USA resident you have to get the RSAREF2
	@${ECHO} library \(RSA Inc. holds a patent on RSA and public key
	@${ECHO} cypto in general - using RSA implementations other than
	@${ECHO} RSAREF will violate the US patent law\)
	@${ECHO} and extract it to ${WRKSRC}.
	@${FALSE}
.endif

post-extract:
.if defined(USA_RESIDENT) && ${USA_RESIDENT} == YES
	@${MV} ${WRKDIR}/rsaref2 ${WRKSRC}/rsaref2
.endif

post-install:
	@if [ ! -f ${PREFIX}/etc/ssh_host_key ]; then \
		${ECHO} "Generating a secret host key..."; \
		${PREFIX}/bin/ssh-keygen -f ${PREFIX}/etc/ssh_host_key -N ""; \
	fi
.if !defined(NOMANCOMPRESS)
	for file in make-ssh-known-hosts scp ssh-add ssh-agent \
	    ssh-keygen ssh; do \
		${RM} -f ${PREFIX}/man/man1/$${file}.1; \
		${LN} -sf $${file}1.1.gz ${PREFIX}/man/man1/$${file}.1.gz; \
	done
	${RM} -f ${PREFIX}/man/man1/slogin.1
	${RM} -f ${PREFIX}/man/man8/sshd.8
	${LN} -sf ssh.1.gz ${PREFIX}/man/man1/slogin.1.gz
	${LN} -sf sshd1.8.gz ${PREFIX}/man/man8/sshd.8.gz
.endif
	@if [ ! -f ${PREFIX}/etc/rc.d/sshd.sh ]; then \
		${ECHO} "Installing ${PREFIX}/etc/rc.d/sshd.sh startup file."; \
		${ECHO} "#!/bin/sh" > ${PREFIX}/etc/rc.d/sshd.sh; \
		${ECHO} "[ -x ${PREFIX}/sbin/sshd ] && ${PREFIX}/sbin/sshd && ${ECHO} -n ' sshd/46'" >> ${PREFIX}/etc/rc.d/sshd.sh; \
		chmod 751 ${PREFIX}/etc/rc.d/sshd.sh; \
	fi

.include <bsd.port.mk>

# Following stuff must be after <bsd.port.mk> to expand exists() properly

.if defined(USE_PERL) && ${USE_PERL} == YES || \
    exists(${PREFIX}/bin/perl5.00404) && \
    (!defined(USE_PERL) || ${USE_PERL} != NO)
BUILD_DEPENDS+= perl5.00404:${PORTSDIR}/lang/perl5
CONFIGURE_ENV+= PERL=${PREFIX}/bin/perl5.00404
.else
CONFIGURE_ENV+= PERL=/replace_it_with_PERL_path
.endif

# Include tcp-wrapper support (call remote identd)
.if defined(USE_TCPWRAP) && ${USE_TCPWRAP} == YES || \
    exists(${PREFIX}/lib/libwrap.a) && \
    (!defined(USE_TCPWRAP) || ${USE_TCPWRAP} != NO)
CONFIGURE_ENV+= LDFLAGS=-L${PREFIX}/lib CFLAGS="${CFLAGS} -I${PREFIX}/include"
CONFIGURE_ARGS+= --with-libwrap
LIB_DEPENDS+=   wrap\\.7\\.:${PORTSDIR}/security/tcp_wrapper
.endif

# Include SOCKS firewall support
.if defined(USE_SOCKS) && ${USE_SOCKS} == YES
CONFIGURE_ARGS+= --with-socks="-L${PREFIX}/lib -lsocks5" --with-socks5
.endif
