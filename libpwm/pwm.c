/* AVR interrupts */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>
#include <string.h>

/* Dim over-bright patterns - necessary if LEDs are on 6v and 62706 is
   overheating.  This is pretty expensive in code and memory use.
*/
#define AUTODIM	0

#define IRCOMM 0


#define PWM_C
#define USE_FLASH

#define	PWM_SCALE	9	/* larger values make the IR pulse trains longer */

#include "pwm.h"
#include "golomb.h"
#include "div.h"
#include "misc.h"
#include "rand.h"

#ifndef FRONDS2
#define FRONDS2 0
#endif

#if FRONDS2
//#define IR_LED	(1 << 0)		/* bottom LED is IR */

/* Port 1 */
#define SIN	(1 << 0)		/* serial out */
#define CLK	(1 << 1)		/* clock */
#define LAT	(1 << 2)		/* latch */
#define nENA	(1 << 3)		/* enable (active low) */
#define PWRCTL	(1 << 4)		/* Control power */

#define CMPDRV	0

#define PORT1	PORTB
#define DDR1	DDRB
#define OUT1	(SIN|CLK|nENA|PWRCTL|LAT)
#define PULLUP1	(0)

/* Port 2 */
#define RXD	(1 << 0)
#define TXD	(1 << 1)
#define IRIN	(1 << 2)
#define POKE	(1 << 3)
#define PEEK	(1 << 4)

#define PORT2	PORTD
#define PORT2IN	PIND
#define DDR2	DDRD
#define OUT2	(0)
#define PULLUP2	(PEEK | POKE | IRIN)

#else  /* !FRONDS2 */
#define IR_LED	0

/* Port 1 */
#if 0
#define SIN	(1 << 7)		/* serial out */
#define CLK	(1 << 6)		/* clock */
#define nENA	(1 << 5)		/* enable (active low) */
#define	CMPDRV	(1 << 4)		/* comparator power */
#define PWRCTL	(1 << 3)		/* Control power */
#define LAT	(1 << 2)		/* latch */
#else
/* swap nENA and PWRCTL to match Fronds2 */
#define SIN	(1 << 7)		/* serial out */
#define CLK	(1 << 6)		/* clock */
#define nENA	(1 << 3)		/* enable (active low) */
#define	CMPDRV	(1 << 4)		/* comparator power */
#define PWRCTL	(1 << 5)		/* Control power */
#define LAT	(1 << 2)		/* latch */
#endif

#define PORT1	PORTB
#define DDR1	DDRB
#define OUT1	(SIN|CLK|nENA|CMPDRV|PWRCTL|LAT)
#define PULLUP1	(0)

/* Port 2 */
#define IRIN	(1 << 2)
#define POKE	(1 << 3)

#define PORT2	PORTD
#define DDR2	DDRD
#define OUT2	(POKE)
#define PULLUP2	(0)
#endif /* FRONDS2 */

#include "assert.h"

/* Carrier frequency for IR receiver (timer 1 ticks between transitions) */
#define CARRIER_DIV	PS_1
#define IRCARRIER	(SEC(1. / (56000 * 2), 1) - 1)

/* deliberately undefined to make sure that various compile-time
 * constraints are met */
extern void __error(void);

/* s seconds in units of PWM cycles (p = PWM clock prescale) */
#define PWMSEC(s,p)	((unsigned char)(((s) * CPUFREQ) / (PWM_LENGTH * (p))))

static volatile unsigned char pixel;		/* which pixel for this pattern */
static unsigned char pixelcount = 1;		/* count until next pixel */
static unsigned char pixeltick = PWMSEC(.02,64);	/* PWM cycles/pixel */
//static unsigned char pixeltick = 5;	/* PWM cycles/pixel */

#define HI(x)	((unsigned char)((x)>>8))
#define LO(x)	((unsigned char)(x))

/*
 * Oh, the slavery of code-size.  we know that base is small,
 * and therefore sizeof(*base)*idx will also fit into 8
 * bits, but the compiler doesn't so it wastes instructions
 * and effort on 16-bit calculations for the index.
 * Circumvent it here.  (Saves an instruction per use)
 *
 * This will break with more than 256 bytes of SRAM.
 */
#if defined(AVR_AT90S2313) && 1
#define IDX(base, idx) ((typeof(&(base)[0]))((unsigned char *)(base) + (unsigned char)(idx * sizeof(base[0]))))
#else
#define IDX(base, idx)	(&(base)[idx])
#endif



static unsigned short ledstate;

inline static unsigned short leds_in(void)
{
	return ledstate;
}

