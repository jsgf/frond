
#include <io.h>

#define MAX 256

#define NB 8
#if 1
static unsigned char b[NB] =	{ 0x7a, 0x1e, 0x54, 0xef, 0x42, 0xca, 0x99, 0xbd };
static const char d[NB] =	{ 1, 2, -1, -3, 2, -1, 3, 2 };
#elif 1
#if 1
static unsigned char b[NB] =	{ 128, 64, 32, 16, 8, 4, 2, 1 };
#else
static unsigned char b[NB] =	{ 1, 2, 4, 8, 16, 32, 64, 128 };
#endif
static const char d[NB] =	{ };
#else
static unsigned char b[NB] =	{ 80, 70, 60, 50, 40, 30, 20, 10 };
static const char d[NB] =	{ };
#endif

static void pwm(unsigned char rep)
{
	while(rep--) {
		register unsigned char c = 0;

		do {
			register unsigned char o = 0;
			register signed char i;
			register unsigned char *cp = b;

			for(i = NB-1; i >= 0; --i) {
#if 0
				o <<= 1;
				if (*cp++ <= c)
					o |= 1;
#else
				unsigned char t;
				asm("ld %2,Z+\n cp %2,%3\n rol %0"
					: "+r" (o), "+z" (cp), "=&r" (t)
					: "r" (c)
					: "cc");
#endif
			}

			outb(o, PORTB);
		} while(--c);
	}
}


int main(void)
{
	outb(0xff, DDRB);
	outb(0x00, DDRD);

	for(;;) {
		register unsigned char i;

		pwm(7);

		for(i = 0; i < NB; i++)
			b[i] += d[i];
	}
}
