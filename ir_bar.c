#include "pwm.h"
#include "golomb.h"
#include <io.h>

struct st
{
};

unsigned char ir_bar_init(void)
{
	pix_per_rev = 50;
	return sizeof(struct st);
}

void ir_bar_pix(unsigned char pix)
{
	unsigned short b = ir_time >> 3;
	b = (1 << b) - 1;
	
	set_led(b, MAX_LVL/2);
}

