#include "pwm.h"
#include "rand.h"

#define DARKTH	((unsigned char)20)
#define LIGHTTH	((unsigned char)64)

struct st
{
	unsigned char st1[16];
	unsigned char st2[16];
	unsigned char dark;
};

unsigned char flame2_init(void)
{
	return sizeof(struct st);
}

void flame2_pix(unsigned char pix)
{
	struct st *st = (struct st *)SHBSS;
	unsigned char i;
	unsigned short scan;
	unsigned char *cp = st->st1;
	unsigned char prev = cp[15];
	unsigned char next = cp[1];
	unsigned char here = cp[0];
	unsigned char sum = 0;
	unsigned char *cp2 = st->st2;
	unsigned char dark;

	for(i = 0; i < 16; i++) {
		unsigned char s;

		s = (prev >> 2) + (here >> 1) + (next >> 4);
		sum += s >> 4;
		
		prev = here;
		here = next;
		next = cp[(i+1) & 0xf];
		
		cp2[i] = s;
	}

	{
		unsigned char *t = &st->dark;
		if (sum < DARKTH)
			*t = 1;
		if (sum > LIGHTTH)
			*t = 0;

		dark = *t;
	}

	if (dark || rand() < 16) {
		unsigned char loops;
		unsigned char r = rand() & 0xf;
		unsigned char end;

		/* assembler-like to make sure there's no stray int
		   intermediates */
		loops = (LIGHTTH - sum) & 0x7f;
		loops >>= 2;
		loops++;
		loops &= 0xe;	/* don't loop 16 times */

		end = r + loops;
		end &= 0xf;

		for(i = r; i != end; i = (i + 1) & 0xf) {
			unsigned char t = cp2[i] + 15;
			if (t & 0xf0)
				t = 15;
			cp2[i] = t;
		}
	}			

	for(i = 0, scan = 1; i < 16; i++, scan <<= 1) {
		unsigned char t = *cp++;
		*cp++ = t;
		set_led(scan, t >> 4);
	}
}

