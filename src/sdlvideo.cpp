
// HEADER FILES ------------------------------------------------------------

#include "wl_def.h"
#include "c_cvars.h"

#include "templates.h"
//#include "i_system.h"
//#include "i_video.h"
#include "v_video.h"
#include "v_pfx.h"
//#include "stats.h"
#include "v_palette.h"
#include "sdlvideo.h"
//#include "r_swrenderer.h"
#include "thingdef/thingdef.h"

#include <SDL.h>

IVideo *Video = NULL;

extern float screenGamma;

DFrameBuffer *I_SetMode (int &width, int &height, DFrameBuffer *old)
{
	bool fs = false;
	switch (Video->GetDisplayType ())
	{
	case DISPLAY_WindowOnly:
		fs = false;
		break;
	case DISPLAY_FullscreenOnly:
		fs = true;
		break;
	case DISPLAY_Both:
		fs = vid_fullscreen;
		break;
	}
	DFrameBuffer *res = Video->CreateFrameBuffer (width, height, fs, old);

	/* Right now, CreateFrameBuffer cannot return NULL
	if (res == NULL)
	{
		I_FatalError ("Mode %dx%d is unavailable\n", width, height);
	}
	*/
	return res;
}

bool I_CheckResolution (int width, int height, int bits)
{
	int twidth, theight;

	Video->StartModeIterator (bits, screen ? screen->IsFullscreen() : vid_fullscreen);
	while (Video->NextMode (&twidth, &theight, NULL))
	{
		if (width == twidth && height == theight)
			return true;
	}
	return false;
}

void I_ClosestResolution (int *width, int *height, int bits)
{
	int twidth, theight;
	int cwidth = 0, cheight = 0;
	int iteration;
	DWORD closest = 0xFFFFFFFFu;

	for (iteration = 0; iteration < 2; iteration++)
	{
		Video->StartModeIterator (bits, screen ? screen->IsFullscreen() : vid_fullscreen);
		while (Video->NextMode (&twidth, &theight, NULL))
		{
			if (twidth == *width && theight == *height)
				return;

			if (iteration == 0 && (twidth < *width || theight < *height))
				continue;

			DWORD dist = (twidth - *width) * (twidth - *width)
				+ (theight - *height) * (theight - *height);

			if (dist < closest)
			{
				closest = dist;
				cwidth = twidth;
				cheight = theight;
			}
		}
		if (closest != 0xFFFFFFFFu)
		{
			*width = cwidth;
			*height = cheight;
			return;
		}
	}
}

//
// V_SetResolution
//
bool V_DoModeSetup (int width, int height, int bits)
{
	DFrameBuffer *buff = I_SetMode (width, height, screen);
	int cx1, cx2;

	if (buff == NULL)
	{
		return false;
	}

	screen = buff;
	GC::WriteBarrier(screen);
	screen->SetGamma (screenGamma);

	// Load fonts now so they can be packed into textures straight away,
	// if D3DFB is being used for the display.
	//FFont::StaticPreloadFonts();

	V_CalcCleanFacs(320, 200, width, height, &CleanXfac, &CleanYfac, &cx1, &cx2);

	CleanWidth = width / CleanXfac;
	CleanHeight = height / CleanYfac;
	assert(CleanWidth >= 320);
	assert(CleanHeight >= 200);

	if (width < 800 || width >= 960)
	{
		if (cx1 < cx2)
		{
			// Special case in which we don't need to scale down.
			CleanXfac_1 = 
			CleanYfac_1 = cx1;
		}
		else
		{
			CleanXfac_1 = MAX(CleanXfac - 1, 1);
			CleanYfac_1 = MAX(CleanYfac - 1, 1);
			// On larger screens this is not enough so make sure it's at most 3/4 of the screen's width
			while (CleanXfac_1 * 320 > screen->GetWidth()*3/4 && CleanXfac_1 > 2)
			{
				CleanXfac_1--;
				CleanYfac_1--;
			}
		}
		CleanWidth_1 = width / CleanXfac_1;
		CleanHeight_1 = height / CleanYfac_1;
	}
	else // if the width is between 800 and 960 the ratio between the screensize and CleanXFac-1 becomes too large.
	{
		CleanXfac_1 = CleanXfac;
		CleanYfac_1 = CleanYfac;
		CleanWidth_1 = CleanWidth;
		CleanHeight_1 = CleanHeight;
	}


	DisplayWidth = width;
	DisplayHeight = height;
	DisplayBits = bits;

	//R_OldBlend = ~0;
	//Renderer->OnModeSet();
	
	//M_RefreshModesList ();

	return true;
}

