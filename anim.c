/*
 * Animation
 */
#include "pwm.h"
#include "golomb.h"
#include <io.h>

#include "anim-pic.h"

#define LPM(x)		lpm(x)

static void anim_init(void)
{
	pix_per_rev = LPM(&anim.timebase);
}

static struct
{
	unsigned char bits;	/* next bits */
	unsigned char left;	/* number of bits left */
	unsigned short bitptr;	/* current pointer in bits */
} bits;

static unsigned short getbits(unsigned char n)
{
	unsigned short ret = 0;
	unsigned char b = bits.bits;
	unsigned char l = bits.left;
	unsigned char bptr = bits.bitptr;

	do {
		if (l == 0) {
			const unsigned char *ptr = lz_data + (bptr >> 3);
			b = LPM(ptr);
			l = 8;
		}
		asm (	"lsl %0" "\n\t"
			"rol r28" "\n\t"
			"rol r29"
			: "+r" (b), "+y" (ret) : : "cc");
		l--;
		bptr++;
	} while (--n);

	bits.left = l;
	bits.bits = b;
	bits.bitptr = bptr;

	return ret;
}

static unsigned short getnum(void)
{
	unsigned char bits = MIN_SCALE;

#if MARK_SCALE != 0
	while(getbits(1))
		bits += MARK_SCALE;
#endif
	if (MIN_SCALE != 0 || bits)
		return getbits(bits);
	return 0;
}

static void seek_bitstream(unsigned short bitptr)
{
	unsigned char i;

	bits.bitptr = bitptr;
	bits.left = 0;

	i = bitptr & 0x7;
	if (i)
		getbits(i);
}

static struct state_stk
{
	unsigned short bitptr;		/* return to */
	unsigned char count;		/* count left */
} state_stk[8];
static unsigned char state_sp = 0;

/* Push LZ decompressor state */
static void lz_push(unsigned short delta, unsigned char count)
{
	struct state_stk *sp = state_stk+state_sp;
	unsigned char ptr = bits.bitptr;

	state_sp++;
	sp->bitptr = ptr;
	sp->count = count;
	seek_bitstream(ptr-delta);
}

/* Pop state up to selected level */
static void lz_pop(unsigned char newidx)
{
	unsigned char idx = state_sp;
	struct state_stk *sp;

	for(idx = state_sp; idx > newidx; idx--) {
		sp = state_stk + idx;
		seek_bitstream(sp->bitptr);
	}

	state_sp = idx;
}

struct led_state
{
	unsigned char val;
	unsigned char count;
};

static void anim_pix(unsigned char pix)
{
	static unsigned char frameidx;
	static unsigned char frametime = 1;
	static struct led_state state[16];

	const struct anim_frame *frame;
	unsigned char i;

	i = LPM(anim.timebase);
	while (pix > i)
		pix -= i;

	if (pix == pix_zero && --frametime == 0) {
		struct led_state *lsp;

		frametime = LPM(anim.rate);

		i = frameidx+1;
		if (i >= LPM(anim.nframe))
			i = 0;
		frameidx = i;

		frame = anim.frames + i;

		state_sp = 0;
		lz_push(LPM(&frame->bitidx), LPM(&frame->tokens));

		for(lsp = state, i = 0; i < 16; i++, lsp++)
			lsp->count = 1;
	}

	for(i = 0; i < 16; i++) {
		struct led_state *lsp = state+i;
		if (--(lsp->count) == 0) {
			struct state_stk *stkp;
			unsigned char j;

			lsp->count = LPM(&anim.ratio[i]);
			
			while (getbits(1)) {
				unsigned short delta = getnum();
				unsigned char count = getnum()+MIN_RUN;
				
				lz_push(delta, count);
			} 
			lsp->val = GETPIX();

			stkp = &state_stk[state_sp-1];
			for(j = state_sp; j > 0; j--, stkp--)
				if (--(stkp->count) == 0)
					lz_pop(j-1);
		}
	}

	{ 
		unsigned short leds;
		struct led_state *lsp;

		for(lsp = state, i = 16, leds = 1; i > 0; i--, leds <<= 1, lsp++)
			set_led(leds, lsp->val);
	}
}

static struct gizmo anim_gizmo = {
	anim_init,
	anim_pix
};

struct gizmo *gizmos[] = { &anim_gizmo };
