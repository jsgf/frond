
#ifndef PWM_H
#define PWM_H

extern unsigned char pix_per_rev;
extern unsigned char pix_zero;

typedef unsigned char (giz_init_t)(void);
typedef void (giz_pix_t)(unsigned char);

giz_pix_t *nextgiz();

void pwm_run() __attribute__((noreturn));

void set_led(unsigned short ledmask, unsigned char bright);

/* Shared BSS */
#ifdef TESTRIG
extern void *SHBSS;
#else  /* !TESTRIG */
extern char __bss_end;
#define SHBSS	((void *)&__bss_end)
#include <avr/io.h>
#endif /* TESTRIG */

unsigned char ir_avg(void);
unsigned char ir_input(void);

void set_framerate(unsigned char fr);

#define IR_LED	(1 << 0)		/* bottom LED is IR */

#ifdef USE_FLASH
#define lpm(addr) ({				\
	unsigned char t;			\
	asm (					\
		"lpm" "\n\t"			\
		"mov	%0,r0" "\n\t"		\
		: "=r" (t)			\
		: "z" (addr)			\
	);					\
	t;					\
})
#else
#define lpm(addr)	(*((unsigned char *)addr))
#endif

#endif /* PWM_H */
