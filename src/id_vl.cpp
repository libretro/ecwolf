// ID_VL.C

#include <string.h>
#include "wl_def.h"
#include "id_vl.h"
#include "w_wad.h"
#include "v_palette.h"
#include "wl_draw.h"
#include "wl_main.h"
#include "wl_play.h"


// Uncomment the following line, if you get destination out of bounds
// assertion errors and want to ignore them during debugging
//#define IGNORE_BAD_DEST

#ifdef IGNORE_BAD_DEST
#undef assert
#define assert(x) if(!(x)) return
#define assert_ret(x) if(!(x)) return 0
#else
#define assert_ret(x) assert(x)
#endif

bool fullscreen = true;
bool usedoublebuffering = true;
unsigned screenWidth = 640;
unsigned screenHeight = 480;
unsigned screenBits = static_cast<unsigned> (-1);      // use "best" color depth according to libSDL

SDL_Surface *screen = NULL;
unsigned screenPitch;

SDL_Surface *screenBuffer = NULL;
unsigned bufferPitch;

SDL_Surface *curSurface = NULL;
unsigned curPitch;

unsigned scaleFactor;

bool	 screenfaded;
unsigned bordercolor;

SDL_Color gamepal[256];

//===========================================================================

void VL_ReadPalette()
{
	int lump = Wads.GetNumForName("WOLFPAL");
	if(lump == -1 || Wads.LumpLength(lump) < 768)
	{
		printf("ERROR: Valid palette data not found.\n");
		exit(0);
	}

	FWadLump palette = Wads.OpenLumpNum(lump);
	unsigned char data[768];
	palette.Read(data, 768);
	for(unsigned int i = 0;i < 768;i += 3)
	{
		SDL_Color color = {data[i], data[i+1], data[i+2], 0};
		gamepal[i/3] = color;
	}
}


/*
=======================
=
= VL_Shutdown
=
=======================
*/

void	VL_Shutdown (void)
{
	//VL_SetTextMode ();
}


/*
=======================
=
= VL_SetVGAPlaneMode
=
=======================
*/

void	VL_SetVGAPlaneMode (void)
{
	SDL_WM_SetCaption("ECWolf", NULL);

	if(screenBits == -1)
	{
		const SDL_VideoInfo *vidInfo = SDL_GetVideoInfo();
		screenBits = vidInfo->vfmt->BitsPerPixel;
	}

	screen = SDL_SetVideoMode(screenWidth, screenHeight, screenBits,
		(usedoublebuffering ? SDL_HWSURFACE | SDL_DOUBLEBUF : 0) |
		(screenBits == 8 ? SDL_HWPALETTE : 0) |
		(fullscreen ? SDL_FULLSCREEN : 0));
	if(!screen)
	{
		printf("Unable to set %ix%ix%i video mode: %s\n", screenWidth,
			screenHeight, screenBits, SDL_GetError());
		exit(1);
	}
	if((screen->flags & SDL_DOUBLEBUF) != SDL_DOUBLEBUF)
		usedoublebuffering = false;
	SDL_ShowCursor(SDL_DISABLE);

	SDL_Color pal[256];
	for(uint16_t i = 0;i < 256;++i)
	{
		pal[i].r = GPalette.BaseColors[i].r;
		pal[i].g = GPalette.BaseColors[i].g;
		pal[i].b = GPalette.BaseColors[i].b;
	}

	SDL_SetColors(screen, pal, 0, 256);

	screenBuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, screenWidth,
		screenHeight, 8, 0, 0, 0, 0);
	if(!screenBuffer)
	{
		printf("Unable to create screen buffer surface: %s\n", SDL_GetError());
		exit(1);
	}
	SDL_SetColors(screenBuffer, pal, 0, 256);

	screenPitch = screen->pitch;
	bufferPitch = screenBuffer->pitch;

	curSurface = screenBuffer;
	curPitch = bufferPitch;

	scaleFactor = screenWidth/320;
	if(screenHeight/200 < scaleFactor) scaleFactor = screenHeight/200;

	pixelangle = (short *) malloc(screenWidth * sizeof(short));
	CHECKMALLOCRESULT(pixelangle);
	wallheight = (int *) malloc(screenWidth * sizeof(int));
	CHECKMALLOCRESULT(wallheight);

	NewViewSize(viewsize);
}

/*
=============================================================================

						PALETTE OPS

		To avoid snow, do a WaitVBL BEFORE calling these

=============================================================================
*/

/*
=================
=
= VL_ConvertPalette
=
=================
*/

