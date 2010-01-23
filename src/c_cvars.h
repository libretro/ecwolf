// There's no console yet, but I'm putting user configurable stuff in here in
// case I get around to it in the future.

#ifndef __C_CVARS__
#define __C_CVARS__

#include "wl_def.h"

void ReadConfig();
void WriteConfig();

extern boolean	r_depthfog;
extern boolean	vid_fullscreen;

#endif /* __C_CVARS__ */
