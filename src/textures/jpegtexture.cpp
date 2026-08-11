/*
** jpegtexture.cpp
** Texture class for JPEG images
**
**---------------------------------------------------------------------------
** Copyright 2006-2007 Randy Heit
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
**
*/

#include <stdio.h>
#include <stdlib.h>
extern "C"
{
#include "formats/rjpeg.h"
}

#include "wl_def.h"
#include "m_swap.h"
#include "files.h"
#include "w_wad.h"
#include "bitmap.h"
#include "textures.h"
#include "v_video.h"

#define TEXTCOLOR_ORANGE


// Decode an entire JPEG lump with rjpeg (libretro-common). Returns a
// malloc'd RGBA buffer (R at byte 0) the caller must free(), or NULL on any
// failure. rjpeg handles baseline and progressive 8-bit grayscale/YCbCr
// streams; CMYK -- which libjpeg accepted here before -- is not supported
// and fails cleanly.
static uint8_t *JPEG_DecodeRGBA (FWadLump &lump, unsigned *width, unsigned *height)
{
	long len = lump.GetLength();
	if (len <= 0)
		return NULL;

	uint8_t *raw = new uint8_t[len];
	lump.Seek(0, SEEK_SET);
	if (lump.Read(raw, len) != len)
	{
		delete[] raw;
		return NULL;
	}

	rjpeg_t *rj = rjpeg_alloc();
	if (rj == NULL)
	{
		delete[] raw;
		return NULL;
	}

	void *out = NULL;
	*width = *height = 0;
	rjpeg_set_buf_ptr(rj, raw, (size_t)len);
	rjpeg_set_out_rgba(rj, true);
	// rjpeg_process_image returns 0 for "call again" (progressive streams
	// decode one scan per call), 1 when the image is complete, negative on
	// error. Bound the loop defensively against a malformed stream that
	// never completes.
	int ok = 0;
	for (int iter = 0; iter < 4096 && ok == 0; iter++)
		ok = rjpeg_process_image(rj, &out, (size_t)len, width, height, true);
	rjpeg_free(rj);
	delete[] raw;

	if (ok <= 0 || out == NULL || *width == 0 || *height == 0)
	{
		if (out != NULL)
			free(out);
		printf("Failed to decode JPEG lump\n");
		return NULL;
	}
	return (uint8_t *)out;
}

//==========================================================================

class FJPEGTexture : public FTexture
{
public:
	FJPEGTexture (int lumpnum, int width, int height);
	~FJPEGTexture ();

	const uint8_t *GetColumn (unsigned int column, const Span **spans_out);
	const uint8_t *GetPixels ();
	void Unload ();
	FTextureFormat GetFormat ();
	int CopyTrueColorPixels(FBitmap *bmp, int x, int y, int rotate, FCopyInfo *inf = NULL);
	bool UseBasePalette();

protected:

	uint8_t *Pixels;
	Span DummySpans[2];

	void MakeTexture ();

	friend class FTexture;
};

//==========================================================================
//
//
//
//==========================================================================

FTexture *JPEGTexture_TryCreate(FileReader & data, int lumpnum)
{
	union
	{
		uint32_t dw;
		uint16_t w[2];
		uint8_t b[4];
	} first4bytes;

	data.Seek(0, SEEK_SET);
	if (data.Read(&first4bytes, 4) < 4) return NULL;

	if (first4bytes.b[0] != 0xFF || first4bytes.b[1] != 0xD8 || first4bytes.b[2] != 0xFF)
		return NULL;

	// Find the SOFn marker to extract the image dimensions,
	// where n is 0, 1, or 2 (other types are unsupported).
	while ((unsigned)first4bytes.b[3] - 0xC0 >= 3)
	{
		if (data.Read (first4bytes.w, 2) != 2)
		{
			return NULL;
		}
		data.Seek (BigShort(first4bytes.w[0]) - 2, SEEK_CUR);
		if (data.Read (first4bytes.b + 2, 2) != 2 || first4bytes.b[2] != 0xFF)
		{
			return NULL;
		}
	}
	if (data.Read (first4bytes.b, 3) != 3)
	{
		return NULL;
	}
	if (BigShort (first4bytes.w[0]) < 5)
	{
		return NULL;
	}
	if (data.Read (first4bytes.b, 4) != 4)
	{
		return NULL;
	}
	return new FJPEGTexture (lumpnum, BigShort(first4bytes.w[1]), BigShort(first4bytes.w[0]));
}

