
#include "pwm.h"
#include "golomb.h"
#include <io.h>

unsigned char l2k_init(void)
{
	return 0;
}

static unsigned char state[8] = { MAX_LVL };
static unsigned char count = 0;

void l2k_pix(unsigned char pix)
{
	unsigned short leds;
	unsigned char i;
	unsigned char in;
	
	in = ~inp(PIND) & 0x7f;
	for(leds = 1, i = 0; i < 8; i++, leds <<= 1) {
		unsigned char v = state[i];

		if (v > 0 && ((count & 3) == 3))
			v--;
		if (in & 1)
			v = MAX_LVL;

		in >>= 1;
		state[i] = v;
		set_led(leds, v);
	}
	
	if (count++ == 5) {
		count = 0;
		
		for(i = 0; i < 8; i++) {
			unsigned char p = i == 0 ? 7 : i - 1;

			state[p] = state[i];
		}
	}
}
