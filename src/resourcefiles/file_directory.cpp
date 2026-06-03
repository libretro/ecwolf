/*
** file_directory.cpp
**
**---------------------------------------------------------------------------
** Copyright 2008-2009 Randy Heit
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


#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#include <io.h>
#define stat _stat
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

#include "tarray.h"
#include "resourcefile.h"
#include "zstring.h"
#include "doomerrors.h"



//==========================================================================
//
// 
//
//==========================================================================

FDirectory::FDirectory(const char * directory)
: FResourceFile(NULL, NULL)
{
	FString dirname;

	#ifdef _WIN32
		directory = _fullpath(NULL, directory, _MAX_PATH);
	#else
		// Todo for Linux: Resolve the path befire using it
	#endif
	dirname = directory;
	dirname.ReplaceChars('\\', '/');
	if (dirname[dirname.Len()-1] != '/') dirname += '/';
	free((void*)directory);
	char* cFilename = new char[strlen(dirname) + 1];
	memcpy(cFilename, dirname, strlen(dirname)+1);
	Filename = cFilename;
}



//==========================================================================
//
//
//
//==========================================================================

bool FDirectory::Open(bool quiet)
{
	NumLumps = AddDirectory(Filename);
	if (!quiet) Printf(", %d lumps\n", NumLumps);
	PostProcessArchive(&Lumps[0], sizeof(FDirectoryLump));
	return true;
}

//==========================================================================
//
//
//
//==========================================================================

void FDirectory::AddEntry(const char *fullpath, int size)
{
	FDirectoryLump *lump_p = &Lumps[Lumps.Reserve(1)];

	// The lump's name is only the part relative to the main directory
	lump_p->LumpNameSetup(fullpath + strlen(Filename));
	lump_p->LumpSize = size;
	lump_p->Owner = this;
	lump_p->Flags = 0;
	lump_p->CheckEmbedded();
}


//==========================================================================
//
//
//
//==========================================================================

FileReader *FDirectoryLump::NewReader()
{
	FString fullpath = Owner->Filename;
	fullpath += FullName;
	return FileReader::SafeOpen(fullpath);
}

//==========================================================================
//
//
//
//==========================================================================

int FDirectoryLump::FillCache()
{
	Cache = new char[LumpSize];
	FileReader *reader = NewReader();
	reader->Read(Cache, LumpSize);
	delete reader;
	RefCount = 1;
	return 1;
}

//==========================================================================
//
// File open
//
//==========================================================================

FResourceFile *CheckDir(const char *filename, FileReader *file, bool quiet)
{
	FResourceFile *rf = new FDirectory(filename);
	if (rf->Open(quiet)) return rf;
	delete rf;
	return NULL;
}

