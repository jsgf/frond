#include "pwm.h"
#include "rand.h"

struct st
{
	unsigned char st[16];
};

unsigned char flame_init(void)
{
	return sizeof(struct st);
}

void flame_pix(unsigned char pix)
{
	struct st *st = (struct st *)SHBSS;
	unsigned char i;
	unsigned short scan;
	unsigned char *cp = st->st;
	unsigned char p = cp[15];

	for(i = 0; i < 16; i++) {
		unsigned char v = cp[i];
		unsigned char l = p;
		unsigned char r = cp[(unsigned char)(i + 1) & 0xf];

		p = cp[i];
		cp[i] = v/4 + l/4 + r/4;
	}
	if (pix & 1)
		cp[rand() & 0xf] = 255;

	for(i = 0, scan = 1; i < 16; i++, scan <<= 1)
		set_led(scan, *cp++/16);
}

