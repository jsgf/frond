#ifndef SERIAL_H
#define SERIAL_H

void serial_init_tx(void);
void serial_init_rx(void);

void tx(unsigned char);
unsigned char rx(void);

#endif /* SERIAL_H */
