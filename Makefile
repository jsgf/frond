include Make.inc

TARGET = main

PROGS = main testpat

GIZLIST = flame2.o flicker.o kitt.o spiral.o flame.o spiral.o ramp.o rps.o full.o sync.o kitt.o echo.o testpat.o null.o xfade.o

GIZMOS = cellauto.o

GC=gizmos-$(subst \ ,-,$(strip $(GIZMOS:%.o=%))).c

testpat: TARGET=testpat
testpat: GIZMOS=testpat.o

$(GIZMOS) main.o: $(TOP)/libpwm/pwm.h $(TOP)/libpwm/golomb.h

all:: progs

progs: libpwm $(TARGET) $(TARGET:%=%.flash) 

obj: $(TARGET:%=%.obj)

$(GC): $(GIZMOS) mkgizmo2.pl Makefile
	perl mkgizmo2.pl $(GIZMOS) > $(GC) || rm -f gizmos.c

$(TARGET): main.o gizmos.o $(GIZMOS) $(PWM) Makefile
	$(CC) $(LDFLAGS) $(LDHEX) -o $@ main.o gizmos.o $(GIZMOS) $(PWM)
	size $@

main.o: 

gizmos.o: $(GC)
	$(CC) $(CFLAGS) -c -o $@ $<

anim.o: anim.h anim-pic.h

anim_compress.o: anim_compress.c anim.h
	$(HOST_CC) -c $(HOST_CFLAGS) anim_compress.c

PICS=raze-0.pgm

anim-pic.h: anim_compress $(PICS) Makefile
	anim_compress $(PICS) > anim-pic.h || rm -f anim-pic.h

anim_compress: anim_compress.o lzss.o
	$(HOST_CC) -g -o $@ anim_compress.o lzss.o -lpgm -lpbm -lm

lzss.o: lzss.c lzss.h
	$(HOST_CC) -c $(HOST_CFLAGS) $<

#anim.flash: anim.o stream.o

$(TOP)/libpwm/%: force
	$(MAKE) -C $(TOP)/libpwm $(@F)

$(TOP)/text/%: force
	$(MAKE) -C $(TOP)/text $(@F)

install: $(TARGET).flash force
	uisp -dlpt=/dev/parport0 -dprog=stk200 -dpart=at90s2313 --erase --upload if=main.flash --verify

test-%: %.c testrig.c 
	$(HOST_CC) -O -Wall -g -Ilibpwm -o $@ -DGIZ=$(filter-out testrig,$(^:%.c=%)) -DTESTRIG testrig.c $(filter-out testrig.c,$^) -lglut -lGLU -lGL

force:

.PHONY: force install clean

clean:
	$(MAKE) -C libpwm clean
	$(MAKE) -C text clean
	rm -f anim_compress anim-pic.h
	rm -f *.o $(PROGS) *.flash golomb.h *.s *.obj *~ *.map *.dis 
	rm -f .stamp.* gizmos*.c gizmos.h test-*