// [BL] HACK: In order to preserve compatibility with the VGAGRAPH files, 
//            palettes containing only bytes with a value <= 63 will have the
//            colors increased in brightness.
void VL_ConvertPalette(const char* srcpal, SDL_Color *destpal)
{
	int lumpNum = Wads.CheckNumForName(srcpal);
	if(lumpNum == -1)
		return;
	FWadLump lump = Wads.OpenLumpNum(lumpNum);
	int length = Wads.LumpLength(lumpNum);
	byte* data = new byte[length];
	lump.Read(data, length);

	// Determine number of colors
	int numColors = length/3;
	if(numColors > 256)
		numColors = 256;

	bool newFormat = false;
	for(int i = 0;i < length;i++)
	{
		if(data[i] > 63)
		{
			newFormat = true;
			break;
		}
	}

	for(int i = 0;i < numColors;i++)
	{
		if(newFormat)
		{
			destpal[i].r = data[i*3];
			destpal[i].g = data[(i*3)+1];
			destpal[i].b = data[(i*3)+2];
		}
		else
		{
			destpal[i].r = data[i*3] * 255/63;
			destpal[i].g = data[(i*3)+1] * 255/63;
			destpal[i].b = data[(i*3)+2] * 255/63;
		}
	}
	delete[] data;
}

//===========================================================================

/*
=================
=
= VL_SetBlend
=
=================
*/

void DoBlending (const PalEntry *from, PalEntry *to, int count, int r, int g, int b, int a);
void VL_SetBlend(uint8_t red, uint8_t green, uint8_t blue, int amount, bool forceupdate)
{
	static PalEntry colors[256];
	if(amount)
	{
		memcpy(colors, GPalette.BaseColors, sizeof(PalEntry)*256);

		DoBlending(GPalette.BaseColors, colors, 256, red, green, blue, amount);
		VL_SetPalette(colors, forceupdate);
	}
	else
		VL_SetPalette(GPalette.BaseColors, forceupdate);
}

//===========================================================================

/*
=================
=
= VL_SetPalette
=
=================
*/

void VL_SetPalette (SDL_Color *palette, bool forceupdate)
{
	if(screenBits == 8)
		SDL_SetPalette(screen, SDL_PHYSPAL, palette, 0, 256);
	else
	{
		SDL_SetPalette(curSurface, SDL_LOGPAL, palette, 0, 256);
		if(forceupdate)
		{
			SDL_BlitSurface(curSurface, NULL, screen, NULL);
			SDL_Flip(screen);
		}
	}
}

void VL_SetPalette (PalEntry *palette, bool forceupdate)
{
	static SDL_Color pal[256];
	for(uint16_t i = 0;i < 256;++i)
	{
		pal[i].r = palette[i].r;
		pal[i].g = palette[i].g;
		pal[i].b = palette[i].b;
	}
	VL_SetPalette(pal, forceupdate);
}


//===========================================================================

/*
=================
=
= VL_FadeOut
=
= Fades the current palette to the given color in the given number of steps
=
=================
*/

static int fadeR = 0, fadeG = 0, fadeB = 0;
void VL_Fade (int start, int end, int red, int green, int blue, int steps)
{
	const int aStep = (end-start)/steps;

	VL_WaitVBL(1);

//
// fade through intermediate frames
//
	for (int a = start;(aStep < 0 ? a > end : a < end);a += aStep)
	{
		if(!usedoublebuffering || screenBits == 8) VL_WaitVBL(1);
		VL_SetBlend(red, green, blue, a, true);
	}

//
// final color
//
	VL_SetBlend (red,green,blue,end, true);

	screenfaded = end != 0;
}

void VL_FadeOut (int start, int end, int red, int green, int blue, int steps)
{
	fadeR = red;
	fadeG = green;
	fadeB = blue;
	VL_Fade(start, end, red, green, blue, steps);
}


/*
=================
=
= VL_FadeIn
=
=================
*/

void VL_FadeIn (int start, int end, SDL_Color *palette, int steps)
{
	if(screenfaded)
		VL_Fade(end, start, fadeR, fadeG, fadeB, steps);
}

/*
=============================================================================

							PIXEL OPS

=============================================================================
*/

byte *VL_LockSurface(SDL_Surface *surface)
{
	if(SDL_MUSTLOCK(surface))
	{
		if(SDL_LockSurface(surface) < 0)
			return NULL;
	}
	return (byte *) surface->pixels;
}

void VL_UnlockSurface(SDL_Surface *surface)
{
	if(SDL_MUSTLOCK(surface))
	{
		SDL_UnlockSurface(surface);
	}
}

/*
=================
=
= VL_Plot
=
=================
*/

void VL_Plot (int x, int y, int color)
{
	byte *ptr;

	assert(x >= 0 && (unsigned) x < screenWidth
			&& y >= 0 && (unsigned) y < screenHeight
			&& "VL_Plot: Pixel out of bounds!");

	ptr = VL_LockSurface(curSurface);
	if(ptr == NULL) return;

	ptr[y * curPitch + x] = color;

	VL_UnlockSurface(curSurface);
}

