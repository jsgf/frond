
#include "pwm.h"
#include "golomb.h"
#include <io.h>

struct st
{
};

unsigned char count_init(void)
{
	pix_per_rev = 50;
	return sizeof(struct st);
}

void count_pix(unsigned char pix)
{
	set_led(pix<<8, MAX_LVL/2);
}

