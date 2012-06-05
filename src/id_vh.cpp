#include "wl_def.h"
#include "id_sd.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "w_wad.h"
#include "v_font.h"
#include "v_palette.h"
#include "r_data/r_translate.h"
#include "textures/textures.h"
#include "templates.h"

int	    pa=MENU_CENTER,px,py;
byte	fontcolor;
int	    fontnumber;

//==========================================================================

void VWB_DrawPropString(const char* string, EColorRange translation, bool stencil, BYTE stencilcolor)
{
	int		    width, height;
	byte	    *dest;
	FTexture	*source;
	byte	    ch;
	int i;
	unsigned sx, sy;
	int tmp1, tmp2;
	int cx = px, cy = py;

	FFont *fonts[2] = { SmallFont, BigFont };

	byte *vbuf = VL_LockSurface(curSurface);
	if(vbuf == NULL) return;

	FFont *font = fonts[fontnumber];
	dest = vbuf + (cy * curPitch + cx);
	height = font->GetHeight();
	FRemapTable *remap = font->GetColorTranslation(translation);

	while ((ch = (byte)*string++)!=0)
	{
		if(ch == '\n')
		{
			cy += height;
			cx = px;
			continue;
		}

		source = font->GetChar(ch, &width);
		if(source)
			VWB_DrawGraphic(source, cx, cy, (MenuOffset)pa, remap, stencil, stencilcolor);
		cx += width;
	}

	VL_UnlockSurface(curSurface);
}

/*
=================
=
= VL_MungePic
=
=================
*/

void VL_MungePic (byte *source, unsigned width, unsigned height)
{
	unsigned x,y,plane,size,pwidth;
	byte *temp, *dest, *srcline;

	size = width*height;

	if (width&0x3f)
		Quit ("VL_MungePic: Not divisable by 4!");

//
// copy the pic to a temp buffer
//
	temp=(byte *) malloc(size);
	CHECKMALLOCRESULT(temp);
	memcpy (temp,source,size);

//
// munge it back into the original buffer
//
	dest = source;
	pwidth = width/4;

	for (plane=0;plane<4;plane++)
	{
		srcline = temp;
		for (y=0;y<height;y++)
		{
			for (x=0;x<pwidth;x++)
				*dest++ = *(srcline+x*4+plane);
			srcline+=width;
		}
	}

	free(temp);
}

void VW_MeasurePropString (const char *string, word &width, word &height, word *finalWidth)
{
	int w = 0;
	static FFont *fonts[2] = { SmallFont, BigFont };
	FFont *font = fonts[fontnumber];

	height = font->GetHeight();
	for(width = 0;*string;++string)
	{
		if(*string == '\n')
		{
			w = 0;
			height += font->GetHeight();
			continue;
		}

		w += font->GetCharWidth(*((byte *)string));
		if(w > width)
			width = w;
	}

	if(finalWidth)
		*finalWidth = w;
}

/*
=============================================================================

				Double buffer management routines

=============================================================================
*/

void VH_UpdateScreen()
{
	SDL_BlitSurface(screenBuffer, NULL, screen, NULL);
	SDL_Flip(screen);
}


void VWB_DrawTile8 (int x, int y, int tile)
{
//	LatchDrawChar(x,y,tile);
}

/*void VWB_DrawTile8M (int x, int y, int tile)
{
	VL_MemToScreen (((byte *)grsegs[STARTTILE8M])+tile*64,8,8,x,y);
}*/

void VWB_DrawPic(int x, int y, const char* chunk, bool scaledCoord)
{
	FTexture *tex = TexMan(chunk);
	if(!tex)
		return;

	BYTE *data = const_cast<BYTE *>(tex->GetPixels());
	if(!scaledCoord)
		VL_MemToScreen(data, tex->GetScaledWidth(), tex->GetScaledHeight(), x, y);
	else
		VL_MemToScreenScaledCoord(data, tex->GetScaledWidth(), tex->GetScaledHeight(), x, y);
}

void VWB_Bar (int x, int y, int width, int height, int color)
{
	VW_Bar (x,y,width,height,color);
}

void VWB_Plot (int x, int y, int color)
{
	if(scaleFactor == 1)
		VW_Plot(x,y,color);
	else
		VW_Bar(x, y, 1, 1, color);
}

