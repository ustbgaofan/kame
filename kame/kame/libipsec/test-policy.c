/*
 * Copyright (C) 1995, 1996, 1997, 1998, and 1999 WIDE Project.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <net/pfkeyv2.h>
#include <netkey/key_debug.h>
#include <netinet6/ipsec.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <err.h>

struct req_t {
	int result;	/* expected result; 0:ok 1:ng */
	char *str;
} reqs[] = {
#if 0
{ 1, "must_error" },
{ 1, "in ipsec must_error" },
{ 1, "out ipsec esp/must_error" },
{ 1, "out discard" },
{ 1, "out none" },
{ 0, "in entrust" },
{ 0, "out entrust" },
{ 1, "out ipsec esp" },
{ 0, "in ipsec ah/transport" },
{ 1, "in ipsec ah/tunnel" },
{ 0, "out ipsec ah/transport/" },
{ 1, "out ipsec ah/tunnel/" },
{ 0, "in ipsec esp / transport / 10.0.0.1-10.0.0.2" },
{ 0, "in ipsec esp/tunnel/::1-::2" },
{ 1, "in ipsec esp/tunnel/10.0.0.1-::2" },
#endif
{ 0, "in ipsec esp/tunnel/::1-::2/require" },
#if 0
{ 0, "out ipsec ah/transport//use" },
{ 0, "out ipsec ah/transport esp/use" },
{ 1, "in ipsec ah/transport esp/tunnel" },
{ 0, "in ipsec
	ah / transport
	esp / tunnel / ::1-::2" },
{ 0, "out ipsec
	ah/transport/::1-::2 esp/tunnel/::3-::4/use ah/transport/::5-::6/require
	ah/transport/::1-::2 esp/tunnel/::3-::4/use ah/transport/::5-::6/require
	ah/transport/::1-::2 esp/tunnel/::3-::4/use ah/transport/::5-::6/require
	" },
{ 0, "out ipsec esp/transport/fec0::10-fec0::11/use" },
#endif
};

int test1 __P((struct req_t *req));
int test1sub __P((char *buf, int family));

int
main(ac, av)
	int ac;
	char **av;
{
	int i;
	int result;

	for (i = 0; i < sizeof(reqs)/sizeof(reqs[0]); i++) {
		printf("#%d [%s]\n", i + 1, reqs[i].str);

		result = test1(&reqs[i]);
		if (result == 0 && reqs[i].result == 1) {
			printf("ERROR: expecting failure.\n");
		} else if (result == 1 && reqs[i].result == 0) {
			printf("ERROR: expecting success.\n");
		}
	}

	exit(0);
}

int
test1(req)
	struct req_t *req;
{
	char *buf;

	buf = ipsec_set_policy(req->str, strlen(req->str));
	if (buf == NULL) {
		printf("ipsec_set_policy: %s\n", ipsec_strerror());
		return 1;
	}

#if 0
	printf("\tsetlen:%d\n", ipsec_get_policylen(buf));
#endif

	if (test1sub(buf, PF_INET) != 0
	 || test1sub(buf, PF_INET6) != 0) {
		free(buf);
		return 1;
	}
#if 0
	kdebug_sadb_x_policy((struct sadb_ext *)buf);
#endif

	free(buf);
	return 0;
}

int
test1sub(policy, family)
	char *policy;
	int family;
{
	int so;
	int proto = 0, optname = 0;
	int len;
	char getbuf[1024];

	switch (family) {
	case PF_INET:
		proto = IPPROTO_IP;
		optname = IP_IPSEC_POLICY;
		break;
	case PF_INET6:
		proto = IPPROTO_IPV6;
		optname = IPV6_IPSEC_POLICY;
		break;
	}

	if ((so = socket(family, SOCK_DGRAM, 0)) < 0)
		err(1, "socket");

	len = ipsec_get_policylen(policy);
	if (setsockopt(so, proto, optname, policy, len) < 0) {
		printf("fail to set sockopt; %s\n", strerror(errno));
		close(so);
		return 1;
	}

	memset(getbuf, 0, sizeof(getbuf));
	memcpy(getbuf, policy, sizeof(struct sadb_x_policy));
	if (getsockopt(so, proto, optname, getbuf, &len) < 0) {
		printf("fail to get sockopt; %s\n", strerror(errno));
		close(so);
		return 1;
	}

    {
	char *buf = NULL;

#if 0
	printf("\tgetlen:%d\n", len);
#endif

	if ((buf = ipsec_dump_policy(getbuf, NULL)) == NULL) {
		printf("%s\n", ipsec_strerror());
		close(so);
		return 1;
	}

	printf("\t[%s]\n", buf);
	free(buf);
    }

	close (so);
	return 0;
}

