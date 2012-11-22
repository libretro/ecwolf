//#include "version.h"

#include "wl_def.h"
#include "wl_main.h"
#include "wl_shade.h"

// Feature flag NOTE:
// Use Lower 8-bit of upper left tile for shade definition?
// Defaults were:
//   0 - No shading
//   1 - Normal shading
//   2 - Fog shading
//   3 - (40,40,40) Normal shading
//   4 - (60,60,60) Fog shading

int GetShade(int scale)
{
	int shade = (scale >> 1) / (((viewwidth * 3) >> 8) + 1);  // TODO: reconsider this...
	if(shade > 32) shade = 32;
	else if(shade < 1) shade = 1;
	shade = 32 - shade;

	return shade;
}
