/*
 * Animation compressor
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <pgm.h>

#include "anim.h"
#include "lzss.h"

#define LEDS	16

static const char *progname;

struct state {
	int hold;
	int val;
	gray *row;
	struct token {
		unsigned char tok;	/* token */
		unsigned char bits;	/* size of token */
	} *toks;
	int tokidx;
	int totalbits;
};

/* Calculate pixels per radius, arranged so they're an integer
   multiple of the timebase */
static int radius_pixels(int inner, int res, int prec, int *rad_counts, int *rad_ratio)
{
	int i;
	int outer = M_PI * (16 + inner);
	int total = 0;

	for(i = 0; i < LEDS; i++) {
		int count = (M_PI * (inner + i) * res) / outer;
		int r = prec/count;

		rad_ratio[i] = r;
		rad_counts[i] = prec/r;

		total += prec/r;

//		printf("/* count[%2d]=%d; ratio=%d */\n", i, 
//		       prec/r, r);
	}

//	printf("/* pixels/frame = %d */\n", total);
	return total;
}

struct frame
{
	const char *name;
	unsigned int start;	/* start in tokens */
	unsigned int length;	/* length in tokens */
};

int main(int argc, char **argv)
{
	int opt, err = 0;
	int inner = 1;
	int res = 50;
	int prec = 255;
	FILE *imgfp;
	int rad_counts[LEDS];
	int rad_ratio[LEDS];
	int totalpix;
	int frameno;
	struct lz_state *lzs;
	int i;
	struct frame *frames;
	int lasttok;

	progname = argv[0];
	
	while((opt = getopt(argc, argv, "i:r:p:")) != EOF) {
		switch(opt)
		{
		case 'i':
			inner = atoi(optarg);
			break;

		case 'r':
			res = atoi(optarg);
			break;

		case 'p':
			prec = atoi(optarg);
			break;

		default:
			err++;
		}
	}

	if (err || optind == argc) {
		fprintf(stderr, "Usage: %s [-i innerled] frame0.pgm frame1.pgm...\n",
			progname);
		exit(1);
	}

	prec = (prec/res) * res;

	totalpix = radius_pixels(inner, res, prec, rad_counts, rad_ratio);

	printf("/* Generated */\n"
	       "#ifndef _ANIM_PIC\n"
	       "#define _ANIM_PIC\n\n"
	       "#include \"anim.h\"\n"
	       "#ifdef USE_FLASH\n"
	       "#include <progmem.h>\n"
	       "#define FLASH PROGMEM\n"
	       "#else\n"
	       "#define FLASH /* */\n"
	       "#endif\n");

	pgm_init(&argc, argv);

	frames = malloc(sizeof(*frames) * (argc-optind));

	lasttok = 0;
	lzs = lz_init();
	for(frameno = 0, opt = optind; opt < argc; opt++, frameno++) {
		int rows, cols, fmt;
		int i, a;
		gray maxval;
		gray *pgmpix;

		imgfp = fopen(argv[opt], "r");
		
		frames[frameno].name = argv[opt];
		frames[frameno].start = lasttok;
		
		if (imgfp == NULL) {
			fprintf(stderr, "%s: can't open %s: %s\n",
				progname, argv[opt], strerror(errno));
			exit(1);
		}

		pgm_readpgminit(imgfp, &cols, &rows, &maxval, &fmt);

		fprintf(stderr, "image %dx%d pixels\n", cols, rows);
		if (rows != cols)
			fprintf(stderr, "%s: warning: %s not square (%dx%d)\n",
				progname, argv[opt], cols, rows);

		pgmpix = malloc(sizeof(*pgmpix) * rows * cols);
		
		for(i = 0; i < rows; i++)
			pgm_readpgmrow(imgfp, &pgmpix[i * cols], cols, maxval, fmt);

		{
			int radidx[LEDS];
			int radcount[LEDS];

			for(i = 0; i < LEDS; i++) {
				radidx[i] = 0;
				radcount[i] = 1;
			}

			for(a = 0; a < prec; a++) {
				for(i = 0; i < LEDS; i++) {
					float th, r;
					int x, y;
					int mindim = cols < rows ? cols : rows;
					unsigned char p;
					int idx = radidx[i];
					int next = 1;
//#define NOCOUNT

					if (--radcount[i] > 0)
						next = 0;
					if (!next) {
#ifndef NOCOUNT
						continue;
#endif
					} else {
						radcount[i] = rad_ratio[i];
						radidx[i]++;
					}

					{
						th = (idx * M_PI * 2) / rad_counts[i] + M_PI_2;
						r = (i+inner) * (mindim / (2.0 * (LEDS+inner)));
						x = (int)(cos(th) * r + cols/2);
						y = (int)(sin(th) * r + rows/2);
						
						//printf("%d %d\n", x, y);
						p = pgmpix[y*cols+x] * 15 / maxval;
						lz_add(lzs, &p, 1);
					}
				}
			}
		}
		i = lz_mark(lzs);
		frames[frameno].length = i - lasttok;
		lasttok = i;
		free(pgmpix);

		fclose(imgfp);
	}

	lz_output(lzs);

	printf("static const struct anim anim FLASH = {\n");
	printf("\t%d, /* nframes */\n"
	       "\t%d, /* rate */\n"
	       "\t%d, /* timebase */\n", argc-optind, 1, prec);
	printf("\t{ ");
	for(i = 0; i < LEDS; i++)
		printf("%d, ", rad_ratio[i]);
	printf("},\n");

	printf("\t{\n");
	for(i = 0; i < argc-optind; i++)
		printf("\t\t{ %4d, %3d },\t/* %s */\n",
		       lz_bitoff(lzs, frames[i].start), frames[i].length,
		       frames[i].name);
	printf("\t}\n};\n\n"
	       "#endif /* ANIM_PIC */\n");
	exit(0);
}

