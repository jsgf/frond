
#include "pwm.h"
#include "golomb.h"

struct st
{
	unsigned char state[16];
};

unsigned char kitt_init(void)
{
	return sizeof(struct st);
}

void kitt_pix(unsigned char pix)
{
	struct st *st = (struct st *)SHBSS;
	unsigned short leds;
	unsigned char i;
	unsigned short scan;

	pix >>= 1;
	if (pix & 0x10)
		pix = ~pix;
	pix &= 0x0f;
	scan = 1 << pix;

	for(leds = 1, i = 0; i < 16; i++, leds <<= 1) {
		unsigned char v = st->state[i];

		if (v > 0)
			v--;
		if (leds & scan)
			v = MAX_LVL;

		set_led(leds, st->state[i] = v);
	}
}

