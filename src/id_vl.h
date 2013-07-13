// ID_VL.H

#ifndef __ID_VL_H__
#define __ID_VL_H__

// wolf compatability

void Quit (const char *error,...);

//===========================================================================

#if SDL_VERSION_ATLEAST(2,0,0)
extern SDL_Window *window;
extern SDL_Renderer *screenRenderer;
extern SDL_Texture *screen;
#else
//extern SDL_Surface *screen;
#endif
extern SDL_Surface *screenBuffer, *curSurface;

extern  bool	fullscreen, usedoublebuffering;
extern  unsigned screenWidth, screenHeight, screenBits, bufferPitch, curPitch;
extern  unsigned scaleFactorX, scaleFactorY;

extern	bool  screenfaded;

extern SDL_Color gamepal[256];

//===========================================================================

//
// VGA hardware routines
//

#define VL_WaitVBL(a) SDL_Delay((a)*8)

void VL_ReadPalette(const char* lump);

void VL_SetVGAPlaneMode (bool forSignon=false);
void VL_SetTextMode (void);

void VL_SetBlend(uint8_t red, uint8_t green, uint8_t blue, int amount, bool forceupdate=false);
void VL_SetPalette  (struct PalEntry *palette, bool forceupdate);
void VL_SetPalette  (SDL_Color *palette, bool forceupdate);
void VL_FadeOut     (int start, int end, int red, int green, int blue, int steps);
void VL_FadeIn      (int start, int end, SDL_Color *palette, int steps);

byte *VL_LockSurface(SDL_Surface *surface);
void VL_UnlockSurface(SDL_Surface *surface);

#include "v_video.h"
#define VL_ClearScreen(color) VWB_Clear(color, 0, 0, SCREENWIDTH, SCREENHEIGHT)

#endif
