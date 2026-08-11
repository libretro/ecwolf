/*
** file_7z.cpp
**
**---------------------------------------------------------------------------
** Copyright 2009 Randy Heit
** Copyright 2009 Christoph Oelckers
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

extern "C"
{
#include "7z/r7z_archive.h"
}

#include "resourcefile.h"
#include "w_zip.h"
#include "w_wad.h"
#include "zstring.h"
#include "zdoomsupport.h"
#ifdef _WIN32
#include <malloc.h>
#endif

#define TEXTCOLOR_RED

//-----------------------------------------------------------------------
//
// Interface classes to 7z library
//
//-----------------------------------------------------------------------

// Memory-backed archive wrapper over libretro-common's r7z. The old
// LZMA-SDK path streamed the container through a LookToRead window but
// still decompressed and cached whole folders; r7z takes the archive
// resident and extracts whole entries, which for the mod-sized archives
// this path serves is the same memory story without the SDK.
struct C7zArchive
{
	TArray<uint8_t> Data;
	r7z_archive_t *Arch;

	C7zArchive(FileReader *file) : Arch(NULL)
	{
		file->Seek(0, SEEK_SET);
		long len = file->GetLength();
		if (len > 0)
		{
			Data.Resize(len);
			if (file->Read(&Data[0], len) != len)
				Data.Resize(0);
		}
	}

	~C7zArchive()
	{
		if (Arch != NULL)
			r7z_archive_close(Arch);
	}

	int Open()
	{
		if (Data.Size() == 0)
			return -1;
		return r7z_archive_open(&Arch, &Data[0], Data.Size());
	}

	int Extract(uint32_t file_index, char *buffer)
	{
		uint8_t *out = NULL;
		size_t out_len = 0;
		int res = r7z_archive_extract(Arch, file_index, &out, &out_len);
		if (res == 0 && out != NULL)
		{
			memcpy(buffer, out, out_len);
			free(out);
			return 0;
		}
		if (out != NULL)
			free(out);
		return res != 0 ? res : -1;
	}
};

//==========================================================================
//
// Zip Lump
//
//==========================================================================

struct F7ZLump : public FResourceLump
{
	int		Position;

	virtual int FillCache();

};


//==========================================================================
//
// 7-zip file
//
//==========================================================================

class F7ZFile : public FResourceFile
{
	friend struct F7ZLump;

	F7ZLump *Lumps;
	C7zArchive *Archive;

public:
	F7ZFile(const char * filename, FileReader *filer);
	bool Open();
	virtual ~F7ZFile();
	virtual FResourceLump *GetLump(int no) { return ((unsigned)no < NumLumps)? &Lumps[no] : NULL; }
};



//==========================================================================
//
// 7Z file
//
//==========================================================================

F7ZFile::F7ZFile(const char * filename, FileReader *filer)
	: FResourceFile(filename, filer)
{
	Lumps = NULL;
	Archive = NULL;
}


//==========================================================================
//
// Open it
//
//==========================================================================

bool F7ZFile::Open()
{
	Archive = new C7zArchive(Reader);
	int skipped = 0;

	if (Archive->Open() != 0)
	{
		delete Archive;
		Archive = NULL;
		return false;
	}


	NumLumps = r7z_archive_num_entries(Archive->Arch);
	Lumps = new F7ZLump[NumLumps];

	F7ZLump *lump_p = Lumps;
	TArray<char> nameASCII;

	for (uint32_t i = 0; i < NumLumps; ++i)
	{
		const r7z_entry_t *ent = r7z_archive_entry(Archive->Arch, i);

		// skip Directories
		if (ent == NULL || ent->is_dir)
		{
			skipped++;
			continue;
		}

		// Entry names are NUL-terminated UTF-16.
		size_t nameLength = 0;
		while (ent->name != NULL && ent->name[nameLength] != 0)
			nameLength++;
		nameLength++; /* include the terminator, as the SDK count did */

		if (nameLength <= 1)
		{
			++skipped;
			continue;
		}

		nameASCII.Resize((unsigned)nameLength);
		for (size_t c = 0; c < nameLength; ++c)
		{
			nameASCII[c] = static_cast<char>(ent->name[c]);
		}

		/* Build the lump name with plain C strings: copy the ASCII name, then
		** fix path separators ('\\' -> '/') and lowercase in place using only
		** ISO C89 (tolower). LumpNameSetup takes a string object, which constructs
		** implicitly from this const char* at the call (as the directory reader
		** already relies on). */
		{
			char name[256];
			size_t nl = strlen(&nameASCII[0]);
			size_t k;
			if(nl >= sizeof(name))
				nl = sizeof(name) - 1;
			memcpy(name, &nameASCII[0], nl);
			name[nl] = '\0';
			for(k = 0; k < nl; ++k)
			{
				if(name[k] == '\\')
					name[k] = '/';
				name[k] = (char)tolower((unsigned char)name[k]);
			}
			lump_p->LumpNameSetup(name);
		}
		lump_p->LumpSize = static_cast<int>(ent->size);
		lump_p->Owner = this;
		lump_p->Flags = LUMPF_ZIPFILE;
		lump_p->Position = i;
		lump_p->CheckEmbedded();
		lump_p++;
	}
	// Resize the lump record array to its actual size
	NumLumps -= skipped;

	if (NumLumps > 0)
	{
		// Quick check for unsupported compression method

		TArray<char> temp;
		temp.Resize(Lumps[0].LumpSize);

		if (0 != Archive->Extract(Lumps[0].Position, &temp[0]))
		{
			return false;
		}
	}

	PostProcessArchive(&Lumps[0], sizeof(F7ZLump));
	return true;
}

//==========================================================================
//
//
//
//==========================================================================

F7ZFile::~F7ZFile()
{
	if (Lumps != NULL)
	{
		delete[] Lumps;
	}
	if (Archive != NULL)
	{
		delete Archive;
	}
}

//==========================================================================
//
// Fills the lump cache and performs decompression
//
//==========================================================================

int F7ZLump::FillCache()
{
	Cache = new char[LumpSize];
	static_cast<F7ZFile*>(Owner)->Archive->Extract(Position, Cache);
	RefCount = 1;
	return 1;
}

//==========================================================================
//
// File open
//
//==========================================================================

FResourceFile *Check7Z(const char *filename, FileReader *file)
{
	/* 7z signature: '7' 'z' 0xBC 0xAF 0x27 0x1C (was k7zSignature in the SDK). */
	static const unsigned char sig[6] = { '7', 'z', 0xBC, 0xAF, 0x27, 0x1C };
	char head[sizeof(sig)];

	if (file->GetLength() >= (long)sizeof(sig))
	{
		file->Seek(0, SEEK_SET);
		file->Read(&head, sizeof(sig));
		file->Seek(0, SEEK_SET);
		if (!memcmp(head, sig, sizeof(sig)))
		{
			FResourceFile *rf = new F7ZFile(filename, file);
			if (rf->Open()) return rf;

			rf->Reader = NULL; // to avoid destruction of reader
			delete rf;
		}
	}
	return NULL;
}



