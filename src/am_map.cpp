/*
** am_map.cpp
**
**---------------------------------------------------------------------------
** Copyright 2013 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include "wl_def.h"
#include "id_ca.h"
#include "id_vl.h"
#include "gamemap.h"
#include "r_data/colormaps.h"
#include "v_video.h"
#include "wl_agent.h"
#include "wl_draw.h"

int amsizex, amsizey, amx, amy;
angle_t amangle;

FVector2 GetClipIntersection(const FVector2 &p1, const FVector2 &p2, unsigned edge)
{
	// If we are clipping a vertical line, it should be because our angle is
	// 90 degrees and it's clip against the top or bottom
	if((edge&1) == 1 && ((amangle>>ANGLETOFINESHIFT)&(ANG90-1)) == 0)
	{
		if(edge == 1)
			return FVector2(p1[0], amy);
		return FVector2(p1[0], amy+amsizey);
	}
	else
	{
		const float A = p2[1] - p1[1];
		const float B = p1[0] - p2[0];
		const float C = A*p1[0] + B*p1[1];
		switch(edge)
		{
			default:
			case 0: // Left
				return FVector2(amx, (C - A*amx)/B);
			case 1: // Top
				return FVector2((C - B*amy)/A, amy);
			case 2: // Right
				return FVector2(amx+amsizex, (C - A*(amx+amsizex))/B);
			case 3: // Bottom
				return FVector2((C - B*(amy+amsizey))/A, amy+amsizey);
		}
	}
}

void BasicOverhead (void)
{
	TArray<FVector2> points;
	const int windowsize = 16;
	const int mapwidth = map->GetHeader().width;
	const int mapheight = map->GetHeader().height;

	amsizex = screenWidth*2/3;
	amsizey = screenHeight*2/3;
	amx = (screenWidth - amsizex)/2;
	amy = (screenHeight - amsizey)/2;
	const fixed scale = (amsizey<<FRACBITS)/windowsize;

	screen->Clear(amx, amy, amx+amsizex, amy+amsizey, GPalette.BlackIndex, 0);

	// Since the simple polygon fill function seems to be off by one in the y
	// direction, lets shift this up!
	--amy;

	amangle = players[0].mo->angle-ANGLE_90;
	const fixed playerx = players[0].mo->x;
	const fixed playery = players[0].mo->y;
	const fixed amsin = finesine[amangle>>ANGLETOFINESHIFT];
	const fixed amcos = finecosine[amangle>>ANGLETOFINESHIFT];

	// For rotating the tiles, this table includes the point offsets based on the current scale
	const double rottable[2][2] =
	{
		{ FIXED2FLOAT(FixedMul(scale, amcos)), FIXED2FLOAT(FixedMul(scale, amsin)) },
		{ FIXED2FLOAT(FixedMul(scale, amcos) - FixedMul(scale, amsin)), FIXED2FLOAT(FixedMul(scale, amsin) + FixedMul(scale, amcos)) }
	};

	// Culling table
	// minmax[minmaxSel][min/max][x/y]
	static const unsigned minmax[4][2][2] = {
		{ {3, 1}, {0, 2} },
		{ {2, 0}, {3, 1} },
		{ {1, 3}, {2, 0} },
		{ {0, 2}, {1, 3} }
	};
	const unsigned minmaxSel = amangle/ANGLE_90;

	for(int my = 0;my < mapheight;++my)
	{
		MapSpot spot = map->GetSpot(0, my, 0);
		for(int mx = 0;mx < mapwidth;++mx, ++spot)
		{
			if(!(spot->amFlags & AM_Visible))
				continue;

			fixed x = FixedMul((mx<<FRACBITS)-playerx, scale);
			fixed y = FixedMul((my<<FRACBITS)-playery, scale);
			fixed rotx = FixedMul(x, amcos) - FixedMul(y, amsin) + (amsizex<<(FRACBITS-1));
			fixed roty = FixedMul(x, amsin) + FixedMul(y, amcos) + (amsizey<<(FRACBITS-1));
			double x1 = amx + FIXED2FLOAT(rotx);
			double y1 = amy + FIXED2FLOAT(roty);
			points.Resize(4);
			points[0] = FVector2(x1, y1);
			points[1] = FVector2(x1 + rottable[0][0], y1 + rottable[0][1]);
			points[2] = FVector2(x1 + rottable[1][0], y1 + rottable[1][1]);
			points[3] = FVector2(x1 - rottable[0][1], y1 + rottable[0][0]);


			const float xmin = points[minmax[minmaxSel][0][0]][0];
			const float xmax = points[minmax[minmaxSel][0][1]][0];
			const float ymin = points[minmax[minmaxSel][1][0]][1];
			const float ymax = points[minmax[minmaxSel][1][1]][1];

			// Cull out tiles which never touch the automap area
			if((xmax < amx || xmin > amx+amsizex) || (ymax < amy || ymin > amy+amsizey))
				continue;

			// If the tile is partially in the automap area, clip
			if((ymax > amy+amsizey || ymin < amy) || (xmax > amx+amsizex || xmin < amx))
			{
				for(int i = 0;i < 4;++i)
				{
					TArray<FVector2> input = points;
					points.Clear();
					FVector2 *s = &input[0];
					for(unsigned j = input.Size();j-- > 0;)
					{
						FVector2 &e = input[j];

						bool Ein, Sin;
						switch(i)
						{
							case 0: // Left
								Ein = e[0] > amx;
								Sin = s->operator[](0) > amx;
								break;
							case 1: // Top
								Ein = e[1] > amy;
								Sin = s->operator[](1) > amy;
								break;
							case 2: // Right
								Ein = e[0] < amx+amsizex;
								Sin = s->operator[](0) < amx+amsizex;
								break;
							case 3: // Bottom
								Ein = e[1] < amy+amsizey;
								Sin = s->operator[](1) < amy+amsizey;
								break;
						}
						if(Ein)
						{
							if(!Sin)
								points.Push(GetClipIntersection(e, *s, i));
							points.Push(e);
						}
						else if(Sin)
							points.Push(GetClipIntersection(e, *s, i));
						s = &e;
					}
				}
			}

			FTexture *tex;
			int brightness;
			if(spot->tile)
			{
				brightness = 256;
				tex = TexMan(spot->texture[0]);
			}
			else if(spot->sector)
			{
				brightness = 128;
				tex = TexMan(spot->sector->texture[MapSector::Floor]);
			}
			else
				tex = NULL;

			if(tex)
				screen->FillSimplePoly(tex, &points[0], points.Size(), x1, y1, FIXED2FLOAT(scale/64), FIXED2FLOAT(scale/64), ~amangle, &NormalLight, brightness);
		}
	}
}
