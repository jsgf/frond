
#include "pwm.h"
#include "golomb.h"

unsigned char one_init(void)
{
	return 0;
}

void one_pix(unsigned char pix)
{
	set_led(1, MAX_LVL);
}
