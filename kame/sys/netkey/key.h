/*	$KAME: key.h,v 1.32 2003/09/07 05:25:20 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _NETKEY_KEY_H_
#define _NETKEY_KEY_H_

#ifdef _KERNEL

#include <sys/queue.h>

extern struct key_cb key_cb;

extern TAILQ_HEAD(_satailq, secasvar) satailq;
extern TAILQ_HEAD(_sptailq, secpolicy) sptailq;

struct secpolicy;
struct secpolicyindex;
struct ipsecrequest;
struct secasvar;
struct sockaddr;
struct socket;
struct sadb_msg;
struct sadb_x_policy;

extern struct secpolicy *key_allocsp __P((u_int16_t, struct secpolicyindex *,
	u_int));
extern struct secpolicy *key_gettunnel __P((struct sockaddr *,
	struct sockaddr *, struct sockaddr *, struct sockaddr *));
extern int key_checkrequest
	__P((struct ipsecrequest *isr, struct secasindex *));
extern struct secasvar *key_allocsa __P((u_int, caddr_t, caddr_t,
					u_int, u_int32_t));
extern void key_freesp __P((struct secpolicy *));
extern void key_freesav __P((struct secasvar *));
extern struct secpolicy *key_newsp __P((u_int32_t));
extern struct secpolicy *key_msg2sp __P((struct sadb_x_policy *,
	size_t, int *));
extern struct mbuf *key_sp2msg __P((struct secpolicy *));
extern int key_cmpspidx_exactly
	__P((struct secpolicyindex *, struct secpolicyindex *));
extern int key_cmpspidx_withmask
	__P((struct secpolicyindex *, struct secpolicyindex *));
extern int key_spdacquire __P((struct secpolicy *));
extern void key_timehandler __P((void *));
extern void key_randomfill __P((void *, size_t));
extern void key_freereg __P((struct socket *));
extern int key_parse __P((struct mbuf *, struct socket *));
extern void key_init __P((void));
extern int key_checktunnelsanity __P((struct secasvar *, u_int,
					caddr_t, caddr_t));
extern void key_sa_recordxfer __P((struct secasvar *, struct mbuf *));
extern void key_sa_routechange __P((struct sockaddr *));
extern void key_sa_stir_iv __P((struct secasvar *));

#if defined(MIP6) && !defined(MIP6_NOHAIPSEC)
int key_mip6_update_mobile_node_ipsecdb(struct sockaddr_in6 *,
    struct sockaddr_in6 *, struct sockaddr_in6 *, struct sockaddr_in6 *);
int key_mip6_update_home_agent_ipsecdb(struct sockaddr_in6 *,
    struct sockaddr_in6 *, struct sockaddr_in6 *, struct sockaddr_in6 *);
#endif

#ifdef __FreeBSD__
#ifdef MALLOC_DECLARE
MALLOC_DECLARE(M_SECA);
#endif /* MALLOC_DECLARE */
#endif

#if defined(__bsdi__) || defined(__NetBSD__)
extern int key_sysctl __P((int *, u_int, void *, size_t *, void *, size_t));
#endif

#endif /* defined(_KERNEL) */
#endif /* _NETKEY_KEY_H_ */