/*
=================
=
= VL_GetPixel
=
=================
*/

byte VL_GetPixel (int x, int y)
{
	byte *ptr;
	byte col;

	assert_ret(x >= 0 && (unsigned) x < screenWidth
			&& y >= 0 && (unsigned) y < screenHeight
			&& "VL_GetPixel: Pixel out of bounds!");

	ptr = VL_LockSurface(curSurface);
	if(ptr == NULL) return 0;

	col = ((byte *) curSurface->pixels)[y * curPitch + x];

	VL_UnlockSurface(curSurface);

	return col;
}


/*
=================
=
= VL_Hlin
=
=================
*/

void VL_Hlin (unsigned x, unsigned y, unsigned width, int color)
{
	byte *ptr;

	assert(x >= 0 && x + width <= screenWidth
			&& y >= 0 && y < screenHeight
			&& "VL_Hlin: Destination rectangle out of bounds!");

	ptr = VL_LockSurface(curSurface);
	if(ptr == NULL) return;

	memset(ptr + y * curPitch + x, color, width);

	VL_UnlockSurface(curSurface);
}


/*
=================
=
= VL_Vlin
=
=================
*/

void VL_Vlin (int x, int y, int height, int color)
{
	byte *ptr;

	assert(x >= 0 && (unsigned) x < screenWidth
			&& y >= 0 && (unsigned) y + height <= screenHeight
			&& "VL_Vlin: Destination rectangle out of bounds!");

	ptr = VL_LockSurface(curSurface);
	if(ptr == NULL) return;
 
	ptr += y * curPitch + x;

	while (height--)
	{
		*ptr = color;
		ptr += curPitch;
	}
	VL_UnlockSurface(curSurface);
}


/*
=================
=
= VL_Bar
=
=================
*/

void VL_BarScaledCoord (int scx, int scy, int scwidth, int scheight, int color)
{
	byte *ptr;

	assert(scx >= 0 && (unsigned) scx + scwidth <= screenWidth
			&& scy >= 0 && (unsigned) scy + scheight <= screenHeight
			&& "VL_BarScaledCoord: Destination rectangle out of bounds!");

	ptr = VL_LockSurface(curSurface);
	if(ptr == NULL) return;
 
	ptr += scy * curPitch + scx;

	while (scheight--)
	{
		memset(ptr, color, scwidth);
		ptr += curPitch;
	}
	VL_UnlockSurface(curSurface);
}

/*
============================================================================

							MEMORY OPS

============================================================================
*/

/*
=================
=
= VL_MemToLatch
=
=================
*/

void VL_MemToLatch(byte *source, int width, int height,
	SDL_Surface *destSurface, int x, int y)
{
	byte *ptr;
	int xsrc, ysrc, pitch;

	assert(x >= 0 && (unsigned) x + width <= screenWidth
			&& y >= 0 && (unsigned) y + height <= screenHeight
			&& "VL_MemToLatch: Destination rectangle out of bounds!");

	ptr = VL_LockSurface(destSurface);
	if(ptr == NULL) return;

	pitch = destSurface->pitch;
	ptr += y * pitch + x;
	for(ysrc = 0; ysrc < height; ysrc++)
	{
		for(xsrc = 0; xsrc < width; xsrc++)
		{
			ptr[ysrc * pitch + xsrc] = source[(ysrc * (width >> 2) + (xsrc >> 2))
				+ (xsrc & 3) * (width >> 2) * height];
		}
	}
	VL_UnlockSurface(destSurface);
}

//===========================================================================


/*
=================
=
= VL_MemToScreenScaledCoord
=
= Draws a block of data to the screen with scaling according to scaleFactor.
=
=================
*/

void VL_MemToScreenScaledCoord (byte *source, int width, int height, int destx, int desty)
{
	byte *ptr;
	int i, j, sci, scj;
	unsigned m, n;

	assert(destx >= 0 && destx + width * scaleFactor <= screenWidth
			&& desty >= 0 && desty + height * scaleFactor <= screenHeight
			&& "VL_MemToScreenScaledCoord: Destination rectangle out of bounds!");

	ptr = VL_LockSurface(curSurface);
	if(ptr == NULL) return;

	for(j = 0, scj = 0; j < height; j++, scj += scaleFactor)
	{
		for(i = 0, sci = 0; i < width; i++, sci += scaleFactor)
		{
			byte col = source[(j * (width >> 2) + (i >> 2)) + (i & 3) * (width >> 2) * height];
			for(m = 0; m < scaleFactor; m++)
			{
				for(n = 0; n < scaleFactor; n++)
				{
					ptr[(scj + m + desty) * curPitch + sci + n + destx] = col;
				}
			}
		}
	}
	VL_UnlockSurface(curSurface);
}

