/*
** g_mapinfo.cpp
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
 
#include "gamemap.h"
#include "g_mapinfo.h"
#include "language.h"
#include "tarray.h"
#include "scanner.h"
#include "w_wad.h"

static LevelInfo defaultMap;
static TArray<LevelInfo> levelInfos;

LevelInfo::LevelInfo() : UseMapInfoName(false)
{
	DefaultTexture[0].SetInvalid();
	DefaultTexture[1].SetInvalid();
	FloorNumber = 1;
	Par = 0;
}

LevelInfo &LevelInfo::Find(const char* level)
{
	for(unsigned int i = 0;i < levelInfos.Size();++i)
	{
		if(levelInfos[i].Name.CompareNoCase(level) == 0)
			return levelInfos[i];
	}
	return defaultMap;
}

static void ParseMap(Scanner &sc, LevelInfo &mapInfo, bool parseHeader=true)
{
	if(parseHeader)
	{
		bool useLanguage = false;
		if(sc.CheckToken(TK_Identifier))
		{
			if(sc->str.CompareNoCase("lookup") != 0)
				sc.ScriptMessage(Scanner::ERROR, "Expected lookup keyword but got '%s' instead.", sc->str.GetChars());
			else
				useLanguage = true;
		}
		if(sc.CheckToken(TK_StringConst))
		{
			mapInfo.UseMapInfoName = true;
			if(useLanguage)
				mapInfo.Name = language[sc->str];
			else
				mapInfo.Name = sc->str;
		}
	}

	sc.MustGetToken('{');
	while(!sc.CheckToken('}'))
	{
		sc.MustGetToken(TK_Identifier);
		FString key = sc->str;

		sc.MustGetToken('=');

		if(key.CompareNoCase("next") == 0)
		{
			sc.MustGetToken(TK_StringConst);
			strncpy(mapInfo.NextMap, sc->str.GetChars(), 8);
			mapInfo.NextMap[8] = 0;
		}
		else if(key.CompareNoCase("secretnext") == 0)
		{
			sc.MustGetToken(TK_StringConst);
			strncpy(mapInfo.NextSecret, sc->str.GetChars(), 8);
			mapInfo.NextSecret[8] = 0;
		}
		else if(key.CompareNoCase("defaultfloor") == 0)
		{
			sc.MustGetToken(TK_StringConst);
			mapInfo.DefaultTexture[MapSector::Floor] = TexMan.GetTexture(sc->str, FTexture::TEX_Flat);
		}
		else if(key.CompareNoCase("defaultceiling") == 0)
		{
			sc.MustGetToken(TK_StringConst);
			mapInfo.DefaultTexture[MapSector::Ceiling] = TexMan.GetTexture(sc->str, FTexture::TEX_Flat);
		}
		else if(key.CompareNoCase("FloorNumber") == 0)
		{
			sc.MustGetToken(TK_IntConst);
			mapInfo.FloorNumber = sc->number;
		}
		else if(key.CompareNoCase("Par") == 0)
		{
			sc.MustGetToken(TK_IntConst);
			mapInfo.Par = sc->number;
		}
		else
		{
			sc.ScriptMessage(Scanner::WARNING, "Unknown map property '%s'!", key.GetChars());
			do
			{
				sc.GetNextToken();
			}
			while(sc.CheckToken(','));
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

static TArray<EpisodeInfo> episodes;

EpisodeInfo::EpisodeInfo() : Shortcut(0), NoSkill(false)
{
}

unsigned int EpisodeInfo::GetNumEpisodes()
{
	return episodes.Size();
}

EpisodeInfo &EpisodeInfo::GetEpisode(unsigned int index)
{
	return episodes[index];
}

// Returns true if the episode should be added to the list
static bool ParseEpisode(Scanner &sc, EpisodeInfo &episode)
{
	bool useEpisode = true;

	sc.MustGetToken(TK_StringConst);
	episode.StartMap = sc->str;

	sc.MustGetToken('{');
	while(!sc.CheckToken('}'))
	{
		sc.MustGetToken(TK_Identifier);
		FString key = sc->str;

		if(key.CompareNoCase("name") == 0)
		{
			sc.MustGetToken('=');
			sc.MustGetToken(TK_StringConst);
			episode.StartMap = sc->str;
		}
		else if(key.CompareNoCase("lookup") == 0)
		{
			sc.MustGetToken('=');
			sc.MustGetToken(TK_StringConst);
			episode.EpisodeName = language[sc->str];
		}
		else if(key.CompareNoCase("picname") == 0)
		{
			sc.MustGetToken('=');
			sc.MustGetToken(TK_StringConst);
			episode.EpisodePicture = sc->str;
		}
		else if(key.CompareNoCase("key") == 0)
		{
			sc.MustGetToken('=');
			sc.MustGetToken(TK_StringConst);
			episode.Shortcut = sc->str[0];
		}
		else if(key.CompareNoCase("remove") == 0)
		{
			useEpisode = false;
		}
		else if(key.CompareNoCase("noskillmenu") == 0)
		{
			episode.NoSkill = true;
		}
		else if(key.CompareNoCase("optional") == 0)
		{
			if(Wads.CheckNumForName(episode.StartMap) == -1)
				useEpisode = false;
		}
		else
		{
			sc.ScriptMessage(Scanner::WARNING, "Unknown episode property '%s'!", key.GetChars());
			do
			{
				sc.GetNextToken();
			}
			while(sc.CheckToken(','));
		}
	}

	return useEpisode;
}

////////////////////////////////////////////////////////////////////////////////

static void ParseMapInfoLump(int lump)
{
	FMemLump data = Wads.ReadLump(lump);
	Scanner sc((const char*)data.GetMem(), data.GetSize());
	sc.SetScriptIdentifier(Wads.GetLumpFullName(lump));

	while(sc.TokensLeft())
	{
		sc.MustGetToken(TK_Identifier);
		if(sc->str.CompareNoCase("defaultmap") == 0)
		{
			defaultMap = LevelInfo();
			ParseMap(sc, defaultMap, false);
		}
		else if(sc->str.CompareNoCase("adddefaultmap") == 0)
		{
			ParseMap(sc, defaultMap, false);
		}
		else if(sc->str.CompareNoCase("episode") == 0)
		{
			EpisodeInfo episode;
			if(ParseEpisode(sc, episode))
				episodes.Push(episode);
		}
		else if(sc->str.CompareNoCase("map") == 0)
		{
			LevelInfo newMap = defaultMap;
			ParseMap(sc, newMap);
			levelInfos.Push(newMap);
		}
	}
}

void G_ParseMapInfo()
{
	int lastlump = 0;
	int lump;

	if((lump = Wads.GetNumForFullName("mapinfo/wolf3d.txt")) != -1)
		ParseMapInfoLump(lump);

	while((lump = Wads.FindLump("ZMAPINFO", &lastlump)) != -1)
		ParseMapInfoLump(lump);
}
