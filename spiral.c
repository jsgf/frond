/*
 * A spiral
 */
#include "pwm.h"
#include "golomb.h"
#include <io.h>

#define PPR 50

struct st
{
	unsigned char state[16];
};

unsigned char spiral_init(void)
{
	pix_per_rev = PPR;

	return sizeof(struct st);
}

void spiral_pix(unsigned char pix)
{
	struct st *st = (struct st *)SHBSS;
	unsigned short leds;
	unsigned char i;
	unsigned short scan;

	pix >>= 2;
	pix &= 0x7;
	scan = 0x0101 << pix;

	for(leds = 1, i = 0; i < 16; i++, leds <<= 1) {
		char v = st->state[i];

		v -= 2;
		if (v < 0)
			v = 0;

		if (leds & scan)
			v = MAX_LVL;

		set_led(leds, st->state[i] = v);
	}
}
