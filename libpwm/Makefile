
CC=avr-gcc -mmcu=at90s2313 #-mtiny-stack
AS=avr-gcc -mmcu=at90s2313 
CFLAGS=-Wall -Os -DNDEBUG -DFRONDS2
LDFLAGS=-Wl,-m,avr23xx 
LD_HEX=-Wl,--oformat,ihex
#LDFLAGS=-mava -Wl,--intel -L. -lpwm

OBJ=pwm.o div16x8u.o rand.o
#OBJ+=serial_rx.o serial_tx.o

libpwm all: .lib

.lib: libpwm.a
	touch .lib

libpwm.a: $(OBJ)
	rm -f libpwm.a && avr-ar rcv libpwm.a $(OBJ)

pwm.s pwm.o: golomb.h pwm.h Makefile assert.h div.h

#%.o: %.S
#	$(AS) -c -o $@ $<

%.s: %.c
	$(CC) $(CFLAGS) -S $<

golomb.h: mkgolomb.pl
	perl ./mkgolomb.pl > $@ || rm -f $@

rand.o: rand.c force
	$(CC) -c $(CFLAGS) -DSEED="0x`od -N2 -h < /dev/urandom | awk '{print $$2}'`" rand.c

force:

.PHONY: force

clean:
	rm -f *.o $(PROGS) *.flash golomb.h *.s *.obj *~
