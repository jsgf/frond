#include "pwm.h"
#include "rand.h"
#include "golomb.h"

struct st
{
	unsigned short mask, prev;
	unsigned char w;
};

unsigned char xfade_init(void)
{
	return sizeof(struct st);
}

void xfade_pix(unsigned char pix)
{
	struct st *st = (struct st *)SHBSS;
	unsigned short same;
	unsigned char w = pix & 0x0f;

	if (w == 0) {
		st->prev = st->mask;
		st->mask = rand() << 8 | rand();
	}

	same = st->mask & st->prev;
	set_led(same, MAX_LVL);

	same = ~same;

	set_led(st->mask & same, w);
	set_led(st->prev & same, MAX_LVL-w);
}

