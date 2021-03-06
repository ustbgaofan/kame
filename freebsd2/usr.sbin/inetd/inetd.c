/*
 * Copyright (c) 1983, 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static const char copyright[] =
"@(#) Copyright (c) 1983, 1991, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)from: inetd.c	8.4 (Berkeley) 4/13/94";
#endif
static const char rcsid[] =
	"$Id: inetd.c,v 1.15.2.9 1998/10/09 11:50:52 des Exp $";
#endif /* not lint */

/*
 * Inetd - Internet super-server
 *
 * This program invokes all internet services as needed.  Connection-oriented
 * services are invoked each time a connection is made, by creating a process.
 * This process is passed the connection as file descriptor 0 and is expected
 * to do a getpeername to find out the source host and port.
 *
 * Datagram oriented services are invoked when a datagram
 * arrives; a process is created and passed a pending message
 * on file descriptor 0.  Datagram servers may either connect
 * to their peer, freeing up the original socket for inetd
 * to receive further messages on, or ``take over the socket'',
 * processing all arriving datagrams and, eventually, timing
 * out.	 The first type of server is said to be ``multi-threaded'';
 * the second type of server ``single-threaded''.
 *
 * Inetd uses a configuration file which is read at startup
 * and, possibly, at some later time in response to a hangup signal.
 * The configuration file is ``free format'' with fields given in the
 * order shown below.  Continuation lines for an entry must being with
 * a space or tab.  All fields must be present in each entry.
 *
 *	service name			must be in /etc/services or must
 *					name a tcpmux service
 *	socket type			stream/dgram/raw/rdm/seqpacket
 *	protocol			must be in /etc/protocols
 *	wait/nowait			single-threaded/multi-threaded
 *	user				user to run daemon as
 *	server program			full path name
 *	server program arguments	maximum of MAXARGS (20)
 *
 * TCP services without official port numbers are handled with the
 * RFC1078-based tcpmux internal service. Tcpmux listens on port 1 for
 * requests. When a connection is made from a foreign host, the service
 * requested is passed to tcpmux, which looks it up in the servtab list
 * and returns the proper entry for the service. Tcpmux returns a
 * negative reply if the service doesn't exist, otherwise the invoked
 * server is expected to return the positive reply if the service type in
 * inetd.conf file has the prefix "tcpmux/". If the service type has the
 * prefix "tcpmux/+", tcpmux will return the positive reply for the
 * process; this is for compatibility with older server code, and also
 * allows you to invoke programs that use stdin/stdout without putting any
 * special server code in them. Services that use tcpmux are "nowait"
 * because they do not have a well-known port and hence cannot listen
 * for new requests.
 *
 * For RPC services
 *	service name/version		must be in /etc/rpc
 *	socket type			stream/dgram/raw/rdm/seqpacket
 *	protocol			must be in /etc/protocols
 *	wait/nowait			single-threaded/multi-threaded
 *	user				user to run daemon as
 *	server program			full path name
 *	server program arguments	maximum of MAXARGS
 *
 * Comment lines are indicated by a `#' in column 1.
 *
 * #ifdef IPSEC
 * Comment lines that start with "#@" denote IPsec policy string, as described
 * in ipsec_set_policy(3).  This will affect all the following items in
 * inetd.conf(8).  To reset the policy, just use "#@" line.  By default,
 * there's no IPsec policy.
 * #endif
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>

#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <grp.h>
#include <netdb.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <libutil.h>
#include <sysexits.h>
#include <ctype.h>

#ifdef LOGIN_CAP
#include <login_cap.h>

/* see init.c */
#define RESOURCE_RC "daemon"

#endif

#include "pathnames.h"

#ifdef IPSEC
#include <netinet6/ipsec.h>
#ifndef IPSEC_POLICY_IPSEC	/* no ipsec support on old ipsec */
#undef IPSEC
#endif
#include "ipsec.h"
#endif

#ifndef	MAXCHILD
#define	MAXCHILD	-1		/* maximum number of this service
					   < 0 = no limit */
#endif

#ifndef	MAXCPM
#define	MAXCPM		-1		/* rate limit invocations from a
					   single remote address,
					   < 0 = no limit */
#endif

#define	TOOMANY		256		/* don't start more than TOOMANY */
#define	CNT_INTVL	60		/* servers in CNT_INTVL sec. */
#define	RETRYTIME	(60*10)		/* retry after bind or server fail */
#define MAX_MAXCHLD	32767		/* max allowable max children */

#define	SIGBLOCK	(sigmask(SIGCHLD)|sigmask(SIGHUP)|sigmask(SIGALRM))

int	debug = 0;
int	log = 0;
int	nsock, maxsock;
fd_set	allsock;
int	options;
int	timingout;
int	toomany = TOOMANY;
int	maxchild = MAXCPM;
int	maxcpm = MAXCHILD;
struct	rpcent *rpc;
char *bind_address = NULL;
#ifdef INET6
char *bind_address6 = NULL;
#endif

struct	servtab {
	char	*se_service;		/* name of service */
	int	se_socktype;		/* type of socket to use */
	char	*se_proto;		/* protocol used */
	int	se_maxchild;		/* max number of children */
	int	se_maxcpm;		/* max connects per IP per minute */
	int	se_numchild;		/* current number of children */
	pid_t	*se_pids;		/* array of child pids */
	char	*se_user;		/* user name to run as */
	char    *se_group;              /* group name to run as */
#ifdef  LOGIN_CAP
	char    *se_class;              /* login class name to run with */
#endif
	struct	biltin *se_bi;		/* if built-in, description */
	char	*se_server;		/* server program */
#define	MAXARGV 20
	char	*se_argv[MAXARGV+1];	/* program arguments */
#ifdef IPSEC
	char	*se_policy;		/* IPsec poilcy string */
#endif
	int	se_fd;			/* open descriptor */
	int	se_family;		/* address family */
	struct	sockaddr_storage se_ctrladdr;/* bound address */
	u_char	se_type;		/* type: normal, mux, or mux+ */
	u_char	se_checked;		/* looked at during merge */
	u_char	se_accept;		/* i.e., wait/nowait mode */
	u_char	se_rpc;			/* ==1 if RPC service */
	int	se_rpc_prog;		/* RPC program number */
	u_int	se_rpc_lowvers;		/* RPC low version */
	u_int	se_rpc_highvers;	/* RPC high version */
	int	se_count;		/* number started since se_time */
	struct	timeval se_time;	/* start of se_count */
	struct	servtab *se_next;
} *servtab;

#define NORM_TYPE	0
#define MUX_TYPE	1
#define MUXPLUS_TYPE	2
#define TTCP_TYPE	3
#define ISMUX(sep)	(((sep)->se_type == MUX_TYPE) || \
			 ((sep)->se_type == MUXPLUS_TYPE))
#define ISMUXPLUS(sep)	((sep)->se_type == MUXPLUS_TYPE)
#define ISTTCP(sep)	((sep)->se_type == TTCP_TYPE)


void		chargen_dg __P((int, struct servtab *));
void		chargen_stream __P((int, struct servtab *));
void		close_sep __P((struct servtab *));
void		config __P((int));
void		daytime_dg __P((int, struct servtab *));
void		daytime_stream __P((int, struct servtab *));
void		discard_dg __P((int, struct servtab *));
void		discard_stream __P((int, struct servtab *));
void		echo_dg __P((int, struct servtab *));
void		echo_stream __P((int, struct servtab *));
void		endconfig __P((void));
struct servtab *enter __P((struct servtab *));
void		freeconfig __P((struct servtab *));
struct servtab *getconfigent __P((void));
void		machtime_dg __P((int, struct servtab *));
void		machtime_stream __P((int, struct servtab *));
char	       *newstr __P((char *));
char	       *nextline __P((FILE *));
void		print_service __P((char *, struct servtab *));
void		addchild __P((struct servtab *, int));
void		reapchild __P((int));
void		enable __P((struct servtab *));
void		disable __P((struct servtab *));
void		retry __P((int));
int		setconfig __P((void));
void		setup __P((struct servtab *));
char	       *sskip __P((char **));
char	       *skip __P((char **));
struct servtab *tcpmux __P((int));
int		cpmip __P((struct servtab *, int));

