#include "pwm.h"
#include "golomb.h"

unsigned char ramp_init(void)
{
	return 0;
}

void ramp_pix(unsigned char pix)
{
	unsigned char i;
	unsigned short j;

	for(j = 1, i = 0; i < 16; i++, j <<= 1)
		set_led(j, i);
}
