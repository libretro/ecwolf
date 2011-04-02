#include "colormatcher.h"
#include "v_video.h"

BYTE RGB32k[32][32][32];

void GenerateLookupTables()
{
	for(int r = 0;r < 32;r++)
		for(int g = 0;g < 32;g++)
			for(int b = 0;b < 32;b++)
				RGB32k[r][g][b] = ColorMatcher.Pick((r<<3)|(r>>2), (g<<3)|(g>>2), (b<<3)|(b>>2));
}