void		unregisterrpc __P((register struct servtab *sep));

struct biltin {
	char	*bi_service;		/* internally provided service name */
	int	bi_socktype;		/* type of socket supported */
	short	bi_fork;		/* 1 if should fork before call */
	int	bi_maxchild;		/* max number of children (default) */
	void	(*bi_fn)();		/* function which performs it */
} biltins[] = {
	/* Echo received data */
	{ "echo",	SOCK_STREAM,	1, 0,	echo_stream },
	{ "echo",	SOCK_DGRAM,	0, 0,	echo_dg },

	/* Internet /dev/null */
	{ "discard",	SOCK_STREAM,	1, 0,	discard_stream },
	{ "discard",	SOCK_DGRAM,	0, 0,	discard_dg },

	/* Return 32 bit time since 1970 */
	{ "time",	SOCK_STREAM,	0, 0,	machtime_stream },
	{ "time",	SOCK_DGRAM,	0, 0,	machtime_dg },

	/* Return human-readable time */
	{ "daytime",	SOCK_STREAM,	0, 0,	daytime_stream },
	{ "daytime",	SOCK_DGRAM,	0, 0,	daytime_dg },

	/* Familiar character generator */
	{ "chargen",	SOCK_STREAM,	1, 0,	chargen_stream },
	{ "chargen",	SOCK_DGRAM,	0, 0,	chargen_dg },

	{ "tcpmux",	SOCK_STREAM,	1, 0,	(void (*)())tcpmux },

	{ NULL }
};

#define NUMINT	(sizeof(intab) / sizeof(struct inent))
char	*CONFIG = _PATH_INETDCONF;
char	*pid_file = _PATH_INETDPID;

#ifdef OLD_SETPROCTITLE
char	**Argv;
char 	*LastArg;
#endif

int
getvalue(arg, value, whine)
	char *arg, *whine;
	int  *value;
{
	int  tmp;
	char *p;

	tmp = strtol(arg, &p, 0);
	if (tmp < 1 || *p) {
		syslog(LOG_ERR, whine, arg);
		return 1;			/* failure */
	}
	*value = tmp;
	return 0;				/* success */
}

int
main(argc, argv, envp)
	int argc;
	char *argv[], *envp[];
{
	struct servtab *sep;
	struct passwd *pwd;
	struct group *grp;
	struct sigaction sa, sapipe;
	int tmpint, ch, dofork;
	pid_t pid;
	char buf[50];
	struct  sockaddr_storage peer;
	int i;
#ifdef LOGIN_CAP
	login_cap_t *lc = NULL;
#endif
	char hbuf[MAXHOSTNAMELEN];


#ifdef OLD_SETPROCTITLE
	Argv = argv;
	if (envp == 0 || *envp == 0)
		envp = argv;
	while (*envp)
		envp++;
	LastArg = envp[-1] + strlen(envp[-1]);
#endif

	openlog("inetd", LOG_PID | LOG_NOWAIT, LOG_DAEMON);

	while ((ch = getopt(argc, argv, "dlR:a:c:C:p:")) != -1)
		switch(ch) {
		case 'd':
			debug = 1;
			options |= SO_DEBUG;
			break;
		case 'l':
			log = 1;
			break;
		case 'R':
			getvalue(optarg, &toomany,
				"-R %s: bad value for service invocation rate");
			break;
		case 'c':
			getvalue(optarg, &maxchild,
				"-c %s: bad value for maximum children");
			break;
		case 'C':
			getvalue(optarg, &maxcpm,
				"-C %s: bad value for maximum children/minute");
			break;
		case 'a':
		    {
			struct in_addr a4;
#ifdef INET6
			struct in6_addr a6;
#endif

			if (inet_pton(AF_INET, optarg, &a4) == 1)
				bind_address = optarg;
#ifdef INET6
			else if (inet_pton(AF_INET6, optarg, &a6) == 1)
				bind_address6 = optarg;
#endif
			else {
				syslog(LOG_ERR, "-a %s: invalid address",
					optarg);
				exit(EX_USAGE);
			}
		    }
		case 'p':
			pid_file = optarg;
			break;
		case '?':
		default:
			syslog(LOG_ERR,
				"usage: inetd [-dl] [-a address] [-R rate]"
				" [-c maximum] [-C rate]"
				" [-p pidfile] [conf-file]");
			exit(EX_USAGE);
		}
	argc -= optind;
	argv += optind;

	if (argc > 0)
		CONFIG = argv[0];
	if (debug == 0) {
		FILE *fp;
		if (daemon(0, 0) < 0) {
			syslog(LOG_WARNING, "daemon(0,0) failed: %m");
		}
		/*
		 * In case somebody has started inetd manually, we need to
		 * clear the logname, so that old servers run as root do not
		 * get the user's logname..
		 */
		if (setlogin("") < 0) {
			syslog(LOG_WARNING, "cannot clear logname: %m");
			/* no big deal if it fails.. */
		}
		pid = getpid();
		fp = fopen(pid_file, "w");
		if (fp) {
			fprintf(fp, "%ld\n", (long)pid);
			fclose(fp);
		} else {
			syslog(LOG_WARNING, "%s: %m", pid_file);
		}
	}
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGALRM);
	sigaddset(&sa.sa_mask, SIGCHLD);
	sigaddset(&sa.sa_mask, SIGHUP);
	sa.sa_handler = retry;
	sigaction(SIGALRM, &sa, (struct sigaction *)0);
	config(SIGHUP);
	sa.sa_handler = config;
	sigaction(SIGHUP, &sa, (struct sigaction *)0);
	sa.sa_handler = reapchild;
	sigaction(SIGCHLD, &sa, (struct sigaction *)0);
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, &sapipe);

	{
		/* space for daemons to overwrite environment for ps */
#define	DUMMYSIZE	100
		char dummy[DUMMYSIZE];

		(void)memset(dummy, 'x', DUMMYSIZE - 1);
		dummy[DUMMYSIZE - 1] = '\0';
		(void)setenv("inetd_dummy", dummy, 1);
	}

	for (;;) {
	    int n, ctrl;
	    fd_set readable;

	    if (nsock == 0) {
		(void) sigblock(SIGBLOCK);
		while (nsock == 0)
		    sigpause(0L);
		(void) sigsetmask(0L);
	    }
	    readable = allsock;
	    if ((n = select(maxsock + 1, &readable, (fd_set *)0,
		(fd_set *)0, (struct timeval *)0)) <= 0) {
		    if (n < 0 && errno != EINTR) {
			syslog(LOG_WARNING, "select: %m");
			sleep(1);
		    }
		    continue;
	    }
	    for (sep = servtab; n && sep; sep = sep->se_next)
	        if (sep->se_fd != -1 && FD_ISSET(sep->se_fd, &readable)) {
		    n--;
		    if (debug)
			    warnx("someone wants %s", sep->se_service);
		    if (sep->se_accept && sep->se_socktype == SOCK_STREAM) {
			    ctrl = accept(sep->se_fd, (struct sockaddr *)0,
				(int *)0);
			    if (debug)
				    warnx("accept, ctrl %d", ctrl);
			    if (ctrl < 0) {
				    if (errno != EINTR)
					    syslog(LOG_WARNING,
						"accept (for %s): %m",
						sep->se_service);
				    continue;
			    }
			    if (cpmip(sep, ctrl) < 0) {
				close(ctrl);
				continue;
			    }
			    if (log) {
				i = sizeof peer;
				if (getpeername(ctrl, (struct sockaddr *)
						&peer, &i)) {
					syslog(LOG_WARNING,
						"getpeername(for %s): %m",
						sep->se_service);
					close(ctrl);
					continue;
				}
				getnameinfo((struct sockaddr *)&peer, i,
					hbuf, sizeof(hbuf), NULL, 0,
					NI_NUMERICHOST);
				syslog(LOG_INFO,"%s from %s",
					sep->se_service, hbuf);
			    }
		    } else
			    ctrl = sep->se_fd;
		    (void) sigblock(SIGBLOCK);
		    pid = 0;
		    dofork = (sep->se_bi == 0 || sep->se_bi->bi_fork);
		    if (dofork) {
			    if (sep->se_count++ == 0)
				(void)gettimeofday(&sep->se_time,
						   (struct timezone *)NULL);
			    else if (sep->se_count >= toomany) {
				struct timeval now;

				(void)gettimeofday(&now,
						   (struct timezone *)NULL);
				if (now.tv_sec - sep->se_time.tv_sec >
				    CNT_INTVL) {
					sep->se_time = now;
					sep->se_count = 1;
				} else {
					syslog(LOG_ERR,
			"%s/%s server failing (looping), service terminated",
					    sep->se_service, sep->se_proto);
					close_sep(sep);
					sigsetmask(0L);
					if (!timingout) {
						timingout = 1;
						alarm(RETRYTIME);
					}
					continue;
				}
			    }
			    pid = fork();
		    }
		    if (pid < 0) {
			    syslog(LOG_ERR, "fork: %m");
			    if (sep->se_accept &&
				sep->se_socktype == SOCK_STREAM)
				    close(ctrl);
			    sigsetmask(0L);
			    sleep(1);
			    continue;
		    }
		    if (pid)
			addchild(sep, pid);
		    sigsetmask(0L);
		    if (pid == 0) {
			    if (dofork) {
				if (debug)
					warnx("+ closing from %d", maxsock);
				for (tmpint = maxsock; tmpint > 2; tmpint--)
					if (tmpint != ctrl)
						(void) close(tmpint);
			    }
			    /*
			     * Call tcpmux to find the real service to exec.
			     */
			    if (sep->se_bi &&
				sep->se_bi->bi_fn == (void (*)()) tcpmux) {
				    sep = tcpmux(ctrl);
				    if (sep == NULL) {
					    close(ctrl);
					    _exit(0);
				    }
			    }
			    if (sep->se_bi) {
				(*sep->se_bi->bi_fn)(ctrl, sep);
				/* NOTREACHED */
			    } else {
				if (debug)
					warnx("%d execl %s",
						getpid(), sep->se_server);
				dup2(ctrl, 0);
				close(ctrl);
				dup2(0, 1);
				dup2(0, 2);
				if ((pwd = getpwnam(sep->se_user)) == NULL) {
					syslog(LOG_ERR,
					    "%s/%s: %s: No such user",
						sep->se_service, sep->se_proto,
						sep->se_user);
					if (sep->se_socktype != SOCK_STREAM)
						recv(0, buf, sizeof (buf), 0);
					_exit(EX_NOUSER);
				}
				grp = NULL;
				if (   sep->se_group != NULL
				    && (grp = getgrnam(sep->se_group)) == NULL
				   ) {
					syslog(LOG_ERR,
					    "%s/%s: %s: No such group",
						sep->se_service, sep->se_proto,
						sep->se_group);
					if (sep->se_socktype != SOCK_STREAM)
						recv(0, buf, sizeof (buf), 0);
					_exit(EX_NOUSER);
				}
				if (grp != NULL)
					pwd->pw_gid = grp->gr_gid;
#ifdef LOGIN_CAP
				if ((lc = login_getclass(sep->se_class)) == NULL) {
					/* error syslogged by getclass */
					syslog(LOG_ERR,
					    "%s/%s: %s: login class error",
						sep->se_service, sep->se_proto,
						sep->se_class);
					if (sep->se_socktype != SOCK_STREAM)
						recv(0, buf, sizeof (buf), 0);
					_exit(EX_NOUSER);
				}
#endif
				if (setsid() < 0) {
					syslog(LOG_ERR,
						"%s: can't setsid(): %m",
						 sep->se_service);
					/* _exit(EX_OSERR); not fatal yet */
				}
#ifdef LOGIN_CAP
				if (setusercontext(lc, pwd, pwd->pw_uid,
				    LOGIN_SETALL) != 0) {
					syslog(LOG_ERR,
					 "%s: can't setusercontext(..%s..): %m",
					 sep->se_service, sep->se_user);
					_exit(EX_OSERR);
				}
#else
				if (pwd->pw_uid) {
					if (setlogin(sep->se_user) < 0) {
						syslog(LOG_ERR,
						 "%s: can't setlogin(%s): %m",
						 sep->se_service, sep->se_user);
						/* _exit(EX_OSERR); not yet */
					}
					if (setgid(pwd->pw_gid) < 0) {
						syslog(LOG_ERR,
						  "%s: can't set gid %d: %m",
						  sep->se_service, pwd->pw_gid);
						_exit(EX_OSERR);
					}
					(void) initgroups(pwd->pw_name,
							pwd->pw_gid);
					if (setuid(pwd->pw_uid) < 0) {
						syslog(LOG_ERR,
						  "%s: can't set uid %d: %m",
						  sep->se_service, pwd->pw_uid);
						_exit(EX_OSERR);
					}
				}
#endif
				sigaction(SIGPIPE, &sapipe,
				    (struct sigaction *)0);
				execv(sep->se_server, sep->se_argv);
				if (sep->se_socktype != SOCK_STREAM)
					recv(0, buf, sizeof (buf), 0);
				syslog(LOG_ERR,
				    "cannot execute %s: %m", sep->se_server);
				_exit(EX_OSERR);
			    }
		    }
		    if (sep->se_accept && sep->se_socktype == SOCK_STREAM)
			    close(ctrl);
		}
	}
}

