/*
 * Display wheel speed in revs per sec bargraph
 */
#include "pwm.h"
#include "misc.h"
#include "golomb.h"
#include <io.h>

#define PPR 64

struct st
{
	unsigned short last;
	unsigned char persec;
};

unsigned char rps_init(void)
{
	pix_per_rev = PPR;

	return sizeof(struct st);
}

void rps_pix(unsigned char pix)
{
	unsigned short leds;
	unsigned char i;
	struct st *st = (struct st *)SHBSS;

	if (pix == 0) {
		unsigned short now = time_now();
		unsigned short delta = now - st->last;

		st->persec = (CPUFREQ/1024) / delta;

		st->last = now;
	}

	for(leds = 1, i = 0; i < 16; i++, leds <<= 1)
		set_led(leds, i < st->persec ? MAX_LVL/2 : 0);
}
