
#include "pwm.h"
#include "golomb.h"

#define B(x)	((x) * MAX_LVL / 100)

//static unsigned char state[8];
static unsigned char state[16] = { 0, B(10), B(20), B(30), B(40), B(50), B(60), B(70) };
static char delta[16] = { 1, 1, 1, 1, 1, 1, 1, 1 };

static void flub_init(void)
{
	pix_per_rev = 50;
}

static void flub_pix(unsigned char pix)
{
	unsigned short leds;
	unsigned char i;
	
	for(leds = 1, i = 0; i < 16; i++, leds <<= 1) {
		state[i] += delta[i];
		set_led(leds, state[i]);
		if (state[i] == 0)
			delta[i] = 1;
		else if (state[i] == MAX_LVL)
			delta[i] = -1;
	}
}

static struct gizmo flub_gizmo = {
	flub_init,
	flub_pix
};

struct gizmo *gizmos[] = { &flub_gizmo };