/*
 * Record a new child pid for this service. If we've reached the
 * limit on children, then stop accepting incoming requests.
 */

void
addchild(struct servtab *sep, pid_t pid)
{
#ifdef SANITY_CHECK
	if (sep->se_numchild >= sep->se_maxchild) {
		syslog(LOG_ERR, "%s: %d >= %d",
		    __FUNCTION__, sep->se_numchild, sep->se_maxchild);
		exit(EX_SOFTWARE);
	}
#endif
	if (sep->se_maxchild == 0)
		return;
	sep->se_pids[sep->se_numchild++] = pid;
	if (sep->se_numchild == sep->se_maxchild)
		disable(sep);
}

/*
 * Some child process has exited. See if it's on somebody's list.
 */

void
reapchild(signo)
	int signo;
{
	int k, status;
	pid_t pid;
	struct servtab *sep;

	for (;;) {
		pid = wait3(&status, WNOHANG, (struct rusage *)0);
		if (pid <= 0)
			break;
		if (debug)
			warnx("%d reaped, status %#x", pid, status);
		for (sep = servtab; sep; sep = sep->se_next) {
			for (k = 0; k < sep->se_numchild; k++)
				if (sep->se_pids[k] == pid)
					break;
			if (k == sep->se_numchild)
				continue;
			if (sep->se_numchild == sep->se_maxchild)
				enable(sep);
			sep->se_pids[k] = sep->se_pids[--sep->se_numchild];
			if (status)
				syslog(LOG_WARNING,
				    "%s[%d]: exit status 0x%x",
				    sep->se_server, pid, status);
			break;
		}
	}
}

