/*
 * Display wheel speed in revs per sec bargraph
 */
#include "pwm.h"
#include "golomb.h"
#include <io.h>

#define PPR 64

unsigned char s60ms_init(void)
{
	pix_per_rev = PPR;

	return 0;
}

void s60ms_pix(unsigned char pix)
{
	static unsigned short last = SEC(.060, 1024);
	unsigned char out;
	unsigned short now = time_now();

	out = 0;
	if (last == now)
		out = MAX_LVL;
	last = now + SEC(.060, 1024);

	set_led(~0, out);
}
