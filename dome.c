/* Dome wavefront */

//#include <progmem.h>
#include <string.h>
#include "pwm.h"
#include "rand.h"
#include "golomb.h"

enum state {
	IDLE,			/* all off */
	WOOGLE,			/* making a small, local show */
	OUCH,			/* we have been poked */
	BROOD,			/* wait until its time to poke our neighbours */
	POKE,			/* poke our neighbours */
	FLASHY,			/* keep flashing */
	NUMB,			/* ignore pokes */
};

#ifdef TESTRIG
#include <stdio.h>

static const char *stname(enum state s)
{
	switch(s) {
#define S(x)	case x: return #x;
		S(IDLE);
		S(WOOGLE);
		S(OUCH);
		S(BROOD);
		S(POKE);
		S(FLASHY);
		S(NUMB);
#undef S
	}
	return "???";
}

static enum state _prev_state = -1;

#define DIM	0
#define TEST(x)	if (0 && st->state != _prev_state) { x; _prev_state = st->state; }
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

#define SHORTPOKE	2	/* shortest POKE time */
#define POKERAND	7	/* random bump on SHORTPOKE */

#define BROODSCALE	2	/* scale on our poketime we spend brooding */
#define BROODRAND	15	/* random bump on brooding time */

#define FLASHTIME      	25	/* time we keep flashing after a poke */
#define FLASHRAND	31	/* random bump on FLASHTIME */

#define NUMBTIME	25	/* time we spend ignoring POKEs */
#define NUMBRAND	31	/* random bump on NUMBTIME */

#define STUBBORNNUMB	0	/* if true, NUMB stays while peek is
				   asserted, even after timeout */
#define LOWTEMP		5	/* cellauto temp when in WOOGLE */

#define RARE()		(rand() == 0 && rand() == 0)
#define RANDPROB(p)	(rand16() <= ((unsigned short)(65536 * (p))))


struct st
{
	unsigned char timeout;	/* time remaining for next timeout */
	unsigned char state;	/* our current state */

	unsigned char poketime;	/* length of poke; also used as cellauto temp */

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
	unsigned char i;
	unsigned short l;
			
	for(i = 1, l = (1 << 1); i < 16; i++, l <<= 1) {
		unsigned char led = st->ledstate[i] >> (4+DIM);
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

	TEST(printf("state=%s peek=%d timeout=%d\n", stname(st->state), peek, st->timeout));

	/* action */
	switch(st->state) {
	case IDLE:
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
		cellauto(pix, st, st->poketime);
		if ((pix & 0x07) == 0)
			st->poketime++;
		break;

	case NUMB:
		/* fading display as timeout approaches 0 */
		//cellauto(pix, st, -LOWTEMP);
		fade(st);
		break;
	}

	peek = getpeek();

	/* transition */
	switch(st->state) {
	case IDLE:
		if (peek) {
			st->state = OUCH;
			st->poketime = 1;
		}

		if (RANDPROB(WOOGLEPROB)) {
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
			st->state = OUCH;
			st->poketime = 1; /* start - at least one time-unit of poke */
		}
		break;

	case OUCH:
		if (peek)
			st->poketime++;
		else {
		  pretend_poke:
			st->state = BROOD;
			st->timeout = st->poketime * BROODSCALE + (rand() & BROODRAND);
			st->poketime = LOWTEMP;
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
			st->timeout = NUMBTIME+(rand() & NUMBRAND);
		}
		break;

	case NUMB:
		if (st->timeout) {
			st->timeout--;
		} else if (!(STUBBORNNUMB && peek))
			st->state = IDLE;
		break;
	}

	TEST(printf("   state now %s\n", stname(st->state)));
}

