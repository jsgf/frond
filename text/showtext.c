#include "pwm.h"
#include "golomb.h"
#include "textdata.h"

#define PPR		127
#define CHRSPC		1	/* Pixel space between chars */
#define REV_PER_MSG	10	/* Revs for each msg */

#if 0
static unsigned char len(unsigned char str)
{
	unsigned char len = 0;
	const unsigned char *addr = strdata + str;

	while(lpm(addr++) != 255)
		len++;

	return len;
}
#endif

static inline unsigned char width(unsigned char str)
{
	const unsigned char *addr = strdata + str;
	unsigned char width = 0;
	unsigned char ch;

	while((ch = lpm(addr++)) != 255)
		width += lpm(chardata+ch) + CHRSPC;

	return width;
}

struct st
{
	unsigned char string;
	const unsigned char *strptr;
	const unsigned char *charptr;
	char charleft;
	unsigned char start, end;
};

unsigned char text_init(void)
{
	pix_per_rev = PPR;

	return sizeof(struct st);
}

void text_pix(unsigned char pix)
{
	static unsigned char revs = REV_PER_MSG;
	struct st *st = (struct st *)SHBSS;

	if (pix == 0) {
		unsigned char w;

		if (revs-- == 0) {
			revs = REV_PER_MSG;
			if (lpm(&strings[++st->string]) == 0)
				st->string = 0;
		}

		w = width(st->string);
		st->strptr = strdata + st->string;
		st->start = PPR/2 - w/2;
		st->end = PPR/2 + w/2;
		st->charleft = -CHRSPC;
	}

	if (pix < st->start || pix > st->end)
		return;

	if (st->charleft == -CHRSPC) {
		st->charptr = chardata + lpm(st->strptr++);
		st->charleft = lpm(st->charptr++);
	}

	if (st->charleft > 0)
		set_led(lpm(st->charptr++) << 8, MAX_LVL);
	st->charleft--;
}

