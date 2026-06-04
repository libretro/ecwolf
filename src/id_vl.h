// ID_VL.H

#ifndef __ID_VL_H__
#define __ID_VL_H__

//===========================================================================

extern  bool	fullscreen, usedoublebuffering;
extern  unsigned screenWidth, screenHeight, screenBits, curPitch;
extern  unsigned fullScreenWidth, fullScreenHeight;
extern  unsigned windowedScreenWidth, windowedScreenHeight;
extern  unsigned scaleFactorX, scaleFactorY;
extern	float	screenGamma;

extern	bool  screenfaded;

//===========================================================================

//
// VGA hardware routines
//

// VL_WaitVBL intentionally does nothing in the libretro core: frame pacing is
// owned by the frontend, and the engine must not sleep on a wall clock.
#define VL_WaitVBL(a) ((void)0)

void VL_ToggleFullscreen();
void VL_SetFullscreen(bool isFull);

void VL_ReadPalette(const char* lump);

void VL_SetVGAPlaneMode (bool forSignon=false);
void VL_SetTextMode (void);

void VL_Fade (int start, int end, int red, int green, int blue, int steps);
void VL_FadeOut     (int start, int end, int red, int green, int blue, int steps);
void VL_FadeIn      (int start, int end, int steps);

uint8_t *VL_LockSurface();
void VL_UnlockSurface();

#define VL_ClearScreen(color) VWB_Clear(color, 0, 0, SCREENWIDTH, SCREENHEIGHT)

#endif
