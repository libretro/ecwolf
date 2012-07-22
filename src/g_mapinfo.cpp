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
#include "lnspec.h"
#include "tarray.h"
#include "scanner.h"
#include "w_wad.h"
#include "wl_iwad.h"
#include "v_video.h"
#include "thingdef/thingdef.h"

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
	MapName[0] = 0;
	BorderTexture.SetInvalid();
	DefaultTexture[0].SetInvalid();
	DefaultTexture[1].SetInvalid();
	DeathCam = false;
	FloorNumber = 1;
	Par = 0;
	LevelBonus = -1;
	LevelNumber = 0;
	Cluster = 0;
	NoIntermission = false;
}

FTextureID LevelInfo::GetBorderTexture() const
{
	static FTextureID BorderFlat = TexMan.GetTexture(gameinfo.BorderFlat, FTexture::TEX_Flat);
	if(!BorderTexture.isValid())
		return BorderFlat;
	return BorderTexture;
}

FString LevelInfo::GetName(const GameMap *gm) const
{
	if(UseMapInfoName)
		return Name;
	return gm->GetHeader().name;
}

LevelInfo &LevelInfo::Find(const char* level)
{
	for(unsigned int i = 0;i < levelInfos.Size();++i)
	{
		if(stricmp(levelInfos[i].MapName, level) == 0)
			return levelInfos[i];
	}
	return defaultMap;
}

