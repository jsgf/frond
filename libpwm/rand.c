#include "rand.h"

#if SEED == 0
#define SEED 0x8cf5
#endif

static unsigned short rand_pool = SEED;

unsigned char rand(void)
{
	unsigned short r = rand_pool;

	if (r & 1) {
		r >>= 1;
		r ^= 0x8016;
	} else
		r >>= 1;

	rand_pool = r;

	return r;
}
