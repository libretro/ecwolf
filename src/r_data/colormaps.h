#ifndef __RES_CMAP_H
#define __RES_CMAP_H

#include "v_palette.h"

void R_InitColormaps(void);
void R_DeinitColormaps(void);

void R_SetDefaultColormap (const char *name);	// [RH] change normal fadetable
extern uint8_t *realcolormaps;						// [RH] make the colormaps externally visible

struct FDynamicColormap
{
	void BuildLights ();

	uint8_t *Maps;
	PalEntry Color;
	PalEntry Fade;
	int Desaturate;
	FDynamicColormap *Next;
};

// For hardware-accelerated weapon sprites in colored sectors
struct FColormapStyle
{
	PalEntry Color;
	PalEntry Fade;
	int Desaturate;
	float FadeLevel;
};

enum
{
	NOFIXEDCOLORMAP = -1,
	INVERSECOLORMAP,	// the inverse map is used explicitly in a few places.
};


struct FSpecialColormap
{
	float ColorizeStart[3];
	float ColorizeEnd[3];
	uint8_t Colormap[256];
	PalEntry GrayscaleToColor[256];
};

extern TArray<FSpecialColormap> SpecialColormaps;

int AddSpecialColormap(float r1, float g1, float b1, float r2, float g2, float b2);

extern uint8_t DesaturateColormap[31][256];
extern "C" 
{
extern FDynamicColormap NormalLight;
}

FDynamicColormap *GetSpecialLights (PalEntry lightcolor, PalEntry fadecolor, int desaturate);


#endif
