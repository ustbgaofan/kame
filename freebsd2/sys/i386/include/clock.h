/*
 * Kernel interface to machine-dependent clock driver.
 * Garrett Wollman, September 1994.
 * This file is in the public domain.
 *
 *	$Id: clock.h,v 1.21.2.1 1997/01/18 16:16:41 kato Exp $
 */

#ifndef _MACHINE_CLOCK_H_
#define	_MACHINE_CLOCK_H_

#if defined(I586_CPU) || defined(I686_CPU)
#define CPU_CLOCKUPDATE(otime, ntime)	cpu_clockupdate((otime), (ntime))
#else
#define CPU_CLOCKUPDATE(otime, ntime)	(*(otime) = *(ntime))
#endif

#define CPU_THISTICKLEN(dflt) dflt

#define	I586_CTR_COMULTIPLIER_SHIFT	20
#define	I586_CTR_MULTIPLIER_SHIFT	32

#ifdef KERNEL
/*
 * i386 to clock driver interface.
 * XXX almost all of it is misplaced.  i586 stuff is done in isa/clock.c
 * and isa stuff is done in i386/microtime.s and i386/support.s.
 */
extern int	adjkerntz;
extern int	disable_rtc_set;
#if defined(I586_CPU) || defined(I686_CPU)
extern u_int	i586_ctr_bias;
extern u_int	i586_ctr_comultiplier;
extern u_int	i586_ctr_freq;
extern u_int	i586_ctr_multiplier;
#endif
extern int	statclock_disable;
extern u_int	timer_freq;
extern int	timer0_max_count;
extern u_int	timer0_overflow_threshold;
extern u_int	timer0_prescaler_count;
extern int	wall_cmos_clock;

/*
 * Driver to clock driver interface.
 */
struct clockframe;

void	DELAY __P((int usec));
int	acquire_timer0 __P((int rate,
			    void (*function)(struct clockframe *frame)));
int	acquire_timer2 __P((int mode));
int	release_timer0 __P((void));
int	release_timer2 __P((void));
#ifndef PC98
int	rtcin __P((int val));
#else
int	acquire_timer1 __P((int mode));
int	release_timer1 __P((void));
#endif
int	sysbeep __P((int pitch, int period));

#ifdef CLOCK_HAIR

#ifdef PC98
#include <pc98/pc98/pc98.h>		/* XXX */
#else
#include <i386/isa/isa.h>		/* XXX */
#endif
#include <i386/isa/timerreg.h>		/* XXX */

static __inline u_int
clock_latency(void)
{
	u_char high, low;

	outb(TIMER_MODE, TIMER_SEL0 | TIMER_LATCH);
	low = inb(TIMER_CNTR0);
	high = inb(TIMER_CNTR0);
	return (timer0_prescaler_count + timer0_max_count
		- ((high << 8) | low));
}

#if defined(I586_CPU) || defined(I686_CPU)
/*
 * When we update `time', on i586's we also update `i586_ctr_bias'
 * atomically.  `i586_ctr_bias' is the best available approximation to
 * the value of the i586 counter (mod 2^32) at the time of the i8254
 * counter transition that caused the clock interrupt that caused the
 * update.  clock_latency() gives the time between the transition and
 * the update to within a few usec provided another such transition
 * hasn't occurred.  We don't bother checking for counter overflow as
 * in microtime(), since if it occurs then we're close to losing clock
 * interrupts.
 */
static __inline void
cpu_clockupdate(volatile struct timeval *otime, struct timeval *ntime)
{
	if (i586_ctr_freq != 0) {
		u_int i586_count;	/* truncated */
		u_int i8254_count;

		disable_intr();
		i8254_count = clock_latency();
		i586_count = rdtsc();
		i586_ctr_bias = i586_count
				- (u_int)
				  (((unsigned long long)i586_ctr_comultiplier
				    * i8254_count)
				   >> I586_CTR_COMULTIPLIER_SHIFT);
		*otime = *ntime;
		enable_intr();
	} else
		*otime = *ntime;
}
#endif /* I586_CPU || I686_CPU */

#endif /* CLOCK_HAIR */

#endif /* KERNEL */

#endif /* !_MACHINE_CLOCK_H_ */
