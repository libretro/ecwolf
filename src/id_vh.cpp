#include "wl_def.h"
#include "fstring_c.h"
#include "wl_play.h"
#include "am_map.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "w_wad.h"
#include "v_font.h"
#include "v_palette.h"
#include "v_video.h"
#include "r_data/r_translate.h"
#include "textures/textures.h"
#include "templates.h"

int	    pa=MENU_CENTER,px,py;

//==========================================================================

void VWB_DrawPropString(FFont *font, const char* string, EColorRange translation, bool stencil, uint8_t stencilcolor)
{
	int		    width, height;
	FTexture	*source;
	uint8_t	    ch;
	int cx = px, cy = py;

	height = font->GetHeight();
	FRemapTable *remap = font->GetColorTranslation(translation);

	while ((ch = (uint8_t)*string++)!=0)
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

// Prints a string with uint16_t wrapping
void VWB_DrawPropStringWrap(unsigned int wrapWidth, unsigned int wrapHeight, FFont *font, const char* string, EColorRange translation, bool stencil, uint8_t stencilcolor)
{
	const char* lineStart = string;
	const char* lastBreak = string;
	unsigned int lastBreakX = 0;
	unsigned int cx = 0;
	char ch;

	while ((ch = (uint8_t)*string++)!=0)
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
			FString_C part;
			FSTRING_C_INIT(&part);
			FString_C_InitSubstr(&part, lineStart, static_cast<size_t>(lastBreak-lineStart));
			VWB_DrawPropString(font, FSTRING_C_GETCHARS(&part), translation, stencil, stencilcolor);
			FString_C_Release(&part);

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

void VW_MeasurePropString (FFont *font, const char *string, uint16_t &width, uint16_t &height, uint16_t *finalWidth)
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

		w += font->GetCharWidth(*((uint8_t *)string));
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
	V_Update();
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

unsigned int rndbits_y;
unsigned int rndmask;

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

	AM_ChangeResolution();
}

uint8_t *fizzleSurface = NULL;
void FizzleFadeStart()
{
	fizzleSurface = new uint8_t[SCREENHEIGHT*SCREENPITCH];
	memcpy(fizzleSurface, V_GetBuffer(), SCREENHEIGHT*SCREENPITCH);
}
bool FizzleFade (int x1, int y1,
	unsigned width, unsigned height, unsigned frames, bool abortable)
{
	unsigned x, y, frame, pixperframe;
	int32_t  rndval=0;

	assert(fizzleSurface != NULL);

	pixperframe = width * height / frames;

	IN_StartAck ();

	frame = GetTimeCount();
	// The screen buffer currently holds the target image (drawn by the caller
	// after FizzleFadeStart snapshotted the start image into fizzleSurface).
	// Snapshot the target into srcptr, then seed the screen buffer with the
	// start image so the reveal can composite target pixels directly into the
	// screen buffer in place. This avoids the per-frame full-frame copy of the
	// composite back to the screen that the previous implementation performed.
	// The libretro framebuffer keeps GetBuffer() stable across Update()/Lock(),
	// so destptr remains valid for the whole fade.
	uint8_t * const srcptr = new uint8_t[SCREENHEIGHT*SCREENPITCH];
	memcpy(srcptr, V_GetBuffer(), SCREENHEIGHT*SCREENPITCH);
	memcpy(V_GetBuffer(), fizzleSurface, SCREENHEIGHT*SCREENPITCH);
	uint8_t * const destptr = V_GetBuffer();

	do
	{
		IN_ProcessEvents();

		if(abortable && IN_CheckAck ())
		{
			VH_UpdateScreen();
			delete[] fizzleSurface;
			delete[] srcptr;
			fizzleSurface = NULL;
			return true;
		}

		if(destptr != NULL)
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
				*(destptr + (y1 + y) * SCREENPITCH + x1 + x)
					= *(srcptr + (y1 + y) * SCREENPITCH + x1 + x);

				if(rndval == 0)		// entire sequence has been completed
					goto finished;
			}

			VH_UpdateScreen();
		}
		else
		{
			// No surface, so only enhance rndval
			for(unsigned p = 0; p < pixperframe; p++)
			{
				rndval = (rndval >> 1) ^ (rndval & 1 ? 0 : rndmask);
				if(rndval == 0)
					goto finished;
			}
		}

		frame++;
		Delay(frame - GetTimeCount());        // don't go too fast
	} while (1);

finished:
	// destptr is the screen buffer; force the remaining region to the target
	// image in place. The unrevealed area outside the fade rectangle still
	// holds the start image that seeded the buffer, matching the previous
	// behaviour where the full composite was copied to the screen.
	for (y = y1; y < (y1 + height); ++y)
	{
		memcpy(destptr + (y * SCREENPITCH) + x1,
					srcptr + (y * SCREENPITCH) + x1,
					width); 
	}
	VH_UpdateScreen();
	delete[] fizzleSurface;
	delete[] srcptr;
	fizzleSurface = NULL;
	return false;
}

//==========================================================================

void VWB_Clear(int color, int x1, int y1, int x2, int y2)
{
	V_Clear(x1, y1, x2, y2, color, GPalette.BaseColors[color]);
}

void VWB_DrawFill(FTexture *tex, int ix, int iy, int ix2, int iy2, bool local)
{
	V_FlatFill(ix, iy, ix2, iy2, tex, local);
}

void VWB_DrawGraphic(FTexture *tex, int ix, int iy, double wd, double hd, MenuOffset menu, FRemapTable *remap, bool stencil, uint8_t stencilcolor)
{
	double x = ix, y = iy;

	if(menu)
		MenuToRealCoords(x, y, wd, hd, menu);
	else
		V_VirtualToRealCoords(x, y, wd, hd, 320, 200, true, true);

	if(stencil)
	{
		V_DrawTexture(tex, x, y,
			DTA_DestWidthF, wd,
			DTA_DestHeightF, hd,
			DTA_Translation, remap,
			DTA_FillColor, GPalette.BaseColors[stencilcolor].d,
			TAG_DONE);
	}
	else
	{
		V_DrawTexture(tex, x, y,
			DTA_DestWidthF, wd,
			DTA_DestHeightF, hd,
			DTA_Translation, remap,
			TAG_DONE);
	}
}

void VWB_DrawGraphic(FTexture *tex, int ix, int iy, MenuOffset menu, FRemapTable *remap, bool stencil, uint8_t stencilcolor)
{
	if(!tex)
		return;

	VWB_DrawGraphic(tex, ix, iy, tex->GetScaledWidthDouble(), tex->GetScaledHeightDouble(),
		menu, remap, stencil, stencilcolor);
}

void CA_CacheScreen(FTexture* tex, bool noaspect)
{
	V_Clear(0, 0, SCREENWIDTH, SCREENHEIGHT, GPalette.BlackIndex, 0);
	if(noaspect)
	{
		V_DrawTexture(tex, 0, 0,
			DTA_DestWidth, SCREENWIDTH,
			DTA_DestHeight, SCREENHEIGHT,
			TAG_DONE);
	}
	else
	{
		V_DrawTexture(tex, 0, 0,
			DTA_Fullscreen, true,
			TAG_DONE);
	}
}
