/*
** pngtexture.cpp
** Texture class for DDS images
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
** DDS is short for "DirectDraw Surface" and is essentially that. It's
** interesting to us because it is a standard file format for DXTC/S3TC
** encoded images. Look up "DDS File Reference" in the DirectX SDK or
** the online MSDN documentation to the specs for this file format. Look up
** "Compressed Texture Resources" for information about DXTC encoding.
**
** Perhaps the most important part of DXTC to realize is that every 4x4
** pixel block can only have four different colors, and only two of those
** are discrete. So depending on the texture, there may be very noticable
** quality degradation, or it may look virtually indistinguishable from
** the uncompressed texture.
**
** Note: Although this class supports reading RGB textures from a DDS,
** DO NOT use DDS images with plain RGB data. PNG does everything useful
** better. Since DDS lets the R, G, B, and A components lie anywhere in
** the pixel data, it is fairly inefficient to process.
*/

#include "wl_def.h"
#include "files.h"
#include "w_wad.h"
#include "templates.h"
#include "bitmap.h"
#include "textures.h"
#include "v_video.h"
#include "dds_rdds.h"

// Since we want this to compile under Linux too, we need to define this
// stuff ourselves instead of including a DirectX header.

#define ID_DDS						MAKE_ID('D','D','S',' ')
#define ID_DXT1						MAKE_ID('D','X','T','1')
#define ID_DXT2						MAKE_ID('D','X','T','2')
#define ID_DXT3						MAKE_ID('D','X','T','3')
#define ID_DXT4						MAKE_ID('D','X','T','4')
#define ID_DXT5						MAKE_ID('D','X','T','5')

// Bits in dwFlags
#define DDSD_CAPS					0x00000001
#define DDSD_HEIGHT					0x00000002
#define DDSD_WIDTH					0x00000004
#define DDSD_PITCH					0x00000008
#define DDSD_PIXELFORMAT			0x00001000
#define DDSD_MIPMAPCOUNT			0x00020000
#define DDSD_LINEARSIZE				0x00080000
#define DDSD_DEPTH					0x00800000

// Bits in ddpfPixelFormat
#define DDPF_ALPHAPIXELS			0x00000001
#define DDPF_FOURCC					0x00000004
#define DDPF_RGB					0x00000040

// Bits in DDSCAPS2.dwCaps1
#define DDSCAPS_COMPLEX				0x00000008
#define DDSCAPS_TEXTURE				0x00001000
#define DDSCAPS_MIPMAP				0x00400000

// Bits in DDSCAPS2.dwCaps2
#define DDSCAPS2_CUBEMAP			0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX	0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX	0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY	0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY	0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ	0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIZEZ	0x00008000
#define DDSCAPS2_VOLUME				0x00200000

//==========================================================================
//
//
//
//==========================================================================

struct DDPIXELFORMAT
{
	uint32_t			Size;		// Must be 32
	uint32_t			Flags;
	uint32_t			FourCC;
	uint32_t			RGBBitCount;
	uint32_t			RBitMask, GBitMask, BBitMask;
	uint32_t			RGBAlphaBitMask;
};

struct DDCAPS2
{
	uint32_t			Caps1, Caps2;
	uint32_t			Reserved[2];
};

struct DDSURFACEDESC2
{
	uint32_t			Size;		// Must be 124. DevIL claims some writers set it to 'DDS ' instead.
	uint32_t			Flags;
	uint32_t			Height;
	uint32_t			Width;
	union
	{
		int32_t		Pitch;
		uint32_t		LinearSize;
	};
	uint32_t			Depth;
	uint32_t			MipMapCount;
	uint32_t			Reserved1[11];
	DDPIXELFORMAT	PixelFormat;
	DDCAPS2			Caps;
	uint32_t			Reserved2;
};

struct DDSFileHeader
{
	uint32_t			Magic;
	DDSURFACEDESC2	Desc;
};


