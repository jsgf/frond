#include "pwm.h"
#include "rand.h"

struct st
{
	unsigned char count[16];
};

unsigned char flicker_init(void)
{
	return sizeof(struct st);
}

static inline unsigned char swap(unsigned char x)
{
	asm("swap %0" : "=r" (x) : "0" (x));

	return x;
}

void flicker_pix(unsigned char pix)
{
	struct st *st = (struct st *)SHBSS;
	unsigned char *p = &st->count[0];
	unsigned char i;
	unsigned short scan;
	
	for(scan = 1, i = 0;
	    i < 16;
	    i++, scan <<= 1, p++) {
		unsigned char t = *p;

		if (t == 0)
			t = rand() & 0xf;
		else
			t--;

		set_led(scan, t);
		*p = t;
	}
}

