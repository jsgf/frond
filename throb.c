#include "pwm.h"
#include "golomb.h"

struct st
{
	unsigned char level;
	char dir;
};

unsigned char throb_init(void)
{
	return sizeof(struct st);
}

void throb_pix(unsigned char pix)
{
	struct st *st = (struct st *)SHBSS;
	unsigned char l;

	if (st->dir == 0) {
		st->dir = 1;
		st->level = 0;
	}
	
	set_led(~0, st->level);

	l = st->level + st->dir;
	if (l & 0xf0)
		st->dir = -st->dir;
	else
		st->level = l;
}

