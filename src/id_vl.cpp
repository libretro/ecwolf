// ID_VL.C

#include <string.h>
#include "c_cvars.h"
#include "wl_def.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "w_wad.h"
#include "r_2d/r_main.h"
#include "r_data/colormaps.h"
#include "v_font.h"
#include "v_video.h"
#include "v_palette.h"
#include "wl_draw.h"
#include "wl_game.h"
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

unsigned screenWidth = 640;
unsigned screenHeight = 480;
unsigned screenBits = static_cast<unsigned> (-1);      // use "best" color depth according to libSDL

unsigned curPitch;

unsigned scaleFactorX, scaleFactorY;

bool	 screenfaded;

static struct
{
	uint8_t r,g,b;
	int amount;
} currentBlend;

//===========================================================================

//===========================================================================

// The colormaps, the texture manager's colour caches and the font
// translations are all a pure function of the loaded palette, so they are
// rebuilt only when the palette bytes differ from the ones they were built
// from. GPalette.Remap is derived from BaseColors, and the blend only
// reaches screen->Flash, so BaseColors alone identifies the tables.
static bool paletteTablesValid = false;
static PalEntry paletteTablesBase[256];

// Called when the WAD collection is torn down: the next palette read must
// rebuild against the incoming content's colormap lumps and fonts even when
// it loads the same colours.
void VL_InvalidatePaletteTables()
{
	paletteTablesValid = false;
}

void VL_ReadPalette(const char* lump)
{
	InitPalette(lump);
	if(currentBlend.amount)
		V_SetBlend(currentBlend.r, currentBlend.g, currentBlend.b, currentBlend.amount);

	if(paletteTablesValid
	   && memcmp(paletteTablesBase, GPalette.BaseColors, sizeof(paletteTablesBase)) == 0)
		return;

	memcpy(paletteTablesBase, GPalette.BaseColors, sizeof(paletteTablesBase));
	paletteTablesValid = true;

	R_InitColormaps();
	TexMan.InvalidatePalette();
	V_RetranslateFonts();
}

/*
=======================
=
= VL_SetVGAPlaneMode
=
=======================
*/

void	VL_SetVGAPlaneMode (bool /*forSignon*/)
{
	V_SetResolution(screenWidth, screenHeight);
	R_SetupBuffer ();

	scaleFactorX = CleanXfac;
	scaleFactorY = CleanYfac;

	pixelangle = new short[SCREENWIDTH];
	wallheight = new int[SCREENWIDTH];

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

uint8_t *VL_LockSurface()
{
	return (uint8_t *) V_GetBuffer();
}

void VL_UnlockSurface()
{
}
