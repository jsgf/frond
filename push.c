
#include <io.h>

int main(void)
{
	outp(0xff, DDRB);
	outp(0x00, DDRD);
	
	for(;;)
		outp(inp(PIND), PORTB);
}