bool IVideo::SetResolution (int width, int height, int bits)
{
	int oldwidth, oldheight;
	int oldbits;

	if (screen)
	{
		oldwidth = SCREENWIDTH;
		oldheight = SCREENHEIGHT;
		oldbits = DisplayBits;
	}
	else
	{ // Harmless if screen wasn't allocated
		oldwidth = width;
		oldheight = height;
		oldbits = bits;
	}

	I_ClosestResolution (&width, &height, bits);
	if (!I_CheckResolution (width, height, bits))
	{ // Try specified resolution
		if (!I_CheckResolution (oldwidth, oldheight, oldbits))
		{ // Try previous resolution (if any)
	   		return false;
		}
		else
		{
			width = oldwidth;
			height = oldheight;
			bits = oldbits;
		}
	}
	return V_DoModeSetup (width, height, bits);
}

void IVideo::DumpAdapters ()
{
	Printf("Multi-monitor support unavailable.\n");
}

void I_ShutdownGraphics ()
{
	if (screen)
	{
		DFrameBuffer *s = screen;
		screen = NULL;
		s->ObjectFlags |= OF_YesReallyDelete;
		delete s;
	}
	if (Video)
		delete Video, Video = NULL;
}

void I_InitGraphics ()
{
	if(Video)
		return;

	Video = new SDLVideo (0);
	if (Video == NULL)
		I_FatalError ("Failed to initialize display");

//	Video->SetWindowedScale (vid_winscale);
}

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

class SDLFB : public DFrameBuffer
{
	DECLARE_CLASS(SDLFB, DFrameBuffer)
public:
	SDLFB (int width, int height, bool fullscreen);
	~SDLFB ();

	bool Lock (bool buffer);
	void Unlock ();
	bool Relock ();
	void ForceBuffering (bool force);
	bool IsValid ();
	void Update ();
	PalEntry *GetPalette ();
	void GetFlashedPalette (PalEntry pal[256]);
	void UpdatePalette ();
	bool SetGamma (float gamma);
	bool SetFlash (PalEntry rgb, int amount);
	void GetFlash (PalEntry &rgb, int &amount);
	int GetPageCount ();
	bool IsFullscreen ();

	void PaletteChanged () { }
	int QueryNewPalette () { return 0; }
	bool Is8BitMode() { return true; }

	friend class SDLVideo;

private:
	PalEntry SourcePalette[256];
	BYTE GammaTable[3][256];
	PalEntry Flash;
	int FlashAmount;
	float Gamma;
	bool UpdatePending;
	
	SDL_Surface *Screen;
	
	bool NeedPalUpdate;
	bool NeedGammaUpdate;
	bool NotPaletted;
	
	void UpdateColors ();

	SDLFB () {}
};
IMPLEMENT_INTERNAL_CLASS(SDLFB)

struct MiniModeInfo
{
	WORD Width, Height;
};

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern IVideo *Video;
//extern SDL_Surface *cursorSurface;
//extern SDL_Rect cursorBlit;
//extern bool GUICapture;

#define vid_maxfps 70
#define cl_capfps false
//EXTERN_CVAR (Float, Gamma)
//EXTERN_CVAR (Int, vid_maxfps)
//EXTERN_CVAR (Bool, cl_capfps)

// PUBLIC DATA DEFINITIONS -------------------------------------------------

#define vid_displaybits 8
//CVAR (Int, vid_displaybits, 8, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)

