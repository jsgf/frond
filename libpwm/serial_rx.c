#include <interrupt.h>
#include <sig-avr.h>
#include <io.h>

#include "serial.h"
#include "misc.h"

/* Set up uart stuff; expects DDR register to already be set up */
void serial_init_rx(void)
{
	outp(BPS(115200, CPUFREQ), UBRR);
	outp(inp(UCR) | (1 << RXCIE) | (1 << RXEN), UCR);
}

#define RXBUFSZ		4
#define RXBUFMSK	(RXBUFSZ-1)

#if (RXBUFSZ & (RXBUFSZ-1))
#error "Want RXBUFSZ power of 2"
#endif

static unsigned char rxbuf[RXBUFSZ];
static volatile unsigned char rxin = 0, rxout = 0;

SIGNAL(SIG_UART_RECV)
{
	unsigned char ch = inp(UDR);
	unsigned char newrxin;

	newrxin = (rxin + 1) & RXBUFMSK;

	if (inp(USR) & ((1 << FE) | (1 << OVR)))
		return;		/* framing or overrun */

	if (newrxin == rxout)
		return;		/* drop; overrun */

	rxbuf[newrxin] = ch;
	rxin = newrxin;
}

unsigned char rx(void)
{
	unsigned char ch;

	WAITWHILE(rxin == rxout);

	ch = rxbuf[rxout];

	rxout = (rxout + 1) & RXBUFMSK;

	sei();

	return ch;
}
