# KOBJ
#
# Copyright (c) 2000 Cameron Grant <cg@freebsd.org>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# $FreeBSD: src/sys/dev/sound/pcm/feeder_if.m,v 1.1.2.1 2001/02/03 01:29:12 cg Exp $
#

#include <dev/sound/pcm/sound.h>

INTERFACE feeder;

CODE {

	static int
	feeder_noinit(pcm_feeder* feeder)
	{
		return 0;
	}

	static int
	feeder_nofree(pcm_feeder* feeder)
	{
		return 0;
	}

};

METHOD int init {
	pcm_feeder* feeder;
} DEFAULT feeder_noinit;

METHOD int free {
	pcm_feeder* feeder;
} DEFAULT feeder_nofree;

METHOD int set {
	pcm_feeder* feeder;
	int what;
	int value;
};

METHOD int feed {
	pcm_feeder* feeder;
	pcm_channel* c;
	u_int8_t* buffer;
	u_int32_t count;
	struct uio* stream;
};


