/*
** wl_iwad.cpp
**
**---------------------------------------------------------------------------
** Copyright 2012 Braden Obrzut
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

#include "resourcefiles/resourcefile.h"
#include "file.h"
#include "lumpremap.h"
#include "scanner.h"
#include "tarray.h"
#include "w_wad.h"
#include "wl_iwad.h"
#include "zstring.h"

struct WadStuff
{
	WadStuff() : Type(0) {}

	TArray<FString> Path;
	FString Extension;
	FString Name;
	int Type;
};

#include "wl_iwad_picker.cpp"

namespace IWad {

#if defined(_WIN32) || defined(__APPLE__)
#define ISCASEINSENSITIVE 1
#else
#define ISCASEINSENSITIVE 0
#endif

static TArray<IWadData> iwadTypes;

static bool SplitFilename(const FString &filename, FString &name, FString &extension)
{
	long extensionSep = filename.LastIndexOf('.');
	if(extensionSep == -1 || filename.Len() < 3)
		return false;
	name = filename.Left(extensionSep);
	extension = filename.Mid(extensionSep+1);
	return true;
}

static int CheckData(WadStuff &wad)
{
	unsigned int* valid = new unsigned int[iwadTypes.Size()];
	memset(valid, 0, sizeof(unsigned int)*iwadTypes.Size());

	for(unsigned int i = 0;i < wad.Path.Size();++i)
	{
		FResourceFile *file = FResourceFile::OpenResourceFile(wad.Path[i], NULL, true);
		LumpRemapper::RemapAll(); // Fix lump names if needed
		if(file)
		{
			for(unsigned int j = file->LumpCount();j-- > 0;)
			{
				FResourceLump *lump = file->GetLump(j);

				for(unsigned int k = 0;k < iwadTypes.Size();++k)
				{
					for(unsigned int l = iwadTypes[k].Ident.Size();l-- > 0;)
					{
						if(iwadTypes[k].Ident[l].CompareNoCase(lump->Name) == 0 ||
							(strnicmp(lump->FullName, "maps/", 5) == 0 &&
							iwadTypes[k].Ident[l].CompareNoCase(FString(lump->FullName+5, strcspn(lump->FullName+5, ".")))))
						{
							valid[k] |= 1<<l;
						}
					}
				}
			}
			delete file;
		}
	}

	wad.Type = -1;
	for(unsigned int i = 0;i < iwadTypes.Size();++i)
	{
		if(static_cast<unsigned>((1<<iwadTypes[i].Ident.Size())-1) == valid[i])
		{
			wad.Name = iwadTypes[i].Name;
			wad.Type = i;
			break;
		}
	}
	delete[] valid;
	return wad.Type;
}

enum
{
	FILE_AUDIOHED,
	FILE_AUDIOT,
	FILE_GAMEMAPS,
	FILE_MAPHEAD,
	FILE_VGADICT,
	FILE_VGAHEAD,
	FILE_VGAGRAPH,
	FILE_VSWAP,

	BASEFILES
};
struct BaseFile
{
	FString	extension;
	FString	filename[BASEFILES];
	BYTE	isValid;
};
/* Find valid game data.  Due to the nature of WOlf3D we must collect
 * information by extensions.  An extension is considered valid if it has all
 * files needed.  If the OS is case sensitive then the case sensitivity only
 * applies to the extension itself.
 */
