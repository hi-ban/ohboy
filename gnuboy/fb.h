

#ifndef __FB_H__
#define __FB_H__


#include "defs.h"



struct fb
{
	un16 *ptr;
	int w, h;
	int pelsize;
	int pitch;
	int indexed;
	struct
	{
		int l, r;
	} cc[4];
	int yuv;
	int enabled;
	int dirty;
	int first_paint;
};


extern struct fb fb;


#endif

