/* Dome wavefront */

//#include <progmem.h>
#include <string.h>
#include "pwm.h"
#include "rand.h"
#include "golomb.h"

enum state {
	POST = 0,		/* self-test */
	IDLE,			/* all off */
	WOOGLE,			/* making a small, local show */
	DEBOUNCE,		/* intermediate state to make sure poke is real */
	OUCH,			/* we have been poked */
	BROOD,			/* wait until its time to poke our neighbours */
	POKE,			/* poke our neighbours */
	FLASHY,			/* keep flashing */
	NUMB,			/* ignore pokes */
	STUCK,			/* poke appears stuck */
};

#ifdef TESTRIG
#include <stdio.h>

static const char *stname(enum state s)
{
	switch(s) {
#define S(x)	case x: return #x;
		S(POST);
		S(IDLE);
		S(WOOGLE);
		S(OUCH);
		S(DEBOUNCE);
		S(BROOD);
		S(POKE);
		S(FLASHY);
		S(NUMB);
		S(STUCK);
#undef S
	}
	return "???";
}

#define DIM	0
#define TEST(x)	if (1 && st->state != st->_prev_state) { x; st->_prev_state = st->state; }
#else  /* !TESTRIG */
#define TEST(x)
#define DIM	1
#endif /* TESTRIG */


#define WOOGLEPROB	(.0002)	/* probability of entering WOOGLE at
				   any particular moment */
#define WOOGLETIME	75	/* time of WOOGLE display */
#define WOOGLERAND	63	/* random bump on WOOGLETIME */
#define EXCITABILITY	(.002)	/* probability of getting so excited
				   in a WOOGLE that we poke our
				   neighbours  */

#define SHORTPOKE	3	/* shortest POKE time */
#define POKERAND	7	/* random bump on SHORTPOKE */

#define BROODSCALE	2	/* scale on our poketime we spend brooding */
#define BROODRAND	15	/* random bump on brooding time */

#define FLASHTIME      	25	/* time we keep flashing after a poke */
#define FLASHRAND	31	/* random bump on FLASHTIME */

#define NUMBTIME	25	/* time we spend ignoring POKEs */
#define NUMBSCALE	2	/* scale NUMB depending on poketime */
#define NUMBRAND	31	/* random bump on NUMBTIME */

#define STUBBORNNUMB	0	/* if true, NUMB stays while peek is
				   asserted, even after timeout */
#define LOWTEMP		5	/* cellauto temp when in WOOGLE */

#define RARE()		(rand() == 0 && rand() == 0)
#define RANDPROB(p)	(rand16() <= ((unsigned short)(65536 * (p))))


struct st
{
	unsigned char state;	/* our current state */
#ifdef TESTRIG
	unsigned char _prev_state;
#endif

	unsigned char timeout;	/* time remaining for next timeout */

	unsigned char poketime;	/* length of poke */
	unsigned char temp;	/* cellauto temperature */

	unsigned char ledstate[16];
	unsigned char next[16];
};

/* just so we don't get more than one rand() */
static inline unsigned char rand()
{
	return rand16();
}

static void led_out(struct st *st, unsigned char dim)
{
	unsigned char *p;
	unsigned short l;

	p = &st->ledstate[1];
	for(l = (1 << 1); l != 0; l <<= 1) {
		unsigned char led = (*p++) >> (4+DIM);
		if (dim)
			led >>= (1+DIM);
		set_led(l, led);
	}
}

static void fade(struct st *st)
{
	unsigned char i;

	for(i = 0; i < 16; i++) {
		if (st->ledstate[i] > 8)
			st->ledstate[i] -= 8;
	}

	led_out(st, 0);
}

