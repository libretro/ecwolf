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
#include <setjmp.h>
extern "C"
{
#define boolean jboolean
#include <jpeglib.h>
#undef boolean
}

#include "wl_def.h"
#include "m_swap.h"
#include "files.h"
#include "w_wad.h"
#include "bitmap.h"
#include "textures.h"
#include "v_video.h"

#define TEXTCOLOR_ORANGE


struct FLumpSourceMgr : public jpeg_source_mgr
{
	FileReader *Lump;
	JOCTET Buffer[4096];
	bool StartOfFile;

	FLumpSourceMgr (FileReader *lump, j_decompress_ptr cinfo);
	static void InitSource (j_decompress_ptr cinfo);
	static jboolean FillInputBuffer (j_decompress_ptr cinfo);
	static void SkipInputData (j_decompress_ptr cinfo, long num_bytes);
	static void TermSource (j_decompress_ptr cinfo);
};

//==========================================================================
//
//
//
//==========================================================================

void FLumpSourceMgr::InitSource (j_decompress_ptr cinfo)
{
	((FLumpSourceMgr *)(cinfo->src))->StartOfFile = true;
}

//==========================================================================
//
//
//
//==========================================================================

jboolean FLumpSourceMgr::FillInputBuffer (j_decompress_ptr cinfo)
{
	FLumpSourceMgr *me = (FLumpSourceMgr *)(cinfo->src);
	long nbytes = me->Lump->Read (me->Buffer, sizeof(me->Buffer));

	if (nbytes <= 0)
	{
		me->Buffer[0] = (JOCTET)0xFF;
		me->Buffer[1] = (JOCTET)JPEG_EOI;
		nbytes = 2;
	}
	me->next_input_byte = me->Buffer;
	me->bytes_in_buffer = nbytes;
	me->StartOfFile = false;
	return TRUE;
}

//==========================================================================
//
//
//
//==========================================================================

void FLumpSourceMgr::SkipInputData (j_decompress_ptr cinfo, long num_bytes)
{
	FLumpSourceMgr *me = (FLumpSourceMgr *)(cinfo->src);
	if (num_bytes <= (long)me->bytes_in_buffer)
	{
		me->bytes_in_buffer -= num_bytes;
		me->next_input_byte += num_bytes;
	}
	else
	{
		num_bytes -= (long)me->bytes_in_buffer;
		me->Lump->Seek (num_bytes, SEEK_CUR);
		FillInputBuffer (cinfo);
	}
}

//==========================================================================
//
//
//
//==========================================================================

void FLumpSourceMgr::TermSource (j_decompress_ptr cinfo)
{
}

//==========================================================================
//
//
//
//==========================================================================

FLumpSourceMgr::FLumpSourceMgr (FileReader *lump, j_decompress_ptr cinfo)
: Lump (lump)
{
	cinfo->src = this;
	init_source = InitSource;
	fill_input_buffer = FillInputBuffer;
	skip_input_data = SkipInputData;
	resync_to_restart = jpeg_resync_to_restart;
	term_source = TermSource;
	bytes_in_buffer = 0;
	next_input_byte = NULL;
}

//==========================================================================
//
//
//
//==========================================================================

// libjpeg's default error handler calls exit(); the original code instead
// threw a C++ exception out of error_exit and caught it. To remove exceptions
// while keeping the same recover-and-clean-up behaviour, we use libjpeg's
// documented mechanism: an error manager that carries a setjmp buffer. On a
// fatal libjpeg error we longjmp back to the decode function, which then
// performs its normal cleanup. The only automatic object on the unwound path
// (FLumpSourceMgr) is trivially destructible, so the longjmp is well defined.
struct FJPEGErrorMgr
{
	struct jpeg_error_mgr pub;	// must be first: libjpeg treats this as jpeg_error_mgr
	jmp_buf setjmp_buffer;
};

void JPEG_ErrorExit (j_common_ptr cinfo)
{
	(*cinfo->err->output_message) (cinfo);
	longjmp (((FJPEGErrorMgr *)cinfo->err)->setjmp_buffer, 1);
}

//==========================================================================
//
//
//
//==========================================================================

void JPEG_OutputMessage (j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];
	(*cinfo->err->format_message) (cinfo, buffer);
}

