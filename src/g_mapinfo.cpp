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
#include "v_video.h"

////////////////////////////////////////////////////////////////////////////////
// MapInfoBlockParser
//
// Base class to handle syntax of mapinfo lump for common types.

class MapInfoBlockParser
{
public:
	MapInfoBlockParser(Scanner &sc, const char* block) : sc(sc), block(block)
	{
	}

	void Parse()
	{
		ParseHeader();
		ParseBlock(block);
	}

protected:
	virtual bool CheckKey(FString key)=0;
	virtual void ParseHeader() {};

	void ParseBoolAssignment(bool &dest)
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_BoolConst);
		dest = sc->boolean;
	}

	void ParseColorAssignment(int &dest)
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_StringConst);
		dest = V_GetColorFromString(NULL, sc->str);
	}

	void ParseColorArrayAssignment(TArray<int> &dest)
	{
		sc.MustGetToken('=');
		do
		{
			sc.MustGetToken(TK_StringConst);
			dest.Push(V_GetColorFromString(NULL, sc->str));
		}
		while(sc.CheckToken(','));
	}

	void ParseColorArrayAssignment(int *dest, unsigned int max)
	{
		sc.MustGetToken('=');
		do
		{
			sc.MustGetToken(TK_StringConst);
			*dest++ = V_GetColorFromString(NULL, sc->str);

			if(--max == 0)
				break;
		}
		while(sc.CheckToken(','));
	}

	void ParseIntAssignment(int &dest)
	{
		sc.MustGetToken('=');
		bool negative = sc.CheckToken('-');
		sc.MustGetToken(TK_IntConst);
		dest = negative ? -sc->number : sc->number;
	}

	void ParseIntAssignment(unsigned int &dest)
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_IntConst);
		dest = sc->number;
	}

	void ParseIntArrayAssignment(TArray<int> &dest)
	{
		sc.MustGetToken('=');
		do
		{
			bool negative = sc.CheckToken('-');
			sc.MustGetToken(TK_IntConst);
			dest.Push(negative ? -sc->number : sc->number);
		}
		while(sc.CheckToken(','));
	}

	void ParseIntArrayAssignment(TArray<unsigned int> &dest)
	{
		sc.MustGetToken('=');
		do
		{
			sc.MustGetToken(TK_IntConst);
			dest.Push(sc->number);
		}
		while(sc.CheckToken(','));
	}

	void ParseIntArrayAssignment(int *dest, unsigned int max)
	{
		sc.MustGetToken('=');
		do
		{
			bool negative = sc.CheckToken('-');
			sc.MustGetToken(TK_IntConst);
			*dest++ = negative ? -sc->number : sc->number;

			if(--max == 0)
				break;
		}
		while(sc.CheckToken(','));
	}

	void ParseIntArrayAssignment(unsigned int *dest, unsigned int max)
	{
		sc.MustGetToken('=');
		do
		{
			sc.MustGetToken(TK_IntConst);
			*dest++ = sc->number;

			if(--max == 0)
				break;
		}
		while(sc.CheckToken(','));
	}

	void ParseStringAssignment(FString &dest)
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_StringConst);
		dest = sc->str;
	}

	void ParseStringArrayAssignment(TArray<FString> &dest)
	{
		sc.MustGetToken('=');
		do
		{
			sc.MustGetToken(TK_StringConst);
			dest.Push(sc->str);
		}
		while(sc.CheckToken(','));
	}

	void ParseStringArrayAssignment(FString *dest, unsigned int max)
	{
		sc.MustGetToken('=');
		do
		{
			sc.MustGetToken(TK_StringConst);
			*dest++ = sc->str;

			if(--max == 0)
				break;
		}
		while(sc.CheckToken(','));
	}

	void ParseFontColorAssignment(EColorRange &dest)
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_StringConst);
		dest = V_FindFontColor(sc->str);
	}

	Scanner &sc;
	const char* const block;

private:
	void ParseBlock(const char* block)
	{
		sc.MustGetToken('{');
		while(!sc.CheckToken('}'))
		{
			sc.MustGetToken(TK_Identifier);
			if(!CheckKey(sc->str))
			{
				sc.ScriptMessage(Scanner::WARNING, "Unknown %s property '%s'.", block, sc->str.GetChars());
				if(sc.CheckToken('='))
				{
					do
					{
						sc.GetNextToken();
					}
					while(sc.CheckToken(','));
				}
			}
		}
	}
};

////////////////////////////////////////////////////////////////////////////////

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

