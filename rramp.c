#include "pwm.h"
#include "golomb.h"

unsigned char rramp_init(void)
{
	return 0;
}

void rramp_pix(unsigned char pix)
{
	unsigned char i;
	unsigned short j;

	for(j = 1, i = 15; i < 16; i--, j <<= 1)
		set_led(j, i);
}
