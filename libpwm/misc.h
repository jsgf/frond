#ifndef MISC_H
#define MISC_H

/* Enable interrupts and wait for an event;
   leaves with interrupts disabled */
#define WAITWHILE(e)				\
do {						\
	while(e) {				\
		sei();				\
		asm volatile("sleep");		\
		cli();				\
	}					\
	/* leave interrupts disabled */		\
} while(0)

#define CPUFREQ	12000000

/* s seconds in units of a clock prescaled by p */
#define SEC(s, p)	((int)(((s)*CPUFREQ)/(p)))
#define BPS(r, f)	(((f) / (16 * (r))) - 1)

#endif /* MISC_H */
