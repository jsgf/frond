
#ifndef _DIV_H
#define _DIV_H

extern unsigned short div16x8u(unsigned short, unsigned char);
extern short div16x8s(short, signed char);

static inline unsigned short _div16x8u(unsigned short a, unsigned char b)
{
	unsigned char loop, rem;
	unsigned char ah, al;

	rem = 0;
	loop = 17;
	ah = a >> 8;
	al = a;
	
	asm(    "\n"
	   "1: \trol %1" "\n\t"		/* al */
		"rol %0" "\n\t"		/* ah */
		"dec %4" "\n\t"		/* loop */
		"breq 3f" "\n\t"
		"rol %2" "\n\t"		/* rem */
		"sub %2, %3" "\n\t"	/* rem -= b */
		"brcc 2f" "\n\t"
		"add %2, %3" "\n\t"	/* rem += b */
		"clc" "\n\t"
		"rjmp 1b" "\n"
	   "2: \tsec" "\n\t"
		"rjmp 1b" "\n"
	   "3:"
		: "+r" (ah), "+r" (al)
		: "r" (rem), "r" (b), "r" (loop)
		: "cc");
	return (ah << 8) + al;
}

extern unsigned short div16x8u(unsigned short a, unsigned char b);

#endif /* DIV_H */