/*
=================
=
= VL_MemToScreenScaledCoord
=
= Draws a part of a block of data to the screen.
= The block has the size origwidth*origheight.
= The part at (srcx, srcy) has the size width*height
= and will be painted to (destx, desty) with scaling according to scaleFactor.
=
=================
*/

void VL_MemToScreenScaledCoord (byte *source, int origwidth, int origheight, int srcx, int srcy,
								int destx, int desty, int width, int height)
{
	byte *ptr;
	int i, j, sci, scj;
	unsigned m, n;

	assert(destx >= 0 && destx + width * scaleFactor <= screenWidth
			&& desty >= 0 && desty + height * scaleFactor <= screenHeight
			&& "VL_MemToScreenScaledCoord: Destination rectangle out of bounds!");

	ptr = VL_LockSurface(curSurface);
	if(ptr == NULL) return;

	for(j = 0, scj = 0; j < height; j++, scj += scaleFactor)
	{
		for(i = 0, sci = 0; i < width; i++, sci += scaleFactor)
		{
			byte col = source[((j + srcy) * (origwidth >> 2) + ((i + srcx) >>2 ))
				+ ((i + srcx) & 3) * (origwidth >> 2) * origheight];

			for(m = 0; m < scaleFactor; m++)
			{
				for(n = 0; n < scaleFactor; n++)
				{
					ptr[(scj + m + desty) * curPitch + sci + n + destx] = col;
				}
			}
		}
	}
	VL_UnlockSurface(curSurface);
}

//==========================================================================

/*
=================
=
= VL_LatchToScreen
=
=================
*/

void VL_LatchToScreenScaledCoord(SDL_Surface *source, int xsrc, int ysrc,
	int width, int height, int scxdest, int scydest)
{
	assert(scxdest >= 0 && scxdest + width * scaleFactor <= screenWidth
			&& scydest >= 0 && scydest + height * scaleFactor <= screenHeight
			&& "VL_LatchToScreenScaledCoord: Destination rectangle out of bounds!");

	if(scaleFactor == 1)
	{
		// HACK: If screenBits is not 8 and the screen is faded out, the
		//       result will be black when using SDL_BlitSurface. The reason
		//       is that the logical palette needed for the transformation
		//       to the screen color depth is not equal to the logical
		//       palette of the latch (the latch is not faded). Therefore,
		//       SDL tries to map the colors...
		//       The result: All colors are mapped to black.
		//       So, we do the blit on our own...
		if(screenBits != 8)
		{
			byte *src, *dest;
			unsigned srcPitch;
			int i, j;

			src = VL_LockSurface(source);
			if(src == NULL) return;

			srcPitch = source->pitch;

			dest = VL_LockSurface(curSurface);
			if(dest == NULL) return;

			for(j = 0; j < height; j++)
			{
				for(i = 0; i < width; i++)
				{
					byte col = src[(ysrc + j)*srcPitch + xsrc + i];
					dest[(scydest + j) * curPitch + scxdest + i] = col;
				}
			}
			VL_UnlockSurface(curSurface);
			VL_UnlockSurface(source);
		}
		else
		{
			SDL_Rect srcrect = { xsrc, ysrc, width, height };
			SDL_Rect destrect = { scxdest, scydest, 0, 0 }; // width and height are ignored
			SDL_BlitSurface(source, &srcrect, curSurface, &destrect);
		}
	}
	else
	{
		byte *src, *dest;
		unsigned srcPitch;
		int i, j, sci, scj;
		unsigned m, n;

		src = VL_LockSurface(source);
		if(src == NULL) return;

		srcPitch = source->pitch;

		dest = VL_LockSurface(curSurface);
		if(dest == NULL) return;

		for(j = 0, scj = 0; j < height; j++, scj += scaleFactor)
		{
			for(i = 0, sci = 0; i < width; i++, sci += scaleFactor)
			{
				byte col = src[(ysrc + j)*srcPitch + xsrc + i];
				for(m = 0; m < scaleFactor; m++)
				{
					for(n = 0; n < scaleFactor; n++)
					{
						dest[(scydest + scj + m) * curPitch + scxdest + sci + n] = col;
					}
				}
			}
		}
		VL_UnlockSurface(curSurface);
		VL_UnlockSurface(source);
	}
}

//===========================================================================

/*
=================
=
= VL_ScreenToScreen
=
=================
*/

void VL_ScreenToScreen (SDL_Surface *source, SDL_Surface *dest)
{
	SDL_BlitSurface(source, NULL, dest, NULL);
}
