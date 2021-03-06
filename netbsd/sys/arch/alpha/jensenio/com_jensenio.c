/* $NetBSD: com_jensenio.c,v 1.1.2.1 2000/07/12 20:59:11 thorpej Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: com_jensenio.c,v 1.1.2.1 2000/07/12 20:59:11 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/eisa/eisavar.h>

#include <dev/isa/isavar.h>

#include <alpha/jensenio/jenseniovar.h>

struct com_jensenio_softc {
	struct	com_softc sc_com;	/* real "com" softc */

	/* Jensen-specific goo. */
	void	*sc_ih;			/* interrupt handler */
};

int	com_jensenio_match(struct device *, struct cfdata *, void *);
void	com_jensenio_attach(struct device *, struct device *, void *);
void	com_jensenio_cleanup(void *);

struct cfattach com_jensenio_ca = {
	sizeof(struct com_jensenio_softc), com_jensenio_match,
	    com_jensenio_attach
};

int
com_jensenio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct jensenio_attach_args *ja = aux;

	/* Always present. */
	if (strcmp(ja->ja_name, match->cf_driver->cd_name) == 0)
		return (1);

	return (0);
}

void
com_jensenio_attach(struct device *parent, struct device *self, void *aux)
{
	struct com_jensenio_softc *jsc = (void *)self;
	struct com_softc *sc = &jsc->sc_com;
	struct jensenio_attach_args *ja = aux;
	const char *intrstr;

	sc->sc_iot = ja->ja_iot;
	sc->sc_iobase = ja->ja_ioaddr;

	if (com_is_console(sc->sc_iot, sc->sc_iobase, &sc->sc_ioh) == 0 &&
	    bus_space_map(sc->sc_iot, sc->sc_iobase, COM_NPORTS, 0,
			  &sc->sc_ioh) != 0) {
		printf(": can't map i/o space\n");
		return;
	}

	sc->sc_frequency = COM_FREQ;

	com_attach_subr(sc);

	intrstr = eisa_intr_string(ja->ja_ec, ja->ja_irq[0]);
	jsc->sc_ih = eisa_intr_establish(ja->ja_ec, ja->ja_irq[0],
	    IST_EDGE, IPL_SERIAL, comintr, sc);
	if (jsc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	/*
	 * Shutdown hook for buggy BIOSs that don't recognize the UART
	 * without a disabled FIFO.
	 */
	if (shutdownhook_establish(com_jensenio_cleanup, sc) == NULL)
		panic("com_jensenio_attach: could not establish shutdown hook");
}

void
com_jensenio_cleanup(void *arg)
{
	struct com_softc *sc = arg;

	if (ISSET(sc->sc_hwflags, COM_HW_FIFO))
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, com_fifo, 0);
}
