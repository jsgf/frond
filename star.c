#include "pwm.h"

#define MIN	0x0080
#define MAX	0x8000

struct st {
	unsigned char phase;
	unsigned char dir;
};

unsigned char star_init(void)
{
	pix_per_rev = 80;
	return sizeof(st);
}

void star_pix(unsigned char pix)
{
	struct st *st = (struct st *)SHBSS;

	if (st->phase == 0) {
		st->phase = MIN;
		st->dir = 1;
	}

	
}
