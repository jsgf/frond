#include "pwm.h"
#include "rand.h"
#include "golomb.h"

struct st
{
};

unsigned char testpat_init(void)
{
	return sizeof(struct st);
}

/*
 * Test pattern for verifying a newly made board
 *
 * The CPU is presumeably minimally OK, otherwise this wouldn't work
 * at all.  Therefore, we're mainly concerned about the wiring between
 * the CPU and the LED driver, and the wiring between the driver and
 * the LEDs themselves.
 *
 * Also, the IR input should really be tested, as well as some way of
 * showing the IR LEDs are working.
 *
 * Use 50% brightness where possible in case the IR carrier modulation
 * is shorted to ground.
 *
 * Pattern:
 * 1. all leds on at 50% bright
 * 2. all LEDs off (blink led 15)
 * 3. turn LEDs on one at a time
 * 4. all on (50%) except for 1
 * 5. ramp
 * 6. all fade up
 * 7. all fade down
 * 8-16. IR input level (bar graph average & bright LED for last sample)
 */
void testpat_pix(unsigned char pix)
{
	unsigned char i;
	unsigned short l;
	unsigned char phase = pix >> 4;
	unsigned char step = pix & 0x0f;

	if (phase == 0) {
		set_led(~0, MAX_LVL/2);
	} else if (phase == 1) {
		if (step & 1)
			set_led(1 << 15, MAX_LVL/2);
	} else if (phase == 2) {
		set_led(1 << step, MAX_LVL);
	} else if (phase == 3) {
		set_led(~(1 << step), MAX_LVL/2);
	} else if (phase == 4) {
		for(i = 0, l=1; i < 16; i++, l <<= 1)
			set_led(l, i);
	} else if (phase == 5) {
		set_led(~0, step);
	} else if (phase == 6) {
		set_led(~0, MAX_LVL-step);
	} else {
		unsigned short ir = 1 << (ir_input() >> 3);
		unsigned short iravg = ((1 << (ir_avg() >> 3)) - 1) & ~ir;
		
		ir &= ~1;
		iravg &= ~1;

		set_led(iravg, MAX_LVL/4);
		set_led(ir, MAX_LVL);
	}
}

