//#include <progmem.h>
#include <string.h>
#include "pwm.h"
#include "rand.h"
#include "golomb.h"

struct st
{
	unsigned char state[16];
	unsigned char next[16];
};

unsigned char cellauto_init(void)
{
	return sizeof(struct st);
}

void cellauto_pix(unsigned char pix)
{
	struct st *st = (struct st *)SHBSS;
	unsigned char i;
	signed char ir;

	for(i = 0; i < 16; i++)
		if (st->state[i] != 0)
			break;

	if (i == 16) {
		/* all dead; kick */
		for(i = 0; i < 16; i++)
			st->state[i] = rand();
	}

	{
		unsigned char r = rand();
		if (r < 20) {
			/* poke */
			//st->state[rand() & 0xf] += 128;
			st->state[rand() & 0xf] = rand() / 8;
		}
	}

	ir = ir_avg();
	ir -= 30;
	ir /= 2;

	ir += 2;		/* small bias */

	for(i = 0; i < 16; i++) {
		unsigned short w = st->state[i];
		unsigned char w1;
		unsigned char w2;
#define SCALE	2

		if (ir > 0) {
			w1 = st->state[(i+1*SCALE) & 0xf];
			w2 = st->state[(i+2*SCALE) & 0xf];
		} else {
			w1 = st->state[(i-1*SCALE) & 0xf];
			w2 = st->state[(i-2*SCALE) & 0xf];
		}
		w += w1 * 2;
		w += w2;

		st->next[i] = w / 4 + ir;
	}

	memcpy(st->state, st->next, sizeof(st->state));

	set_led(IR_LED, ir >> 4);

	{
		unsigned short l;

		for(i = 1, l = (1 << 1); i < 16; i++, l <<= 1)
			set_led(l, st->state[i] >> 4);
	}
}

