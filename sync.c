/*
 * Show sync points for calibration (also interesting effect)
 */
#include "pwm.h"
#include "golomb.h"
#include <io.h>

#define PPR 128

unsigned char sync_init(void)
{
	pix_per_rev = PPR;

	return 0;
}

void sync_pix(unsigned char pix)
{
	unsigned short leds;
	unsigned char i;
	unsigned char l;

	l = pix & (PPR/4-1) ? 0 : MAX_LVL;

	for(leds = 1, i = 0; i < 16; i++, leds <<= 1)
		set_led(leds, l);
}
