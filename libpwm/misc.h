#ifndef MISC_H
#define MISC_H

/* Enable interrupts and wait for an event;
   leaves with interrupts disabled.

   NOTE: The AVR docs say that the sleep will run before any
   interrupts are delivered after the sei.
 */
#define WAITWHILE(e)				\
do {						\
	while(e) {				\
		asm volatile("sei\n"		\
			     "sleep");		\
		cli();				\
	}					\
	/* leave interrupts disabled */		\
} while(0)

#define CPUFREQ	12000000

/* s seconds in units of a clock prescaled by p */
#define SEC(s, p)	((int)(((s)*CPUFREQ)/(p)))
#define BPS(r, f)	(((f) / (16 * (r))) - 1)

#endif /* MISC_H */