/* LED out for TB62706 output driver,rev C/2 board */
static inline void leds_out(unsigned short led)
{
	unsigned char i;

	ledstate = led;

	for(i = 8; i > 0; i--) {
		unsigned char reg;

		if (SIN == (1 << 7)) {
			reg = (PULLUP1|PWRCTL|CMPDRV) << 1;
			asm ("rol %B0" "\n\t"
			     "ror %1" :
			     "+r" (led), "+r" (reg) : : "cc");
		} else if (SIN == (1 << 0)) {
			reg = (PULLUP1|PWRCTL|CMPDRV) >> 1;
			asm ("rol %B0" "\n\t"
			     "rol %1" :
			     "+r" (led), "+r" (reg) : : "cc");
		} else
			__error();
			
		outp(reg, PORT1);
		
		reg |= CLK;
		
		outp(reg, PORT1);
	}

	for(i = 8; i > 0; i--) {
		unsigned char reg;
		if (SIN == (1 << 7)) {
			reg = (PULLUP1|PWRCTL|CMPDRV) << 1;
			asm ("rol %A0" "\n\t"
			     "ror %1" :
			     "+r" (led), "+r" (reg) : : "cc");
		} else if (SIN == (1 << 0)) {
			reg = (PULLUP1|PWRCTL|CMPDRV) >> 1;
			asm ("rol %A0" "\n\t"
			     "rol %1" :
			     "+r" (led), "+r" (reg) : : "cc");
		} else
			__error();

		outp(reg, PORT1);
		
		reg |= CLK;
		
		outp(reg, PORT1);
	}
	outp(PULLUP1|LAT|PWRCTL|CMPDRV, PORT1); /* assert latch */
}

struct golomb_pwm
{
	unsigned short leds;
};

static struct pwmstate {
	struct golomb_pwm showing[PWM_MARKS];
	struct golomb_pwm drawing[PWM_MARKS];
	unsigned char idx;	/* current mark in showing cycle */
} pwmstate;

#define PS_STOP	0
#define PS_1	1
#define PS_8	2
#define PS_64	3
#define PS_256	4
#define PS_1024	5

/* Expects to be called with interrupts disabled; leaves them disabled
   when done */
inline static void start_pwm(void)
{
	outp(-1, TCNT0);
	sbi(TIMSK, TOIE0);			/* Timer 0 overflow */
	pwmstate.idx = 0;
	outp(PS_64, TCCR0);			/* 64 prescale */
	outp(PULLUP1|PWRCTL|nENA|CMPDRV, PORT1);/* power on LED driver, no output */
}

/* Stop PWM timer */
inline static void stop_pwm(void)
{
	outp(PS_STOP, TCCR0);
	cbi(TIMSK, TOIE0);
	leds_out(0);
	outp(PULLUP1|CMPDRV, PORT1);	/* power off LED driver; turn on light meter */
}

/* Start LED carrier modulation */
inline static void start_carrier(void)
{
	/* configure timer 1 with:
	   - prescale of 1
	   - output match
	   - auto-zero
	   - output toggle
	*/

	outp((1<<COM1A0), TCCR1A);		/* OC1 output toggle */
	outp(0, TCNT1H);			/* reset counter */
	outp(0, TCNT1L);
	outp(HI(IRCARRIER), OCR1H);		/* set counter match */
	outp(LO(IRCARRIER), OCR1L);
	outp(CARRIER_DIV | (1<<CTC1), TCCR1B);	/* start timer; enable 0 on match */
}

/* Stop LED carrier modulation */
static inline void stop_carrier(void)
{
	outp(PS_STOP, TCCR1B);			/* stop counter */
	outp(0, TCCR1A);			/* disconnect output */
}

/* timer is updated every time timer0 overflows (lastdelta - snapshot
 * of current timer0 value + timer gives instantaneous timestamp) */
static unsigned char timer;
static signed char prevdelta;

/* Scale factor to stop expected intervals from 
 * overrunning 8 bits of timer. */
#define TIMERSHIFT	1

/* PWM timebase */
SIGNAL(SIG_OVERFLOW0)
{
//	assert(idx >= 0 && idx < PWM_MARKS, 1);

	/* Update LED state for this mark */
	{
		unsigned char idx = pwmstate.idx;
		unsigned short t1;

		t1 =  pwmstate.showing[idx].leds;
		if (idx != 0)
			t1 ^= leds_in();
		leds_out(t1);
	}

	/* Set up timer for next mark */
	{
		unsigned char idx = pwmstate.idx;
		signed char delta;

		timer -= prevdelta;	/* prevdelta is -ve */

		delta = lpm(pwm_delta + idx);
		outp(delta, TCNT0);
		prevdelta = delta >> TIMERSHIFT;
		
		idx++;

		if (idx == PWM_MARKS) {
			idx = 0;
			if (--pixelcount == 0) {
				pixel++;
				pixelcount = pixeltick;
			}
		}
		pwmstate.idx = idx;
	}
}

