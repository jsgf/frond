/* Peek wavefront */

//#include <progmem.h>
#include <string.h>
#include "pwm.h"
#include "rand.h"
#include "golomb.h"

struct st
{
};

unsigned char peek_init(void)
{
	return sizeof(struct st);
}

void peek_pix(unsigned char pix)
{
	unsigned char p = getpeek();

	if (pix & 16)
		set_led(0x00f0, MAX_LVL);
	if (p)
		set_led(0xff00, MAX_LVL);
}