class LevelInfoBlockParser : public MapInfoBlockParser
{
private:
	bool parseHeader;
	LevelInfo &mapInfo;

public:
	LevelInfoBlockParser(Scanner &sc, LevelInfo &mapInfo, bool parseHeader) :
		MapInfoBlockParser(sc, "map"), mapInfo(mapInfo), parseHeader(parseHeader) {}

protected:
	void ParseHeader()
	{
		if(!parseHeader)
			return;

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

	bool CheckKey(FString key)
	{
		if(key.CompareNoCase("next") == 0)
		{
			FString lump;
			ParseStringAssignment(lump);
			strncpy(mapInfo.NextMap, lump.GetChars(), 8);
			mapInfo.NextMap[8] = 0;
		}
		else if(key.CompareNoCase("secretnext") == 0)
		{
			FString lump;
			ParseStringAssignment(lump);
			strncpy(mapInfo.NextSecret, lump.GetChars(), 8);
			mapInfo.NextSecret[8] = 0;
		}
		else if(key.CompareNoCase("defaultfloor") == 0)
		{
			FString textureName;
			ParseStringAssignment(textureName);
			mapInfo.DefaultTexture[MapSector::Floor] = TexMan.GetTexture(textureName, FTexture::TEX_Flat);
		}
		else if(key.CompareNoCase("defaultceiling") == 0)
		{
			FString textureName;
			ParseStringAssignment(textureName);
			mapInfo.DefaultTexture[MapSector::Ceiling] = TexMan.GetTexture(textureName, FTexture::TEX_Flat);
		}
		else if(key.CompareNoCase("Cluster") == 0)
			ParseIntAssignment(mapInfo.Cluster);
		else if(key.CompareNoCase("FloorNumber") == 0)
			ParseIntAssignment(mapInfo.FloorNumber);
		else if(key.CompareNoCase("Music") == 0)
			ParseStringAssignment(mapInfo.Music);
		else if(key.CompareNoCase("Par") == 0)
			ParseIntAssignment(mapInfo.Par);
		else
			return false;
		return true;
	}
};

////////////////////////////////////////////////////////////////////////////////

GameInfo gameinfo;

class GameInfoBlockParser : public MapInfoBlockParser
{
public:
	GameInfoBlockParser(Scanner &sc) : MapInfoBlockParser(sc, "gameinfo") {}

protected:
	bool CheckKey(FString key)
	{
		if(key.CompareNoCase("creditpage") == 0)
			ParseStringAssignment(gameinfo.CreditPage);
		else if(key.CompareNoCase("drawreadthis") == 0)
			ParseBoolAssignment(gameinfo.DrawReadThis);
		else if(key.CompareNoCase("gamepalette") == 0)
			ParseStringAssignment(gameinfo.GamePalette);
		else if(key.CompareNoCase("signon") == 0)
			ParseStringAssignment(gameinfo.SignonLump);
		else if(key.CompareNoCase("signoncolors") == 0)
			// Fill, Main, EMS, XMS
			ParseColorArrayAssignment(gameinfo.SignonColors, 4);
		else if(key.CompareNoCase("menufade") == 0)
			ParseColorAssignment(gameinfo.MenuFadeColor);
		else if(key.CompareNoCase("menucolors") == 0)
			// Border1, Border2, Border3, Background, Stripe
			ParseColorArrayAssignment(gameinfo.MenuColors, 5);
		else if(key.CompareNoCase("titlemusic") == 0)
			ParseStringAssignment(gameinfo.TitleMusic);
		else if(key.CompareNoCase("titlepalette") == 0)
			ParseStringAssignment(gameinfo.TitlePalette);
		else if(key.CompareNoCase("titlepage") == 0)
			ParseStringAssignment(gameinfo.TitlePage);
		else if(key.CompareNoCase("titletime") == 0)
			ParseIntAssignment(gameinfo.TitleTime);
		else if(key.CompareNoCase("pagetime") == 0)
			ParseIntAssignment(gameinfo.PageTime);
		else if(key.CompareNoCase("menumusic") == 0)
			ParseStringAssignment(gameinfo.MenuMusic);
		else if(key.CompareNoCase("scoresmusic") == 0)
			ParseStringAssignment(gameinfo.ScoresMusic);
		else if(key.CompareNoCase("finalemusic") == 0)
			ParseStringAssignment(gameinfo.FinaleMusic);
		else if(key.CompareNoCase("intermissionmusic") == 0)
			ParseStringAssignment(gameinfo.IntermissionMusic);
		else if(key.CompareNoCase("menufontcolor_title") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::MENU_TITLE]);
		else if(key.CompareNoCase("menufontcolor_label") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::MENU_LABEL]);
		else if(key.CompareNoCase("menufontcolor_selection") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::MENU_SELECTION]);
		else if(key.CompareNoCase("menufontcolor_disabled") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::MENU_DISABLED]);
		else if(key.CompareNoCase("menufontcolor_invalid") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::MENU_INVALID]);
		else if(key.CompareNoCase("menufontcolor_invalidselection") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::MENU_INVALIDSELECTION]);
		else if(key.CompareNoCase("menufontcolor_highlight") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::MENU_HIGHLIGHT]);
		else if(key.CompareNoCase("menufontcolor_highlightselection") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::MENU_HIGHLIGHTSELECTION]);
		else if(key.CompareNoCase("highscoresfontcolor") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::HIGHSCORES]);
		else
			return false;
		return true;
	}
};

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

class EpisodeBlockParser : public MapInfoBlockParser
{
private:
	EpisodeInfo &episode;
	bool useEpisode;

public:
	EpisodeBlockParser(Scanner &sc, EpisodeInfo &episode) :
		MapInfoBlockParser(sc, "episode"), episode(episode), useEpisode(true)
		{}

