////////////////////////////////////////////////////////////////////
//
// WL_MENU.C
// by John Romero (C) 1992 Id Software, Inc.
//
////////////////////////////////////////////////////////////////////

#include "m_classes.h"
#include "m_random.h"
#include "wl_def.h"
#include "wl_menu.h"
#include "wl_iwad.h"
#include "id_ca.h"
#include "id_sd.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#include "language.h"
#include "w_wad.h"
#include "c_cvars.h"
#include "g_mapinfo.h"
#include "v_video.h"
#include "wl_agent.h"
#include "wl_inter.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_play.h"
#include "wl_text.h"
#include "v_palette.h"
#include "colormatcher.h"
#include "v_font.h"
#include "templates.h"
#include "thingdef/thingdef.h"
#include "wl_loadsave.h"
#include "am_map.h"

#include <climits>

static int	lastgamemusicoffset;
const ClassDef *playerClass = NULL;
EpisodeInfo	*episode = 0;
int BORDCOLOR, BORD2COLOR, BORD3COLOR, BKGDCOLOR, STRIPE, STRIPEBG,
	MENUWIN_BACKGROUND, MENUWIN_TOPBORDER, MENUWIN_BOTBORDER,
	MENUWINHGLT_BACKGROUND, MENUWINHGLT_TOPBORDER, MENUWINHGLT_BOTBORDER;
static MenuItem	*readThis;
// Android version reads this elsewhere so non-static.
bool menusAreFaded = true;

EMenuStyle MenuStyle = MENUSTYLE_Wolf;


Menu mainMenu(MENU_X, MENU_Y, MENU_W, 24);
Menu playerClasses(NM_X, NM_Y, NM_W, 24);
Menu episodes(NE_X+4, NE_Y-1, NE_W+7, 83);
Menu skills(NM_X, NM_Y, NM_W, 24);
MENU_LISTENER(SetEpisodeAndSwitchToSkill)
{
	EpisodeInfo &ep = EpisodeInfo::GetEpisode(which);

	if(!GameMap::CheckMapExists(ep.StartMap))
	{
		SD_PlaySound("player/usefail");
		Message("Please select \"Read This!\"\n"
				"from the Options menu to\n"
				"find out how to order this\n" "episode from Apogee.");
		IN_ClearKeysDown();
		IN_Ack();
		episodes.draw();
		return false;
	}
	episode = &ep;
	return true;
}
MENU_LISTENER(StartNewGame)
{
	// libretro handles it separately

	return true;
}
void CreateMenus()
{
	// HACK: Determine menu style by IWAD
	if(IWad::CheckGameFilter("Blake"))
		MenuStyle = MENUSTYLE_Blake;

	// Extract the palette
	BORDCOLOR = ColorMatcher.Pick(RPART(gameinfo.MenuColors[0]), GPART(gameinfo.MenuColors[0]), BPART(gameinfo.MenuColors[0]));
	BORD2COLOR = ColorMatcher.Pick(RPART(gameinfo.MenuColors[1]), GPART(gameinfo.MenuColors[1]), BPART(gameinfo.MenuColors[1]));
	BORD3COLOR = ColorMatcher.Pick(RPART(gameinfo.MenuColors[2]), GPART(gameinfo.MenuColors[2]), BPART(gameinfo.MenuColors[2]));
	BKGDCOLOR = ColorMatcher.Pick(RPART(gameinfo.MenuColors[3]), GPART(gameinfo.MenuColors[3]), BPART(gameinfo.MenuColors[3]));
	STRIPE = ColorMatcher.Pick(RPART(gameinfo.MenuColors[4]), GPART(gameinfo.MenuColors[4]), BPART(gameinfo.MenuColors[4]));
	STRIPEBG = ColorMatcher.Pick(RPART(gameinfo.MenuColors[5]), GPART(gameinfo.MenuColors[5]), BPART(gameinfo.MenuColors[5]));
	MENUWIN_BACKGROUND = ColorMatcher.Pick(RPART(gameinfo.MenuWindowColors[0]), GPART(gameinfo.MenuWindowColors[0]), BPART(gameinfo.MenuWindowColors[0])),
	MENUWIN_TOPBORDER = ColorMatcher.Pick(RPART(gameinfo.MenuWindowColors[1]), GPART(gameinfo.MenuWindowColors[1]), BPART(gameinfo.MenuWindowColors[1])),
	MENUWIN_BOTBORDER = ColorMatcher.Pick(RPART(gameinfo.MenuWindowColors[2]), GPART(gameinfo.MenuWindowColors[2]), BPART(gameinfo.MenuWindowColors[2])),
	MENUWINHGLT_BACKGROUND = ColorMatcher.Pick(RPART(gameinfo.MenuWindowColors[3]), GPART(gameinfo.MenuWindowColors[3]), BPART(gameinfo.MenuWindowColors[3])),
	MENUWINHGLT_TOPBORDER = ColorMatcher.Pick(RPART(gameinfo.MenuWindowColors[4]), GPART(gameinfo.MenuWindowColors[4]), BPART(gameinfo.MenuWindowColors[4])),
	MENUWINHGLT_BOTBORDER = ColorMatcher.Pick(RPART(gameinfo.MenuWindowColors[5]), GPART(gameinfo.MenuWindowColors[5]), BPART(gameinfo.MenuWindowColors[5]));

	// Actually initialize the menus

	episodes.setHeadText(language["STR_WHICHEPISODE"]);
	for(unsigned int i = 0;i < EpisodeInfo::GetNumEpisodes();++i)
	{
		EpisodeInfo &episode = EpisodeInfo::GetEpisode(i);
		MenuItem *tmp = new MenuSwitcherMenuItem(episode.EpisodeName, skills, SetEpisodeAndSwitchToSkill);
		if(!episode.EpisodePicture.IsEmpty())
			tmp->setPicture(episode.EpisodePicture);
		if(!GameMap::CheckMapExists(episode.StartMap))
			tmp->setHighlighted(2);
		episodes.addItem(tmp);
	}

	skills.setHeadText(language["STR_HOWTOUGH"]);
	skills.setHeadPicture("M_HOWTGH", true);
	for(unsigned int i = 0;i < SkillInfo::GetNumSkills();++i)
	{
		SkillInfo &skill = SkillInfo::GetSkill(i);
		MenuItem *tmp = new MenuItem(skill.Name, StartNewGame);
		if(!skill.SkillPicture.IsEmpty())
			tmp->setPicture(skill.SkillPicture, NM_X + 185, NM_Y + 7);
		skills.addItem(tmp);
	}
	skills.setCurrentPosition(2);

}


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
//
// SUPPORT ROUTINES
//
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//
// Clear Menu screens to dark red
//
////////////////////////////////////////////////////////////////////
void ClearMScreen (void)
{
	static FTextureID backdropID = TexMan.CheckForTexture("BACKDROP", FTexture::TEX_Any);
	if(!backdropID.isValid())
		VWB_Clear (BORDCOLOR, 0, 0, screenWidth, screenHeight);
	else
		CA_CacheScreen(TexMan(backdropID), true);
}


