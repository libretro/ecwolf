/*
** file_audiot.cpp
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

#include "wl_def.h"
#include "resourcefile.h"
#include "w_wad.h"
#include "lumpremap.h"
#include "zstring.h"

class FAudiot : public FUncompressedFile
{
	public:
		FAudiot(const char* filename, FileReader *file) : FUncompressedFile(filename, file), audiotFile(filename)
		{
			FString path(filename);
			int lastSlash = path.LastIndexOfAny("/\\");
			extension = path.Mid(lastSlash+8);
			path = path.Left(lastSlash+1);

			audiohedFile = path + "audiohed." + extension;
		}

		bool Open(bool quiet)
		{
			FileReader audiohedReader;
			if(!audiohedReader.Open(audiohedFile))
				return false;
			audiohedReader.Seek(0, SEEK_END);
			NumLumps = (audiohedReader.Tell()/4)-1;
			audiohedReader.Seek(0, SEEK_SET);
			Lumps = new FUncompressedLump[NumLumps];
			// The vgahead has 24-bit ints.
			DWORD* positions = new DWORD[NumLumps];
			DWORD* sizes = new DWORD[NumLumps];
			audiohedReader.Read(positions, NumLumps*4);

			// Since the music is at the end and is easy to identify we'll look for the end of the music.
			// HACK: I'm using the fact that the IMFs have the string IMF near
			//       the end of the files to identify them.
			int musicStart = -1;
			for(unsigned int i = NumLumps-1;i > 0;i--)
			{
				if(i != 0)
					sizes[i-1] = positions[i] - positions[i-1];
				if(i != NumLumps-1 && musicStart == -1) // See if we can find the first music lump.
				{
					if(sizes[i] < 64)
					{
						musicStart = i+1;
						continue;
					}
					Reader->Seek(positions[i]+sizes[i]-64, SEEK_SET);
					char data[64];
					Reader->Read(data, 64);
					bool isMusic = false;
					for(unsigned int j = 0;j+3 < 64;j++)
					{
						if(strnicmp(&data[j], "IMF", 3) == 0)
						{
							isMusic = true;
							break;
						}
					}
					if(!isMusic)
					{
						musicStart = i+1;
						continue;
					}
				}
			}
			for(unsigned int i = 0;i < NumLumps;i++)
			{
				// Give the lump a temporary name.
				char lumpname[9];
				sprintf(lumpname, "AUD%05d", i);
				Lumps[i].Owner = this;
				Lumps[i].LumpNameSetup(lumpname);
				Lumps[i].Namespace = i >= (unsigned int)musicStart ? ns_music : ns_sounds;
				Lumps[i].Position = positions[i];
				Lumps[i].LumpSize = sizes[i];
			}
			delete[] positions;
			delete[] sizes;
			if(!quiet) Printf(", %d lumps\n", NumLumps);

			LumpRemapper::AddFile(extension, this, LumpRemapper::AUDIOT);
			return true;
		}

	private:
		FString		extension;
		FString		audiotFile;
		FString		audiohedFile;
};

FResourceFile *CheckAudiot(const char *filename, FileReader *file, bool quiet)
{
	FString fname(filename);
	int lastSlash = fname.LastIndexOfAny("/\\");
	if(lastSlash != -1)
		fname = fname.Mid(lastSlash+1, 6);
	else
		fname = fname.Left(6);

	if(fname.Len() == 6 && fname.CompareNoCase("audiot") == 0) // file must be audiot.something
	{
		FResourceFile *rf = new FAudiot(filename, file);
		if(rf->Open(quiet)) return rf;
		delete rf;
	}
	return NULL;
}
