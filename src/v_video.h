/*
** v_video.h
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

#ifndef __V_VIDEO_H__
#define __V_VIDEO_H__

#include "actor.h"
#include "dobject.h"
#include "vectors.h"
#include "zdoomsupport.h"
#include "r_data/renderstyle.h"

extern int CleanWidth, CleanHeight, CleanXfac, CleanYfac;
extern int CleanWidth_1, CleanHeight_1, CleanXfac_1, CleanYfac_1;
extern int DisplayWidth, DisplayHeight, DisplayBits;

bool V_DoModeSetup (int width, int height, int bits);
void V_CalcCleanFacs (int designwidth, int designheight, int realwidth, int realheight, int *cleanx, int *cleany, int *cx1=NULL, int *cx2=NULL);

struct framebuffer_t;

// Create the framebuffer for the given mode and make it current (replaces any
// existing one, preserving the palette). Was IVideo::SetResolution; the
// single-backend IVideo/LibretroVideo factory it lived on has been removed.
bool V_SetResolution (int width, int height);

class FTexture;

// TagItem definitions for DrawTexture. As far as I know, tag lists
// originated on the Amiga.
//
// Think of TagItems as an array of the following structure:
//
// struct TagItem {
//     uint32_t ti_Tag;
//     uint32_t ti_Data;
// };

#define TAG_DONE	(0)  /* Used to indicate the end of the Tag list */
#define TAG_END		(0)  /* Ditto									*/
#define TAG_IGNORE	(1)  /* Ignore this Tag							*/
#define TAG_MORE	(2)  /* Ends this list and continues with the	*/
						 /* list pointed to in ti_Data 				*/

#define TAG_USER	((uint32_t)(1u<<30))

enum
{
	DTA_Base = TAG_USER + 5000,
	DTA_DestWidth,		// width of area to draw to
	DTA_DestHeight,		// height of area to draw to
	DTA_Alpha,			// alpha value for translucency
	DTA_FillColor,		// color to stencil onto the destination
	DTA_Translation,	// translation table to recolor the source
	DTA_AlphaChannel,	// bool: the source is an alpha channel; used with DTA_FillColor
	DTA_Clean,			// bool: scale texture size and position by CleanXfac and CleanYfac
	DTA_320x200,		// bool: scale texture size and position to fit on a virtual 320x200 screen
	DTA_Bottom320x200,	// bool: same as DTA_320x200 but centers virtual screen on bottom for 1280x1024 targets
	DTA_CleanNoMove,	// bool: like DTA_Clean but does not reposition output position
	DTA_CleanNoMove_1,	// bool: like DTA_CleanNoMove, but uses Clean[XY]fac_1 instead
	DTA_FlipX,			// bool: flip image horizontally	//FIXME: Does not work with DTA_Window(Left|Right)
	DTA_ShadowColor,	// color of shadow
	DTA_ShadowAlpha,	// alpha of shadow
	DTA_Shadow,			// set shadow color and alphas to defaults
	DTA_VirtualWidth,	// pretend the canvas is this wide
	DTA_VirtualHeight,	// pretend the canvas is this tall
	DTA_TopOffset,		// override texture's top offset
	DTA_LeftOffset,		// override texture's left offset
	DTA_CenterOffset,	// bool: override texture's left and top offsets and set them for the texture's middle
	DTA_CenterBottomOffset,// bool: override texture's left and top offsets and set them for the texture's bottom middle
	DTA_WindowLeft,		// don't draw anything left of this column (on source, not dest)
	DTA_WindowRight,	// don't draw anything at or to the right of this column (on source, not dest)
	DTA_ClipTop,		// don't draw anything above this row (on dest, not source)
	DTA_ClipBottom,		// don't draw anything at or below this row (on dest, not source)
	DTA_ClipLeft,		// don't draw anything to the left of this column (on dest, not source)
	DTA_ClipRight,		// don't draw anything at or to the right of this column (on dest, not source)
	DTA_Masked,			// true(default)=use masks from texture, false=ignore masks
	DTA_HUDRules,		// use fullscreen HUD rules to position and size textures
	DTA_KeepRatio,		// doesn't adjust screen size for DTA_Virtual* if the aspect ratio is not 4:3
	DTA_RenderStyle,	// same as render style for actors
	DTA_ColorOverlay,	// uint32_t: ARGB to overlay on top of image; limited to black for software
	DTA_BilinearFilter,	// bool: apply bilinear filtering to the image
	DTA_SpecialColormap,// pointer to FSpecialColormapParameters (likely to be forever hardware-only)
	DTA_ColormapStyle,	// pointer to FColormapStyle (hardware-only)
	DTA_Fullscreen,		// Draw image fullscreen (same as DTA_VirtualWidth/Height with graphics size.)

	// floating point duplicates of some of the above:
	DTA_DestWidthF,
	DTA_DestHeightF,
	DTA_TopOffsetF,
	DTA_LeftOffsetF,
	DTA_VirtualWidthF,
	DTA_VirtualHeightF,
	DTA_WindowLeftF,
	DTA_WindowRightF,

	// For DrawText calls:
	DTA_TextLen,		// stop after this many characters, even if \0 not hit
	DTA_CellX,			// horizontal size of character cell
	DTA_CellY,			// vertical size of character cell
};

enum
{
	HUD_Normal,
	HUD_HorizCenter
};

