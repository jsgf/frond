#include "rand.h"

#if SEED16 == 0
#define SEED16 ((unsigned int)0xc3a8623d)
#endif

static unsigned long rand_pool16 = SEED16;

unsigned short rand16(void)
{
	unsigned long r = rand_pool16;

	if (r & 1) {
		r >>= 1;
		r ^= 0x6055F65b;
	} else
		r >>= 1;

	rand_pool16 = r;

	return r;
}