//==========================================================================
//
// A JPEG texture
//
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
	// volatile: modified between setjmp and a potential longjmp, and read in
	// the longjmp recovery path, so its value must survive the non-local jump.
	JSAMPLE * volatile buff = NULL;

	jpeg_decompress_struct cinfo;
	FJPEGErrorMgr jerr;

	Pixels = new uint8_t[Width * Height];
	memset (Pixels, 0xBA, Width * Height);

	cinfo.err = jpeg_std_error(&jerr.pub);
	cinfo.err->output_message = JPEG_OutputMessage;
	cinfo.err->error_exit = JPEG_ErrorExit;
	jpeg_create_decompress(&cinfo);
	{
		FLumpSourceMgr sourcemgr(&lump, &cinfo);
		if (setjmp(jerr.setjmp_buffer))
		{
			// A fatal libjpeg error longjmp'd back here. Clean up and bail.
			jpeg_destroy_decompress(&cinfo);
			if (buff != NULL)
				delete[] buff;
			return;
		}

		jpeg_read_header(&cinfo, TRUE);
		if (!((cinfo.out_color_space == JCS_RGB && cinfo.num_components == 3) ||
			  (cinfo.out_color_space == JCS_CMYK && cinfo.num_components == 4) ||
			  (cinfo.out_color_space == JCS_GRAYSCALE && cinfo.num_components == 1)))
		{
			jpeg_destroy_decompress(&cinfo);
			return;
		}

		jpeg_start_decompress(&cinfo);

		int y = 0;
		buff = new uint8_t[cinfo.output_width * cinfo.output_components];

		while (cinfo.output_scanline < cinfo.output_height)
		{
			uint8_t *in = buff;
			uint8_t *out = Pixels + y;
			switch (cinfo.out_color_space)
			{
			case JCS_RGB:
				for (int x = Width; x > 0; --x)
				{
					*out = RGB32k[in[0]>>3][in[1]>>3][in[2]>>3];
					out += Height;
					in += 3;
				}
				break;

			case JCS_GRAYSCALE:
				for (int x = Width; x > 0; --x)
				{
					*out = GrayMap[in[0]];
					out += Height;
					in += 1;
				}
				break;

			case JCS_CMYK:
				// What are you doing using a CMYK image? :)
				for (int x = Width; x > 0; --x)
				{
					// To be precise, these calculations should use 255, but
					// 256 is much faster and virtually indistinguishable.
					int r = in[3] - (((256-in[0])*in[3]) >> 8);
					int g = in[3] - (((256-in[1])*in[3]) >> 8);
					int b = in[3] - (((256-in[2])*in[3]) >> 8);
					*out = RGB32k[r >> 3][g >> 3][b >> 3];
					out += Height;
					in += 4;
				}
				break;

			default:
				// The other colorspaces were considered above and discarded,
				// but GCC will complain without a default for them here.
				break;
			}
			y++;
		}
		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
	}
	if (buff != NULL)
	{
		delete[] buff;
	}
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
	PalEntry pe[256];

	FWadLump lump = Wads.OpenLumpNum (SourceLump);
	// volatile: see MakeTexture; value must survive the setjmp/longjmp path.
	JSAMPLE * volatile buff = NULL;

	jpeg_decompress_struct cinfo;
	FJPEGErrorMgr jerr;

	cinfo.err = jpeg_std_error(&jerr.pub);
	cinfo.err->output_message = JPEG_OutputMessage;
	cinfo.err->error_exit = JPEG_ErrorExit;
	jpeg_create_decompress(&cinfo);

	{
		FLumpSourceMgr sourcemgr(&lump, &cinfo);
		if (setjmp(jerr.setjmp_buffer))
		{
			// Fatal libjpeg error: fall through to the shared cleanup below.
			jpeg_destroy_decompress(&cinfo);
			if (buff != NULL) delete [] buff;
			return 0;
		}

		jpeg_read_header(&cinfo, TRUE);

		if (!((cinfo.out_color_space == JCS_RGB && cinfo.num_components == 3) ||
			  (cinfo.out_color_space == JCS_CMYK && cinfo.num_components == 4) ||
			  (cinfo.out_color_space == JCS_GRAYSCALE && cinfo.num_components == 1)))
		{
			jpeg_destroy_decompress(&cinfo);
			return 0;
		}
		jpeg_start_decompress(&cinfo);

		int yc = 0;
		buff = new uint8_t[cinfo.output_height * cinfo.output_width * cinfo.output_components];


		while (cinfo.output_scanline < cinfo.output_height)
		{
			uint8_t * ptr = buff + cinfo.output_width * cinfo.output_components * yc;
			jpeg_read_scanlines(&cinfo, &ptr, 1);
			yc++;
		}

		switch (cinfo.out_color_space)
		{
		case JCS_RGB:
			bmp->CopyPixelDataRGB(x, y, buff, cinfo.output_width, cinfo.output_height, 
				3, cinfo.output_width * cinfo.output_components, rotate, CF_RGB, inf);
			break;

		case JCS_GRAYSCALE:
			for(int i=0;i<256;i++) pe[i]=PalEntry(255,i,i,i);	// default to a gray map
			bmp->CopyPixelData(x, y, buff, cinfo.output_width, cinfo.output_height, 
				1, cinfo.output_width, rotate, pe, inf);
			break;

		case JCS_CMYK:
			bmp->CopyPixelDataRGB(x, y, buff, cinfo.output_width, cinfo.output_height, 
				4, cinfo.output_width * cinfo.output_components, rotate, CF_CMYK, inf);
			break;

		default:
			assert(0);
			break;
		}
		jpeg_finish_decompress(&cinfo);
	}
	jpeg_destroy_decompress(&cinfo);
	if (buff != NULL) delete [] buff;
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
