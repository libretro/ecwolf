#ifndef _WL_SHADE_H_
#define _WL_SHADE_H_

#include "templates.h"

// Convert a light level into an unbounded colormap index (shade). Result is
// fixed point. Why the +12? I wish I knew, but experimentation indicates it
// is necessary in order to best reproduce Doom's original lighting.
#define LIGHT2SHADE(l)			((NUMCOLORMAPS*2*FRACUNIT)-(((l)+12)*(FRACUNIT*NUMCOLORMAPS/128)))

// MAXLIGHTSCALE from original DOOM, divided by 2.
#define MAXLIGHTVIS				(24*FRACUNIT)

// Convert a shade and visibility to a clamped colormap index.
// Result is not fixed point.
// Change R_CalcTiltedLighting() when this changes.
#define GETPALOOKUP(vis,shade)	(clamp<int> (((shade)-MIN(MAXLIGHTVIS,(vis)))>>FRACBITS, 0, NUMCOLORMAPS-1))

#define MINZ			(2048*4)

#endif
