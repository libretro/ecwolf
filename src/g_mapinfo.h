/*
** g_mapinfo.h
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
 
#ifndef __G_MAPINFO_H__
#define __G_MAPINFO_H__

#include "textures/textures.h"
#include "v_font.h"
#include "zstring.h"

extern class GameInfo
{
public:
	FString	SignonLump;
	int		SignonColors[4];
	int		MenuFadeColor;
	int		MenuColors[5];
	bool	DrawReadThis;

	int		PageTime;
	int		TitleTime;
	FString	CreditPage;
	FString	GamePalette;
	FString	TitleMusic;
	FString	TitlePage;
	FString	TitlePalette;
	FString	MenuMusic;
	FString	ScoresMusic;
	FString	FinaleMusic;
	FString	IntermissionMusic;

	enum EFontColors
	{
		MENU_TITLE,
		MENU_LABEL,
		MENU_SELECTION,
		MENU_DISABLED,
		MENU_INVALID,
		MENU_INVALIDSELECTION,
		MENU_HIGHLIGHT,
		MENU_HIGHLIGHTSELECTION,
		HIGHSCORES,
		PAGEINDEX,

		NUM_FONTCOLORS
	};
	EColorRange	FontColors[NUM_FONTCOLORS];
} gameinfo;

class LevelInfo
{
public:
	LevelInfo();

	char			NextMap[9];
	char			NextSecret[9];
	bool			UseMapInfoName;
	FString			Name;
	unsigned int	FloorNumber;
	FString			Music;
	unsigned int	Cluster;

	FTextureID		DefaultTexture[2];
	unsigned int	Par;

	static LevelInfo &Find(const char* level);
};

class EpisodeInfo
{
public:
	EpisodeInfo();

	FString		StartMap;
	FString		EpisodeName;
	FString		EpisodePicture;
	char		Shortcut;
	bool		NoSkill;

	static unsigned int GetNumEpisodes();
	static EpisodeInfo &GetEpisode(unsigned int index);
};

class ClusterInfo
{
public:
	ClusterInfo();

	FString	ExitText;
	bool	ExitTextLookup;

	static ClusterInfo &Find(unsigned int index);
};

void G_ParseMapInfo(bool gameinfoPass);

#endif
