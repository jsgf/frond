#ifndef _ANIM_H
#define _ANIM_H

struct anim_frame
{
	unsigned short bitidx;		/* Bit index into data for this frame */
	unsigned short tokens;		/* size (in tokens) */
};

struct anim
{
	unsigned char nframe;		/* number of frames */
	unsigned char rate;		/* frame rate (revolutions per frame) */

	unsigned char timebase;		/* number of pixels as timebase */
	unsigned char ratio[16];	/* ratio of other pixel rows to timebase */

	struct anim_frame frames[0];
};

#endif /* _ANIM_H */