void VWB_Hlin (int x1, int x2, int y, int color)
{
	if(scaleFactor == 1)
		VW_Hlin(x1,x2,y,color);
	else
		VW_Bar(x1, y, x2-x1+1, 1, color);
}

void VWB_Vlin (int y1, int y2, int x, int color)
{
	if(scaleFactor == 1)
		VW_Vlin(y1,y2,x,color);
	else
		VW_Bar(x, y1, 1, y2-y1+1, color);
}


/*
=============================================================================

						WOLFENSTEIN STUFF

=============================================================================
*/

/*
=====================
=
= LatchDrawPic
=
=====================
*/

/*void LatchDrawPic (unsigned x, unsigned y, unsigned picnum)
{
	VL_LatchToScreen (latchpics[2+picnum-LATCHPICS_LUMP_START], x*8, y);
}*/

/*void LatchDrawPicScaledCoord (unsigned scx, unsigned scy, unsigned picnum)
{
	VL_LatchToScreenScaledCoord (latchpics[2+picnum-LATCHPICS_LUMP_START], scx*8, scy);
}*/


//==========================================================================

/*void FreeLatchMem()
{
	int i;
	for(i = 0; i < 2 + LATCHPICS_LUMP_END - LATCHPICS_LUMP_START; i++)
	{
		SDL_FreeSurface(latchpics[i]);
		latchpics[i] = NULL;
	}
}*/

/*
===================
=
= LoadLatchMem
=
===================
*/

void LoadLatchMem (void)
{
	int	i,width,height,start,end;
	byte *src;
	SDL_Surface *surf;

//
// tile 8s
//
	surf = SDL_CreateRGBSurface(SDL_HWSURFACE, 8*8,
		((72 + 7) / 8) * 8, 8, 0, 0, 0, 0);
	if(surf == NULL)
	{
		Quit("Unable to create surface for tiles!");
	}
	SDL_SetColors(surf, gamepal, 0, 256);

	//latchpics[0] = surf;
	int lumpNum = Wads.GetNumForName("TILE8", ns_graphics);
	if(lumpNum == -1)
	{
		printf("\n");
		exit(0);
	}
	FWadLump lump = Wads.OpenLumpNum(lumpNum);
	src = new byte[Wads.LumpLength(lumpNum)];
	byte* src_freeme = src; // Wolf likes to play with pointers
	lump.Read(src, Wads.LumpLength(lumpNum));

	for (i=0;i<72;i++)
	{
		VL_MemToLatch (src, 8, 8, surf, (i & 7) * 8, (i >> 3) * 8);
		src += 64;
	}
	delete[] src_freeme;
}

//==========================================================================

/*
===================
=
= FizzleFade
=
= returns true if aborted
=
= It uses maximum-length Linear Feedback Shift Registers (LFSR) counters.
= You can find a list of them with lengths from 3 to 168 at:
= http://www.xilinx.com/support/documentation/application_notes/xapp052.pdf
= Many thanks to Xilinx for this list!!!
=
===================
*/

// XOR masks for the pseudo-random number sequence starting with n=17 bits
static const uint32_t rndmasks[] = {
					// n    XNOR from (starting at 1, not 0 as usual)
	0x00012000,     // 17   17,14
	0x00020400,     // 18   18,11
	0x00040023,     // 19   19,6,2,1
	0x00090000,     // 20   20,17
	0x00140000,     // 21   21,19
	0x00300000,     // 22   22,21
	0x00420000,     // 23   23,18
	0x00e10000,     // 24   24,23,22,17
	0x01200000,     // 25   25,22      (this is enough for 8191x4095)
};

static unsigned int rndbits_y;
static unsigned int rndmask;

// Returns the number of bits needed to represent the given value
static int log2_ceil(uint32_t x)
{
	int n = 0;
	uint32_t v = 1;
	while(v < x)
	{
		n++;
		v <<= 1;
	}
	return n;
}

void VH_Startup()
{
	int rndbits_x = log2_ceil(screenWidth);
	rndbits_y = log2_ceil(screenHeight);

	int rndbits = rndbits_x + rndbits_y;
	if(rndbits < 17)
		rndbits = 17;       // no problem, just a bit slower
	else if(rndbits > 25)
		rndbits = 25;       // fizzle fade will not fill whole screen

	rndmask = rndmasks[rndbits - 17];
}

