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

//==========================================================================

void VWB_DrawPropString(FFont *font, const char* string, EColorRange translation, bool stencil, BYTE stencilcolor)
{
	int		    width, height;
	byte	    *dest;
	FTexture	*source;
	byte	    ch;
	int i;
	unsigned sx, sy;
	int tmp1, tmp2;
	int cx = px, cy = py;

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
}

// Prints a string with word wrapping
void VWB_DrawPropStringWrap(unsigned int wrapWidth, unsigned int wrapHeight, FFont *font, const char* string, EColorRange translation, bool stencil, BYTE stencilcolor)
{
	const char* lineStart = string;
	const char* lastBreak = string;
	unsigned int lastBreakX = 0;
	unsigned int cx = 0;
	char ch;

	while ((ch = (byte)*string++)!=0)
	{
		if(ch == '\n')
		{
			cx = 0;
			continue;
		}
		else if(ch == ' ' || ch == '\t')
		{
			lastBreak = string;
			lastBreakX = cx;
		}

		cx += font->GetCharWidth(ch);
		if(cx > wrapWidth)
		{
			FString part(lineStart, static_cast<int>(lastBreak-lineStart));
			VWB_DrawPropString(font, part, translation, stencil, stencilcolor);

			lineStart = lastBreak;
			cx -= lastBreakX;
			lastBreakX = 0;
			py += font->GetHeight();

			if((unsigned)py >= wrapHeight)
				break;
		}
	}

	// Flush the rest of the string.
	VWB_DrawPropString(font, lineStart, translation, stencil, stencilcolor);
	py += font->GetHeight();
}

void VW_MeasurePropString (FFont *font, const char *string, word &width, word &height, word *finalWidth)
{
	int w = 0;

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

/*
=============================================================================

						WOLFENSTEIN STUFF

=============================================================================
*/

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
	int32_t  rndval=0, lastrndval;
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
	int myratio = handleaspect ? r_ratio : 0;
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

void VWB_DrawFill(FTexture *tex, int ix, int iy, int iw, int ih, bool local)
{
	if(iw < 0 || ih < 0)
		return;
	if(static_cast<unsigned int>(iw) > screenWidth)
		iw = screenWidth;
	if(static_cast<unsigned int>(ih) > screenHeight)
		ih = screenHeight;

	// origin
	unsigned int ox = 0, oy = 0;
	if(local)
	{
		if(ix < 0)
		{
			ox = -ix;
			ix = 0;
		}
		else
			ox = ix;
		if(iy < 0)
		{
			oy = -iy;
			iy = 0;
		}
		else
			oy = iy;
	}
	else
	{
		if(ix < 0)
			ix = 0;
		if(iy < 0)
			iy = 0;
	}

	byte *vbuf = VL_LockSurface(screenBuffer) + ix + (iy * bufferPitch);

	const unsigned int width = tex->GetWidth();
	const unsigned int height = tex->GetHeight();

	const BYTE *table = NormalLight.Maps;
	const BYTE* src;
	byte* dest = vbuf;
	unsigned int x, y, sy;
	for(x = ix;x < static_cast<unsigned int>(iw);++x)
	{
		src = tex->GetColumn((x+ox)%width, NULL);
		for(y = iy, sy = (iy+oy)%height;y < static_cast<unsigned int>(ih);++y)
		{
			if(src[sy])
				*dest = table[src[sy]];
			sy = (sy+1)%height;
			dest += bufferPitch;
		}

		dest = ++vbuf;
	}

	VL_UnlockSurface(screenBuffer);
}

void VWB_DrawGraphic(FTexture *tex, int ix, int iy, double wd, double hd, MenuOffset menu, FRemapTable *remap, bool stencil, BYTE stencilcolor)
{
	if(!tex)
		return;

	byte *vbuf = VL_LockSurface(screenBuffer);

	double xd = (double)ix - tex->GetScaledLeftOffsetDouble();
	double yd = (double)iy - tex->GetScaledTopOffsetDouble();
	if(menu)
		MenuToRealCoords(xd, yd, wd, hd, menu);
	else
		VirtualToRealCoords(xd, yd, wd, hd, 320, 200, true, true);

	const int x1 = ceil(xd);
	const int y1 = ceil(yd);
	const fixed xStep = (tex->GetWidth()/wd)*FRACUNIT;
	const fixed yStep = (tex->GetHeight()/hd)*FRACUNIT;
	const fixed xRun = MIN<fixed>(tex->GetWidth()<<FRACBITS, xStep*(screenWidth-x1));
	const fixed yRun = MIN<fixed>(tex->GetHeight()<<FRACBITS, yStep*(screenHeight-y1));
	vbuf += x1 + (y1 > 0 ? bufferPitch*y1 : 0);

	const BYTE *table = !stencil && remap ? remap->Remap : NormalLight.Maps;
	const BYTE *src;
	byte *dest = vbuf;
	fixed x, y;
	for(x = 0;x < xRun;x += xStep)
	{
		src = tex->GetColumn(x>>FRACBITS, NULL);

		for(y = 0;y < yRun;y += yStep)
		{
			if(src[y>>FRACBITS])
			{
				if(stencil)
					*dest = table[stencilcolor];
				else
					*dest = table[src[y>>FRACBITS]];
			}
			dest += bufferPitch;
		}

		dest = ++vbuf;
	}

	VL_UnlockSurface(screenBuffer);
}

void VWB_DrawGraphic(FTexture *tex, int ix, int iy, MenuOffset menu, FRemapTable *remap, bool stencil, BYTE stencilcolor)
{
	if(!tex)
		return;

	VWB_DrawGraphic(tex, ix, iy, tex->GetScaledWidthDouble(), tex->GetScaledHeightDouble(),
		menu, remap, stencil, stencilcolor);
}

void CA_CacheScreen(FTexture* tex, bool noaspect)
{
	if(!tex)
		return;

	if(!noaspect)
		VWB_Clear(GPalette.BlackIndex, 0, 0, screenWidth, screenHeight);

	byte *vbuf = VL_LockSurface(curSurface);
	if(!vbuf)
		return;

	double xd = 0;
	double yd = 0;
	double wd;
	double hd;
	if(noaspect)
	{
		wd = screenWidth;
		hd = screenHeight;
	}
	else
	{
		wd = 320;
		hd = 200;
		VirtualToRealCoords(xd, yd, wd, hd, 320, 200, false, true);
	}

	const fixed xStep = (tex->GetWidth()/wd)*FRACUNIT;
	const fixed yStep = (tex->GetHeight()/hd)*FRACUNIT;

	vbuf += static_cast<int>(xd) + bufferPitch*static_cast<int>(yd);
	const BYTE *src;
	byte *dest = vbuf;
	unsigned int i, j;
	fixed x, y;
	for(i = xd, x = 0;x < tex->GetWidth()<<FRACBITS && i < screenWidth;x += xStep, ++i)
	{
		src = tex->GetColumn(x>>FRACBITS, NULL);
		for(j = yd, y = 0;y < tex->GetHeight()<<FRACBITS && j < screenHeight;y += yStep, ++j)
		{
			*dest = NormalLight.Maps[src[y>>FRACBITS]];
			dest += bufferPitch;
		}

		dest = ++vbuf;
	}
	VL_UnlockSurface(curSurface);
}
