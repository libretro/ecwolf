#include "colormatcher.h"
#include "v_video.h"
#include "thingdef/thingdef.h"
#include "wl_main.h"

extern "C" {
uint32_t Col2RGB8[65][256];
uint32_t *Col2RGB8_LessPrecision[65];
uint32_t Col2RGB8_Inverse[65][256];
}

//==========================================================================
//
// DCanvas :: FlatFill
//
// Fill an area with a texture. If local_origin is false, then the origin
// used for the wrapping is (0,0). Otherwise, (left,right) is used.
//
//==========================================================================

void DCanvas::FlatFill (int left, int top, int right, int bottom, FTexture *src, bool local_origin)
{
	int w = src->GetWidth();
	int h = src->GetHeight();

	// Repeatedly draw the texture, left-to-right, top-to-bottom.
	for (int y = local_origin ? top : (top / h * h); y < bottom; y += h)
	{
		for (int x = local_origin ? left : (left / w * w); x < right; x += w)
		{
			DrawTexture (src, x, y,
				DTA_ClipLeft, left,
				DTA_ClipRight, right,
				DTA_ClipTop, top,
				DTA_ClipBottom, bottom,
				DTA_TopOffset, 0,
				DTA_LeftOffset, 0,
				TAG_DONE);
		}
	}
}

//==========================================================================
//
// DCanvas :: Dim
//
// Applies a colored overlay to the entire screen.
//
//==========================================================================

void DCanvas::Dim (PalEntry color)
{
	PalEntry dimmer = PalEntry(0xffd700);
	float amount    = -1.0f;

	// Add the cvar's dimming on top of the color passed to the function
	if (color.a != 0)
	{
		float dim[4] = { color.r/255.f, color.g/255.f, color.b/255.f, color.a/255.f };
		dimmer = PalEntry (uint8_t(dim[0]*255), uint8_t(dim[1]*255), uint8_t(dim[2]*255));
		amount = dim[3];
	}
	Dim (dimmer, amount, 0, 0, Width, Height);
}

//==========================================================================
//
// DCanvas :: Dim
//
// Applies a colored overlay to an area of the screen.
//
//==========================================================================

void DCanvas::Dim (PalEntry color, float damount, int x1, int y1, int w, int h)
{
	if (damount == 0.f)
		return;

	uint32_t *bg2rgb;
	uint32_t fg;
	int gap;
	uint8_t *spot;
	int x, y;

	if (x1 >= Width || y1 >= Height)
		return;
	if (x1 + w > Width)
		w = Width - x1;
	if (y1 + h > Height)
		h = Height - y1;
	if (w <= 0 || h <= 0)
		return;

	{
		int amount = (int)(damount * 64);
		bg2rgb = Col2RGB8[64-amount];

		fg = (((color.r * amount) >> 4) << 20) |
			  ((color.g * amount) >> 4) |
			 (((color.b * amount) >> 4) << 10);
	}

	spot = Buffer + x1 + y1*Pitch;
	gap = Pitch - w;
	for (y = h; y != 0; y--)
	{
		for (x = w; x != 0; x--)
		{
			uint32_t bg = bg2rgb[(*spot)&0xff];
			bg = (fg+bg) | 0x1f07c1f;
			*spot = RGB32k[0][0][bg&(bg>>15)];
			spot++;
		}
		spot += gap;
	}
}

// -----------------------------------------------------------------------------

int DisplayWidth, DisplayHeight, DisplayBits;
uint8_t RGB32k[32][32][32];

// [RH] The framebuffer is no longer a mere uint8_t array.
// There's also only one, not four.
DCanvas *screen;

void GenerateLookupTables(void)
{
	static uint32_t Col2RGB8_2[63][256];

	// create the RGB555 lookup table
	for(int r = 0;r < 32;r++)
		for(int g = 0;g < 32;g++)
			for(int b = 0;b < 32;b++)
				RGB32k[r][g][b] = ColorMatcher.Pick((r<<3)|(r>>2), (g<<3)|(g>>2), (b<<3)|(b>>2));

	// create the swizzled palette
	for (int x = 0; x < 65; x++)
		for (int y = 0; y < 256; y++)
			Col2RGB8[x][y] = (((GPalette.BaseColors[y].r*x)>>4)<<20) |
							  ((GPalette.BaseColors[y].g*x)>>4) |
							 (((GPalette.BaseColors[y].b*x)>>4)<<10);

	// create the swizzled palette with the lsb of red and blue forced to 0
	// (for green, a 1 is okay since it never gets added into)
	for (int x = 1; x < 64; x++)
	{
		Col2RGB8_LessPrecision[x] = Col2RGB8_2[x-1];
		for (int y = 0; y < 256; y++)
		{
			Col2RGB8_2[x-1][y] = Col2RGB8[x][y] & 0x3feffbff;
		}
	}
	Col2RGB8_LessPrecision[0] = Col2RGB8[0];
	Col2RGB8_LessPrecision[64] = Col2RGB8[64];

	// create the inverse swizzled palette
	for (int x = 0; x < 65; x++)
		for (int y = 0; y < 256; y++)
		{
			Col2RGB8_Inverse[x][y] = (((((255-GPalette.BaseColors[y].r)*x)>>4)<<20) |
									  (((255-GPalette.BaseColors[y].g)*x)>>4) |
									  ((((255-GPalette.BaseColors[y].b)*x)>>4)<<10)) & 0x3feffbff;
		}
}

