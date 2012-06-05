// ID_VH.H

#ifndef __ID_VH_H__
#define __ID_VH_H__

#include "id_vl.h"
#include "wl_main.h"
#include "v_font.h"

#define WHITE			15			// graphics mode independant colors
#define BLACK			0
#define FIRSTCOLOR		1
#define SECONDCOLOR		12
#define F_WHITE			15
#define F_BLACK			0
#define F_FIRSTCOLOR	1
#define F_SECONDCOLOR	12

//===========================================================================

#define MAXSHIFTS	1

typedef struct
{
	int16_t height;
	int16_t location[256];
	int8_t width[256];
} fontstruct;


//===========================================================================

extern  byte            fontcolor;
extern	int             fontnumber;
extern	int             pa,px,py;

//
// mode independant routines
// coordinates in pixels, rounded to best screen res
// regions marked in double buffer
//

class FTexture;
enum MenuOffset
{
	MENU_NONE = 0,
	MENU_TOP = 1,
	MENU_CENTER = 2,
	MENU_BOTTOM = 3
};
void VWB_Clear(int color, int x1, int y1, int x2, int y2);
void VWB_DrawGraphic(FTexture *tex, int ix, int iy, MenuOffset menu=MENU_NONE, class FRemapTable *remap=NULL, bool stencil=false, BYTE stencilcolor=0);
void VirtualToRealCoords(double &x, double &y, double &w, double &h, double vwidth, double vheight, bool vbottom, bool handleaspect);
template<class T> void MenuToRealCoords(T &x, T &y, T &w, T &h, MenuOffset offset)
{
	x = centerx + (x-160)*scaleFactor;
	switch(offset)
	{
		default:
			y = screenHeight/2 + (y-100)*scaleFactor;
			break;
		case MENU_TOP:
			y *= scaleFactor;
			break;
		case MENU_BOTTOM:
			y = screenHeight + (y-200)*scaleFactor;
			break;
	}
	w *= scaleFactor;
	h *= scaleFactor;
}

void VWB_DrawPropString	 (const char *string, EColorRange translation=CR_UNTRANSLATED, bool stencil=false, BYTE stencilcolor=0);

void VWB_DrawTile8 (int x, int y, int tile);
//void VWB_DrawTile8M (int x, int y, int tile);
void VWB_DrawTile16 (int x, int y, int tile);
void VWB_DrawTile16M (int x, int y, int tile);
void VWB_DrawPic (int x, int y, const char* chunk, bool scaledCoord=false);
void VWB_DrawMPic(int x, int y, int chunknum);
void VWB_Bar (int x, int y, int width, int height, int color);
#define VWB_BarScaledCoord VL_BarScaledCoord
void VWB_Plot (int x, int y, int color);
#define VWB_PlotScaledCoord VW_Plot
void VWB_Hlin (int x1, int x2, int y, int color);
void VWB_Vlin (int y1, int y2, int x, int color);
#define VWB_HlinScaledCoord VW_Hlin
#define VWB_VlinScaledCoord VW_Vlin

void VH_UpdateScreen();
#define VW_UpdateScreen VH_UpdateScreen

//
// wolfenstein EGA compatability stuff
//


#define VW_Shutdown		    VL_Shutdown
#define VW_Bar			    VL_Bar
#define VW_Plot			    VL_Plot
#define VW_Hlin(x,z,y,c)	VL_Hlin(x,y,(z)-(x)+1,c)
#define VW_Vlin(y,z,x,c)	VL_Vlin(x,y,(z)-(y)+1,c)
#define VW_DrawPic		    VH_DrawPic
#define VW_WaitVBL		    VL_WaitVBL
#define VW_FadeIn()		    VL_FadeIn(0,255,gamepal,30);
#define VW_FadeOut()	    VL_FadeOut(0,255,0,0,0,30);
#define VW_ScreenToScreen	VL_ScreenToScreen
void	VW_MeasurePropString (const char *string, word &width, word &height, word *finalWidth=NULL);

//#define LatchDrawChar(x,y,p) VL_LatchToScreen(latchpics[0],((p)&7)*8,((p)>>3)*8*64,8,8,x,y)
//#define LatchDrawTile(x,y,p) VL_LatchToScreen(latchpics[1],(p)*64,0,16,16,x,y)

//void    LatchDrawPic (unsigned x, unsigned y, unsigned picnum);
//void    LatchDrawPicScaledCoord (unsigned scx, unsigned scy, unsigned picnum);
void    LoadLatchMem (void);

void    VH_Startup();
bool FizzleFade (SDL_Surface *source, int x1, int y1,
	unsigned width, unsigned height, unsigned frames, bool abortable);

#endif
