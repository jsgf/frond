
#include "pwm.h"
#include "golomb.h"
#include <io.h>

unsigned char full_init(void)
{
	return 0;
}

void full_pix(unsigned char pix)
{
	set_led(0xffff, MAX_LVL);
}
