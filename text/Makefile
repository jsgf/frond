
TOP=..
include $(TOP)/Make.inc

FONT=5x8.bdf

text.o: showtext.o textdata.o
	$(LD) -r -o $@ showtext.o textdata.o

textdata.c: text.txt gentext.pl $(FONT)
	perl gentext.pl $(FONT) < text.txt > textdata.c || rm -f textdata.c

textdata.o: textdata.c

%.s: %.c
	$(CC) $(CFLAGS) -S $<

clean:
	rm -f *.o *.flash $(PROGS)
