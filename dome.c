/* Dome wavefront */

//#include <progmem.h>
#include <string.h>
#include "pwm.h"
#include "rand.h"
#include "golomb.h"

struct st
{
	unsigned char peektime;	/* time remaining before we're sensitive to peek */
	unsigned char poketime;	/* time before we poke our neighbours */
};

unsigned char dome_init(void)
{
	return sizeof(struct st);
}

#define PEEKTIME	100	/* time between peeks */
#define POKETIME	10	/* peek->poke delay */

#define POKEPULSE	4	/* poke pulse length */

void dome_pix(unsigned char pix)
{
	struct st *st = (struct st *)SHBSS;

	if (!st->peektime) {
		/* sensitive to peeks */
		if (getpeek() || (rand()==0 && rand()==0 && rand() < 2)) {
			st->peektime = PEEKTIME + (rand() & 0x1f);
			st->poketime = POKEPULSE + POKETIME + (rand() & 0x0f);
		}
	} else {
		/* our peek has been poked */
		st->peektime--;
		if (st->poketime) {
			if (st->poketime-- < POKEPULSE)
				setpoke(1);
		}
	}

	set_led(0xff, st->poketime);
	set_led(0xff00, st->peektime);
}

