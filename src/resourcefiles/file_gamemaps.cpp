/*
** file_gamemaps.cpp
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
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

#include "file.h"
#include "doomerrors.h"
#include "wl_def.h"
#include "resourcefile.h"
#include "w_wad.h"
#include "m_swap.h"
#include "zstring.h"

struct FMapLump;

class FGamemaps : public FResourceFile
{
	public:
		FGamemaps(const char* filename, FileReader *file);
		~FGamemaps();

		FResourceLump *GetLump(int lump);
		bool Open(bool quiet);

	protected:
		friend struct FMapLump;

		WORD		rlewTag;

	private:
		FMapLump*	Lumps;

		FString		extension;
		FString		gamemapsFile;
		FString		mapheadFile;
};

#define PLANES 3
#define HEADERSIZE 34
#define CARMACK_NEARTAG	static_cast<char>(0xA7)
#define CARMACK_FARTAG	static_cast<char>(0xA8)

struct FMapLump : public FResourceLump
{
	protected:
		// Only important thing to remember is that both
		// Compression methods work on WORDs rather than bytes.

		void ExpandCarmack(const char* in, char* out)
		{
			const char* const end = out + ReadLittleShort((const BYTE*)in);
			const char* const start = out;
			in += 2;

			const char* copy;
			BYTE length;
			while(out < end)
			{
				length = *in++;
				if(length == 0 && (*in == CARMACK_NEARTAG || *in == CARMACK_FARTAG))
				{
					*out++ = in[1];
					*out++ = in[0];
					in += 2;
					continue;
				}
				else if(*in == CARMACK_NEARTAG)
				{
					copy = out-(static_cast<unsigned char>(in[1])*2);
					in += 2;
				}
				else if(*in == CARMACK_FARTAG)
				{
					copy = start+(ReadLittleShort((const BYTE*)(in+1))*2);
					in += 3;
				}
				else
				{
					*out++ = length;
					*out++ = *in++;
					continue;
				}
				if(out+(length*2) > end)
					break;
				while(length-- > 0)
				{
					*out++ = *copy++;
					*out++ = *copy++;
				}
			}
		}

		void ExpandRLEW(const char* in, char* out, const WORD rlewTag)
		{
			const char* const end = out + ReadLittleShort((const BYTE*)in);
			in += 2;

			while(out < end)
			{
				if(ReadLittleShort((const BYTE*)in) != rlewTag)
				{
					*out++ = *in++;
					*out++ = *in++;
				}
				else
				{
					WORD count = ReadLittleShort((const BYTE*)(in+2));
					WORD input = ReadLittleShort((const BYTE*)(in+4));
					in += 6;
					while(count-- > 0)
					{
						WriteLittleShort((BYTE*)out, input);
						out += 2;
					}
				}
			}
		}

		int FillCache()
		{
			if(LumpSize == 0)
				return 1;

			unsigned int PlaneSize = Header.Width*Header.Height*2;

			Cache = new char[LumpSize];
			strcpy(Cache, "WDC3.1");
			WriteLittleShort((BYTE*)&Cache[10], 3);
			WriteLittleShort((BYTE*)&Cache[HEADERSIZE-4], Header.Width);
			WriteLittleShort((BYTE*)&Cache[HEADERSIZE-2], Header.Height);
			memcpy(&Cache[14], Header.Name, 16);

			// Read map data and expand it
			for(unsigned int i = 0;i < PLANES;i++)
			{
				char* output = Cache+HEADERSIZE+i*PlaneSize;
				char* input = new char[Header.PlaneLength[i]];
				Owner->Reader->Seek(Header.PlaneOffset[i], SEEK_SET);
				Owner->Reader->Read(input, Header.PlaneLength[i]);
				char* tempOut = new char[ReadLittleShort((BYTE*)input)];

				ExpandCarmack(input, tempOut);
				ExpandRLEW(tempOut, output, ((FGamemaps*)Owner)->rlewTag);

				delete[] input;
				delete[] tempOut;
			}
			return 1;
		}

	public:
		struct
		{
			DWORD	PlaneOffset[PLANES];
			WORD	PlaneLength[PLANES];
			WORD	Width;
			WORD	Height;
			char	Name[16];
		} Header;

		FMapLump() : FResourceLump()
		{
			LumpSize = HEADERSIZE;
		}
		~FMapLump()
		{
		}
};

FGamemaps::FGamemaps(const char* filename, FileReader *file) : FResourceFile(filename, file), Lumps(NULL), gamemapsFile(filename)
{
	FString path(filename);
	int lastSlash = path.LastIndexOfAny("/\\");
	extension = path.Mid(lastSlash+10);
	path = path.Left(lastSlash+1);

	File directory(path.Len() > 0 ? path : ".");
	mapheadFile = path + directory.getInsensitiveFile(FString("maphead.") + extension, true);
}

FGamemaps::~FGamemaps()
{
	if(Lumps != NULL)
		delete[] Lumps;
}

FResourceLump *FGamemaps::GetLump(int lump)
{
	return &Lumps[lump];
}

bool FGamemaps::Open(bool quiet)
{
	// Read the map head.
	// First two bytes is the tag for the run length encoding
	// Followed by offsets in the gamemaps file, we'll count until we
	// hit a 0 offset.
	FileReader mapheadReader;
	if(!mapheadReader.Open(mapheadFile))
	{
		FString error;
		error.Format("Could not open gamemaps since %s is missing.", mapheadFile.GetChars());
		throw CRecoverableError(error);
	}
	mapheadReader.Seek(0, SEEK_END);
	unsigned int NumPossibleMaps = (mapheadReader.Tell()-2)/4;
	mapheadReader.Seek(0, SEEK_SET);
	DWORD* offsets = new DWORD[NumPossibleMaps];
	mapheadReader.Read(&rlewTag, 2);
	rlewTag = LittleShort(rlewTag);
	mapheadReader.Read(offsets, NumPossibleMaps*4);
	for(NumLumps = 0;NumLumps < NumPossibleMaps && offsets[NumLumps] != 0;++NumLumps)
		offsets[NumLumps] = LittleLong(offsets[NumLumps]);

	// We allocate 2 lumps per map so...
	static const unsigned int NUM_MAP_LUMPS = 2;
	NumLumps *= NUM_MAP_LUMPS;

	Lumps = new FMapLump[NumLumps];
	for(unsigned int i = 0;i < NumLumps/NUM_MAP_LUMPS;++i)
	{
		// Map marker
		FMapLump &markerLump = Lumps[i*NUM_MAP_LUMPS];
		// Hey we don't need to use a temporary name here!
		// First map is MAP01 and so forth.
		char lumpname[7];
		sprintf(lumpname, "MAP%02d", i+1);
		markerLump.Owner = this;
		markerLump.LumpNameSetup(lumpname);
		markerLump.Namespace = ns_global;
		markerLump.LumpSize = 0;

		// Make the data lump
		FMapLump &dataLump = Lumps[i*NUM_MAP_LUMPS+1];
		BYTE header[PLANES*6+20];
		Reader->Seek(offsets[i], SEEK_SET);
		Reader->Read(&header, PLANES*6+20);

		dataLump.Owner = this;
		dataLump.LumpNameSetup("PLANES");
		dataLump.Namespace = ns_global;
		for(unsigned int j = 0;j < PLANES;j++)
		{
			dataLump.Header.PlaneOffset[j] = ReadLittleLong(&header[4*j]);
			dataLump.Header.PlaneLength[j] = ReadLittleShort(&header[PLANES*4+2*j]);
		}
		dataLump.Header.Width = ReadLittleShort(&header[PLANES*6]);
		dataLump.Header.Height = ReadLittleShort(&header[PLANES*6+2]);
		memcpy(dataLump.Header.Name, &header[PLANES*6+4], 16);
		dataLump.LumpSize += dataLump.Header.Width*dataLump.Header.Height*PLANES*2;
	}
	delete[] offsets;
	if(!quiet) Printf(", %d lumps\n", NumLumps);
	return true;
}

FResourceFile *CheckGamemaps(const char *filename, FileReader *file, bool quiet)
{
	FString fname(filename);
	int lastSlash = fname.LastIndexOfAny("/\\");
	if(lastSlash != -1)
		fname = fname.Mid(lastSlash+1, 8);
	else
		fname = fname.Left(8);

	if(fname.Len() == 8 && fname.CompareNoCase("gamemaps") == 0) // file must be gamemaps.something
	{
		FResourceFile *rf = new FGamemaps(filename, file);
		if(rf->Open(quiet)) return rf;
		delete rf;
	}
	return NULL;
}
