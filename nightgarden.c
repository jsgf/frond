//#include <progmem.h>
#include "pwm.h"
#include "rand.h"
#include "golomb.h"

struct st
{
	unsigned char phbits;
	unsigned char phprev;
	unsigned char ircount;
	unsigned char previr;

	unsigned char state[8];
	unsigned char next[8];
};

unsigned char nightgarden_init(void)
{
	return sizeof(struct st);
}

static inline void nibset(unsigned char *base, unsigned char idx, unsigned char val)
{
	unsigned char v;
	unsigned char m = 0xf0;

	if (idx & 1) {
		m = 0x0f;
		val <<= 4;
	} 

	idx >>= 1;
	base += idx;

	v = *base;
	v &= m;
	v |= val;

	*base = v;
}

static inline unsigned char nibget(unsigned char *base, unsigned char idx)
{
	unsigned char v = base[idx/2];

	if (idx & 1)
		v >>= 4;

	return v & 0x0f;
}

static unsigned short bits2phase(unsigned char bits)
{
	unsigned short phase = 0;

	if (bits & (1 << 0))
		phase |= 0x0055 & ~IR_LED;
	if (bits & (1 << 2))
		phase |= 0x00aa & ~IR_LED;
	if (bits & (1 << 1))
		phase |= 0x5500 & ~IR_LED;
	if (bits & (1 << 3))
		phase |= 0xaa00 & ~IR_LED;

	return phase;
}

static inline unsigned char mul48(unsigned char a, unsigned char b)
{
	unsigned char ret = 0;
	do {
		if (a & 1)
			ret += b;
		b <<= 1;
		a >>= 1;
	} while(a);

	return ret;
}

/* number of successive identical samples before we give up on IR input */
#define IR_TIMEOUT	4

static inline unsigned char get_ir(struct st *st)
{
	unsigned char ir = ir_avg() >> 4;

	if (ir == st->previr) {
		if (st->ircount == 0)
			return 0; /* IR dead */
		st->ircount--;
	} else {
		st->previr = ir;
		st->ircount = IR_TIMEOUT;
	}

	return ir;
}

/*
 * 4 phases, used in combination:
 *  0x0055	left odd
 *  0x00aa	left even
 *  0x5500	right odd
 *  0xaa00	right even
 *
 * Pattern cross-fades between phases.  Phases are sent and received by IR.
 */
void nightgarden_pix(unsigned char pix)
{
	struct st *st = (struct st *)SHBSS;

	if (st->phbits == 0) {
		unsigned char i;

		for(i = 0; i < sizeof(st->state)/sizeof(*st->state); i++)
			st->state[i] = rand();
	}

	{
		unsigned char i;

		for(i = 0; i < 16; i++) {
			char v;
			unsigned char r;

			v = nibget(st->state, i);

			switch(0) {
			case 0:	/* flicker */
				do {
					r = rand() & 0xf;
				} while (r > 9);
				
				v = v  + r - 4;
				break;

			case 1:	/* all on */
				v = MAX_LVL;
				break;
			}

			if (v < 0)
				v = 0;
			if (v > MAX_LVL)
				v = MAX_LVL;
			/* pattern blah blah */
			
			nibset(st->next, i, v);
		}

		memcpy(st->state, st->next, sizeof(st->state)/sizeof(st->state[0]));
	}

	if ((pix & 0x3f) == 0 || st->phbits == 0) {
		st->phprev = st->phbits;
		do {
			unsigned char ir = get_ir(st);

			if (ir == 0 || rand() < 51)	/* 20% = 51.2 */
				st->phbits = rand();
			else
				st->phbits = ir;

			st->phbits &= 0xf;
		} while(st->phbits == 0);
	} else if ((pix & 0x0f) == 0)
		st->phprev = st->phbits;
		

	/* set IR to current phase */
	set_led(IR_LED, st->phbits);

	{
		unsigned char i;
		unsigned short l;
		unsigned short phase = bits2phase(st->phbits);
		unsigned short ophase = bits2phase(st->phprev);
		unsigned short same = phase & ophase;

		phase &= ~same;
		ophase &= ~same;

		for(i = 1, l = (1 << 1); i < 16; i++, l <<= 1) {
			unsigned char s;
			unsigned char w = pix & 0x0f;

			if (l & same) {
				w = 16;
			} else if (l & phase) {
			} else if (l & ophase) {
				w = 16 - w;
			} else {
				w = 0;
			}

			s = nibget(st->state, i);
			//s *= w;
			s = mul48(s, w);
			s /= 16;

			set_led(l, s);
		}
	}
}

