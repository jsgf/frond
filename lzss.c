
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "lzss.h"

//#define DEBUG

#define MIN_DELTA	5
#define MIN_RUN		3
#define MAX_RUN		256
#define BIT_SCALE	4
#define MARK_SCALE	0
#define MIN_SCALE	16

//#define FIXPIX

struct lz_state *lz_init(void)
{
	struct lz_state *sp = malloc(sizeof(*sp));

	sp->toks = malloc(sizeof(*sp->toks) * 128);
	memset(sp->toks, 0, sizeof(*sp->toks) * 128);
	sp->ntoks = 128;
	sp->tokptr = 0;

	/* Window: all data already seen */
	sp->data = NULL;
	sp->datalen = 0;
	sp->offmap = NULL;

	return sp;
}

void lz_add(struct lz_state *sp, unsigned char *data, int ndata)
{
	int i;
	int oldlen = sp->datalen;

	sp->datalen += ndata;

	sp->data = realloc(sp->data, sp->datalen);
	memcpy(sp->data+oldlen, data, ndata);
	sp->offmap = realloc(sp->offmap, sizeof(int) * sp->datalen);

	for(i = oldlen; i < sp->datalen; i++)
		sp->offmap[i] = -1;
}

static void lz_compress(struct lz_state *sp)
{
	int inptr = sp->unlzptr;
	int left = sp->datalen-inptr;
	unsigned char *data = sp->data;

#ifdef DEBUG
	printf("compressing from %d-%d\n", inptr, sp->datalen);
#endif

	while(left) {
		int match, bestmatch, bestlen;
		int inp, matchp;
		int len;

		bestmatch = -1;
		bestlen = -1;
		for(match = 0; match < inptr; match++) {
			if (sp->offmap[match] == -1)
				continue;
			for(len = -1, inp = inptr, matchp = match; matchp < inptr; matchp++, inp++) {
				if (data[inp] != data[matchp])
					break;
				if (len == left)
					break;
				if (len >= MAX_RUN-1)
					break;
				if (len == -1)
					len = 1;
				else
					len++;
			}
			if (len >= bestlen) {
				bestmatch = match;
				bestlen = len;
			}
		}

		if (bestlen < MIN_RUN) {
			int idx = inptr;
			sp->offmap[idx] = sp->tokptr;

#ifdef DEBUG
			printf("d %2d: %x\n", sp->tokptr, data[inptr]);
#endif
			sp->toks[sp->tokptr].type = LZ_DATA;
			sp->toks[sp->tokptr].u.data = data[inptr];
			left--;
			sp->tokptr++;
			inptr++;
		} else {
			int idx = inptr;
			sp->offmap[idx] = sp->tokptr;
#ifdef DEBUG
			{
				int i;
				printf("p %2d: ", sp->tokptr);
				for(i = inptr; i < inptr+bestlen; i++)
					printf("%x ", data[i]);
				printf(" -> (%d %d)\n", 
				       sp->offmap[bestmatch], bestlen);
			}
#endif
			sp->toks[sp->tokptr].type = LZ_PTR;
			sp->toks[sp->tokptr].u.ptr.offset = sp->offmap[bestmatch];
			sp->toks[sp->tokptr].u.ptr.len = bestlen;
			left -= bestlen;
			sp->tokptr++;
			inptr += bestlen;
		}

		if (sp->tokptr == sp->ntoks) {
			sp->ntoks *= 2;
			sp->toks = realloc(sp->toks, sp->ntoks * sizeof(*sp->toks));
		}
	}

	sp->unlzptr = sp->datalen;
//	printf("%d symbols in, %d tokens out\n", ndat, sp->tokptr-stok);
}

static int numbits(int a)
{
	int n = 0;

	while((1<<n) <= a)
		n++;

	return n;
}

int lz_mark(struct lz_state *sp)
{
	lz_compress(sp);

	return sp->tokptr;
}

int lz_bitoff(struct lz_state *sp, int off)
{
	return sp->toks[off].offset;
}

static unsigned int accum=0;	/* bit accumulator */
static int accbptr = 32;	/* ptr in accbits */
static int bitptr = 0;
static int outbytes = 0;