LevelInfo &LevelInfo::FindByNumber(unsigned int num)
{
	for(unsigned int i = 0;i < levelInfos.Size();++i)
	{
		if(levelInfos[i].LevelNumber == num)
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
		MapInfoBlockParser(sc, "map"), parseHeader(parseHeader), mapInfo(mapInfo) {}

protected:
	void ParseHeader()
	{
		if(!parseHeader)
			return;

		sc.MustGetToken(TK_StringConst);
		strncpy(mapInfo.MapName, sc->str, 8);
		mapInfo.MapName[8] = 0;

		if(strnicmp(mapInfo.MapName, "MAP", 3) == 0)
		{
			int num = atoi(mapInfo.MapName+3);
			if(num > 0) // Zero is the default so no need to do anything.
			{
				ClearLevelNumber(num);
				mapInfo.LevelNumber = num;
			}
		}

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

	void ClearLevelNumber(unsigned int num)
	{
		LevelInfo &other = LevelInfo::FindByNumber(num);
		if(other.MapName[0] != 0)
			other.LevelNumber = 0;
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
		else if(key.CompareNoCase("bordertexture") == 0)
		{
			FString textureName;
			ParseStringAssignment(textureName);
			mapInfo.BorderTexture = TexMan.GetTexture(textureName, FTexture::TEX_Flat);
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
		else if(key.CompareNoCase("SpecialAction") == 0)
		{
			LevelInfo::SpecialAction action;
			sc.MustGetToken('=');
			sc.MustGetToken(TK_StringConst);
			action.Class = ClassDef::FindClassTentative(sc->str, NATIVE_CLASS(Actor));
			sc.MustGetToken(',');
			sc.MustGetToken(TK_StringConst);
			action.Special = Specials::LookupFunctionNum(sc->str);

			unsigned int i;
			for(i = 0;i < 5 && sc.CheckToken(',');++i)
			{
				sc.MustGetToken(TK_IntConst);
				action.Args[i] = sc->number;
			}
			while(i < 5)
				action.Args[i++] = 0;

			mapInfo.SpecialActions.Push(action);
		}
		else if(key.CompareNoCase("Cluster") == 0)
			ParseIntAssignment(mapInfo.Cluster);
		else if(key.CompareNoCase("CompletionString") == 0)
			ParseStringAssignment(mapInfo.CompletionString);
		else if(key.CompareNoCase("DeathCam") == 0)
			ParseBoolAssignment(mapInfo.DeathCam);
		else if(key.CompareNoCase("FloorNumber") == 0)
			ParseIntAssignment(mapInfo.FloorNumber);
		else if(key.CompareNoCase("LevelBonus") == 0)
			ParseIntAssignment(mapInfo.LevelBonus);
		else if(key.CompareNoCase("LevelNum") == 0)
		{
			// Get the number and then remove any other map from this slot.
			unsigned int num;
			ParseIntAssignment(num);
			ClearLevelNumber(num);
			mapInfo.LevelNumber = num;
		}
		else if(key.CompareNoCase("Music") == 0)
			ParseStringAssignment(mapInfo.Music);
		else if(key.CompareNoCase("NoIntermission") == 0)
			mapInfo.NoIntermission = true;
		else if(key.CompareNoCase("Par") == 0)
			ParseIntAssignment(mapInfo.Par);
		else if(key.CompareNoCase("Translator") == 0)
			ParseStringAssignment(mapInfo.Translator);
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
		if(key.CompareNoCase("advisorycolor") == 0)
			ParseColorAssignment(gameinfo.AdvisoryColor);
		else if(key.CompareNoCase("advisorypic") == 0)
			ParseStringAssignment(gameinfo.AdvisoryPic);
		else if(key.CompareNoCase("borderflat") == 0)
			ParseStringAssignment(gameinfo.BorderFlat);
		else if(key.CompareNoCase("creditpage") == 0)
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
			// Border1, Border2, Border3, Background, Stripe, StripeBG
			ParseColorArrayAssignment(gameinfo.MenuColors, 6);
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
		else if(key.CompareNoCase("highscoresfont") == 0)
			ParseStringAssignment(gameinfo.HighScoresFont);
		else if(key.CompareNoCase("highscoresfontcolor") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::HIGHSCORES]);
		else if(key.CompareNoCase("pageindexfontcolor") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::PAGEINDEX]);
		else if(key.CompareNoCase("quitmessages") == 0)
			ParseStringArrayAssignment(gameinfo.QuitMessages);
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
			ParseStringAssignment(episode.EpisodeName);
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

ClusterInfo::ClusterInfo() : ExitTextType(ClusterInfo::EXIT_STRING)
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
			bool lookup = false;
			if(sc.CheckToken(TK_Identifier))
			{
				if(sc->str.CompareNoCase("lookup") != 0)
					sc.ScriptMessage(Scanner::ERROR, "Expected lookup but got '%s' instead.", sc->str.GetChars());
				sc.MustGetToken(',');
				lookup = true;
			}
			sc.MustGetToken(TK_StringConst);
			cluster->ExitText = lookup ? FString(language[sc->str]) : sc->str;
		}
		else if(key.CompareNoCase("exittextislump") == 0)
		{
			cluster->ExitTextType = ClusterInfo::EXIT_LUMP;
		}
		else if(key.CompareNoCase("exittextismessage") == 0)
		{
			cluster->ExitTextType = ClusterInfo::EXIT_MESSAGE;
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
		if(sc->str.CompareNoCase("include") == 0)
		{
			sc.MustGetToken(TK_StringConst);
			int includeLump = Wads.GetNumForFullName(sc->str);
			if(includeLump != -1)
				ParseMapInfoLump(includeLump, gameinfoPass);

			continue;
		}

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
			else if(sc->str.CompareNoCase("clearepisodes") == 0)
			{
				episodes.Clear();
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

				LevelInfo &existing = LevelInfo::Find(newMap.MapName);
				if(&existing != &defaultMap)
					existing = newMap;
				else
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
			// Regular key words
			else if(sc->str.CompareNoCase("clearepisodes") == 0) {}
			else
				SkipBlock(sc);
		}
	}
}

void G_ParseMapInfo(bool gameinfoPass)
{
	int lastlump = 0;
	int lump;

	if((lump = Wads.GetNumForFullName(IWad::GetGame().Mapinfo)) != -1)
		ParseMapInfoLump(lump, gameinfoPass);

	while((lump = Wads.FindLump("MAPINFO", &lastlump)) != -1)
		ParseMapInfoLump(lump, gameinfoPass);

	while((lump = Wads.FindLump("ZMAPINFO", &lastlump)) != -1)
		ParseMapInfoLump(lump, gameinfoPass);

	if(!gameinfoPass && episodes.Size() == 0)
		Quit("At least 1 episode must be defined.");
}