static void LookForGameData(FResourceFile *res, TArray<WadStuff> &iwads, const char* directory)
{
	static const unsigned int LoadableBaseFiles[] = { FILE_AUDIOT, FILE_GAMEMAPS, FILE_VGAGRAPH, FILE_VSWAP, BASEFILES };
	static const char* const BaseFileNames[BASEFILES] = {
		"audiohed", "audiot",
		"gamemaps", "maphead",
		"vgadict", "vgahead", "vgagraph",
		"vswap"
	};
	TArray<BaseFile> foundFiles;

	File dir(directory);
	TArray<FString> files = dir.getFileList();
	for(unsigned int i = 0;i < files.Size();++i)
	{
		FString name, extension;
		if(!SplitFilename(files[i], name, extension))
			continue;

		BaseFile *base = NULL;
		for(unsigned int j = 0;j < foundFiles.Size();++j)
		{
			#if ISCASEINSENSITIVE
			if(foundFiles[j].extension.CompareNoCase(extension) == 0)
			#else
			if(foundFiles[j].extension.Compare(extension) == 0)
			#endif
			{
				base = &foundFiles[j];
				break;
			}
		}
		if(!base)
		{
			int index = foundFiles.Push(BaseFile());
			base = &foundFiles[index];
			base->extension = extension;
			base->isValid = 0;
		}

		unsigned int baseName = 0;
		do
		{
			if(name.CompareNoCase(BaseFileNames[baseName]) == 0)
			{
				base->filename[baseName] = files[i];
				base->isValid |= 1<<baseName;
				break;
			}
		}
		while(++baseName < BASEFILES);
	}

	for(unsigned int i = 0;i < foundFiles.Size();++i)
	{
		if(foundFiles[i].isValid == 0xFF)
		{
			WadStuff wadStuff;
			wadStuff.Extension = foundFiles[i].extension;
			for(unsigned int j = 0;LoadableBaseFiles[j] != BASEFILES;++j)
				wadStuff.Path.Push(foundFiles[i].filename[LoadableBaseFiles[j]]);

			// Before checking the data we must load the remap file.
			FString mapFile;
			mapFile.Format("%sMAP", foundFiles[i].extension.GetChars());
			for(unsigned int j = res->LumpCount();j-- > 0;)
			{
				FResourceLump *lump = res->GetLump(j);
				if(mapFile.CompareNoCase(lump->Name) == 0)
					LumpRemapper::LoadMap(foundFiles[i].extension, lump->FullName, (const char*) lump->CacheLump(), lump->LumpSize);
			}

			if(CheckData(wadStuff) > -1)
				iwads.Push(wadStuff);
		}
	}
}

static void ParseIWad(Scanner &sc)
{
	IWadData iwad;

	sc.MustGetToken('{');
	while(!sc.CheckToken('}'))
	{
		sc.MustGetToken(TK_Identifier);
		FString key = sc->str;
		sc.MustGetToken('=');
		if(key.CompareNoCase("Name") == 0)
		{
			sc.MustGetToken(TK_StringConst);
			iwad.Name = sc->str;
		}
		else if(key.CompareNoCase("Mapinfo") == 0)
		{
			sc.MustGetToken(TK_StringConst);
			iwad.Mapinfo = sc->str;
		}
		else if(key.CompareNoCase("MustContain") == 0)
		{
			do
			{
				sc.MustGetToken(TK_StringConst);
				iwad.Ident.Push(sc->str);
			}
			while(sc.CheckToken(','));
		}
	}

	iwadTypes.Push(iwad);
}
static void ParseIWadInfo(FResourceFile *res)
{
	for(unsigned int i = res->LumpCount();i-- > 0;)
	{
		FResourceLump *lump = res->GetLump(i);

		if(lump->Namespace == ns_global && stricmp(lump->Name, "IWADINFO") == 0)
		{
			Scanner sc((const char*)lump->CacheLump(), lump->LumpSize);
			sc.SetScriptIdentifier(lump->FullName);

			while(sc.TokensLeft())
			{
				sc.MustGetToken(TK_Identifier);
				if(sc->str.CompareNoCase("IWad") == 0)
				{
					ParseIWad(sc);
				}
				else
					sc.ScriptMessage(Scanner::ERROR, "Unknown IWADINFO block '%s'.", sc->str.GetChars());
			}
			break;
		}
	}
}

void SelectGame(TArray<FString> &wadfiles, const char* iwad, const char* datawad)
{
	FResourceFile *datawadRes = FResourceFile::OpenResourceFile(datawad, NULL, true);
	if(!datawadRes)
		I_Error("Could not open %s!", datawad);

	ParseIWadInfo(datawadRes);

	TArray<WadStuff> basefiles;
	LookForGameData(datawadRes, basefiles, ".");

	delete datawadRes;

	if(basefiles.Size() == 0)
	{
		I_Error("Can not find base game data. (*.wl6, *.wl1, *.sdm, *.sod)");
	}

	int pick = -1;
	if(iwad)
	{
		for(unsigned int i = 0;i < basefiles.Size();++i)
		{
			#if ISCASEINSENSITIVE
			if(basefiles[i].Extension.CompareNoCase(iwad) == 0)
			#else
			if(basefiles[i].Extension.Compare(iwad) == 0)
			#endif
			{
				pick = i;
				break;
			}
		}
	}
	if(pick < 0)
	{
		if(basefiles.Size() > 1)
			pick = I_PickIWad(&basefiles[0], basefiles.Size(), true, 0);
		else
			pick = 0;
	}
	if(pick < 0)
		Quit("");

	WadStuff &base = basefiles[pick];

	wadfiles.Push("ecwolf.pk3");
	for(unsigned int i = 0;i < base.Path.Size();++i)
	{
		wadfiles.Push(base.Path[i]);
	}
}

}
