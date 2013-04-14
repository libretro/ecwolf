// ID_VL.C

#include <string.h>
#include "wl_def.h"
#include "id_vl.h"
#include "id_vh.h"
#include "w_wad.h"
#include "r_data/colormaps.h"
#include "v_font.h"
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

#if SDL_VERSION_ATLEAST(2,0,0)
SDL_Window *window = NULL;
SDL_Renderer *screenRenderer = NULL;
SDL_Texture *screen = NULL;
#else
SDL_Surface *screen = NULL;
#endif

SDL_Surface *screenBuffer = NULL;
unsigned bufferPitch;

SDL_Surface *curSurface = NULL;
unsigned curPitch;

unsigned scaleFactor;

bool	 screenfaded;

SDL_Color gamepal[256];

static struct
{
	uint8_t r,g,b;
	int amount;
} currentBlend;

//===========================================================================

void VL_ReadPalette(const char* lump)
{
	InitPalette(lump);
	VL_SetPalette(GPalette.BaseColors, false);
	if(currentBlend.amount)
		VL_SetBlend(currentBlend.r, currentBlend.g, currentBlend.b, currentBlend.amount, false);
	R_InitColormaps();
	V_RetranslateFonts();
}

/*
=======================
=
= VL_SetVGAPlaneMode
=
=======================
*/

void	VL_SetVGAPlaneMode (bool forSignon)
{
#if SDL_VERSION_ATLEAST(2,0,0)
#else
	SDL_WM_SetCaption("ECWolf", NULL);
#endif

	SDL_Color pal[256];
	for(uint16_t i = 0;i < 256;++i)
	{
		pal[i].r = GPalette.BaseColors[i].r;
		pal[i].g = GPalette.BaseColors[i].g;
		pal[i].b = GPalette.BaseColors[i].b;
	}

#if SDL_VERSION_ATLEAST(2,0,0)
	if(window) SDL_DestroyWindow(window);
	window = SDL_CreateWindow("ECWolf", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screenWidth, screenHeight,
		(fullscreen && !forSignon ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)
	);
	if(screenRenderer) SDL_DestroyRenderer(screenRenderer);
	screenRenderer = SDL_CreateRenderer(window, -1, 0);
	if(!screenRenderer)
	{
		printf("Unable to set %ix%ix%i video mode: %s\n", screenWidth,
			screenHeight, screenBits, SDL_GetError());
		exit(1);
	}
	if(screen) SDL_DestroyTexture(screen);
	screen = SDL_CreateTexture(screenRenderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, screenWidth, screenHeight);
	if(!screen)
	{
		printf("Unable to create texture: %s\n", SDL_GetError());
		exit(1);
	}
	SDL_ShowCursor(SDL_DISABLE);
#else
	if(screenBits == static_cast<unsigned>(-1))
	{
		const SDL_VideoInfo *vidInfo = SDL_GetVideoInfo();
		screenBits = vidInfo->vfmt->BitsPerPixel;
	}

	screen = SDL_SetVideoMode(screenWidth, screenHeight, screenBits,
		(usedoublebuffering ? SDL_HWSURFACE | SDL_DOUBLEBUF : 0) |
		(screenBits == 8 ? SDL_HWPALETTE : 0) |
		(fullscreen && !forSignon ? SDL_FULLSCREEN : 0));
	if(!screen)
	{
		printf("Unable to set %ix%ix%i video mode: %s\n", screenWidth,
			screenHeight, screenBits, SDL_GetError());
		exit(1);
	}
	if((screen->flags & SDL_DOUBLEBUF) != SDL_DOUBLEBUF)
		usedoublebuffering = false;
	SDL_ShowCursor(SDL_DISABLE);

	SDL_SetColors(screen, pal, 0, 256);
#endif

	SDL_FreeSurface(screenBuffer);
	screenBuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, screenWidth,
		screenHeight, 8, 0, 0, 0, 0);
	if(!screenBuffer)
	{
		printf("Unable to create screen buffer surface: %s\n", SDL_GetError());
		exit(1);
	}
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_SetPaletteColors(screenBuffer->format->palette, pal, 0, 256);
#else
	SDL_SetColors(screenBuffer, pal, 0, 256);
#endif

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
= VL_SetBlend
=
=================
*/

void DoBlending (const PalEntry *from, PalEntry *to, int count, int r, int g, int b, int a);
void VL_SetBlend(uint8_t red, uint8_t green, uint8_t blue, int amount, bool forceupdate)
{
	static PalEntry colors[256];

	currentBlend.r = red;
	currentBlend.g = green;
	currentBlend.b = blue;
	currentBlend.amount = amount;

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
#if SDL_VERSION_ATLEAST(2,0,0)
	if(curSurface)
	{
		SDL_SetPaletteColors(curSurface->format->palette, palette, 0, 256);
		if(forceupdate)
			VH_UpdateScreen();
	}
#else
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
#endif
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
	end <<= FRACBITS;
	start <<= FRACBITS;

	const fixed aStep = (end-start)/steps;

	VL_WaitVBL(1);

//
// fade through intermediate frames
//
	for (int a = start;(aStep < 0 ? a > end : a < end);a += aStep)
	{
		if(!usedoublebuffering || screenBits == 8) VL_WaitVBL(1);
		VL_SetBlend(red, green, blue, a>>FRACBITS, true);
	}

//
// final color
//
	VL_SetBlend (red,green,blue,end>>FRACBITS, true);

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