enum edge { falling = 0, rising = 1 };
static unsigned char edge;

static inline unsigned char now()
{
	signed char t = inp(TCNT0); /* t is -ve */

	t >>= TIMERSHIFT;
	t -= prevdelta;		/* Compute time since start of this PWM
				 * segment to now (rather than time
				 * until next segment) */

	return timer + t;
}

static unsigned char ir_reading;

// input interrupt from IR receiver on external INT0
// on rising edge, sets ir_time to low period
SIGNAL(SIG_INTERRUPT0)
{
#if IRCOMM
	static unsigned char last;
	unsigned char t;

#define DEBUG 0

	t = now();

	if (edge == falling) {
		if (DEBUG)
			outp(PULLUP2, PORT2); /* debug output */
		outp((1<<SE) | (3<<ISC00), MCUCR);	/* rising edge */
		edge = rising;
		last = t;
	} else {
		if (DEBUG)
			outp(POKE | PULLUP2, PORT2); /* debug output */
		outp((1<<SE) | (2<<ISC00), MCUCR);	/* falling edge */
		edge = falling;

		ir_reading = t - last;
	}
#undef DEBUG

#endif /* IRCOMM */
}

/* weighted average (most recent samples get heavier weighting) */
unsigned char ir_avg(void)
{
#if IRCOMM
	static unsigned char ir_weight;
	unsigned char i = ir_reading;

	if (i < ir_weight) {
		i = ir_weight - i;
		if (i < 8)
			i += 7;
		i /= 8;
		ir_weight -= i;
	} else {
		i = i - ir_weight;
		if (i < 8)
			i += 7;
		i /= 8;
		ir_weight += i;
	}
	return ir_weight;
#else	/* !IRCOMM */
	return 0;
#endif	/* IRCOMM */
}

unsigned char ir_input(void)
{
	return ir_reading;
}

#if AUTODIM
/* Dim output if the average intensity over NHIST frames exceeds
 * THRESH.  Output is dimmed by DIMSCALE over DIMTIME frames. */
#define NHIST		((unsigned char)4)
#define THRESH		140	/* threshhold for average over NHIST frames */
#define DIMSCALE	2
#define DIMTIME		10	/* number of frames of dimness after hitting thresh */

static unsigned char hist[NHIST];
static unsigned char dimming;
#endif /* AUTODIM */

/* output level of IR led */
static unsigned char ir_level;

/* Set a set of LEDs to a particular level; assumes that they have not been
   set to anything else since last clear. */
static void _set_led(unsigned short ledmask, unsigned char bright)
{
	unsigned char start, stop;
	const struct golomb_map *m;
	struct golomb_pwm *setup;

	setup = pwmstate.drawing;

	if (bright < MIN_LVL) {
		unsigned char i;
		
		ledmask = ~ledmask;

		for(i = 0; i < PWM_MARKS; i++, setup++)
			setup->leds &= ledmask;
		return;
	}
	
	if (bright > MAX_LVL)
		bright = MAX_LVL;

	assert(bright >= MIN_LVL && bright <= MAX_LVL, 4);

	bright -= MIN_LVL;

#if AUTODIM
	{
		unsigned short l = ledmask;

		/* 
		 * Accumulate intensity for this frame.  This doesn't
		 * overflow because we only have 16 leds and the
		 * brightness is always at most MAX_LVL-MIN_LVL
		 */
		do {
			if (l & 1)
				hist[0] += bright;
			l >>= 1;
		} while(l);

		/* dim output if we were previously too bright */
		if (dimming)
			bright /= DIMSCALE;
	}
#endif /* AUTODIM */

	m = IDX(golomb_map, bright);
	start = lpm(&m->start);
	stop = lpm(&m->stop);

	assert(m >= golomb_map && m < golomb_map+MAX_LVL+1-MIN_LVL, 5);

	IDX(setup, start)->leds |= ledmask;
	if (stop != 0)
		IDX(setup, stop)->leds |= ledmask;
}

void set_led(unsigned short ledmask, unsigned char bright)
{
	if (IRCOMM && (ledmask & IR_LED)) {
		/* allow main loop to control IR LED xmit frequency */
		ir_level = bright;
		ledmask &= ~IR_LED;
	}

	_set_led(ledmask, bright);
}

