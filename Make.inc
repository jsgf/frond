
all::

ifndef TOP
TOP=.
endif

CC=avr-gcc -mmcu=at90s2313 #-mtiny-stack
AS=avr-gcc -mmcu=at90s2313 -P -Wa,-AAT90S2313
LD=avr-ld
CFLAGS=-I$(TOP)/libpwm/ -Wall -Os -g -DUSE_FLASH -DNDEBUG 
#LDFLAGS=-Wl,-m,avr23xx
LDFLAGS=-Wl,-Map,$@.map
LD_HEX=#-Wl,--oformat,ihex

HOST_CC=gcc
HOST_CFLAGS=-g -O -Wall

PWM=$(TOP)/libpwm/libpwm.a

%.o: %.S
	$(AS) -c -o $@ $<

%.s: %.c
	$(CC) $(CFLAGS) -g0 -S $<

%.flash: %
	avr-objcopy -O ihex $< $@ || rm -f $@

%.dis: %
	avr-objdump -D $< > $@

$(TOP)/libpwm/libpwm.a: force
	$(MAKE) -C $(TOP)/libpwm libpwm.a

force:
