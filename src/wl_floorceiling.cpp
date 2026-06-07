#include "textures/textures.h"
#include "c_cvars.h"
#include "id_ca.h"
#include "gamemap.h"
#include "wl_def.h"
#include "wl_draw.h"
#include "wl_main.h"
#include "wl_shade.h"
#include "r_data/colormaps.h"
#include "r_halo.h"

#include <climits>
#include <math.h>

extern int viewshift;
extern fixed viewz;

static void R_DrawPlane(uint8_t *vbuf, unsigned vbufPitch, int min_wallheight, int halfheight, fixed planeheight)
{
	fixed dist;                                // distance to row projection
	fixed tex_step;                            // global step per one screen pixel
	fixed gu, gv, du, dv;                      // global texture coordinates
	const uint8_t *tex = NULL;
	int texwidth, texheight;
	fixed texxscale, texyscale;
	FTextureID lasttex;
	uint8_t *tex_offset;
	bool useOptimized = false;
	bool isMasked = false;

	if(planeheight == 0) // Eye level
		return;

	const fixed heightFactor = abs(planeheight)>>8;
	int y0 = ((min_wallheight*heightFactor)>>FRACBITS) - abs(viewshift);
	if(y0 > halfheight)
		return; // view obscured by walls
	if(y0 <= 0) y0 = 1; // don't let division by zero

	lasttex.SetInvalid();

	const unsigned int mapwidth = map->GetHeader().width;
	const unsigned int mapheight = map->GetHeader().height;

	fixed planenumerator = FixedMul(heightnumerator, planeheight);
	const bool floor = planenumerator < 0;
	int tex_offsetPitch;
	if(floor)
	{
		tex_offset = vbuf + (signed)vbufPitch * (halfheight + y0);
		tex_offsetPitch = vbufPitch-viewwidth;
		planenumerator *= -1;
	}
	else
	{
		tex_offset = vbuf + (signed)vbufPitch * (halfheight - y0 - 1);
		tex_offsetPitch = -viewwidth-vbufPitch;
	}

	// Break viewx/viewy apart so we can use the fractional part for texel selection without overflowing.
	const int viewxTile = viewx>>FRACBITS;
	const int viewxFrac = (viewx&(FRACUNIT-1))<<8; // 8.24
	const int viewyTile = viewy>>FRACBITS;
	const int viewyFrac = (viewy&(FRACUNIT-1))<<8; // 8.24

	unsigned int oldmapx = INT_MAX, oldmapy = INT_MAX;
	const uint8_t* curshades = NormalLight.Maps;

	// Precompute the per-column wall-clip threshold once. The inner pixel loop
	// only needs to compare y against (wallheight[x]*heightFactor)>>FRACBITS,
	// which depends on x but not y, so hoist the multiply+shift out of the
	// O(viewwidth*viewheight) loop into this O(viewwidth) pass.
	static int *wallclip = NULL;
	static int wallclip_size = 0;
	if(viewwidth > wallclip_size)
	{
		delete[] wallclip;
		wallclip = new int[viewwidth];
		wallclip_size = viewwidth;
	}
	for(int x = 0; x < viewwidth; ++x)
		wallclip[x] = (wallheight[x]*heightFactor)>>FRACBITS;

	// Per-column halo light boost for the current row, recomputed each row from
	// the active halos. Allocated lazily and only used when halos are present.
	const int numHalos = Halo_ActiveCount();
	// Whether any halo or zone lighting can contribute this frame. When false
	// (the overwhelmingly common case) the per-row/per-cell/per-pixel light
	// boost work is skipped entirely and the plane draws on the same tight
	// path as before the lighting feature existed.
	const bool anyLight = (numHalos > 0) || (Zone_AnyActive() != 0);
	static int *halolight = NULL;
	static int halolight_size = 0;
	if(numHalos > 0 && viewwidth > halolight_size)
	{
		delete[] halolight;
		halolight = new int[viewwidth];
		halolight_size = viewwidth;
	}

	// Per-row halo cull: capture the eye and view direction in the same
	// (Y-negated) world space Sx/Sy use, and precompute each halo's forward
	// distance. The eye is FIXED2FLOAT(viewx/viewy); the forward direction is
	// (viewcos, -viewsin) because Sy negates the gv (world-Y) term. rowdist
	// below is the forward distance of the row's ray midpoint, so a halo more
	// than its radius off that distance is skipped before the quadratic.
	if(numHalos > 0)
	{
		const double eyeX = FIXED2FLOAT(viewx);
		const double eyeY = FIXED2FLOAT(viewy);
		const double vdX = FIXED2FLOAT(viewcos);
		const double vdY = -FIXED2FLOAT(viewsin);
		Halo_BeginPlanes(eyeX, eyeY, vdX, vdY);
	}

	// draw horizontal lines
	for(int y = y0;floor ? y+halfheight < viewheight : y < halfheight; ++y, tex_offset += tex_offsetPitch)
	{
		if(floor ? (y+halfheight < 0) : (y < halfheight - viewheight))
		{
			tex_offset += viewwidth;
			continue;
		}

		// Shift in some extra bits so that we don't get spectacular round off.
		dist = (planenumerator / (y + 1))<<8;
		gu =  viewxFrac + FixedMul(dist, viewcos);
		gv = -viewyFrac + FixedMul(dist, viewsin);
		tex_step = dist / scale;
		du =  FixedMul(tex_step, viewsin);
		dv = -FixedMul(tex_step, viewcos);
		gu -= (viewwidth >> 1) * du;
		gv -= (viewwidth >> 1) * dv; // starting point (leftmost)

		// Depth fog
		const int shade = LIGHT2SHADE(gLevelLight + r_extralight);
		const int tz = FixedMul(FixedDiv(r_depthvisibility, abs(planeheight)), abs(((halfheight)<<16) - ((halfheight-y)<<16)));
		curshades = &NormalLight.Maps[GETPALOOKUP(MAX(tz, MINZ), shade)<<8];

		// Halo lighting: for each active halo, find the screen-x span where this
		// row's ray crosses the halo circle (ray-circle intersection) and add
		// the halo's light to the per-column boost. Overlapping halos sum, which
		// is how stacked concentric halolight rings form a radial gradient.
		//
		// Ray in map space: P(t) = S + V*t, t in [0,1], S=(gu,gv) leftmost,
		// E=S+V=rightmost. A halo covers P where ||P-C|| <= R, i.e.
		// (V.V)t^2 + 2(V.(S-C))t + ((S-C).(S-C) - R^2) <= 0. Solve the quadratic
		// for the t-interval, clamp to [0,1], scale to screen columns.
		if(numHalos > 0)
		{
			memset(halolight, 0, sizeof(int)*viewwidth);

			// Our gu/gv are view-relative with 8 extra fractional bits, while
			// halo centers are absolute world coordinates. Convert the ray to
			// world space using the same signed mapping the tile lookup uses:
			//   world_x = (viewxTile<<FRACBITS) + (gu>>8)
			//   world_y = (viewyTile<<FRACBITS) - (gv>>8)   (gv is negated for Y)
			// so S and C live in the same space and the intersection lands
			// under the lamp. The Y term must be negated to match cury's
			// (viewyTile + (-(gv>>(TILESHIFT+8)) - 1)) mapping; the texture
			// sampler hides this because its lookup wraps mod texheight.
			const double Sx = FIXED2FLOAT((viewxTile<<FRACBITS) + (gu>>8));
			const double Sy = FIXED2FLOAT((viewyTile<<FRACBITS) - (gv>>8));
			const double Vx = FIXED2FLOAT(du>>8) * (double)viewwidth;
			const double Vy = FIXED2FLOAT(-(dv>>8)) * (double)viewwidth;
			const double a = Vx*Vx + Vy*Vy;

			// Forward distance of the row's ray midpoint from the eye along the
			// view direction, in the same space as the halos' precomputed s_fd.
			const double midX = Sx + 0.5*Vx;
			const double midY = Sy + 0.5*Vy;
			const double rowdist = (midX - FIXED2FLOAT(viewx)) * FIXED2FLOAT(viewcos)
				+ (midY - FIXED2FLOAT(viewy)) * (-FIXED2FLOAT(viewsin));

			Halo_RowSpans(halolight, viewwidth, rowdist, Sx, Sy, Vx, Vy, a);
		}

		int curzonelight = 0;
		for(unsigned int x = 0;x < (unsigned)viewwidth; ++x, ++tex_offset)
		{
			if(wallclip[x] <= y)
			{
				unsigned int curx = viewxTile + (gu >> (TILESHIFT+8));
				unsigned int cury = viewyTile + (-(gv >> (TILESHIFT+8)) - 1);

				if(curx != oldmapx || cury != oldmapy)
				{
					oldmapx = curx;
					oldmapy = cury;
					const MapSpot spot = map->GetSpot(oldmapx%mapwidth, oldmapy%mapheight, 0);

					// Zone light boost for this map cell (0 when the zone has no
					// active zonelight, which is the common case).
					curzonelight = (anyLight && spot->zone != NULL)
						? Zone_LightForIndex(spot->zone->index) : 0;

					if(spot->sector)
					{
						FTextureID curtex = spot->sector->texture[floor ? MapSector::Floor : MapSector::Ceiling];
						if (curtex != lasttex && curtex.isValid())
						{
							FTexture * const texture = TexMan(curtex);
							lasttex = curtex;
							tex = texture->GetPixels();
							texwidth = texture->GetWidth();
							texheight = texture->GetHeight();
							texxscale = texture->xScale>>10;
							texyscale = -texture->yScale>>10;

							useOptimized = texwidth == 64 && texheight == 64 && texxscale == FRACUNIT>>10 && texyscale == -FRACUNIT>>10;

							// Masked flats (textures with holes) skip their
							// transparent texels so whatever was drawn behind
							// them (e.g. the opposite plane) shows through,
							// matching upstream ECWolf.
							isMasked = texture->bMasked;
						}
					}
					else
						tex = NULL;
				}

				if(tex)
				{
					// Per-pixel shade: boost the light level where a halo covers
					// this column, recomputing the colormap row with the added
					// light. Cheap branch keeps the no-halo fast path intact.
					const uint8_t *pixshades = curshades;
					if(anyLight)
					{
						const int hb = (numHalos > 0 ? halolight[x] : 0) + curzonelight;
						if(hb != 0)
						{
							const int bshade = LIGHT2SHADE(gLevelLight + r_extralight + hb);
							pixshades = &NormalLight.Maps[GETPALOOKUP(MAX(tz, MINZ), bshade)<<8];
						}
					}

					if(useOptimized)
					{
						const int u = (gu>>18) & 63;
						const int v = (-gv>>18) & 63;
						const unsigned texoffs = (u * 64) + v;
						if(isMasked)
						{
							if(const uint8_t c = tex[texoffs])
								*tex_offset = pixshades[c];
						}
						else
							*tex_offset = pixshades[tex[texoffs]];
					}
					else
					{
						const int u = (FixedMul((viewxTile<<16)+(gu>>8)-512, texxscale)) & (texwidth-1);
						const int v = (FixedMul((viewyTile<<16)-(gv>>8)+512, texyscale)) & (texheight-1);
						const unsigned texoffs = (u * texheight) + v;
						if(isMasked)
						{
							if(const uint8_t c = tex[texoffs])
								*tex_offset = pixshades[c];
						}
						else
							*tex_offset = pixshades[tex[texoffs]];
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
void DrawFloorAndCeiling(uint8_t *vbuf, unsigned vbufPitch, int min_wallheight)
{
	const int halfheight = (viewheight >> 1) - viewshift;

	R_DrawPlane(vbuf, vbufPitch, min_wallheight, halfheight, viewz);
	R_DrawPlane(vbuf, vbufPitch, min_wallheight, halfheight, viewz+(map->GetPlane(0).depth<<FRACBITS));
}