////////////////////////////////////////////////////////////////////
//
// Draw a window for a menu
//
////////////////////////////////////////////////////////////////////
void DrawWindow (int x, int y, int w, int h, int wcolor, int color1, int color2)
{
	int wx = x, wy = y, ww = w, wh = h;
	MenuToRealCoords(wx, wy, ww, wh, MENU_CENTER);

	VWB_Clear (wcolor, wx, wy, wx+ww, wy+wh);
	DrawOutline (x, y, w, h, color1, color2);
}

void DrawOutline (int x, int y, int w, int h, int color1, int color2)
{
	MenuToRealCoords(x, y, w, h, MENU_CENTER);

	VWB_Clear(color2, x-scaleFactorX, y, x+w+scaleFactorX, y+scaleFactorY);
	VWB_Clear(color2, x-scaleFactorX, y, x, y+h);
	VWB_Clear(color1, x-scaleFactorX, y+h, x+w+scaleFactorX, y+h+scaleFactorY);
	VWB_Clear(color1, x+w, y, x+w+scaleFactorX, y+h);
}

////////////////////////////////////////////////////////////////////
//
// Setup Control Panel stuff - graphics, etc.
//
////////////////////////////////////////////////////////////////////
void SetupControlPanel (void)
{
	WindowH = 200;
	if(screenHeight % 200 != 0)
		VL_ClearScreen(0);

	//
	// CENTER MOUSE
	//
	if(IN_IsInputGrabbed())
		IN_CenterMouse();
}

