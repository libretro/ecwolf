// ID_VL.C

#include <string.h>
#include "wl_def.h"
#include "id_vl.h"
#include "id_vh.h"
#include "w_wad.h"
#include "r_2d/r_main.h"
#include "r_data/colormaps.h"
#include "v_font.h"
#include "v_video.h"
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
//SDL_Surface *screen = NULL;
#endif

SDL_Surface *curSurface = NULL;
unsigned curPitch;

unsigned scaleFactorX, scaleFactorY;

bool	 screenfaded;

static struct
{
	uint8_t r,g,b;
	int amount;
} currentBlend;

//===========================================================================

void VL_ReadPalette(const char* lump)
{
	InitPalette(lump);
	if(currentBlend.amount)
		V_SetBlend(currentBlend.r, currentBlend.g, currentBlend.b, currentBlend.amount);
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

void I_InitGraphics ();
void	VL_SetVGAPlaneMode (bool forSignon)
{
	I_InitGraphics();
	Video->SetResolution(screenWidth, screenHeight, 8);
	R_SetupBuffer ();

	scaleFactorY = SCREENHEIGHT/200;
	scaleFactorX = SCREENWIDTH/320;
	// 1600x1200 can do clean aspect correction so why not?
	if(scaleFactorY % 6 == 0)
	{
		unsigned newXfactor = scaleFactorY - scaleFactorY/6;
		if(newXfactor <= scaleFactorX)
			scaleFactorX = newXfactor;
		else
			scaleFactorY = scaleFactorX;
	}
	else
		scaleFactorX = scaleFactorY = MIN(scaleFactorX, scaleFactorY);

	pixelangle = (short *) malloc(SCREENWIDTH * sizeof(short));
	CHECKMALLOCRESULT(pixelangle);
	wallheight = (int *) malloc(SCREENWIDTH * sizeof(int));
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
		V_SetBlend(red, green, blue, a>>FRACBITS);
		VH_UpdateScreen();
	}

//
// final color
//
	V_SetBlend (red,green,blue,end>>FRACBITS);
	// Not quite sure why I need to call this twice.
	VH_UpdateScreen();
	VH_UpdateScreen();

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

void VL_FadeIn (int start, int end, int steps)
{
	if(screenfaded)
		VL_Fade(end, start, fadeR, fadeG, fadeB, steps);
}

/*
=============================================================================

							PIXEL OPS

=============================================================================
*/

byte *VL_LockSurface()
{
	screen->Lock(false);
	return (byte *) screen->GetBuffer();
}

void VL_UnlockSurface()
{
	screen->Unlock();
}