void
config(signo)
	int signo;
{
	struct servtab *sep, *new, **sepp;
	long omask;

	if (!setconfig()) {
		syslog(LOG_ERR, "%s: %m", CONFIG);
		return;
	}
	for (sep = servtab; sep; sep = sep->se_next)
		sep->se_checked = 0;
	while ((new = getconfigent())) {
		if (getpwnam(new->se_user) == NULL) {
			syslog(LOG_ERR,
				"%s/%s: No such user '%s', service ignored",
				new->se_service, new->se_proto, new->se_user);
			continue;
		}
		if (new->se_group && getgrnam(new->se_group) == NULL) {
			syslog(LOG_ERR,
				"%s/%s: No such group '%s', service ignored",
				new->se_service, new->se_proto, new->se_group);
			continue;
		}
#ifdef LOGIN_CAP
		if (login_getclass(new->se_class) == NULL) {
			/* error syslogged by getclass */
			syslog(LOG_ERR,
				"%s/%s: %s: login class error, service ignored",
				new->se_service, new->se_proto, new->se_class);
			continue;
		}
#endif
		for (sep = servtab; sep; sep = sep->se_next)
			if (strcmp(sep->se_service, new->se_service) == 0 &&
			    strcmp(sep->se_proto, new->se_proto) == 0)
				break;
		if (sep != 0) {
			int i;

#define SWAP(a, b) { typeof(a) c = a; a = b; b = c; }
			omask = sigblock(SIGBLOCK);
			/* copy over outstanding child pids */
			if (sep->se_maxchild && new->se_maxchild) {
				new->se_numchild = sep->se_numchild;
				if (new->se_numchild > new->se_maxchild)
					new->se_numchild = new->se_maxchild;
				memcpy(new->se_pids, sep->se_pids,
				    new->se_numchild * sizeof(*new->se_pids));
			}
			SWAP(sep->se_pids, new->se_pids);
			sep->se_maxchild = new->se_maxchild;
			sep->se_numchild = new->se_numchild;
			sep->se_maxcpm = new->se_maxcpm;
			/* might need to turn on or off service now */
			if (sep->se_fd >= 0) {
			      if (sep->se_maxchild
				  && sep->se_numchild == sep->se_maxchild) {
				      if (FD_ISSET(sep->se_fd, &allsock))
					  disable(sep);
			      } else {
				      if (!FD_ISSET(sep->se_fd, &allsock))
					  enable(sep);
			      }
			}
			sep->se_accept = new->se_accept;
			SWAP(sep->se_user, new->se_user);
			SWAP(sep->se_group, new->se_group);
#ifdef LOGIN_CAP
			SWAP(sep->se_class, new->se_class);
#endif
			SWAP(sep->se_server, new->se_server);
			for (i = 0; i < MAXARGV; i++)
				SWAP(sep->se_argv[i], new->se_argv[i]);
#ifdef IPSEC
			SWAP(sep->se_policy, new->se_policy);
			if (sep->se_fd >= 0) {
				if (ipsecsetup(sep->se_family, sep->se_fd,
				    sep->se_policy) && sep->se_policy) {
					syslog(LOG_ERR, "%s/%s: "
					    "ipsec initialization failed",
					    sep->se_service, sep->se_proto);
					sep->se_checked = 0;
					sigsetmask(omask);
					continue;
				}
			}
#endif
			sigsetmask(omask);
			freeconfig(new);
			if (debug)
				print_service("REDO", sep);
		} else {
			sep = enter(new);
			if (debug)
				print_service("ADD ", sep);
		}
		sep->se_checked = 1;
		if (ISMUX(sep)) {
			sep->se_fd = -1;
			continue;
		}
		if (!sep->se_rpc) {
			struct addrinfo hints, *res;
			int e;
			char *s;

			memset(&hints, 0, sizeof(hints));
			hints.ai_family = sep->se_family;
			hints.ai_socktype = sep->se_socktype;
			hints.ai_flags = AI_PASSIVE;
			switch (hints.ai_family) {
			case AF_INET:
				s = bind_address;
				break;
#ifdef INET6
			case AF_INET6:
				s = bind_address6;
				break;
#endif
			default:
				syslog(LOG_ERR, "%s/%s: unsupported af %d",
					sep->se_service, sep->se_proto,
					hints.ai_family);
				sep->se_checked = 0;
				continue;
			}
			e = getaddrinfo(s, sep->se_service, &hints, &res);
			if (e) {
				syslog(LOG_ERR, "%s/%s: unknown service",
					sep->se_service, sep->se_proto);
				sep->se_checked = 0;
				continue;
			}
			if (memcmp(&sep->se_ctrladdr, res->ai_addr, res->ai_addrlen) != 0) {
				memcpy(&sep->se_ctrladdr, res->ai_addr,
					res->ai_addrlen);
				if (sep->se_fd >= 0)
					close_sep(sep);
			}
		} else {
			rpc = getrpcbyname(sep->se_service);
			if (rpc == 0) {
				syslog(LOG_ERR, "%s/%s unknown RPC service.",
					sep->se_service, sep->se_proto);
				if (sep->se_fd != -1)
					(void) close(sep->se_fd);
				sep->se_fd = -1;
					continue;
			}
			if (rpc->r_number != sep->se_rpc_prog) {
				if (sep->se_rpc_prog)
					unregisterrpc(sep);
				sep->se_rpc_prog = rpc->r_number;
				if (sep->se_fd != -1)
					(void) close(sep->se_fd);
				sep->se_fd = -1;
			}
		}
		if (sep->se_fd == -1)
			setup(sep);
	}
	endconfig();
	/*
	 * Purge anything not looked at above.
	 */
	omask = sigblock(SIGBLOCK);
	sepp = &servtab;
	while ((sep = *sepp)) {
		if (sep->se_checked) {
			sepp = &sep->se_next;
			continue;
		}
		*sepp = sep->se_next;
		if (sep->se_fd >= 0)
			close_sep(sep);
		if (debug)
			print_service("FREE", sep);
		if (sep->se_rpc && sep->se_rpc_prog > 0)
			unregisterrpc(sep);
		freeconfig(sep);
		free((char *)sep);
	}
	(void) sigsetmask(omask);
}

void
unregisterrpc(sep)
	struct servtab *sep;
{
        int i;
        struct servtab *sepp;
	long omask;

	omask = sigblock(SIGBLOCK);
        for (sepp = servtab; sepp; sepp = sepp->se_next) {
                if (sepp == sep)
                        continue;
		if (sep->se_checked == 0 ||
                    !sepp->se_rpc ||
                    sep->se_rpc_prog != sepp->se_rpc_prog)
			continue;
                return;
        }
        if (debug)
                print_service("UNREG", sep);
        for (i = sep->se_rpc_lowvers; i <= sep->se_rpc_highvers; i++)
                pmap_unset(sep->se_rpc_prog, i);
        if (sep->se_fd != -1)
                (void) close(sep->se_fd);
        sep->se_fd = -1;
	(void) sigsetmask(omask);
}

void
retry(signo)
	int signo;
{
	struct servtab *sep;

	timingout = 0;
	for (sep = servtab; sep; sep = sep->se_next)
		if (sep->se_fd == -1 && !ISMUX(sep))
			setup(sep);
}

void
setup(sep)
	struct servtab *sep;
{
	int on = 1;

	if ((sep->se_fd = socket(sep->se_family, sep->se_socktype, 0)) < 0) {
		if (debug)
			warn("socket failed on %s/%s",
				sep->se_service, sep->se_proto);
		syslog(LOG_ERR, "%s/%s: socket: %m",
		    sep->se_service, sep->se_proto);
		return;
	}
#define	turnon(fd, opt) \
setsockopt(fd, SOL_SOCKET, opt, (char *)&on, sizeof (on))
	if (strncmp(sep->se_proto, "tcp", 3) == 0 && (options & SO_DEBUG) &&
	    turnon(sep->se_fd, SO_DEBUG) < 0)
		syslog(LOG_ERR, "setsockopt (SO_DEBUG): %m");
	if (turnon(sep->se_fd, SO_REUSEADDR) < 0)
		syslog(LOG_ERR, "setsockopt (SO_REUSEADDR): %m");
#ifdef SO_PRIVSTATE
	if (turnon(sep->se_fd, SO_PRIVSTATE) < 0)
		syslog(LOG_ERR, "setsockopt (SO_PRIVSTATE): %m");
#endif
#undef turnon
	if (sep->se_type == TTCP_TYPE)
		if (setsockopt(sep->se_fd, IPPROTO_TCP, TCP_NOPUSH,
		    (char *)&on, sizeof (on)) < 0)
			syslog(LOG_ERR, "setsockopt (TCP_NOPUSH): %m");
#ifdef IPSEC
	if (ipsecsetup(sep->se_family, sep->se_fd, sep->se_policy) < 0
	 && sep->se_policy) {
		syslog(LOG_ERR, "%s/%s: ipsec setup failed",
		    sep->se_service, sep->se_proto);
		(void)close(sep->se_fd);
		sep->se_fd = -1;
		return;
	}
#endif
	if (bind(sep->se_fd, (struct sockaddr *)&sep->se_ctrladdr,
	    sep->se_ctrladdr.ss_len) < 0) {
		if (debug)
			warn("bind failed on %s/%s",
				sep->se_service, sep->se_proto);
		syslog(LOG_ERR, "%s/%s: bind: %m",
		    sep->se_service, sep->se_proto);
		(void) close(sep->se_fd);
		sep->se_fd = -1;
		if (!timingout) {
			timingout = 1;
			alarm(RETRYTIME);
		}
		return;
	}
        if (sep->se_rpc) {
                int i, len = sizeof(sep->se_ctrladdr);
		u_short port;

                if (getsockname(sep->se_fd,
				(struct sockaddr*)&sep->se_ctrladdr, &len) < 0){
                        syslog(LOG_ERR, "%s/%s: getsockname: %m",
                               sep->se_service, sep->se_proto);
                        (void) close(sep->se_fd);
                        sep->se_fd = -1;
                        return;
                }
                if (debug)
                        print_service("REG ", sep);
                for (i = sep->se_rpc_lowvers; i <= sep->se_rpc_highvers; i++) {
                        pmap_unset(sep->se_rpc_prog, i);
			switch (sep->se_family) {
			case AF_INET:
				port = ((struct sockaddr_in *)&sep->se_ctrladdr)->sin_port;
				break;
#ifdef INET6
			case AF_INET6:
				port = ((struct sockaddr_in6 *)&sep->se_ctrladdr)->sin6_port;
				break;
#endif
			default:
				port = 0;
			}
                        pmap_set(sep->se_rpc_prog, i,
                                 (sep->se_socktype == SOCK_DGRAM)
                                 ? IPPROTO_UDP : IPPROTO_TCP,
                                 ntohs(port));
                }

        }
	if (sep->se_socktype == SOCK_STREAM)
		listen(sep->se_fd, 64);
	enable(sep);
	if (debug) {
		warnx("registered %s on %d",
			sep->se_server, sep->se_fd);
	}
}