static void emit(int x, int y)
{
	assert(y > 0 && y < 16);
	while (accbptr < y) {
		printf("0x%02x,%s",
		       (accum >> 24) & 0xff,
		       (outbytes++ % 8) == 7 ?"\n\t":" ");
		accum <<= 8;
		accbptr += 8;
	}
	accum |= x << (accbptr-y);
	accbptr -= y;
	bitptr += y;
}

static void emitnum(int n)
{
	int bits = numbits(n);
	int marker;

	for(marker = 0; 1 << (MIN_SCALE+marker*MARK_SCALE) < n; marker++)
		;

	/* emit 1*0 length marker */
	emit(((1<<marker)-1) << 1, marker+1);
	if (bits)
		emit(n, bits);
}

struct freq {
	int sym, count;
	int code;
};

static int freq_sort_count(const void *a, const void *b)
{
	const struct freq *fa = (const struct freq *)a;
	const struct freq *fb = (const struct freq *)b;

	return fb->count - fa->count;
}

static int freq_sort_sym(const void *a, const void *b)
{
	const struct freq *fa = (const struct freq *)a;
	const struct freq *fb = (const struct freq *)b;

	return fa->sym - fb->sym;
}

void lz_output(struct lz_state *sp)
{
	struct freq freq[16];
	int i;
	int maxcode;
	int pixbits;

	accum = 0;
	accbptr = 32;
	bitptr = 0;
	outbytes = 0;

	lz_compress(sp);
	
	for(i = 0; i < 16; i++) {
		freq[i].sym = i;
		freq[i].count = 0;
		freq[i].code = i;
	}

	for(i = 0; i < sp->tokptr; i++) {
		if (sp->toks[i].type == LZ_PTR)
			continue;
		freq[sp->toks[i].u.data].count++;
	}

	printf("#define MIN_DELTA\t%d\n", MIN_DELTA);
	printf("#define MIN_RUN\t\t%d\n", MIN_RUN);
	printf("#define MAX_RUN\t\t%d\n", MAX_RUN);
	printf("#define BIT_SCALE\t%d\n", BIT_SCALE);
	printf("#define MARK_SCALE\t%d\n", MARK_SCALE);
	printf("#define MIN_SCALE\t%d\n", MIN_SCALE);

	// sort by frequency
	qsort(freq, 16, sizeof(*freq), freq_sort_count);

	printf("static const unsigned char tokmap[] FLASH = {\n\t");

	// map symbols to output codes
	for(maxcode = i = 0; i < 16; i++) {
		freq[i].code = i;
		if (freq[i].count != 0)
			maxcode = i;
	}

	for(i = 0; i <= maxcode; i++)
		printf("%d, ", freq[i].sym);
	printf("\n};\n");

	pixbits = numbits(maxcode);

#ifdef FIXPIX
	printf("#define GETPIX()  (LPM(&tokmap[getbits(%d)]))\n", pixbits);
#else
	printf("#define GETPIX()  (LPM(&tokmap[getnum()]))\n");
#endif

	// index by symbols again
	qsort(freq, 16, sizeof(*freq), freq_sort_sym);

	printf("static const unsigned char lz_data[] FLASH = {\n\t");
		
	for(i = 0; i < sp->tokptr; i++) {
		struct lztok *tp = &sp->toks[i];

		tp->offset = bitptr;
		switch (tp->type) {
		case LZ_PTR: {
			int delta = (bitptr - sp->toks[tp->u.ptr.offset].offset);
			
			emit(1, 1);	/* Pointer */
			assert(delta > 0);
			emitnum(delta);

			assert(tp->u.ptr.len >= MIN_RUN);
			assert(tp->u.ptr.len < MAX_RUN);
			emitnum(tp->u.ptr.len - MIN_RUN);
			break;
		}

		case LZ_DATA:
			emit(0, 1);	/* Data */
#ifdef FIXPIX
			emit(freq[tp->u.data].code, pixbits);
#else
			emitnum(freq[tp->u.data].code);
#endif
			break;
		}
	}

	while(accbptr < 32) {
		printf("0x%02x,%s", (accum >> 24) & 0xff,
		       (outbytes++ % 8) == 7 ? "\n\t":" ");
		accum <<= 8;
		accbptr += 8;
	}

	printf("\n};\n");

	printf("/* %d tokens, %d bits (%d bytes) */\n", sp->tokptr, bitptr, outbytes);
}
