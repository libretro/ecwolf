// There's no console yet, but I'm putting user configurable stuff in here in
// case I get around to it in the future.

#ifndef __C_CVARS__
#define __C_CVARS__

#include "wl_def.h"

void FinalReadConfig();
void ReadConfig();
void WriteConfig();

enum Aspect
{
	ASPECT_NONE,
	ASPECT_16_9,
	ASPECT_16_10,
	ASPECT_4_3,
	ASPECT_5_4
};

extern boolean	r_depthfog;
extern boolean	vid_fullscreen;
extern Aspect	vid_aspect;

#endif /* __C_CVARS__ */