static void cellauto(unsigned char pix, struct st *st, signed char ir)
{
	unsigned char i;

	/* copy old state from next, because ledstate may have been cleared by fade() */
	memcpy(st->ledstate, st->next, sizeof(st->ledstate));

	for(i = 0; i < 16; i++)
		if (st->ledstate[i] != 0)
			break;

	if (i == 16) {
		/* all dead; kick */
		for(i = 0; i < 16; i++)
			st->ledstate[i] = rand();
	}

	if (0)
	{
		unsigned char r = rand();
		if (r < 20) {
			/* poke */
			//st->ledstate[rand() & 0xf] += 128;
			st->ledstate[rand() & 0xf] = rand() / 8;
		}
	}

	for(i = 0; i < 16; i++) {
		unsigned short w = st->ledstate[i];
		unsigned char w1;
		unsigned char w2;
#define SCALE	1

		if (ir > 0) {
			w1 = st->ledstate[(i+1*SCALE) & 0xf];
			w2 = st->ledstate[(i+2*SCALE) & 0xf];
		} else {
			w1 = st->ledstate[(i-1*SCALE) & 0xf];
			w2 = st->ledstate[(i-2*SCALE) & 0xf];
		}
		w += w1 * 2;
		w += w2;

		st->next[i] = w / 4 + ir;
	}

	memcpy(st->ledstate, st->next, sizeof(st->ledstate));

	ir = ir < LOWTEMP;
	led_out(st, ir);
}


unsigned char dome_init(void)
{
	return sizeof(struct st);
}

void dome_pix(unsigned char pix)
{
	struct st *st = (struct st *)SHBSS;
	unsigned char peek;

	TEST(printf("state=%s timeout=%d\n", stname(st->state), st->timeout));

	/* action */
	switch(st->state) {
	case POST:
		set_led(1<<(pix/8), MAX_LVL);
		break;

	case IDLE:
	case DEBOUNCE:
		break;

	case OUCH:
	case WOOGLE:
		/* low intensity display */
		cellauto(pix, st, LOWTEMP);
		break;

	case BROOD:
	case POKE:
	case FLASHY:
		/* full display */
		cellauto(pix, st, st->temp);
		if ((pix & 0x07) == 0)
			st->temp++;
		break;

	case NUMB:
		/* fading display as timeout approaches 0 */
		fade(st);
		break;

	case STUCK:
		/* distress blink if input stuck on */
		if (pix < 10)
			set_led(0x8888, MAX_LVL/3);
		break;

	}

	peek = getpeek();
	/* transition */
	switch(st->state) {
	case POST:
		if (pix > 16*8)
			st->state = IDLE;
		break;

	case IDLE:
		if (peek)
			st->state = DEBOUNCE;
		else if (RANDPROB(WOOGLEPROB)) {
			st->state = WOOGLE;
			st->timeout = WOOGLETIME+(rand() & WOOGLERAND);
		}
		break;

	case WOOGLE:
		/* low intensity display */
		if (--st->timeout == 0) {
			st->state = IDLE;

			if (RANDPROB(EXCITABILITY)) {
				/* get excited and pretend we were poked */
				st->poketime = SHORTPOKE + (rand() & POKERAND);
				goto pretend_poke;
			}
		} else if (peek) {
			st->state = DEBOUNCE;
		}
		break;

	case DEBOUNCE:
		if (peek) {
			st->state = OUCH;
			st->poketime = 2; /* two time-units of poke seen */
		} else
			st->state = IDLE;
		break;

	case OUCH:
		if (peek) {
			st->poketime++;
			if (st->poketime == 0)
				st->state = STUCK;
		} else {
			if (st->poketime == 1)
				/* one-off poke - must have been a glitch */
				st->state = IDLE;
			else {
			  pretend_poke:
				st->state = BROOD;
				st->timeout = st->poketime * BROODSCALE + (rand() & BROODRAND);
				st->temp = LOWTEMP;
			}
		}
		break;

	case BROOD:
		if (--st->timeout == 0) {
			st->timeout = st->poketime;
			st->state = POKE;
		}
		break;

	case POKE:
		if (--st->timeout != 0)
			setpoke(1);
		else {
			st->state = FLASHY;
			st->timeout = FLASHTIME+(rand() & FLASHRAND);
		}
		break;

	case FLASHY:
		if (--st->timeout == 0) {
			st->state = NUMB;
			st->timeout = NUMBTIME + st->poketime * NUMBSCALE;
			if (st->timeout < NUMBTIME)
				st->timeout = 255;
		}
		break;

	case NUMB:
		if (st->timeout) {
			st->timeout--;
		} else if (!(STUBBORNNUMB && peek))
			st->state = IDLE;
		break;

	case STUCK:
		if (!peek)
			st->state = IDLE;
		break;
	}

	TEST(printf("   state now %s timeout %d\n", stname(st->state), st->timeout));
}