//==========================================================================
//
// A DDS image, with DXTx compression
//
//==========================================================================

class FDDSTexture : public FTexture
{
public:
	FDDSTexture (FileReader &lump, int lumpnum, void *surfdesc);
	~FDDSTexture ();

	const uint8_t *GetColumn (unsigned int column, const Span **spans_out);
	const uint8_t *GetPixels ();
	void Unload ();
	FTextureFormat GetFormat ();

protected:

	uint8_t *Pixels;
	Span **Spans;

	void MakeTexture ();

	int CopyTrueColorPixels(FBitmap *bmp, int x, int y, int rotate, FCopyInfo *inf = NULL);
	bool UseBasePalette();

	friend class FTexture;
};


//==========================================================================
//
//
//
//==========================================================================

static bool CheckDDS (FileReader &file)
{
	DDSFileHeader Header;

	file.Seek (0, SEEK_SET);
	if (file.Read (&Header, sizeof(Header)) != sizeof(Header))
	{
		return false;
	}
	return Header.Magic == ID_DDS &&
		(LittleLong(Header.Desc.Size) == sizeof(DDSURFACEDESC2) || Header.Desc.Size == ID_DDS) &&
		LittleLong(Header.Desc.PixelFormat.Size) == sizeof(DDPIXELFORMAT) &&
		(LittleLong(Header.Desc.Flags) & (DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT)) == (DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT) &&
		Header.Desc.Width != 0 &&
		Header.Desc.Height != 0;
}

//==========================================================================
//
//
//
//==========================================================================