/*
 * Finish with a service and its socket.
 */
void
close_sep(sep)
	struct servtab *sep;
{
	if (sep->se_fd >= 0) {
		if (FD_ISSET(sep->se_fd, &allsock))
			disable(sep);
		(void) close(sep->se_fd);
		sep->se_fd = -1;
	}
	sep->se_count = 0;
	sep->se_numchild = 0;	/* forget about any existing children */
}

struct servtab *
enter(cp)
	struct servtab *cp;
{
	struct servtab *sep;
	long omask;

	sep = (struct servtab *)malloc(sizeof (*sep));
	if (sep == (struct servtab *)0) {
		syslog(LOG_ERR, "Out of memory.");
		exit(EX_OSERR);
	}
	*sep = *cp;
	sep->se_fd = -1;
	omask = sigblock(SIGBLOCK);
	sep->se_next = servtab;
	servtab = sep;
	sigsetmask(omask);
	return (sep);
}

void
enable(struct servtab *sep)
{
	if (debug)
		warnx(
		    "enabling %s, fd %d", sep->se_service, sep->se_fd);
#ifdef SANITY_CHECK
	if (sep->se_fd < 0) {
		syslog(LOG_ERR,
		    "%s: %s: bad fd", __FUNCTION__, sep->se_service);
		exit(EX_SOFTWARE);
	}
	if (ISMUX(sep)) {
		syslog(LOG_ERR,
		    "%s: %s: is mux", __FUNCTION__, sep->se_service);
		exit(EX_SOFTWARE);
	}
	if (FD_ISSET(sep->se_fd, &allsock)) {
		syslog(LOG_ERR,
		    "%s: %s: not off", __FUNCTION__, sep->se_service);
		exit(EX_SOFTWARE);
	}
#endif
	FD_SET(sep->se_fd, &allsock);
	nsock++;
	if (sep->se_fd > maxsock)
		maxsock = sep->se_fd;
}

void
disable(struct servtab *sep)
{
	if (debug)
		warnx(
		    "disabling %s, fd %d", sep->se_service, sep->se_fd);
#ifdef SANITY_CHECK
	if (sep->se_fd < 0) {
		syslog(LOG_ERR,
		    "%s: %s: bad fd", __FUNCTION__, sep->se_service);
		exit(EX_SOFTWARE);
	}
	if (ISMUX(sep)) {
		syslog(LOG_ERR,
		    "%s: %s: is mux", __FUNCTION__, sep->se_service);
		exit(EX_SOFTWARE);
	}
	if (!FD_ISSET(sep->se_fd, &allsock)) {
		syslog(LOG_ERR,
		    "%s: %s: not on", __FUNCTION__, sep->se_service);
		exit(EX_SOFTWARE);
	}
	if (nsock == 0) {
		syslog(LOG_ERR, "%s: nsock=0", __FUNCTION__);
		exit(EX_SOFTWARE);
	}
#endif
	FD_CLR(sep->se_fd, &allsock);
	nsock--;
	if (sep->se_fd == maxsock)
		maxsock--;
}

FILE	*fconfig = NULL;
struct	servtab serv;
char	line[LINE_MAX];

int
setconfig()
{

	if (fconfig != NULL) {
		fseek(fconfig, 0L, SEEK_SET);
		return (1);
	}
	fconfig = fopen(CONFIG, "r");
	return (fconfig != NULL);
}

void
endconfig()
{
	if (fconfig) {
		(void) fclose(fconfig);
		fconfig = NULL;
	}
}