bool FizzleFade (SDL_Surface *source, int x1, int y1,
	unsigned width, unsigned height, unsigned frames, bool abortable)
{
	unsigned x, y, frame, pixperframe;
	int32_t  rndval, lastrndval;
	int      first = 1;

	lastrndval = 0;
	pixperframe = width * height / frames;

	IN_StartAck ();

	frame = GetTimeCount();
	byte *srcptr = VL_LockSurface(source);
	if(srcptr == NULL) return false;

	do
	{
		IN_ProcessEvents();

		if(abortable && IN_CheckAck ())
		{
			VL_UnlockSurface(source);
			SDL_BlitSurface(source, NULL, screen, NULL);
			SDL_Flip(screen);
			return true;
		}

		byte *destptr = VL_LockSurface(screen);

		if(destptr != NULL)
		{
			rndval = lastrndval;

			// When using double buffering, we have to copy the pixels of the last AND the current frame.
			// Only for the first frame, there is no "last frame"
			for(int i = first; i < 2; i++)
			{
				for(unsigned p = 0; p < pixperframe; p++)
				{
					//
					// seperate random value into x/y pair
					//

					x = rndval >> rndbits_y;
					y = rndval & ((1 << rndbits_y) - 1);

					//
					// advance to next random element
					//

					rndval = (rndval >> 1) ^ (rndval & 1 ? 0 : rndmask);

					if(x >= width || y >= height)
					{
						if(rndval == 0)     // entire sequence has been completed
							goto finished;
						p--;
						continue;
					}

					//
					// copy one pixel
					//

					if(screenBits == 8)
					{
						*(destptr + (y1 + y) * screen->pitch + x1 + x)
							= *(srcptr + (y1 + y) * source->pitch + x1 + x);
					}
					else
					{
						byte col = *(srcptr + (y1 + y) * source->pitch + x1 + x);
						uint32_t fullcol = SDL_MapRGB(screen->format, GPalette.BaseColors[col].r, GPalette.BaseColors[col].g, GPalette.BaseColors[col].b);
						memcpy(destptr + (y1 + y) * screen->pitch + (x1 + x) * screen->format->BytesPerPixel,
							&fullcol, screen->format->BytesPerPixel);
					}

					if(rndval == 0)		// entire sequence has been completed
						goto finished;
				}

				if(!i || first) lastrndval = rndval;
			}

			// If there is no double buffering, we always use the "first frame" case
			if(usedoublebuffering) first = 0;

			VL_UnlockSurface(screen);
			SDL_Flip(screen);
		}
		else
		{
			// No surface, so only enhance rndval
			for(int i = first; i < 2; i++)
			{
				for(unsigned p = 0; p < pixperframe; p++)
				{
					rndval = (rndval >> 1) ^ (rndval & 1 ? 0 : rndmask);
					if(rndval == 0)
						goto finished;
				}
			}
		}

		frame++;
		Delay(frame - GetTimeCount());        // don't go too fast
	} while (1);

finished:
	VL_UnlockSurface(source);
	VL_UnlockSurface(screen);
	SDL_BlitSurface(source, NULL, screen, NULL);
	SDL_Flip(screen);
	return false;
}

//==========================================================================
/*
** VirtualToRealCoords
**
**---------------------------------------------------------------------------
** Copyright 1998-2008 Randy Heit
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
*/

#include "c_cvars.h"
#include "textures/textures.h"
#include "r_data/colormaps.h"
void VirtualToRealCoords(double &x, double &y, double &w, double &h, double vwidth, double vheight, bool vbottom, bool handleaspect)
{
	int myratio = handleaspect ? vid_aspect : 0;
	double right = x + w;
	double bottom = y + h;

	if (AspectCorrection[myratio].isWide)
	{ // The target surface is either 16:9 or 16:10, so expand the
	  // specified virtual size to avoid undesired stretching of the
	  // image. Does not handle non-4:3 virtual sizes. I'll worry about
	  // those if somebody expresses a desire to use them.
		x = (x - vwidth * 0.5) * screenWidth * 960 / (vwidth * AspectCorrection[myratio].baseWidth) + screenWidth * 0.5;
		w = (right - vwidth * 0.5) * screenWidth * 960 / (vwidth * AspectCorrection[myratio].baseWidth) + screenWidth * 0.5 - x;
	}
	else
	{
		x = x * screenWidth / vwidth;
		w = right * screenWidth / vwidth - x;
	}
	if (AspectCorrection[myratio].tallscreen)
	{ // The target surface is 5:4
		y = (y - vheight * 0.5) * screenHeight * 600 / (vheight * AspectCorrection[myratio].baseHeight) + screenHeight * 0.5;
		h = (bottom - vheight * 0.5) * screenHeight * 600 / (vheight * AspectCorrection[myratio].baseHeight) + screenHeight * 0.5 - y;
		if (vbottom)
		{
			y += (screenHeight - screenHeight * AspectCorrection[myratio].multiplier / 48.0) * 0.5;
		}
	}
	else
	{
		y = y * screenHeight / vheight;
		h = bottom * screenHeight / vheight - y;
	}
}

