/*	$KAME: cnd.c,v 1.7 2005/05/17 10:31:24 keiichi Exp $	*/

/*
 * Copyright (C) 2004 WIDE Project.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <poll.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <net/if.h>
#ifdef __FreeBSD__
#include <net/if_var.h>
#endif
#include <netinet/in.h>
#include <netinet/icmp6.h>
#include <netinet/ip6mh.h>
#include <net/mipsock.h>
#include <netinet6/mip6.h>

#include "callout.h"
#include "stat.h"
#include "shisad.h"
#include "fdlist.h"
#include "command.h"
#include "config.h"

/*static void command_show_status(int, char *);*/
static void command_flush(int, char *);
static void terminate(int);

struct command_table show_command_table[] = {
	{"bc", command_show_bc, "binding chache"},
	{"kbc", command_show_kbc, "binding chache in kernel"},
	{"stat", command_show_stat, "statisticts"},
	{"callout", show_callout_table, "show callout table "},
	{NULL}
};

struct command_table command_table[] = {
	{"show", NULL, "Show status", show_command_table},
	{"flush", command_flush, "Flush binding caches"},
};

/* Global Variables */
int mhsock, mipsock, icmp6sock;
int debug = 0, namelookup = 1;
struct mip6stat mip6stat;
int homeagent_mode = 0;

static char *pid_file = CND_PIDFILE;

static void cn_lists_init(void);
void flush_bc(void);

void
cn_usage(path)
	char *path;
{
	char *cmd;

	cmd = strrchr(path, '/');
	if (!cmd)
		cmd = path;
	else
		cmd++;
	fprintf(stderr, "%s [-dn] [-i if]\n", cmd);
	exit(0);
} 


int
main(argc, argv)
	int argc;
	char **argv;
{
	int pfds;
	int pid;
	int ch = 0;
	FILE *pidfp;

	/* get options */
	while ((ch = getopt(argc, argv, "dni:")) != -1) {
		switch (ch) {
		case 'd':
			debug = 1;
			break;
		case 'n':
			namelookup = 0;
			break;
		default:
			fprintf(stderr, "unknown option\n");
			cn_usage();
			break;
		}
	}

	/* open syslog infomation. */
	openlog("shisad(cnd)", 0, LOG_DAEMON);
	syslog(LOG_INFO, "-- Start CN daemon at -- \n");

	/* parse configuration file and set default values. */
	if (parse_config(CFM_CND, CND_CONFFILE) == 0) {
		config_get_number(CFT_DEBUG, &debug, config_result);
		config_get_number(CFT_NAMELOOKUP, &namelookup, config_result);
	}

	/* open sockets */
	mhsock_open();
	mipsock_open();
	icmp6sock_open();

	/* start timer */
	callout_init();

	/* initialization */
	fdlist_init();
	if (command_init("cn> ", command_table, sizeof(command_table) / sizeof(struct command_table), 7777) < 0) {
		fprintf(stderr, "Unable to open user interface\n");
	}
	cn_lists_init();
	init_nonces();

	/* dump current PID */
	pid = getpid();
	if ((pidfp = fopen(pid_file, "w")) != NULL) {
		fprintf(pidfp, "%d\n", pid);
		fclose(pidfp);
	}

	/* register signal handlers. */
	signal(SIGTERM, terminate);
	signal(SIGINT, terminate);

	if (debug == 0)
		daemon(0, 0);

	new_fd_list(mhsock, POLLIN, mh_input_common);
        new_fd_list(mipsock, POLLIN, mipsock_input_common);
        new_fd_list(icmp6sock, POLLIN, icmp6_input_common);

	/* notify a kernel to behave as a correspondent node. */
	mipsock_nodetype_request(MIP6_NODETYPE_CORRESPONDENT_NODE, 1);

	while (1) {
		clear_revents();
	    
		if ((pfds = poll(fdl_fds, fdl_nfds, get_next_timeout())) < 0) {
			perror("poll");
			continue;
		}

		if (pfds != 0) {
			dispatch_fdfunctions(fdl_fds, fdl_nfds);
		}
		/* Timeout */
		callout_expire_check();
	}

	return (0);
}

static void
cn_lists_init(void)
{
	mip6_bc_init();

	return;
}


int
mipsock_input(miphdr)
	struct mip_msghdr *miphdr;
{
	int err = 0;
	struct mipm_nodetype_info *nodeinfo;

	switch (miphdr->miph_type) {
	case MIPM_NODETYPE_INFO:
		nodeinfo = (struct mipm_nodetype_info *)miphdr;
		homeagent_mode = nodeinfo->mipmni_enable & MIP6_NODETYPE_HOME_AGENT;
	case MIPM_BE_HINT:
		mipsock_behint_input(miphdr);
		break;
	default:
		break;
	}

	return (err);
}

static void
command_flush(s, arg)
	int s;
	char *arg;
{
	flush_bc();
}

static void
terminate(dummy)
	int dummy;
{
	mip6_flush_kernel_bc();
	mipsock_nodetype_request(MIP6_NODETYPE_CORRESPONDENT_NODE, 0);
	unlink(pid_file);
	exit(1);
}