struct servtab *
getconfigent()
{
	struct servtab *sep = &serv;
	int argc;
	char *cp, *arg, *s;
	char *versp;
	static char TCPMUX_TOKEN[] = "tcpmux/";
#define MUX_LEN		(sizeof(TCPMUX_TOKEN)-1)
#ifdef IPSEC
	char *policy = NULL;
#endif
	struct sockaddr_storage baddr;
	struct addrinfo hints, *res;
	int e;

more:
	while ((cp = nextline(fconfig)) != NULL) {
#ifdef IPSEC
		/* lines starting with #@ is not a comment, but the policy */
		if (cp[0] == '#' && cp[1] == '@') {
			char *p;
			for (p = cp + 2; p && *p && isspace(*p); p++)
				;
			if (*p == '\0') {
				if (policy)
					free(policy);
				policy = NULL;
			} else {
				if (ipsecsetup_test(p) < 0) {
					syslog(LOG_ERR,
						"%s: invalid ipsec policy \"%s\"",
						CONFIG, p);
					exit(EX_CONFIG);
				} else {
					if (policy)
						free(policy);
					policy = newstr(p);
				}
			}
		}
#endif
		if (*cp == '#' || *cp == '\0')
			continue;
		break;
	}
	if (cp == NULL)
		return ((struct servtab *)0);
	/*
	 * clear the static buffer, since some fields (se_ctrladdr,
	 * for example) don't get initialized here.
	 */
	memset((caddr_t)sep, 0, sizeof *sep);
	arg = skip(&cp);
	if (cp == NULL) {
		/* got an empty line containing just blanks/tabs. */
		goto more;
	}
	if (strncmp(arg, TCPMUX_TOKEN, MUX_LEN) == 0) {
		char *c = arg + MUX_LEN;
		if (*c == '+') {
			sep->se_type = MUXPLUS_TYPE;
			c++;
		} else
			sep->se_type = MUX_TYPE;
		sep->se_service = newstr(c);
	} else {
		sep->se_service = newstr(arg);
		sep->se_type = NORM_TYPE;
	}
	arg = sskip(&cp);
	if (strcmp(arg, "stream") == 0)
		sep->se_socktype = SOCK_STREAM;
	else if (strcmp(arg, "dgram") == 0)
		sep->se_socktype = SOCK_DGRAM;
	else if (strcmp(arg, "rdm") == 0)
		sep->se_socktype = SOCK_RDM;
	else if (strcmp(arg, "seqpacket") == 0)
		sep->se_socktype = SOCK_SEQPACKET;
	else if (strcmp(arg, "raw") == 0)
		sep->se_socktype = SOCK_RAW;
	else
		sep->se_socktype = -1;

	arg = sskip(&cp);
	if (strcmp(arg, "tcp/ttcp") == 0) {
		sep->se_type = TTCP_TYPE;
		sep->se_proto = newstr("tcp");
	} else {
		sep->se_proto = newstr(arg);
	}

	s = strchr(sep->se_proto, '/');
	if (s && s > sep->se_proto)
		s--;
	else if (!s && strlen(sep->se_proto))
		s = &sep->se_proto[strlen(sep->se_proto) - 1];
	else
		s = NULL;
	if (!s) {
		syslog(LOG_ERR, "bad protocol specifier; %s\n",
			sep->se_proto);
		freeconfig(sep);
		goto more;
	}
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = sep->se_socktype;
	switch (*s) {
	case '4':
		sep->se_family = hints.ai_family = AF_INET;
		s = bind_address;
		break;
#ifdef INET6
	case '6':
		sep->se_family = hints.ai_family = AF_INET6;
		s = bind_address6;
		break;
#endif
	default:
		/* will be AF_INET6 */
		sep->se_family = hints.ai_family = AF_INET;
		s = bind_address;
		break;
	}
	hints.ai_flags = AI_PASSIVE;
	e = getaddrinfo(s, "0", &hints, &res);
	if (e) {
		syslog(LOG_ERR, "bad bind address; %s\n", s);
		freeconfig(sep);
		goto more;
	}
	memcpy(&baddr, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);

        if (strncmp(sep->se_proto, "rpc/", 4) == 0) {
                memmove(sep->se_proto, sep->se_proto + 4,
                    strlen(sep->se_proto) + 1 - 4);
                sep->se_rpc = 1;
                sep->se_rpc_prog = sep->se_rpc_lowvers =
			sep->se_rpc_lowvers = 0;
		memcpy(&sep->se_ctrladdr, &baddr, baddr.ss_len);
                if ((versp = rindex(sep->se_service, '/'))) {
                        *versp++ = '\0';
                        switch (sscanf(versp, "%d-%d",
                                       &sep->se_rpc_lowvers,
                                       &sep->se_rpc_highvers)) {
                        case 2:
                                break;
                        case 1:
                                sep->se_rpc_highvers =
                                        sep->se_rpc_lowvers;
                                break;
                        default:
                                syslog(LOG_ERR,
					"bad RPC version specifier; %s\n",
					sep->se_service);
                                freeconfig(sep);
                                goto more;
                        }
                }
                else {
                        sep->se_rpc_lowvers =
                                sep->se_rpc_highvers = 1;
                }
        }
	arg = sskip(&cp);
	if (!strncmp(arg, "wait", 4))
		sep->se_accept = 0;
	else if (!strncmp(arg, "nowait", 6))
		sep->se_accept = 1;
	else {
		syslog(LOG_ERR,
			"%s: bad wait/nowait for service %s",
			CONFIG, sep->se_service);
		goto more;
	}
	sep->se_maxchild = maxchild;
	sep->se_maxcpm = maxcpm;
	if ((s = strchr(arg, '/')) != NULL) {
		char *eptr;
		u_long val;

		val = strtoul(s + 1, &eptr, 10);
		if (eptr == s + 1 || val > MAX_MAXCHLD) {
			syslog(LOG_ERR,
				"%s: bad max-child for service %s",
				CONFIG, sep->se_service);
			goto more;
		}
		sep->se_maxchild = val;
		if (*eptr == '/')
			sep->se_maxcpm = strtol(eptr + 1, &eptr, 10);
		/*
		 * explicitly do not check for \0 for future expansion /
		 * backwards compatibility
		 */
	}
	if (ISMUX(sep)) {
		/*
		 * Silently enforce "nowait" mode for TCPMUX services
		 * since they don't have an assigned port to listen on.
		 */
		sep->se_accept = 1;
		if (strncmp(sep->se_proto, "tcp", 3)) {
			syslog(LOG_ERR,
				"%s: bad protocol for tcpmux service %s",
				CONFIG, sep->se_service);
			goto more;
		}
		if (sep->se_socktype != SOCK_STREAM) {
			syslog(LOG_ERR,
				"%s: bad socket type for tcpmux service %s",
				CONFIG, sep->se_service);
			goto more;
		}
	}
	sep->se_user = newstr(sskip(&cp));
#ifdef LOGIN_CAP
	if ((s = strrchr(sep->se_user, '/')) != NULL) {
		*s = '\0';
		sep->se_class = newstr(s + 1);
	} else
		sep->se_class = newstr(RESOURCE_RC);
#endif
	if ((s = strrchr(sep->se_user, ':')) != NULL) {
		*s = '\0';
		sep->se_group = newstr(s + 1);
	} else
		sep->se_group = NULL;
	sep->se_server = newstr(sskip(&cp));
	if (strcmp(sep->se_server, "internal") == 0) {
		struct biltin *bi;

		for (bi = biltins; bi->bi_service; bi++)
			if (bi->bi_socktype == sep->se_socktype &&
			    strcmp(bi->bi_service, sep->se_service) == 0)
				break;
		if (bi->bi_service == 0) {
			syslog(LOG_ERR, "internal service %s unknown",
				sep->se_service);
			goto more;
		}
		sep->se_accept = 1;	/* force accept mode for built-ins */
		sep->se_bi = bi;
	} else
		sep->se_bi = NULL;
	if (sep->se_maxchild < 0)	/* apply default max-children */
		if (sep->se_bi)
			sep->se_maxchild = sep->se_bi->bi_maxchild;
		else
			sep->se_maxchild = sep->se_accept ? 0 : 1;
	if (sep->se_maxchild) {
		sep->se_pids = malloc(sep->se_maxchild * sizeof(*sep->se_pids));
		if (sep->se_pids == NULL) {
			syslog(LOG_ERR, "Out of memory.");
			exit(EX_OSERR);
		}
	}
	argc = 0;
	for (arg = skip(&cp); cp; arg = skip(&cp))
		if (argc < MAXARGV) {
			sep->se_argv[argc++] = newstr(arg);
		} else {
			syslog(LOG_ERR,
				"%s: too many arguments for service %s",
				CONFIG, sep->se_service);
			goto more;
		}
	while (argc <= MAXARGV)
		sep->se_argv[argc++] = NULL;
#ifdef IPSEC
	sep->se_policy = policy ? newstr(policy) : NULL;
#endif
	return (sep);
}

void
freeconfig(cp)
	struct servtab *cp;
{
	int i;

	if (cp->se_service)
		free(cp->se_service);
	if (cp->se_proto)
		free(cp->se_proto);
	if (cp->se_user)
		free(cp->se_user);
	if (cp->se_group)
		free(cp->se_group);
#ifdef LOGIN_CAP
	if (cp->se_class)
		free(cp->se_class);
#endif
	if (cp->se_server)
		free(cp->se_server);
	if (cp->se_pids)
		free(cp->se_pids);
	for (i = 0; i < MAXARGV; i++)
		if (cp->se_argv[i])
			free(cp->se_argv[i]);
#ifdef IPSEC
	if (cp->se_policy)
		free(cp->se_policy);
#endif
}


/*
 * Safe skip - if skip returns null, log a syntax error in the
 * configuration file and exit.
 */
char *
sskip(cpp)
	char **cpp;
{
	char *cp;

	cp = skip(cpp);
	if (cp == NULL) {
		syslog(LOG_ERR, "%s: syntax error", CONFIG);
		exit(EX_DATAERR);
	}
	return (cp);
}

char *
skip(cpp)
	char **cpp;
{
	char *cp = *cpp;
	char *start;
	char quote = '\0';

again:
	while (*cp == ' ' || *cp == '\t')
		cp++;
	if (*cp == '\0') {
		int c;

		c = getc(fconfig);
		(void) ungetc(c, fconfig);
		if (c == ' ' || c == '\t')
			if ((cp = nextline(fconfig)))
				goto again;
		*cpp = (char *)0;
		return ((char *)0);
	}
	if (*cp == '"' || *cp == '\'')
		quote = *cp++;
	start = cp;
	if (quote)
		while (*cp && *cp != quote)
			cp++;
	else
		while (*cp && *cp != ' ' && *cp != '\t')
			cp++;
	if (*cp != '\0')
		*cp++ = '\0';
	*cpp = cp;
	return (start);
}

char *
nextline(fd)
	FILE *fd;
{
	char *cp;

	if (fgets(line, sizeof (line), fd) == NULL)
		return ((char *)0);
	cp = strchr(line, '\n');
	if (cp)
		*cp = '\0';
	return (line);
}

char *
newstr(cp)
	char *cp;
{
	if ((cp = strdup(cp ? cp : "")))
		return (cp);
	syslog(LOG_ERR, "strdup: %m");
	exit(EX_OSERR);
}

