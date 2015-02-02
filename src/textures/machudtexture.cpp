/*
** machudtexture.cpp
**
**---------------------------------------------------------------------------
** Copyright 2015 Braden Obrzut
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

#include "textures.h"
#include "tmemory.h"
#include "v_palette.h"
#include "w_wad.h"

class FMacHudTexture : public FTexture
{
public:
	FMacHudTexture (const char *name, int lumpnum, int offset, FileReader &file, bool masked);

	const BYTE *GetColumn (unsigned int column, const Span **spans_out)
	{
		if (!Pixels)
			MakeTexture ();

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
				Spans = CreateSpans(Pixels);
			}
			*spans_out = Spans[column];
		}
		return Pixels.Get() + column*Height;
	}
	const BYTE *GetPixels () { return Pixels; }
	void Unload () { Pixels.Reset(); }

protected:
	TUniquePtr<BYTE[]> Pixels;
	Span **Spans;
	int Offset;
	bool Masked;

	virtual void MakeTexture ();
};

FMacHudTexture::FMacHudTexture(const char* name, int lumpnum, int offset, FileReader &file, bool masked)
: FTexture(name, lumpnum), Spans(NULL), Offset(offset), Masked(masked)
{
	file.Seek(offset, SEEK_SET);

	WORD width, height;
	file >> width >> height;
	Width = BigShort(width);
	Height = BigShort(height);
	yScale = xScale = 2*FRACUNIT;
	Offset += 4;

	if(Masked)
	{
		// In this case the previous was the x/y offset.
		LeftOffset = -Width;
		TopOffset = -Height;

		file >> width >> height;
		Width = BigShort(width);
		Height = BigShort(height);
		Offset += 4;
	}

	CalcBitSize();
}

void FMacHudTexture::MakeTexture()
{
	unsigned int Size = Width*Height;
	if(Masked) Size <<= 1;

	Pixels = new BYTE[Width*Height];
	TUniquePtr<BYTE[]> newpix(new BYTE[Size]);

	FWadLump lump = Wads.OpenLumpNum(SourceLump);
	lump.Seek(Offset, SEEK_SET);
	lump.Read(newpix.Get(), Size);

	FlipNonSquareBlockRemap (Pixels, newpix, Width, Height, Width, GPalette.Remap);
	if(Masked)
	{
		Size >>= 1;
		for(unsigned int y = Height;y-- > 0;)
		{
			for(unsigned int x = Width;x-- > 0;)
			{
				if(newpix[Size+y*Width+x])
					Pixels[x*Height+y] = 0;
			}
		}
	}
}

void FTextureManager::InitMacHud()
{
	enum { NUM_HUDGRAPHICS = 47 };

	int lumpnum = Wads.GetNumForName("FACE640", ns_graphics);
	FWadLump lump = Wads.OpenLumpNum(lumpnum);
	if(lump.GetLength() < NUM_HUDGRAPHICS*4) return;

	DWORD offsets[NUM_HUDGRAPHICS];
	lump.Read(offsets, NUM_HUDGRAPHICS*4);
	// Check potentially valid data.
	for(unsigned int i = 0;i < NUM_HUDGRAPHICS;++i)
	{
		offsets[i] = BigLong(offsets[i]);
		if(lump.GetLength() < offsets[i]) return;
	}

	for(unsigned int i = 0;i < NUM_HUDGRAPHICS;++i)
	{
		char name[9];
		sprintf(name, "HUD%02d", i);
		AddTexture(new FMacHudTexture(name, lumpnum, offsets[i], lump, i >= 12 && i <= 35));
	}
}