void VWB_Clear(int color, int x1, int y1, int x2, int y2)
{
	byte *vbuf = VL_LockSurface(screenBuffer);
	for(int i = y1;i < y2;++i)
		memset(vbuf+(i*bufferPitch)+x1, color, (x2-x1));
	VL_UnlockSurface(screenBuffer);
}

void VWB_DrawGraphic(FTexture *tex, int ix, int iy, MenuOffset menu, FRemapTable *remap, bool stencil, BYTE stencilcolor)
{
	byte *vbuf = VL_LockSurface(screenBuffer);

	double xd = (double)ix - tex->GetScaledLeftOffsetDouble();
	double yd = (double)iy - tex->GetScaledTopOffsetDouble();
	double wd = tex->GetScaledWidthDouble();
	double hd = tex->GetScaledHeightDouble();
	if(menu)
		MenuToRealCoords(xd, yd, wd, hd, menu);
	else
		VirtualToRealCoords(xd, yd, wd, hd, 320, 200, true, true);

	const int x1 = ceil(xd);
	const int y1 = ceil(yd);
	const fixed xStep = (tex->GetWidth()/wd)*FRACUNIT;
	const fixed yStep = (tex->GetHeight()/hd)*FRACUNIT;

	const BYTE *table = !stencil && remap ? remap->Remap : NormalLight.Maps;
	const BYTE *src;
	byte *dest;
	unsigned int i, j;
	fixed x, y;
	for(i = 0, x = 0;x < tex->GetWidth()<<FRACBITS;x += xStep, ++i)
	{
		src = tex->GetColumn(x>>FRACBITS, NULL);
		dest = vbuf+x1+i;
		if((signed)(x1+i) >= (signed)screenWidth)
			break;
		if(y1 > 0)
			dest += bufferPitch*y1;

		for(j = 0, y = 0;y < tex->GetHeight()<<FRACBITS;y += yStep, ++j)
		{
			if((signed)(y1+j) >= (signed)(screenHeight))
				break;
			if(src[y>>FRACBITS])
			{
				if(stencil)
					*dest = table[stencilcolor];
				else
					*dest = table[src[y>>FRACBITS]];
			}
			dest += bufferPitch;
		}
	}

	VL_UnlockSurface(screenBuffer);
}

void CA_CacheScreen(const char* chunk)
{
	FTexture *tex = TexMan(chunk);
	if(!tex)
		return;

	VWB_Clear(0, 0, 0, screenWidth, screenHeight);
	byte *vbuf = VL_LockSurface(curSurface);
	if(!vbuf)
		return;

	double xd = 0;
	double yd = 0;
	double wd = 320;
	double hd = 200;
	VirtualToRealCoords(xd, yd, wd, hd, 320, 200, false, true);

	const fixed xStep = (tex->GetWidth()/wd)*FRACUNIT;
	const fixed yStep = (tex->GetHeight()/hd)*FRACUNIT;

	const BYTE *src;
	byte *dest;
	unsigned int i, j;
	fixed x, y;
	for(i = xd, x = 0;x < tex->GetWidth()<<FRACBITS;x += xStep, ++i)
	{
		src = tex->GetColumn(x>>FRACBITS, NULL);
		dest = vbuf+i+bufferPitch*static_cast<int>(yd);
		for(j = yd, y = 0;y < tex->GetHeight()<<FRACBITS;y += yStep, ++j)
		{
			if(j >= screenHeight)
				break;
			*dest = NormalLight.Maps[src[y>>FRACBITS]];
			dest += bufferPitch;
		}
	}
	VL_UnlockSurface(curSurface);
}
