#include <io.h>
#include <interrupt.h>

#include "serial.h"
#include "misc.h"

void serial_init_tx(void)
{
	outp(BPS(115200, CPUFREQ), UBRR);
	outp(inp(UCR) | (1 << TXEN), UCR);
}

void tx(unsigned char ch)
{
	WAITWHILE((inp(USR) & (1 << UDRE)) == 0);

	outp(ch, UDR);
}

