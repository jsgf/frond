#include "pwm.h"
#include "rand.h"

struct st
{
	unsigned char prev, count;
	unsigned char state[16];
};

unsigned char echo_init(void)
{
	return sizeof(struct st);
}

void echo_pix(unsigned char pix)
{
	struct st *st = (struct st *)SHBSS;
	unsigned char *sp;
	unsigned char b = ir_input() >> 3;
	unsigned char i;
	unsigned short leds;

	if (b == st->prev) {
		if (++st->count > 4)
			b = 0;
		if (st->count > 30+(rand()&0x3f)) {
			b = rand() & 0xf;
			st->count = 0;
		}
	} else {
		st->prev = b;
		st->count = 0;
	}

	for(i = 0, sp = &st->state[0]; i < 15; i++, sp++)
		*sp = sp[1];
	*sp = b;

	for(i = 0, leds = 1; i < 16; i++, leds <<= 1)
		set_led(leds, st->state[i]);
}