//==========================================================================
//
//
//
//==========================================================================

FJPEGTexture::FJPEGTexture (int lumpnum, int width, int height)
: FTexture(NULL, lumpnum), Pixels(0)
{
	UseType = TEX_MiscPatch;
	LeftOffset = 0;
	TopOffset = 0;
	bMasked = false;

	Width = width;
	Height = height;
	CalcBitSize ();

	DummySpans[0].TopOffset = 0;
	DummySpans[0].Length = Height;
	DummySpans[1].TopOffset = 0;
	DummySpans[1].Length = 0;
}

//==========================================================================
//
//
//
//==========================================================================

FJPEGTexture::~FJPEGTexture ()
{
	Unload ();
}

//==========================================================================
//
//
//
//==========================================================================

void FJPEGTexture::Unload ()
{
	if (Pixels != NULL)
	{
		delete[] Pixels;
		Pixels = NULL;
	}
}

//==========================================================================
//
//
//
//==========================================================================

FTextureFormat FJPEGTexture::GetFormat()
{
	return TEX_RGB;
}

//==========================================================================
//
//
//
//==========================================================================

const uint8_t *FJPEGTexture::GetColumn (unsigned int column, const Span **spans_out)
{
	if (Pixels == NULL)
	{
		MakeTexture ();
	}
	if ((unsigned)column >= (unsigned)Width)
	{
		if (WidthMask + 1 == Width)
		{
			column &= WidthMask;
		}
		else
		{
			column %= Width;
		}
	}
	if (spans_out != NULL)
	{
		*spans_out = DummySpans;
	}
	return Pixels + column*Height;
}

//==========================================================================
//
//
//
//==========================================================================

const uint8_t *FJPEGTexture::GetPixels ()
{
	if (Pixels == NULL)
	{
		MakeTexture ();
	}
	return Pixels;
}

//==========================================================================
//
//
//
//==========================================================================

void FJPEGTexture::MakeTexture ()
{
	FWadLump lump = Wads.OpenLumpNum (SourceLump);

	Pixels = new uint8_t[Width * Height];
	memset (Pixels, 0xBA, Width * Height);

	unsigned w = 0, h = 0;
	uint8_t *rgba = JPEG_DecodeRGBA (lump, &w, &h);
	if (rgba == NULL)
		return;

	// Convert to the paletted, column-major layout the software renderer
	// expects. rjpeg hands back straight RGBA for every stream it accepts
	// (grayscale decodes with r == g == b, which lands on the same gray
	// palette entries the old GrayMap path chose).
	int cw = MIN<int>(Width, (int)w);
	int ch = MIN<int>(Height, (int)h);
	for (int y = 0; y < ch; y++)
	{
		const uint8_t *in = rgba + (size_t)y * w * 4;
		uint8_t *out = Pixels + y;
		for (int x = 0; x < cw; x++)
		{
			*out = RGB32k[in[0] >> 3][in[1] >> 3][in[2] >> 3];
			out += Height;
			in += 4;
		}
	}
	free (rgba);
}


//===========================================================================
//
// FJPEGTexture::CopyTrueColorPixels
//
// Preserves the full color information (unlike software mode)
//
//===========================================================================

int FJPEGTexture::CopyTrueColorPixels(FBitmap *bmp, int x, int y, int rotate, FCopyInfo *inf)
{
	FWadLump lump = Wads.OpenLumpNum (SourceLump);

	unsigned w = 0, h = 0;
	uint8_t *rgba = JPEG_DecodeRGBA (lump, &w, &h);
	if (rgba == NULL)
		return 0;

	bmp->CopyPixelDataRGB (x, y, rgba, MIN<int>(Width, (int)w), MIN<int>(Height, (int)h),
		4, w * 4, rotate, CF_RGBA, inf);
	free (rgba);
	return 0;
}


//===========================================================================
//
//
//===========================================================================

bool FJPEGTexture::UseBasePalette() 
{ 
	return false; 
}
