/*
** lumpremap.cpp
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

#include "lumpremap.h"
#include "w_wad.h"
#include "zstring.h"
#include "scanner.h"

TMap<FName, LumpRemaper> remaps;
TMap<int, FName> spriteReverseMap;
TMap<int, FName> vgaReverseMap;

LumpRemaper::LumpRemaper(const char* extension) : mapLumpName(extension)
{
	mapLumpName.ToUpper();
	mapLumpName += "MAP";
}

void LumpRemaper::AddFile(const char* extension, FResourceFile *file, Type type)
{
	LumpRemaper *iter = remaps.CheckKey(extension);
	if(iter == NULL)
	{
		LumpRemaper remaper(extension);
		remaper.AddFile(file, type);
		remaps.Insert(extension, remaper);
		return;
	}
	iter->AddFile(file, type);
}

void LumpRemaper::AddFile(FResourceFile *file, Type type)
{
	RemapFile rFile;
	rFile.file = file;
	rFile.type = type;
	files.Push(rFile);
}

const char* LumpRemaper::ConvertSpriteIndexToLump(int num)
{
	return spriteReverseMap[num];
}

const char* LumpRemaper::ConvertVGAIndexToLump(int num)
{
	return vgaReverseMap[num];
}

void LumpRemaper::DoRemap()
{
	if(!LoadMap())
		return;

	for(unsigned int i = 0;i < files.Size();i++)
	{
		RemapFile &file = files[i];
		int temp = 0; // Use to count something
		int temp2 = 0;
		for(unsigned int i = 0;i < file.file->LumpCount();i++)
		{
			FResourceLump *lump = file.file->GetLump(i);
			switch(file.type)
			{
				case AUDIOT:
					if(lump->Namespace == ns_sounds)
					{
						if(i < sounds.Size())
							lump->LumpNameSetup(sounds[i]);
						lump->Namespace = ns_sounds;
						temp++;
					}
					else if(lump->Namespace == ns_music && i-temp < music.Size())
					{
						lump->LumpNameSetup(music[i-temp]);
						lump->Namespace = ns_music;
					}
					break;
				case VGAGRAPH:
					if(i < graphics.Size())
					{
						lump->LumpNameSetup(graphics[i]);
						lump->Namespace = ns_graphics;
					}
					break;
				case VSWAP:
					if(lump->Namespace == ns_flats)
					{
						if(i < textures.Size())
							lump->LumpNameSetup(textures[i]);
						lump->Namespace = ns_flats;
						temp++;
						temp2++;
					}
					else if(lump->Namespace == ns_sprites)
					{
						if(i-temp < sprites.Size())
							lump->LumpNameSetup(sprites[i-temp]);
						lump->Namespace = ns_sprites;
						temp2++;
					}
					else if(lump->Namespace == ns_sounds && i-temp2 < digitalsounds.Size())
					{
						lump->LumpNameSetup(digitalsounds[i-temp2]);
						lump->Namespace = ns_sounds;
					}
					break;
				default:
					break;
			}
		}
	}
	Wads.InitHashChains();
}

bool LumpRemaper::LoadMap()
{
	int lump = Wads.GetNumForName(mapLumpName);
	if(lump == -1)
	{
		printf("\n");
		return false;
	}
	FWadLump mapLump = Wads.OpenLumpNum(lump);

	char* mapData = new char[Wads.LumpLength(lump)];
	mapLump.Read(mapData, Wads.LumpLength(lump));
	Scanner sc(mapData, Wads.LumpLength(lump));
	sc.SetScriptIdentifier(Wads.GetLumpFullName(lump));

	while(sc.TokensLeft() > 0)
	{
		if(!sc.CheckToken(TK_Identifier))
			sc.ScriptMessage(Scanner::ERROR, "Expected identifier in map.\n");

		TMap<int, FName> *reverse = NULL;
		TArray<FName> *map = NULL;
		if(sc->str.Compare("graphics") == 0)
		{
			reverse = &vgaReverseMap;
			map = &graphics;
		}
		else if(sc->str.Compare("sprites") == 0)
		{
			reverse = &spriteReverseMap;
			map = &sprites;
		}
		else if(sc->str.Compare("sounds") == 0)
			map = &sounds;
		else if(sc->str.Compare("digitalsounds") == 0)
			map = &digitalsounds;
		else if(sc->str.Compare("music") == 0)
			map = &music;
		else if(sc->str.Compare("textures") == 0)
			map = &textures;
		else
			sc.ScriptMessage(Scanner::ERROR, "Unknown map section '%s'.\n", sc->str.GetChars());

		if(!sc.CheckToken('{'))
			sc.ScriptMessage(Scanner::ERROR, "Expected '{'.");
		if(!sc.CheckToken('}'))
		{
			int i = 0;
			while(true)
			{
				if(!sc.CheckToken(TK_StringConst))
					sc.ScriptMessage(Scanner::ERROR, "Expected string constant.\n");
				if(reverse != NULL)
					(*reverse)[i++] = sc->str;
				map->Push(sc->str);
				if(sc.CheckToken('}'))
					break;
				if(!sc.CheckToken(','))
					sc.ScriptMessage(Scanner::ERROR, "Expected ','.\n");
			}
		}
	}
	return true;
}

void LumpRemaper::RemapAll()
{
	TMap<FName, LumpRemaper>::Pair *pair;
	for(TMap<FName, LumpRemaper>::Iterator iter(remaps);iter.NextPair(pair);)
	{
		pair->Value.DoRemap();
	}
}

