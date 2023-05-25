

#ifndef __FB_H__
#define __FB_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "defs.h"



struct fb
{
	byte *ptr;
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
	int delegate_scaling;
};


extern struct fb fb;

#ifdef __cplusplus
}
#endif

#endif