class FFont;
struct FRemapTable;
class player_t;

//
// VIDEO
//
// [RH] Made screens more implementation-independant:
//
// The framebuffer. Pure data (no methods): the libretro core has exactly one
// video surface, held by the global 'screen' below. The 8bpp indexed Buffer is
// rendered through the palette into a 16- or 32-bpp output (bpp, runtime). All
// behaviour lives in the free functions declared after this struct and in
// libretro.cpp (present path); v_draw/v_text/v_video hold the 2D drawing.
struct framebuffer_t
{
	uint8_t *Buffer;          // 8bpp indexed render target
	int Width;
	int Height;
	int Pitch;
	int bpp;                  // 16 or 32: output pixel format, runtime

	void        *lr_buffer_;       // fallback output buffer (bpp-sized pixels)
	int          lr_pitch_;        // in bytes
	int          width_, height_;
	int          FlashAmount;
	bool         PaletteNeedsUpdate;
	PalEntry     Flash;
	PalEntry     SourcePalette[256];
	PalEntry     FlashedPalette[256];
	uint32_t     effective_palette_[256];   // widest form; used as 16/32 per bpp
};

// This is the screen updated by I_FinishUpdate.
extern framebuffer_t *screen;

// Lifecycle / present (libretro.cpp)
framebuffer_t *V_NewFrameBuffer (int width, int height, int bpp);
void V_DeleteFrameBuffer (framebuffer_t *fb);
void V_Update ();
void V_ShowFrame ();
bool V_SetFlash (PalEntry rgb, int amount);
void V_GetFlash (PalEntry &rgb, int &amount);

// Accessors
inline uint8_t *V_GetBuffer () { return screen->Buffer; }
inline int      V_GetWidth ()  { return screen->Width; }
inline int      V_GetHeight () { return screen->Height; }
inline int      V_GetPitch ()  { return screen->Pitch; }
inline bool     V_IsValid ()   { return screen->Buffer != NULL; }
inline PalEntry *V_GetPalette () { return screen->SourcePalette; }
inline void     V_UpdatePalette () { screen->PaletteNeedsUpdate = true; }

// 2D drawing (v_draw.cpp / v_text.cpp / v_video.cpp)
void V_Dim (PalEntry color = 0);
void V_DimRect (PalEntry color, float amount, int x1, int y1, int w, int h);
void V_FlatFill (int left, int top, int right, int bottom, FTexture *src, bool local_origin=false);
void V_FillSimplePoly(FTexture *tex, FVector2 *points, int npoints,
	double originx, double originy, double scalex, double scaley, angle_t rotation,
	struct FDynamicColormap *colormap, int lightlevel, int palcolor=0, uint32_t rgbcolor=0);
void V_Clear (int left, int top, int right, int bottom, int palcolor, uint32_t color);
void V_DrawLine(int x0, int y0, int x1, int y1, int palColor, uint32_t realcolor);
void STACK_ARGS V_DrawTexture (FTexture *img, double x, double y, int tags, ...);
void V_VirtualToRealCoords(double &x, double &y, double &w, double &h, double vwidth, double vheight, bool vbottom=false, bool handleaspect=true);
void STACK_ARGS V_DrawText (FFont *font, int normalcolor, int x, int y, const char *string, ...);
void V_DrawTextV (FFont *font, int normalcolor, int x, int y, const char *string, va_list tags);

#define SCREENWIDTH (screen->Width)
#define SCREENHEIGHT (screen->Height)
#define SCREENPITCH (screen->Pitch)

// Col2RGB8 is a pre-multiplied palette for color lookup. It is stored in a
// special R10B10G10 format for efficient blending computation.
//		--RRRRRrrr--BBBBBbbb--GGGGGggg--   at level 64
//		--------rrrr------bbbb------gggg   at level 1
extern "C" uint32_t Col2RGB8[65][256];

// Col2RGB8_LessPrecision is the same as Col2RGB8, but the LSB for red
// and blue are forced to zero, so if the blend overflows, it won't spill
// over into the next component's value.
//		--RRRRRrrr-#BBBBBbbb-#GGGGGggg--  at level 64
//      --------rrr#------bbb#------gggg  at level 1
extern "C" uint32_t *Col2RGB8_LessPrecision[65];

// Col2RGB8_Inverse is the same as Col2RGB8_LessPrecision, except the source
// palette has been inverted.
extern "C" uint32_t Col2RGB8_Inverse[65][256];

// "Magic" numbers used during the blending:
//		--000001111100000111110000011111	= 0x01f07c1f
//		-0111111111011111111101111111111	= 0x3FEFFBFF
//		-1000000000100000000010000000000	= 0x40100400
//		------10000000001000000000100000	= 0x40100400 >> 5
//		--11111-----11111-----11111-----	= 0x40100400 - (0x40100400 >> 5) aka "white"
//		--111111111111111111111111111111	= 0x3FFFFFFF

// 16-bit Lookup Table
extern uint8_t RGB32k[32][32][32];
void GenerateLookupTables();

// Returns the closest color to the one desired. String
// should be of the form "rr gg bb".
int V_GetColorFromString (const uint32_t *palette, const char *colorstring);
// Similar to above, but can handle names
int V_GetColor (const uint32_t *palette, const char *str);

#endif /* __V_VIDEO_H__ */