/* Flips buffers
 * Expects interrupts disabled; leaves them disabled
 */
static inline void flip(void)
{
	memcpy(pwmstate.showing, pwmstate.drawing, sizeof(pwmstate.showing));

	pwmstate.idx = 0;	/* restart pwm cycle */

#if AUTODIM
	{
		/* compute sum of history and see if we're over the 
		   threshhold */
		unsigned char i, s;
		unsigned char *hp = hist;

		for(s = i = 0; i < NHIST; i++)
			s += *hp++ / NHIST;

		if (dimming)
			dimming--;

		if (s >= THRESH)
			dimming = DIMTIME;

		hp--;		/* hp = &hist[NHIST-1] */
		for(i = NHIST; i > 1; i--, hp--)
			hp[0] = hp[-1];
		hist[0] = 0;
	}
#endif /* AUTODIM */
}

/*
 * State machine:
 */
#define ST_OFF		0	/* power down */
#define ST_ON		1	/* power up */
#define ST_SWITCH	2	/* switch to a new gizmo */

static volatile unsigned char state = ST_ON;

/* Whether we're poking or not */
static unsigned char pokestate;

void pwm_run(void)
{
	giz_pix_t *gizmo = 0;
	
	while(gizmo == 0)
		gizmo = nextgiz();

	/* Chip init */
	cli();
	outp(OUT1, DDR1);
	outp(OUT2, DDR2);
	outp(PULLUP2, PORT2);

	outp((1<<ACD), ACSR);	/* disable comparator */

	/* clear unwanted timer interrupts */
	outp((1<<TOV1) | (1<<OCF1A) | (1<<TOV0), TIFR);

	for(;;)
	{
		/* Make sure everything set up for powerdown */
		stop_carrier();
		stop_pwm();
		outp(0, GIMSK);	/* disable INT0 */
		outp(1 << INTF0, GIFR);	/* clear spurious pending */

		/* Enable sleep, power-down */
		outp((1<<SM)|(1<<SE), MCUCR);

		/* turn everything off */
		if (1) {
			/* turn off led driver */
			outp(PULLUP1|CMPDRV, PORT1);
		} else {
			/* leave LED driver on but dark */
			outp(PULLUP1|CMPDRV|PWRCTL|nENA, PORT1);
		}

		/* Go idle until we are woken up */
		WAITWHILE(state == ST_OFF);

		if (state == ST_ON) {
			//pixeltick = PWMSEC(1./5,64);
			//pixeltick = (rand() & 0x1f) + PWMSEC(1./48,64);
			//pixeltick = (rand() & 0x3fff) | 0x3f;

			start_carrier();		/* start LED modulation */
			start_pwm();			/* start PWM timebase */

			edge = falling;
			outp((1<<SE)|(2<<ISC00), MCUCR);	/* disable powerdown; falling edge */
			outp((1<<INTF0), GIFR);			/* clear spurious pending */
			outp((1<<INT0), GIMSK);			/* enable int0 */
			sei();

			for(;;) {
				unsigned char pix;
				unsigned char st;

				_set_led(~0, 0); /* clear frame */
				pix = pixel;
				(*gizmo)(pix);
				
				/* output IR at random intervals */
				if (IRCOMM && (rand() < (unsigned char)(.2 * 256))) /* 20% */
					_set_led(IR_LED, ir_level);

				/* Wait for either
				 * - power to go off,
				 * or
				 * - it's time for the pixel to change
				 */
				cli();
				WAITWHILE((st = state) == ST_ON && 
					  pix == pixel);

				if (st != ST_ON)
					break;
			
				flip();
				
				if (pokestate) {
					/* assert POKE */
					outp(PULLUP2 & ~POKE, PORT2);	/* clear POKE */
					outp(POKE, DDR2);		/* make output */
				} else {
					/* deassert POKE */
					outp(PULLUP2|POKE, PORT2);	/* with pullup */
					outp(OUT2, DDR2);		/* make POKE an input */
				}
				pokestate = 0; /* need explicit re-triggering */

				sei();
			}
			/* interrupts disabled here */

			{
				giz_pix_t *g = nextgiz();

				if (g != 0)
					gizmo = g;
			}

			if (state == ST_SWITCH)
				state = ST_ON;
		}
	}
}

/* poke and peek signals are active-low */
unsigned char getpeek()
{
	unsigned char p = inp(PORT2IN);

	p &= PEEK;

	return !p;
}

void setpoke(unsigned char p)
{
	pokestate = p;
}
