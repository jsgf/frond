#ifndef _ASSERT_H
#define _ASSERT_H

#ifndef NDEBUG
#define assert(x, y)	do { if (!(x)) { _assert_fail(y); } } while(0)

static int _assert_fail(unsigned char y)
{
	long i;

	y = (~y) & 0x0f;

	for(;;) {
		outp(0x50 | y, PORTB);
		for(i = 0; i < 500000; i++)
			;
		outp(0xa0 | y, PORTB);
		for(i = 0; i < 500000; i++)
			;
	}
}

#else
#define assert(x, y)
#endif

#endif /* ASSERT_H */
