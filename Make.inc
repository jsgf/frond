
all::

ifndef TOP
TOP=.
endif

CC=avr-gcc -mmcu=at90s2313 #-mtiny-stack
AS=avr-gcc -mmcu=at90s2313 -P -Wa,-AAT90S2313
LD=avr-ld
# With avr-gcc 3.3.1, -O is smaller than -Os
OPT=-O -fcse-follow-jumps -fstrength-reduce -fexpensive-optimizations -fcrossjumping -fno-rerun-cse-after-loop
CFLAGS=-I$(TOP)/libpwm/ -Wall $(OPT)  -g -DUSE_FLASH -DNDEBUG 
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
