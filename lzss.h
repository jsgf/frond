#ifndef _LZSS_H
#define _LZSS_H

struct lztok
{
	union {
		unsigned char data;
		struct lz_ptr {
			int offset, len;
		} ptr;
	} u;
	enum { LZ_DATA, LZ_PTR } type;
	int offset;
};

struct lz_state {
	struct lztok *toks;
	int ntoks;		/* tokens allocated */
	int tokptr;		/* pointer to next free */
	int unlzptr;		/* pointer to next uncompressed */

	unsigned char *data;
	int datalen;
	int *offmap;
};

struct lz_state *lz_init(void);
void lz_add(struct lz_state *, unsigned char *, int);
void lz_output(struct lz_state *);
int lz_mark(struct lz_state *);
int lz_bitoff(struct lz_state *, int off);

#endif /* _LZSS */
