#include "textures/textures.h"
#include "c_cvars.h"
#include "id_ca.h"
#include "gamemap.h"
#include "wl_def.h"
#include "wl_draw.h"
#include "wl_main.h"
#include "wl_shade.h"
#include "r_data/colormaps.h"

#include <climits>

extern fixed viewshift;
extern fixed viewz;

static void R_DrawPlane(byte *vbuf, unsigned vbufPitch, int min_wallheight, int halfheight, fixed planeheight)
{
	fixed dist;                                // distance to row projection
	fixed tex_step;                            // global step per one screen pixel
	fixed gu, gv, du, dv;                      // global texture coordinates
	const byte *tex = NULL;
	int texwidth, texheight;
	fixed texxscale, texyscale;
	FTextureID lasttex;
	byte *tex_offset;

	const fixed heightFactor = abs(planeheight/32);
	int y0 = (((min_wallheight >> 3)*heightFactor)>>FRACBITS) - abs(viewshift);
	if(y0 > halfheight)
		return; // view obscured by walls
	if(y0 <= 0) y0 = 1; // don't let division by zero

	lasttex.SetInvalid();

	const unsigned int mapwidth = map->GetHeader().width;
	const unsigned int mapheight = map->GetHeader().height;

	const unsigned int texDivisor = viewwidth*AspectCorrection[r_ratio].multiplier*175/48;

	int planenumerator = FixedMul(heightnumerator, planeheight)/32;
	const bool floor = planenumerator < 0;
	if(floor)
	{
		tex_offset = vbuf + (signed)vbufPitch * (halfheight + y0);
		planenumerator *= -1;
	}
	else
		tex_offset = vbuf + (signed)vbufPitch * (halfheight - y0 - 1);

	int oldmapx = INT_MAX, oldmapy = INT_MAX;
	// draw horizontal lines
	for(int y = y0;true; ++y, floor ? tex_offset += vbufPitch : tex_offset -= vbufPitch)
	{
		dist = (planenumerator / (y + 1)) << 5;
		gu =  viewx + FixedMul(dist, viewcos);
		gv = -viewy + FixedMul(dist, viewsin);
		tex_step = (dist << 8) / texDivisor;
		du =  FixedMul(tex_step, viewsin);
		dv = -FixedMul(tex_step, viewcos);
		gu -= (viewwidth >> 1) * du;
		gv -= (viewwidth >> 1) * dv; // starting point (leftmost)
		const BYTE *curshades;
		if(r_depthfog)
		{
			const int shade = LIGHT2SHADE(192);
			const int tz = FixedMul(FixedDiv(r_depthvisibility, abs(planeheight)), abs(((halfheight)<<16) - ((halfheight-y)<<16)));
			curshades = &NormalLight.Maps[GETPALOOKUP(tz, shade)<<8];
		}
		else
			curshades = NormalLight.Maps;

		MapSpot spot = NULL;
		for(unsigned int x = 0;x < (unsigned)viewwidth; ++x)
		{
			if(((wallheight[x] >> 3)*heightFactor)>>FRACBITS <= y)
			{
				int curx = (gu >> TILESHIFT)%mapwidth;
				int cury = (-(gv >> TILESHIFT) - 1)%mapheight;

				if(spot == NULL || curx != oldmapx || cury != oldmapy)
				{
					oldmapx = curx;
					oldmapy = cury;
					spot = map->GetSpot(curx, cury, 0);
				}

				if(spot->sector)
				{

#define CHECKTEXTURE(side, termcond) \
{ \
	FTextureID curtex = spot->sector->texture[side]; \
	if (curtex != lasttex && curtex.isValid()) \
	{ \
		FTexture * const texture = TexMan(curtex); \
		lasttex = curtex; \
		tex = texture->GetPixels(); \
		texwidth = texture->GetWidth(); \
		texheight = texture->GetHeight(); \
		texxscale = FixedDiv(1<<26, texture->xScale); \
		texyscale = -FixedDiv(1<<26, texture->yScale); \
	} \
} \
if(termcond) return; \
else if(tex) \
{ \
	const int u = (FixedDiv(gu, texxscale)) % texwidth; \
	const int v = (FixedDiv(gv, texyscale) - 1) % texheight; \
	const unsigned texoffs = ((u+1) * texheight) - v - 1; \
	tex_offset[x] = curshades[tex[texoffs]]; \
}

					if(floor)
					{
						if(y+halfheight >= 0)
						{
							CHECKTEXTURE(MapSector::Floor, y+halfheight >= viewheight);
						}
					}
					else
					{
						if(y >= halfheight - viewheight)
						{
							CHECKTEXTURE(MapSector::Ceiling, y >= halfheight);
						}
					}
				}
			}
			gu += du;
			gv += dv;
		}
	}
}

// Textured Floor and Ceiling by DarkOne
// With multi-textured floors and ceilings stored in lower and upper bytes of
// according tile in third mapplane, respectively.
void DrawFloorAndCeiling(byte *vbuf, unsigned vbufPitch, int min_wallheight)
{
	const int halfheight = (viewheight >> 1) - viewshift;

	R_DrawPlane(vbuf, vbufPitch, min_wallheight, halfheight, viewz-(64<<FRACBITS));
	R_DrawPlane(vbuf, vbufPitch, min_wallheight, halfheight, viewz);
}