FTexture *DDSTexture_TryCreate (FileReader &data, int lumpnum)
{
	union
	{
		DDSURFACEDESC2	surfdesc;
		uint32_t			byteswapping[sizeof(DDSURFACEDESC2) / 4];
	};

	if (!CheckDDS(data)) return NULL;

	data.Seek (4, SEEK_SET);
	data.Read (&surfdesc, sizeof(surfdesc));

#ifdef MSB_FIRST
	/* Every single element of the header is a uint32_t */
	for (unsigned int i = 0; i < sizeof(DDSURFACEDESC2) / 4; ++i)
		byteswapping[i] = LittleLong(byteswapping[i]);
	// Undo the byte swap for the pixel format
	surfdesc.PixelFormat.FourCC = LittleLong(surfdesc.PixelFormat.FourCC);
#endif

	if (surfdesc.PixelFormat.Flags & DDPF_FOURCC)
	{
		// Check for supported FourCC
		if (surfdesc.PixelFormat.FourCC != ID_DXT1 &&
			surfdesc.PixelFormat.FourCC != ID_DXT2 &&
			surfdesc.PixelFormat.FourCC != ID_DXT3 &&
			surfdesc.PixelFormat.FourCC != ID_DXT4 &&
			surfdesc.PixelFormat.FourCC != ID_DXT5)
		{
			return NULL;
		}
		if (!(surfdesc.Flags & DDSD_LINEARSIZE))
		{
			return NULL;
		}
	}
	else if (surfdesc.PixelFormat.Flags & DDPF_RGB)
	{
		if ((surfdesc.PixelFormat.RGBBitCount >> 3) < 1 ||
			(surfdesc.PixelFormat.RGBBitCount >> 3) > 4)
		{
			return NULL;
		}
		if ((surfdesc.Flags & DDSD_PITCH) && (surfdesc.Pitch <= 0))
		{
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
	return new FDDSTexture (data, lumpnum, &surfdesc);
}

//==========================================================================
//
//
//
//==========================================================================

FDDSTexture::FDDSTexture (FileReader &lump, int lumpnum, void *vsurfdesc)
: FTexture(NULL, lumpnum), Pixels(0), Spans(0)
{
	DDSURFACEDESC2 *surf = (DDSURFACEDESC2 *)vsurfdesc;

	UseType = TEX_MiscPatch;
	LeftOffset = 0;
	TopOffset = 0;
	bMasked = false;

	Width = uint16_t(surf->Width);
	Height = uint16_t(surf->Height);
	CalcBitSize ();

	// The DDS byte stream (header + mip data) is handed verbatim to the
	// rdds decoder in MakeTexture()/CopyTrueColorPixels(), so no pixel
	// format, mask or pitch bookkeeping is needed here; the caller
	// (DDSTexture_TryCreate) has already screened for a supported layout.

}

//==========================================================================
//
//
//
//==========================================================================

FDDSTexture::~FDDSTexture ()
{
	Unload ();
	if (Spans != NULL)
	{
		FreeSpans (Spans);
		Spans = NULL;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void FDDSTexture::Unload ()
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

FTextureFormat FDDSTexture::GetFormat()
{
	// For now, create a true color texture to preserve all colors.
	return TEX_RGB;
}

//==========================================================================
//
//
//
//==========================================================================

const uint8_t *FDDSTexture::GetColumn (unsigned int column, const Span **spans_out)
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
		if (Spans == NULL)
		{
			Spans = CreateSpans (Pixels);
		}
		*spans_out = Spans[column];
	}
	return Pixels + column*Height;
}

//==========================================================================
//
//
//
//==========================================================================

const uint8_t *FDDSTexture::GetPixels ()
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

void FDDSTexture::MakeTexture ()
{
	FWadLump lump = Wads.OpenLumpNum (SourceLump);
	int lumplen = Wads.LumpLength (SourceLump);

	Pixels = new uint8_t[Width*Height];

	uint8_t *raw = new uint8_t[lumplen > 0 ? lumplen : 1];
	lump.Seek (0, SEEK_SET);
	lump.Read (raw, lumplen);

	unsigned dw = 0, dh = 0;
	uint8_t *rgba = DDS_DecodeRGBA (raw, (size_t)lumplen, &dw, &dh);
	delete[] raw;

	if (rgba != NULL && dw == (unsigned)Width && dh == (unsigned)Height)
	{
		int masked = 0;
		DDS_PalettizeColumnMajor (rgba, Width, Height, &RGB32k[0][0][0], Pixels, &masked);
		bMasked = masked ? true : false;
		free(rgba);
	}
	else
	{
		// Decode failed or produced unexpected dimensions: leave a
		// defined, fully transparent texture rather than garbage.
		if (rgba != NULL)
			free(rgba);
		memset(Pixels, 0, (size_t)Width*Height);
		bMasked = true;
	}
}


//===========================================================================
//
// FDDSTexture::CopyTrueColorPixels
//
//===========================================================================

int FDDSTexture::CopyTrueColorPixels(FBitmap *bmp, int x, int y, int rotate, FCopyInfo *inf)
{
	FWadLump lump = Wads.OpenLumpNum (SourceLump);
	int lumplen = Wads.LumpLength (SourceLump);

	uint8_t *raw = new uint8_t[lumplen > 0 ? lumplen : 1];
	lump.Seek (0, SEEK_SET);
	lump.Read (raw, lumplen);

	unsigned dw = 0, dh = 0;
	uint8_t *rgba = DDS_DecodeRGBA (raw, (size_t)lumplen, &dw, &dh);
	delete[] raw;

	// rdds decodes mip 0 to row-major RGBA8 (bytes R,G,B,A), exactly the
	// layout CopyPixelDataRGB expects for CF_RGBA.
	if (rgba != NULL && dw == (unsigned)Width && dh == (unsigned)Height)
	{
		bmp->CopyPixelDataRGB(x, y, rgba, Width, Height, 4, Width*4, rotate, CF_RGBA, inf);
	}
	if (rgba != NULL)
	{
		free(rgba);
	}
	return -1;
}


//===========================================================================
//
//
//===========================================================================

bool FDDSTexture::UseBasePalette() 
{ 
	return false; 
}