////////////////////////////////////////////////////////////////////
//
// Clean up all the Control Panel stuff
//
////////////////////////////////////////////////////////////////////
void CleanupControlPanel (void)
{
	VWB_Clear(ColorMatcher.Pick(RPART(gameinfo.MenuFadeColor), GPART(gameinfo.MenuFadeColor), BPART(gameinfo.MenuFadeColor)),
		0, 0, screenWidth, screenHeight);
}


////////////////////////////////////////////////////////////////////
//
// PRINT A MESSAGE IN A WINDOW
//
////////////////////////////////////////////////////////////////////
void Message (const char *string)
{
	static const int
		MESSAGE_BG = ColorMatcher.Pick(RPART(gameinfo.MessageColors[0]), GPART(gameinfo.MessageColors[0]), BPART(gameinfo.MessageColors[0])),
		TOPBRDR = ColorMatcher.Pick(RPART(gameinfo.MessageColors[1]), GPART(gameinfo.MessageColors[1]), BPART(gameinfo.MessageColors[1])),
		BOTBRDR = ColorMatcher.Pick(RPART(gameinfo.MessageColors[2]), GPART(gameinfo.MessageColors[2]), BPART(gameinfo.MessageColors[2]));

	uint16_t width, height;

	FString measureString;
	measureString.Format("%s_", string);
	VW_MeasurePropString(BigFont, measureString, width, height);
	width = MIN<int>(width, 320 - 10);
	height = MIN<int>(height, 200 - 10);

	PrintY = (WindowH / 2) - height / 2;
	PrintX = WindowX = 160 - width / 2;

	DrawWindow (WindowX - 5, PrintY - 5, width + 10, height + 10, MESSAGE_BG);
	DrawOutline (WindowX - 5, PrintY - 5, width + 10, height + 10, BOTBRDR, TOPBRDR);
	US_Print (BigFont, string, gameinfo.FontColors[GameInfo::MESSAGEFONT]);
	VW_UpdateScreen ();
}

////////////////////////////////////////////////////////////////////
//
// THIS MAY BE FIXED A LITTLE LATER...
//
////////////////////////////////////////////////////////////////////

int StartCPMusic (const char* song)
{
	int lastoffs;

	//lastmusic = song;
	lastoffs = SD_MusicOff ();

	SD_StartMusic(song);
	return lastoffs;
}

///////////////////////////////////////////////////////////////////////////
//
// CHECK FOR PAUSE KEY (FOR MUSIC ONLY)
//
///////////////////////////////////////////////////////////////////////////
void CheckPause (void)
{
	static int SoundStatus = 1;
	static int pauseofs = 0;
	if (Paused & 1)
	{
		switch (SoundStatus)
		{
			case 0:
				SD_ContinueMusic(gameinfo.MenuMusic, pauseofs);
				break;
			case 1:
				pauseofs = SD_MusicOff();
				break;
		}

		SoundStatus ^= 1;
		IN_ClearKeysDown ();
		Paused &= ~1;
	}
}

///////////////////////////////////////////////////////////////////////////
//
// DRAW SCREEN TITLE STRIPES
//
///////////////////////////////////////////////////////////////////////////
void DrawStripes (int y)
{
	static unsigned int calcStripes = INT_MAX;
	static unsigned int sy, sh;
	static unsigned int ly, lh;
	if(calcStripes != scaleFactorY)
	{
		unsigned int dummyx = 0, dummyw = 320;
		sy = y;
		sh = 24;
		ly = y+22;
		lh = 1;
		calcStripes = scaleFactorY;

		MenuToRealCoords(dummyx, sy, dummyw, sh, MENU_TOP);
		MenuToRealCoords(dummyx, ly, dummyw, lh, MENU_TOP);
	}

	VWB_Clear(STRIPEBG, 0, sy, screenWidth, sy+sh);
	VWB_Clear(STRIPE, 0, ly, screenWidth, ly+lh);
}

void ShootSnd (void)
{
	SD_PlaySound ("menu/activate");
}

void MenuFadeOut()
{
	assert(!menusAreFaded);
	menusAreFaded = true;

	VL_FadeOut(0, 255,
		RPART(gameinfo.MenuFadeColor), GPART(gameinfo.MenuFadeColor), BPART(gameinfo.MenuFadeColor),
		10);
}

void MenuFadeIn()
{
	assert(menusAreFaded);
	menusAreFaded = false;

	VL_FadeIn(0, 255, 10);
}