int ParseHex(const char* hex)
{
	int num = 0;
	int i = 1;
	do
	{
		int digit;
		char cdigit = *hex++;
		// Switch to uppercase for valid range
		if(cdigit >= 'a' && cdigit <= 'f')
			cdigit += 'A'-'a';
		// Check range and convert to integer
		if(cdigit >= '0' && cdigit <= '9')
			digit = cdigit - '0';
		else if(cdigit >= 'A' && cdigit <= 'F')
			digit = cdigit - 'A' + 0xA;
		else
			return 0;

		num |= digit<<(4*i);
		--i;
	}
	while(i >= 0 && *hex != '\0');

	return num;
}

//==========================================================================
//
// V_GetColorFromString
//
// Passed a string of the form "#RGB", "#RRGGBB", "R G B", or "RR GG BB",
// returns a number representing that color. If palette is non-NULL, the
// index of the best match in the palette is returned, otherwise the
// RRGGBB value is returned directly.
//
//==========================================================================

int V_GetColorFromString (const uint32_t *palette, const char *cstr)
{
	int c[3], i, p;
	char val[3];

	val[2] = '\0';

	// Check for HTML-style #RRGGBB or #RGB color string
	if (cstr[0] == '#')
	{
		size_t len = strlen (cstr);

		if (len == 7)
		{
			// Extract each eight-bit component into c[].
			for (i = 0; i < 3; ++i)
			{
				val[0] = cstr[1 + i*2];
				val[1] = cstr[2 + i*2];
				c[i] = ParseHex (val);
			}
		}
		else if (len == 4)
		{
			// Extract each four-bit component into c[], expanding to eight bits.
			for (i = 0; i < 3; ++i)
			{
				val[1] = val[0] = cstr[1 + i];
				c[i] = ParseHex (val);
			}
		}
		else
		{
			// Bad HTML-style; pretend it's black.
			c[2] = c[1] = c[0] = 0;
		}
	}
	else
	{
		if (strlen(cstr) == 6)
		{
			char *p;
			int color = strtol(cstr, &p, 16);
			if (*p == 0)
			{
				// RRGGBB string
				c[0] = (color & 0xff0000) >> 16;
				c[1] = (color & 0xff00) >> 8;
				c[2] = (color & 0xff);
			}
			else goto normal;
		}
		else
		{
normal:
			// Treat it as a space-delemited hexadecimal string
			for (i = 0; i < 3; ++i)
			{
				// Skip leading whitespace
				while (*cstr <= ' ' && *cstr != '\0')
					cstr++;
				// Extract a component and convert it to eight-bit
				for (p = 0; *cstr > ' '; ++p, ++cstr)
				{
					if (p < 2)
						val[p] = *cstr;
				}
				if (p == 0)
					c[i] = 0;
				else
				{
					if (p == 1)
						val[1] = val[0];
					c[i] = ParseHex (val);
				}
			}
		}
	}
	if (palette)
		return ColorMatcher.Pick (c[0], c[1], c[2]);
	return MAKERGB(c[0], c[1], c[2]);
}

//==========================================================================
//
// V_GetColor
//
// Works like V_GetColorFromString(), but also understands X11 color names.
//
//==========================================================================

int V_GetColor (const uint32_t *palette, const char *str)
{
	return V_GetColorFromString(palette, str);
}

void V_CalcCleanFacs (int designwidth, int designheight, int realwidth, int realheight, int *cleanx, int *cleany, int *_cx1, int *_cx2)
{
	int cwidth;
	int cheight;
	int cx1, cy1, cx2, cy2;
	int ratio = CheckRatio(realwidth, realheight);
	if (ratio & 4)
	{
		cwidth = realwidth;
		cheight = realheight * AspectCorrection[ratio].multiplier / 48;
	}
	else
	{
		cwidth = realwidth * AspectCorrection[ratio].multiplier / 48;
		cheight = realheight;
	}
	// Use whichever pair of cwidth/cheight or width/height that produces less difference
	// between CleanXfac and CleanYfac.
	cx1 = MAX(cwidth / designwidth, 1);
	cy1 = MAX(cheight / designheight, 1);
	cx2 = MAX(realwidth / designwidth, 1);
	cy2 = MAX(realheight / designheight, 1);
	if (abs(cx1 - cy1) <= abs(cx2 - cy2))
	{ // e.g. 640x360 looks better with this.
		*cleanx = cx1;
		*cleany = cy1;
	}
	else
	{ // e.g. 720x480 looks better with this.
		*cleanx = cx2;
		*cleany = cy2;
	}

	// 1600x1200 can do clean aspect correction so why not?
	if (*cleany % 6 == 0)
	{
		int newXfactor = *cleany - *cleany/6;
		if(newXfactor <= *cleanx)
			*cleanx = newXfactor;
		else
			*cleany = *cleanx;
	}
	else if (*cleanx > 1 && *cleany > 1 && *cleanx != *cleany)
	{
		if (*cleanx < *cleany)
			*cleany = *cleanx;
		else
			*cleanx = *cleany;
	}
	if (_cx1 != NULL)	*_cx1 = cx1;
	if (_cx2 != NULL)	*_cx2 = cx2;
}