#ifdef OLD_SETPROCTITLE
void
inetd_setproctitle(a, s)
	char *a;
	int s;
{
	int size;
	char *cp;
	struct sockaddr_storage sin;
	char buf[80];
	char hbuf[MAXHOSTNAMELEN];

	cp = Argv[0];
	size = sizeof(sin);
	if (getpeername(s, (struct sockaddr *)&sin, &size) == 0) {
		getnameinfo((struct sockaddr *)&sin, size, hbuf, sizeof(hbuf),
			NULL, 0, NI_NUMERICHOST);
		(void) sprintf(buf, "-%s [%s]", a, hbuf);
	} else
		(void) sprintf(buf, "-%s", a);
	strncpy(cp, buf, LastArg - cp);
	cp += strlen(cp);
	while (cp < LastArg)
		*cp++ = ' ';
}
#else
void
inetd_setproctitle(a, s)
	char *a;
	int s;
{
	int size;
	struct sockaddr_storage sin;
	char buf[80];
	char hbuf[MAXHOSTNAMELEN];

	size = sizeof(sin);
	if (getpeername(s, (struct sockaddr *)&sin, &size) == 0) {
		getnameinfo((struct sockaddr *)&sin, size, hbuf, sizeof(hbuf),
			NULL, 0, NI_NUMERICHOST);
		(void) sprintf(buf, "%s [%s]", a, hbuf);
	} else
		(void) sprintf(buf, "%s", a);
	setproctitle("%s", buf);
}
#endif


/*
 * Internet services provided internally by inetd:
 */
#define	BUFSIZE	8192

/* ARGSUSED */
void
echo_stream(s, sep)		/* Echo service -- echo data back */
	int s;
	struct servtab *sep;
{
	char buffer[BUFSIZE];
	int i;

	inetd_setproctitle(sep->se_service, s);
	while ((i = read(s, buffer, sizeof(buffer))) > 0 &&
	    write(s, buffer, i) > 0)
		;
	exit(0);
}

int check_loop(sin, sep)
	struct sockaddr *sin;
	struct servtab *sep;
{
	struct servtab *se2;
	u_short port[2];
	char hbuf[MAXHOSTNAMELEN];

	for (se2 = servtab; se2; se2 = se2->se_next) {
		if (!se2->se_bi || se2->se_socktype != SOCK_DGRAM)
			continue;
		if (sin->sa_family != se2->se_ctrladdr.ss_family)
			continue;
		switch (sin->sa_family) {
		case AF_INET:
			port[0] = ((struct sockaddr_in *)sin)->sin_port;
			port[1] = ((struct sockaddr_in *)&se2->se_ctrladdr)->sin_port;
			break;
#ifdef INET6
		case AF_INET6:
			port[0] = ((struct sockaddr_in6 *)sin)->sin6_port;
			port[1] = ((struct sockaddr_in6 *)&se2->se_ctrladdr)->sin6_port;
			break;
#endif
		default:
			continue;
		}

		if (port[0] == port[1]) {
			getnameinfo(sin, sin->sa_len, hbuf, sizeof(hbuf),
				NULL, 0, NI_NUMERICHOST);
			syslog(LOG_WARNING,
			       "%s/%s:%s/%s loop request REFUSED from %s",
			       sep->se_service, sep->se_proto,
			       se2->se_service, se2->se_proto, hbuf);
			return 1;
		}
	}
	return 0;
}

/* ARGSUSED */
void
echo_dg(s, sep)			/* Echo service -- echo data back */
	int s;
	struct servtab *sep;
{
	char buffer[BUFSIZE];
	int i, size;
	struct sockaddr_storage sin;

	size = sizeof(sin);
	if ((i = recvfrom(s, buffer, sizeof(buffer), 0,
			  (struct sockaddr *)&sin, &size)) < 0)
		return;

	if (check_loop((struct sockaddr *)&sin, sep))
		return;

	(void) sendto(s, buffer, i, 0, (struct sockaddr *)&sin,
		      size);
}

/* ARGSUSED */
void
discard_stream(s, sep)		/* Discard service -- ignore data */
	int s;
	struct servtab *sep;
{
	int ret;
	char buffer[BUFSIZE];

	inetd_setproctitle(sep->se_service, s);
	while (1) {
		while ((ret = read(s, buffer, sizeof(buffer))) > 0)
			;
		if (ret == 0 || errno != EINTR)
			break;
	}
	exit(0);
}

/* ARGSUSED */
void
discard_dg(s, sep)		/* Discard service -- ignore data */
	int s;
	struct servtab *sep;
{
	char buffer[BUFSIZE];

	(void) read(s, buffer, sizeof(buffer));
}

#include <ctype.h>
#define LINESIZ 72
char ring[128];
char *endring;

void
initring()
{
	int i;

	endring = ring;

	for (i = 0; i <= 128; ++i)
		if (isprint(i))
			*endring++ = i;
}

/* ARGSUSED */
void
chargen_stream(s, sep)		/* Character generator */
	int s;
	struct servtab *sep;
{
	int len;
	char *rs, text[LINESIZ+2];

	inetd_setproctitle(sep->se_service, s);

	if (!endring) {
		initring();
		rs = ring;
	}

	text[LINESIZ] = '\r';
	text[LINESIZ + 1] = '\n';
	for (rs = ring;;) {
		if ((len = endring - rs) >= LINESIZ)
			memmove(text, rs, LINESIZ);
		else {
			memmove(text, rs, len);
			memmove(text + len, ring, LINESIZ - len);
		}
		if (++rs == endring)
			rs = ring;
		if (write(s, text, sizeof(text)) != sizeof(text))
			break;
	}
	exit(0);
}

/* ARGSUSED */
void
chargen_dg(s, sep)		/* Character generator */
	int s;
	struct servtab *sep;
{
	struct sockaddr_storage sin;
	static char *rs;
	int len, size;
	char text[LINESIZ+2];

	if (endring == 0) {
		initring();
		rs = ring;
	}

	size = sizeof(sin);
	if (recvfrom(s, text, sizeof(text), 0,
		     (struct sockaddr *)&sin, &size) < 0)
		return;

	if (check_loop((struct sockaddr *)&sin, sep))
		return;

	if ((len = endring - rs) >= LINESIZ)
		memmove(text, rs, LINESIZ);
	else {
		memmove(text, rs, len);
		memmove(text + len, ring, LINESIZ - len);
	}
	if (++rs == endring)
		rs = ring;
	text[LINESIZ] = '\r';
	text[LINESIZ + 1] = '\n';
	(void) sendto(s, text, sizeof(text), 0,
		      (struct sockaddr *)&sin, size);
}

/*
 * Return a machine readable date and time, in the form of the
 * number of seconds since midnight, Jan 1, 1900.  Since gettimeofday
 * returns the number of seconds since midnight, Jan 1, 1970,
 * we must add 2208988800 seconds to this figure to make up for
 * some seventy years Bell Labs was asleep.
 */

long
machtime()
{
	struct timeval tv;

	if (gettimeofday(&tv, (struct timezone *)NULL) < 0) {
		if (debug)
			warnx("unable to get time of day");
		return (0L);
	}
#define	OFFSET ((u_long)25567 * 24*60*60)
	return (htonl((long)(tv.tv_sec + OFFSET)));
#undef OFFSET
}

/* ARGSUSED */
void
machtime_stream(s, sep)
	int s;
	struct servtab *sep;
{
	long result;

	result = machtime();
	(void) write(s, (char *) &result, sizeof(result));
}

/* ARGSUSED */
void
machtime_dg(s, sep)
	int s;
	struct servtab *sep;
{
	long result;
	struct sockaddr_storage sin;
	int size;

	size = sizeof(sin);
	if (recvfrom(s, (char *)&result, sizeof(result), 0,
		     (struct sockaddr *)&sin, &size) < 0)
		return;

	if (check_loop((struct sockaddr *)&sin, sep))
		return;

	result = machtime();
	(void) sendto(s, (char *) &result, sizeof(result), 0,
		      (struct sockaddr *)&sin, size);
}

/* ARGSUSED */
void
daytime_stream(s, sep)		/* Return human-readable time of day */
	int s;
	struct servtab *sep;
{
	char buffer[256];
	time_t clock;