#define rgamma 1.f
#define ggamma 1.f
#define bgamma 1.f
#if 0
CUSTOM_CVAR (Float, rgamma, 1.f, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
{
	if (screen != NULL)
	{
		screen->SetGamma (Gamma);
	}
}
CUSTOM_CVAR (Float, ggamma, 1.f, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
{
	if (screen != NULL)
	{
		screen->SetGamma (Gamma);
	}
}
CUSTOM_CVAR (Float, bgamma, 1.f, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
{
	if (screen != NULL)
	{
		screen->SetGamma (Gamma);
	}
}
#endif

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// Dummy screen sizes to pass when windowed
static MiniModeInfo WinModes[] =
{
	{ 320, 200 },
	{ 320, 240 },
	{ 400, 225 },	// 16:9
	{ 400, 300 },
	{ 480, 270 },	// 16:9
	{ 480, 360 },
	{ 512, 288 },	// 16:9
	{ 512, 384 },
	{ 640, 360 },	// 16:9
	{ 640, 400 },
	{ 640, 480 },
	{ 720, 480 },	// 16:10
	{ 720, 540 },
	{ 800, 450 },	// 16:9
	{ 800, 500 },	// 16:10
	{ 800, 600 },
	{ 848, 480 },	// 16:9
	{ 960, 600 },	// 16:10
	{ 960, 720 },
	{ 1024, 576 },	// 16:9
	{ 1024, 600 },	// 17:10
	{ 1024, 640 },	// 16:10
	{ 1024, 768 },
	{ 1088, 612 },	// 16:9
	{ 1152, 648 },	// 16:9
	{ 1152, 720 },	// 16:10
	{ 1152, 864 },
	{ 1280, 720 },	// 16:9
	{ 1280, 800 },	// 16:10
	{ 1280, 960 },
	{ 1360, 768 },	// 16:9
	{ 1400, 787 },	// 16:9
	{ 1400, 875 },	// 16:10
	{ 1400, 1050 },
	{ 1600, 900 },	// 16:9
	{ 1600, 1000 },	// 16:10
	{ 1600, 1200 },
	{ 1920, 1080 },
	{ 1920, 1200 },
};

//static cycle_t BlitCycles;
//static cycle_t SDLFlipCycles;

// CODE --------------------------------------------------------------------

SDLVideo::SDLVideo (int parm)
{
	IteratorBits = 0;
	IteratorFS = false;
}

SDLVideo::~SDLVideo ()
{
}

void SDLVideo::StartModeIterator (int bits, bool fs)
{
	IteratorMode = 0;
	IteratorBits = bits;
	IteratorFS = fs;
}

bool SDLVideo::NextMode (int *width, int *height, bool *letterbox)
{
	if (IteratorBits != 8)
		return false;
	
	if (!IteratorFS)
	{
		if ((unsigned)IteratorMode < sizeof(WinModes)/sizeof(WinModes[0]))
		{
			*width = WinModes[IteratorMode].Width;
			*height = WinModes[IteratorMode].Height;
			++IteratorMode;
			return true;
		}
	}
	else
	{
		SDL_Rect **modes = SDL_ListModes (NULL, SDL_FULLSCREEN|SDL_HWSURFACE);
		if (modes != NULL && modes[IteratorMode] != NULL)
		{
			*width = modes[IteratorMode]->w;
			*height = modes[IteratorMode]->h;
			++IteratorMode;
			return true;
		}
	}
	return false;
}

DFrameBuffer *SDLVideo::CreateFrameBuffer (int width, int height, bool fullscreen, DFrameBuffer *old)
{
	static int retry = 0;
	static int owidth, oheight;
	
	PalEntry flashColor;
	int flashAmount;

	if (old != NULL)
	{ // Reuse the old framebuffer if its attributes are the same
		SDLFB *fb = static_cast<SDLFB *> (old);
		if (fb->Width == width &&
			fb->Height == height)
		{
			bool fsnow = (fb->Screen->flags & SDL_FULLSCREEN) != 0;
	
			if (fsnow == fullscreen)
				return old;
			if (fsnow != fullscreen)
			{
				if(SDL_WM_ToggleFullScreen (fb->Screen))
					return old;
			}
		}
		old->GetFlash (flashColor, flashAmount);
		old->ObjectFlags |= OF_YesReallyDelete;
		if (screen == old) screen = NULL;
		delete old;
	}
	else
	{
		flashColor = 0;
		flashAmount = 0;
	}
	
	SDLFB *fb = new SDLFB (width, height, fullscreen);
	retry = 0;
	
	// If we could not create the framebuffer, try again with slightly
	// different parameters in this order:
	// 1. Try with the closest size
	// 2. Try in the opposite screen mode with the original size
	// 3. Try in the opposite screen mode with the closest size
	// This is a somewhat confusing mass of recursion here.

	while (fb == NULL || !fb->IsValid ())
	{
		if (fb != NULL)
		{
			delete fb;
		}

		switch (retry)
		{
		case 0:
			owidth = width;
			oheight = height;
		case 2:
			// Try a different resolution. Hopefully that will work.
			I_ClosestResolution (&width, &height, 8);
			break;

		case 1:
			// Try changing fullscreen mode. Maybe that will work.
			width = owidth;
			height = oheight;
			fullscreen = !fullscreen;
			break;

		default:
			// I give up!
			I_FatalError ("Could not create new screen (%d x %d)", owidth, oheight);
		}

		++retry;
		fb = static_cast<SDLFB *>(CreateFrameBuffer (width, height, fullscreen, NULL));
	}

	fb->SetFlash (flashColor, flashAmount);

	return fb;
}

void SDLVideo::SetWindowedScale (float scale)
{
}

// FrameBuffer implementation -----------------------------------------------

extern bool usedoublebuffering;
SDLFB::SDLFB (int width, int height, bool fullscreen)
	: DFrameBuffer (width, height)
{
	int i;
	
	NeedPalUpdate = false;
	NeedGammaUpdate = false;
	UpdatePending = false;
	NotPaletted = false;
	FlashAmount = 0;
	
	Screen = SDL_SetVideoMode (width, height, vid_displaybits,
		SDL_HWSURFACE|SDL_HWPALETTE|SDL_DOUBLEBUF|SDL_ANYFORMAT|
		(fullscreen ? SDL_FULLSCREEN : 0));

	if (Screen == NULL)
		return;

	if((Screen->flags & SDL_DOUBLEBUF) != SDL_DOUBLEBUF)
		usedoublebuffering = false;

	for (i = 0; i < 256; i++)
	{
		GammaTable[0][i] = GammaTable[1][i] = GammaTable[2][i] = i;
	}
	if (Screen->format->palette == NULL)
	{
		NotPaletted = true;
		GPfx.SetFormat (Screen->format->BitsPerPixel,
			Screen->format->Rmask,
			Screen->format->Gmask,
			Screen->format->Bmask);
	}
	memcpy (SourcePalette, GPalette.BaseColors, sizeof(PalEntry)*256);
	UpdateColors ();
}

SDLFB::~SDLFB ()
{
}

bool SDLFB::IsValid ()
{
	return DFrameBuffer::IsValid() && Screen != NULL;
}

int SDLFB::GetPageCount ()
{
	return 1;
}

bool SDLFB::Lock (bool buffered)
{
	return DSimpleCanvas::Lock ();
}

bool SDLFB::Relock ()
{
	return DSimpleCanvas::Lock ();
}

void SDLFB::Unlock ()
{
	if (UpdatePending && LockCount == 1)
	{
		Update ();
	}
	else if (--LockCount <= 0)
	{
		Buffer = NULL;
		LockCount = 0;
	}
}

void SDLFB::Update ()
{
	if (LockCount != 1)
	{
		if (LockCount > 0)
		{
			UpdatePending = true;
			--LockCount;
		}
		return;
	}

	DrawRateStuff ();

#if 0
#ifndef __APPLE__
	if(vid_maxfps && !cl_capfps)
	{
		SEMAPHORE_WAIT(FPSLimitSemaphore)
	}
#endif
#endif

	Buffer = NULL;
	LockCount = 0;
	UpdatePending = false;

	//BlitCycles.Reset();
	//SDLFlipCycles.Reset();
	//BlitCycles.Clock();

	if (SDL_LockSurface (Screen) == -1)
		return;

	if (NotPaletted)
	{
		GPfx.Convert (MemBuffer, Pitch,
			Screen->pixels, Screen->pitch, Width, Height,
			FRACUNIT, FRACUNIT, 0, 0);
	}
	else
	{
		if (Screen->pitch == Pitch)
		{
			memcpy (Screen->pixels, MemBuffer, Width*Height);
		}
		else
		{
			for (int y = 0; y < Height; ++y)
			{
				memcpy ((BYTE *)Screen->pixels+y*Screen->pitch, MemBuffer+y*Pitch, Width);
			}
		}
	}
	
	SDL_UnlockSurface (Screen);

#if 0
	if (cursorSurface != NULL && GUICapture)
	{
		// SDL requires us to draw a surface to get true color cursors.
		SDL_BlitSurface(cursorSurface, NULL, Screen, &cursorBlit);
	}
#endif

	//SDLFlipCycles.Clock();
	SDL_Flip (Screen);
	//SDLFlipCycles.Unclock();

	//BlitCycles.Unclock();

	if (NeedGammaUpdate)
	{
		bool Windowed = false;
		NeedGammaUpdate = false;
		CalcGamma ((Windowed || rgamma == 0.f) ? Gamma : (Gamma * rgamma), GammaTable[0]);
		CalcGamma ((Windowed || ggamma == 0.f) ? Gamma : (Gamma * ggamma), GammaTable[1]);
		CalcGamma ((Windowed || bgamma == 0.f) ? Gamma : (Gamma * bgamma), GammaTable[2]);
		NeedPalUpdate = true;
	}
	
	if (NeedPalUpdate)
	{
		NeedPalUpdate = false;
		UpdateColors ();
	}
}

void SDLFB::UpdateColors ()
{
	if (NotPaletted)
	{
		PalEntry palette[256];
		
		for (int i = 0; i < 256; ++i)
		{
			palette[i].r = GammaTable[0][SourcePalette[i].r];
			palette[i].g = GammaTable[1][SourcePalette[i].g];
			palette[i].b = GammaTable[2][SourcePalette[i].b];
		}
		if (FlashAmount)
		{
			DoBlending (palette, palette,
				256, GammaTable[0][Flash.r], GammaTable[1][Flash.g], GammaTable[2][Flash.b],
				FlashAmount);
		}
		GPfx.SetPalette (palette);
	}
	else
	{
		SDL_Color colors[256];
		
		for (int i = 0; i < 256; ++i)
		{
			colors[i].r = GammaTable[0][SourcePalette[i].r];
			colors[i].g = GammaTable[1][SourcePalette[i].g];
			colors[i].b = GammaTable[2][SourcePalette[i].b];
		}
		if (FlashAmount)
		{
			DoBlending ((PalEntry *)colors, (PalEntry *)colors,
				256, GammaTable[2][Flash.b], GammaTable[1][Flash.g], GammaTable[0][Flash.r],
				FlashAmount);
		}
		SDL_SetPalette (Screen, SDL_LOGPAL|SDL_PHYSPAL, colors, 0, 256);
	}
}

PalEntry *SDLFB::GetPalette ()
{
	return SourcePalette;
}

void SDLFB::UpdatePalette ()
{
	NeedPalUpdate = true;
}

bool SDLFB::SetGamma (float gamma)
{
	Gamma = gamma;
	NeedGammaUpdate = true;
	return true;
}

bool SDLFB::SetFlash (PalEntry rgb, int amount)
{
	Flash = rgb;
	FlashAmount = amount;
	NeedPalUpdate = true;
	return true;
}

void SDLFB::GetFlash (PalEntry &rgb, int &amount)
{
	rgb = Flash;
	amount = FlashAmount;
}

// Q: Should I gamma adjust the returned palette?
void SDLFB::GetFlashedPalette (PalEntry pal[256])
{
	memcpy (pal, SourcePalette, 256*sizeof(PalEntry));
	if (FlashAmount)
	{
		DoBlending (pal, pal, 256, Flash.r, Flash.g, Flash.b, FlashAmount);
	}
}

bool SDLFB::IsFullscreen ()
{
	return (Screen->flags & SDL_FULLSCREEN) != 0;
}

#if 0
ADD_STAT (blit)
{
	FString out;
	out.Format ("blit=%04.1f ms  flip=%04.1f ms",
		BlitCycles.Time() * 1e-3, SDLFlipCycles.TimeMS());
	return out;
}
#endif