	bool UseEpisode() const { return useEpisode; }

protected:
	void ParseHeader()
	{
		sc.MustGetToken(TK_StringConst);
		episode.StartMap = sc->str;
	}

	bool CheckKey(FString key)
	{
		if(key.CompareNoCase("name") == 0)
			ParseStringAssignment(episode.StartMap);
		else if(key.CompareNoCase("lookup") == 0)
		{
			ParseStringAssignment(episode.EpisodeName);
			episode.EpisodeName = language[episode.EpisodeName];
		}
		else if(key.CompareNoCase("picname") == 0)
			ParseStringAssignment(episode.EpisodePicture);
		else if(key.CompareNoCase("key") == 0)
		{
			FString tmp;
			ParseStringAssignment(tmp);
			episode.Shortcut = tmp[0];
		}
		else if(key.CompareNoCase("remove") == 0)
			useEpisode = false;
		else if(key.CompareNoCase("noskillmenu") == 0)
			episode.NoSkill = true;
		else if(key.CompareNoCase("optional") == 0)
		{
			if(Wads.CheckNumForName(episode.StartMap) == -1)
				useEpisode = false;
		}
		else
			return false;
		return true;
	}
};

////////////////////////////////////////////////////////////////////////////////

static TMap<unsigned int, ClusterInfo> clusters;

ClusterInfo::ClusterInfo() : ExitTextLookup(false)
{
}

ClusterInfo &ClusterInfo::Find(unsigned int index)
{
	return clusters[index];
}

class ClusterBlockParser : public MapInfoBlockParser
{
private:
	ClusterInfo *cluster;

public:
	ClusterBlockParser(Scanner &sc) :
		MapInfoBlockParser(sc, "cluster")
		{}
protected:
	void ParseHeader()
	{
		sc.MustGetToken(TK_IntConst);
		cluster = &ClusterInfo::Find(sc->number);
	}

	bool CheckKey(FString key)
	{
		if(key.CompareNoCase("exittext") == 0)
		{
			sc.MustGetToken('=');
			if(sc.CheckToken(TK_Identifier))
			{
				if(sc->str.CompareNoCase("lookup") != 0)
					sc.ScriptMessage(Scanner::ERROR, "Expected lookup but got '%s' instead.", sc->str.GetChars());
				sc.MustGetToken(',');
				cluster->ExitTextLookup = true;
			}
			sc.MustGetToken(TK_StringConst);
			cluster->ExitText = sc->str;
		}
		else
			return false;
		return true;
	}
};

////////////////////////////////////////////////////////////////////////////////

static void SkipBlock(Scanner &sc)
{
	// Skip header
	while(sc.GetNextToken() && sc->token != '{');
	// Skip content
	while(sc.GetNextToken() && sc->token != '}');
}

static void ParseMapInfoLump(int lump, bool gameinfoPass)
{
	FMemLump data = Wads.ReadLump(lump);
	Scanner sc((const char*)data.GetMem(), data.GetSize());
	sc.SetScriptIdentifier(Wads.GetLumpFullName(lump));

	while(sc.TokensLeft())
	{
		sc.MustGetToken(TK_Identifier);
		if(!gameinfoPass)
		{
			if(sc->str.CompareNoCase("defaultmap") == 0)
			{
				defaultMap = LevelInfo();
				LevelInfoBlockParser(sc, defaultMap, false).Parse();
			}
			else if(sc->str.CompareNoCase("adddefaultmap") == 0)
			{
				LevelInfoBlockParser(sc, defaultMap, false).Parse();
			}
			else if(sc->str.CompareNoCase("cluster") == 0)
			{
				ClusterBlockParser(sc).Parse();
			}
			else if(sc->str.CompareNoCase("episode") == 0)
			{
				EpisodeInfo episode;
				EpisodeBlockParser parser(sc, episode);
				parser.Parse();
				if(parser.UseEpisode())
					episodes.Push(episode);
			}
			else if(sc->str.CompareNoCase("map") == 0)
			{
				LevelInfo newMap = defaultMap;
				LevelInfoBlockParser(sc, newMap, true).Parse();
				levelInfos.Push(newMap);
			}
			else
				SkipBlock(sc);
		}
		else
		{
			if(sc->str.CompareNoCase("gameinfo") == 0)
			{
				GameInfoBlockParser(sc).Parse();
			}
			else
				SkipBlock(sc);
		}
	}
}

void G_ParseMapInfo(bool gameinfoPass)
{
	int lastlump = 0;
	int lump;

	if((lump = Wads.GetNumForFullName("mapinfo/wolf3d.txt")) != -1)
		ParseMapInfoLump(lump, gameinfoPass);

	while((lump = Wads.FindLump("MAPINFO", &lastlump)) != -1)
		ParseMapInfoLump(lump, gameinfoPass);

	while((lump = Wads.FindLump("ZMAPINFO", &lastlump)) != -1)
		ParseMapInfoLump(lump, gameinfoPass);
}