	clock = time((time_t *) 0);

	(void) sprintf(buffer, "%.24s\r\n", ctime(&clock));
	(void) write(s, buffer, strlen(buffer));
}

/* ARGSUSED */
void
daytime_dg(s, sep)		/* Return human-readable time of day */
	int s;
	struct servtab *sep;
{
	char buffer[256];
	time_t clock;
	struct sockaddr_storage sin;
	int size;

	clock = time((time_t *) 0);

	size = sizeof(sin);
	if (recvfrom(s, buffer, sizeof(buffer), 0,
		     (struct sockaddr *)&sin, &size) < 0)
		return;

	if (check_loop((struct sockaddr *)&sin, sep))
		return;

	(void) sprintf(buffer, "%.24s\r\n", ctime(&clock));
	(void) sendto(s, buffer, strlen(buffer), 0,
		      (struct sockaddr *)&sin, size);
}

/*
 * print_service:
 *	Dump relevant information to stderr
 */
void
print_service(action, sep)
	char *action;
	struct servtab *sep;
{
	fprintf(stderr,
	    "%s: %s proto=%s accept=%d max=%d user=%s group=%s"
#ifdef LOGIN_CAP
	    " class=%s"
#endif
	    " builtin=%p server=%s"
#ifdef IPSEC
	    " policy=\"%s\""
#endif
	    "\n",
	    action, sep->se_service, sep->se_proto,
	    sep->se_accept, sep->se_maxchild, sep->se_user, sep->se_group,
#ifdef LOGIN_CAP
	    sep->se_class,
#endif
	    (void *) sep->se_bi, sep->se_server
#ifdef IPSEC
	    , (sep->se_policy ? sep->se_policy : "")
#endif
	    );
}

/*
 *  Based on TCPMUX.C by Mark K. Lottor November 1988
 *  sri-nic::ps:<mkl>tcpmux.c
 */


static int		/* # of characters upto \r,\n or \0 */
getline(fd, buf, len)
	int fd;
	char *buf;
	int len;
{
	int count = 0, n;
	struct sigaction sa;

	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_DFL;
	sigaction(SIGALRM, &sa, (struct sigaction *)0);
	do {
		alarm(10);
		n = read(fd, buf, len-count);
		alarm(0);
		if (n == 0)
			return (count);
		if (n < 0)
			return (-1);
		while (--n >= 0) {
			if (*buf == '\r' || *buf == '\n' || *buf == '\0')
				return (count);
			count++;
			buf++;
		}
	} while (count < len);
	return (count);
}

#define MAX_SERV_LEN	(256+2)		/* 2 bytes for \r\n */

#define strwrite(fd, buf)	(void) write(fd, buf, sizeof(buf)-1)

struct servtab *
tcpmux(s)
	int s;
{
	struct servtab *sep;
	char service[MAX_SERV_LEN+1];
	int len;

	/* Get requested service name */
	if ((len = getline(s, service, MAX_SERV_LEN)) < 0) {
		strwrite(s, "-Error reading service name\r\n");
		return (NULL);
	}
	service[len] = '\0';

	if (debug)
		warnx("tcpmux: someone wants %s", service);

	/*
	 * Help is a required command, and lists available services,
	 * one per line.
	 */
	if (!strcasecmp(service, "help")) {
		for (sep = servtab; sep; sep = sep->se_next) {
			if (!ISMUX(sep))
				continue;
			(void)write(s,sep->se_service,strlen(sep->se_service));
			strwrite(s, "\r\n");
		}
		return (NULL);
	}

	/* Try matching a service in inetd.conf with the request */
	for (sep = servtab; sep; sep = sep->se_next) {
		if (!ISMUX(sep))
			continue;
		if (!strcasecmp(service, sep->se_service)) {
			if (ISMUXPLUS(sep)) {
				strwrite(s, "+Go\r\n");
			}
			return (sep);
		}
	}
	strwrite(s, "-Service not available\r\n");
	return (NULL);
}

#define CPMHSIZE	256
#define CPMHMASK	(CPMHSIZE-1)
#define CHTGRAN		10
#define CHTSIZE		6

typedef struct CTime {
	unsigned long 	ct_Ticks;
	int		ct_Count;
} CTime;

typedef struct CHash {
	struct sockaddr_storage	ch_Addr;
	time_t		ch_LTime;
	char		*ch_Service;
	CTime		ch_Times[CHTSIZE];
} CHash;

CHash	CHashAry[CPMHSIZE];

int
cpmip(sep, ctrl)
	struct servtab *sep;
	int ctrl;
{
	struct sockaddr_storage rsin;
	int rsinLen = sizeof(rsin);
	int r = 0;
	char hbuf[MAXHOSTNAMELEN];

	/*
	 * If getpeername() fails, just let it through (if logging is
	 * enabled the condition is caught elsewhere)
	 */

	if (sep->se_maxcpm > 0 && 
	    getpeername(ctrl, (struct sockaddr *)&rsin, &rsinLen) == 0 ) {
		time_t t = time(NULL);
		int hv = 0xABC3D20F;
		int i;
		int cnt = 0;
		CHash *chBest = NULL;
		unsigned int ticks = t / CHTGRAN;

		{
			char *p;
			int i, l;

			switch (rsin.ss_family) {
			case AF_INET:
				p = (char *)&((struct sockaddr_in *)&rsin)->sin_addr;
				l = sizeof(struct in_addr);
				break;
#ifdef INET6
			case AF_INET6:
				p = (char *)&((struct sockaddr_in6 *)&rsin)->sin6_addr;
				l = sizeof(struct in6_addr);
				break;
#endif
			default:
				p = (char *)&rsin;
				l = rsinLen;
				break;
			}

			for (i = 0; i < l; i++, p++) {
				hv = (hv << 5) ^ (hv >> 23) ^ *p;
			}
			hv = (hv ^ (hv >> 16));
		}
		for (i = 0; i < 5; ++i) {
			CHash *ch = &CHashAry[(hv + i) & CPMHMASK];

			if (rsin.ss_family == ch->ch_Addr.ss_family
			 && rsin.ss_len == ch->ch_Addr.ss_len
			 && memcmp(&rsin, &ch->ch_Addr, rsin.ss_len) == 0
			 && ch->ch_Service
			 && strcmp(sep->se_service, ch->ch_Service) == 0) {
				chBest = ch;
				break;
			}
			if (chBest == NULL || ch->ch_LTime == 0 || 
			    ch->ch_LTime < chBest->ch_LTime) {
				chBest = ch;
			}
		}
		if (rsin.ss_family != chBest->ch_Addr.ss_family
		 || rsin.ss_len != chBest->ch_Addr.ss_len
		 || memcmp(&rsin, &chBest->ch_Addr, rsin.ss_len) != 0
		 || chBest->ch_Service == NULL
		 || strcmp(sep->se_service, chBest->ch_Service) != 0) {
			chBest->ch_Addr = rsin;
			if (chBest->ch_Service)
				free(chBest->ch_Service);
			chBest->ch_Service = strdup(sep->se_service);
			bzero(chBest->ch_Times, sizeof(chBest->ch_Times));
		}
		chBest->ch_LTime = t;
		{
			CTime *ct = &chBest->ch_Times[ticks % CHTSIZE];
			if (ct->ct_Ticks != ticks) {
				ct->ct_Ticks = ticks;
				ct->ct_Count = 0;
			}
			++ct->ct_Count;
		}
		for (i = 0; i < CHTSIZE; ++i) {
			CTime *ct = &chBest->ch_Times[i];
			if (ct->ct_Ticks <= ticks &&
			    ct->ct_Ticks >= ticks - CHTSIZE) {
				cnt += ct->ct_Count;
			}
		}
		if (cnt * (CHTSIZE * CHTGRAN) / 60 > sep->se_maxcpm) {
			r = -1;
			getnameinfo((struct sockaddr *)&rsin, rsinLen,
				hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST);
			syslog(LOG_ERR,
			    "%s from %s exceeded counts/min (limit %d/min)",
			    sep->se_service, hbuf,
			    sep->se_maxcpm);
		}
	}
	return(r);
}
