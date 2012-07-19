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
#include "id_ca.h"
#include "id_sd.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#include "language.h"
#include "w_wad.h"
#include "c_cvars.h"
#include "wl_agent.h"
#include "g_mapinfo.h"
#include "wl_inter.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_play.h"
#include "wl_text.h"
#include "v_palette.h"
#include "colormatcher.h"
#include "v_font.h"
#include "templates.h"
#include "wl_loadsave.h"

extern int	lastgamemusicoffset;
EpisodeInfo	*episode = 0;
int BORDCOLOR, BORD2COLOR, BORD3COLOR, BKGDCOLOR, STRIPE, STRIPEBG;
static MenuItem	*readThis;
static bool menusAreFaded = true;

MENU_LISTENER(EnterControlBase);

Menu mainMenu(MENU_X, MENU_Y, MENU_W, 24);
Menu optionsMenu(80, 85, 180, 28);
Menu soundBase(24, 45, 284, 24);
Menu controlBase(CTL_X, CTL_Y, CTL_W, 56, EnterControlBase);
Menu displayMenu(60, 95, 225, 56);
Menu mouseSensitivity(20, 80, 300, 0);
Menu episodes(NE_X+4, NE_Y-1, NE_W+7, 83);
Menu skills(NM_X, NM_Y, NM_W, 24);
Menu controls(15, 70, 310, 24);
Menu resolutionMenu(90, 25, 150, 24);

MENU_LISTENER(PlayDemosOrReturnToGame)
{
	Menu::closeMenus();
	if (!ingame)
		StartCPMusic(gameinfo.TitleMusic);
	return true;
}
MENU_LISTENER(ViewScoresOrEndGame)
{
	if (ingame)
	{
		if(CP_EndGame(0))
			Menu::closeMenus();
	}
	else
	{
		MenuFadeOut();

		StartCPMusic(gameinfo.ScoresMusic);
	
		DrawHighScores();
		VW_UpdateScreen();
		MenuFadeIn();
	
		IN_Ack();
	
		StartCPMusic(gameinfo.MenuMusic);
		MenuFadeOut();
		mainMenu.draw();
		MenuFadeIn ();
	}
	return true;
}
MENU_LISTENER(QuitGame)
{
	FString endString = gameinfo.QuitMessages[M_Random()%gameinfo.QuitMessages.Size()];
	if(endString[0] == '$')
		endString = language[endString.Mid(1)];

	if(Confirm(endString))
	{
		VW_UpdateScreen();
		SD_MusicOff();
		SD_StopSound();
		if(!menusAreFaded)
			MenuFadeOut();
		else
			VW_FadeOut();
		Quit(NULL);
	}

	// special case
	if(which != -1)
		mainMenu.draw();
	return false;
}
MENU_LISTENER(SetSoundEffects)
{
	SDMode modes[3] = { sdm_Off, sdm_PC, sdm_AdLib };
	if(SoundMode != modes[which])
	{
		SD_WaitSoundDone();
		SD_SetSoundMode(modes[which]);
	}
	return true;
}
MENU_LISTENER(SetDigitalSound)
{
	if(DigiMode != (which == 0 ? sds_Off : sds_SoundBlaster))
		SD_SetDigiDevice(which == 0 ? sds_Off : sds_SoundBlaster);
	return true;
}
MENU_LISTENER(SetMusic)
{
	if(MusicMode != (which == 0 ? smm_Off : smm_AdLib))
	{
		SD_SetMusicMode((which == 0 ? smm_Off : smm_AdLib));
		if(which != 0)
			StartCPMusic(gameinfo.MenuMusic);
	}
	return true;
}
MENU_LISTENER(EnterControlBase)
{
	controlBase[2]->setEnabled(mouseenabled);
	controlBase[3]->setEnabled(mouseenabled);
	controlBase[4]->setEnabled(mouseenabled);
	controlBase[5]->setEnabled(IN_JoyPresent());
	controlBase.draw();
	return true;
}
MENU_LISTENER(SetEpisodeAndSwitchToSkill)
{
	EpisodeInfo &ep = EpisodeInfo::GetEpisode(which);

	if(Wads.CheckNumForName(ep.StartMap) == -1)
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

	if(ingame)
	{
		if(!Confirm(language["CURGAME"]))
		{
			episodes.draw();
			return false;
		}
	}

	episode = &ep;
	return true;
}
MENU_LISTENER(StartNewGame)
{
	if(episode == NULL)
		episode = &EpisodeInfo::GetEpisode(0);

	Menu::closeMenus();
	NewGame(which, episode->StartMap);

	//
	// CHANGE "READ THIS!" TO NORMAL COLOR
	//
	readThis->setHighlighted(false);

	return true;
}
MENU_LISTENER(ReadThis)
{
	MenuFadeOut();
	StartCPMusic(gameinfo.FinaleMusic);
	HelpScreens();
	StartCPMusic(gameinfo.MenuMusic);
	mainMenu.draw();
	MenuFadeIn();
	return true;
}
MENU_LISTENER(ToggleFullscreen)
{
	fullscreen = vid_fullscreen;
	VL_SetVGAPlaneMode();
	displayMenu.draw();
	return true;
}
MENU_LISTENER(SetAspectRatio)
{
	vid_aspect = static_cast<Aspect>(which);
	r_ratio = static_cast<Aspect>(CheckRatio(screenWidth, screenHeight));
	NewViewSize(viewsize);
	displayMenu.draw();
	return true;
}

// Dummy screen sizes to pass when windowed
static struct MiniModeInfo
{
	WORD Width, Height;
} WinModes[] =
{
	{ 320, 200 },
	{ 320, 240 },
	{ 400, 225 },	// 16:9
	{ 400, 300 },
	{ 480, 270 },	// 16:9
	{ 480, 360 },
	{ 512, 288 },	// 16:9
	{ 512, 384 },
	{ 640, 360 },	// 16:9
	{ 640, 400 },
	{ 640, 480 },
	{ 720, 480 },	// 16:10
	{ 720, 540 },
	{ 800, 450 },	// 16:9
	{ 800, 500 },	// 16:10
	{ 800, 600 },
	{ 848, 480 },	// 16:9
	{ 960, 600 },	// 16:10
	{ 960, 720 },
	{ 1024, 576 },	// 16:9
	{ 1024, 600 },	// 17:10
	{ 1024, 640 },	// 16:10
	{ 1024, 768 },
	{ 1088, 612 },	// 16:9
	{ 1152, 648 },	// 16:9
	{ 1152, 720 },	// 16:10
	{ 1152, 864 },
	{ 1280, 720 },	// 16:9
	{ 1280, 800 },	// 16:10
	{ 1280, 960 },
	{ 1280, 1024 },	// 5:4
	{ 1360, 768 },	// 16:9
	{ 1400, 787 },	// 16:9
	{ 1400, 875 },	// 16:10
	{ 1400, 1050 },
	{ 1600, 900 },	// 16:9
	{ 1600, 1000 },	// 16:10
	{ 1600, 1200 },
	{ 1920, 1080 },
};
MENU_LISTENER(EnterResolutionSelection);
MENU_LISTENER(SetResolution)
{
	MenuFadeOut();

	if(!fullscreen)
	{
		screenWidth = WinModes[which].Width;
		screenHeight = WinModes[which].Height;
	}
	else
	{
		SDL_Rect **modes = SDL_ListModes (NULL, SDL_FULLSCREEN|SDL_HWSURFACE);
		screenWidth = modes[which]->w;
		screenHeight = modes[which]->h;
	}

	r_ratio = static_cast<Aspect>(CheckRatio(screenWidth, screenHeight));
	VL_SetVGAPlaneMode();
	EnterResolutionSelection(which);
	resolutionMenu.draw();
	MenuFadeIn();
	return true;
}
MENU_LISTENER(EnterResolutionSelection)
{
	int selected = 0;
	resolutionMenu.clear();
	FString resolution;

	if(!fullscreen)
	{
		for(unsigned int i = 0;i < countof(WinModes);++i)
		{
			resolution.Format("%dx%d", WinModes[i].Width, WinModes[i].Height);
			MenuItem *item = new MenuItem(resolution, SetResolution);
			resolutionMenu.addItem(item);
			if(WinModes[i].Width == screenWidth && WinModes[i].Height == screenHeight)
			{
				selected = resolutionMenu.countItems()-1;
				item->setHighlighted(true);
			}
		}
	}
	else
	{
		SDL_Rect **modes = SDL_ListModes (NULL, SDL_FULLSCREEN|SDL_HWSURFACE);
		if(modes == NULL)
			return false;

		while(*modes)
		{
			resolution.Format("%dx%d", (*modes)->w, (*modes)->h);
			MenuItem *item = new MenuItem(resolution, SetResolution);
			resolutionMenu.addItem(item);

			if((*modes)->w == screenWidth && (*modes)->h == screenHeight)
			{
				selected = resolutionMenu.countItems()-1;
				item->setHighlighted(true);
			}

			++modes;
		}
	}

	resolutionMenu.setCurrentPosition(selected);
	return true;
}

void CreateMenus()
{
	// Extract the palette
	BORDCOLOR = ColorMatcher.Pick(RPART(gameinfo.MenuColors[0]), GPART(gameinfo.MenuColors[0]), BPART(gameinfo.MenuColors[0]));
	BORD2COLOR = ColorMatcher.Pick(RPART(gameinfo.MenuColors[1]), GPART(gameinfo.MenuColors[1]), BPART(gameinfo.MenuColors[1]));
	BORD3COLOR = ColorMatcher.Pick(RPART(gameinfo.MenuColors[2]), GPART(gameinfo.MenuColors[2]), BPART(gameinfo.MenuColors[2]));
	BKGDCOLOR = ColorMatcher.Pick(RPART(gameinfo.MenuColors[3]), GPART(gameinfo.MenuColors[3]), BPART(gameinfo.MenuColors[3]));
	STRIPE = ColorMatcher.Pick(RPART(gameinfo.MenuColors[4]), GPART(gameinfo.MenuColors[4]), BPART(gameinfo.MenuColors[4]));
	STRIPEBG = ColorMatcher.Pick(RPART(gameinfo.MenuColors[5]), GPART(gameinfo.MenuColors[5]), BPART(gameinfo.MenuColors[5]));

	// Actually initialize the menus
	GameSave::InitMenus();

	mainMenu.setHeadPicture("M_OPTION");

	if(EpisodeInfo::GetNumEpisodes() > 1)
		mainMenu.addItem(new MenuSwitcherMenuItem(language["STR_NG"], episodes));
	else
		mainMenu.addItem(new MenuSwitcherMenuItem(language["STR_NG"], skills));

	mainMenu.addItem(new MenuSwitcherMenuItem(language["STR_OPTIONS"], optionsMenu));
	mainMenu.addItem(GameSave::GetLoadMenuItem());
	mainMenu.addItem(GameSave::GetSaveMenuItem());
	readThis = new MenuItem(language["STR_RT"], ReadThis);
	readThis->setVisible(gameinfo.DrawReadThis);
	readThis->setHighlighted(true);
	mainMenu.addItem(readThis);
	mainMenu.addItem(new MenuItem(language["STR_VS"], ViewScoresOrEndGame));
	mainMenu.addItem(new MenuItem(language["STR_BD"], PlayDemosOrReturnToGame));
	mainMenu.addItem(new MenuItem(language["STR_QT"], QuitGame));

	episodes.setHeadText(language["STR_WHICHEPISODE"]);
	for(unsigned int i = 0;i < EpisodeInfo::GetNumEpisodes();++i)
	{
		EpisodeInfo &episode = EpisodeInfo::GetEpisode(i);
		MenuItem *tmp = new MenuSwitcherMenuItem(episode.EpisodeName, skills, SetEpisodeAndSwitchToSkill);
		if(!episode.EpisodePicture.IsEmpty())
			tmp->setPicture(episode.EpisodePicture);
		if(Wads.CheckNumForName(episode.StartMap) == -1)
			tmp->setHighlighted(2);
		episodes.addItem(tmp);
	}

	skills.setHeadText(language["STR_HOWTOUGH"]);
	const char* skillText[4] =
	{
		language["STR_DADDY"],
		language["STR_HURTME"],
		language["STR_BRINGEM"],
		language["STR_DEATH"]
	};
	const char* skillPicture[4] =
	{
		"M_BABY",
		"M_EASY",
		"M_NORMAL",
		"M_HARD"
	};
	for(unsigned int i = 0;i < 4;i++)
	{
		MenuItem *tmp = new MenuItem(skillText[i], StartNewGame);
		tmp->setPicture(skillPicture[i], NM_X + 185, NM_Y + 7);
		skills.addItem(tmp);
	}
	skills.setCurrentPosition(2);

	optionsMenu.setHeadPicture("M_OPTION");
	optionsMenu.addItem(new MenuSwitcherMenuItem(language["STR_CL"], controlBase));
	optionsMenu.addItem(new MenuSwitcherMenuItem(language["STR_SD"], soundBase));
	optionsMenu.addItem(new MenuSwitcherMenuItem(language["STR_DISPLAY"], displayMenu));

	// Collect options and defaults
	const char* soundEffectsOptions[] = {language["STR_NONE"], language["STR_PC"], language["STR_ALSB"] };
	soundEffectsOptions[1] = NULL;
	const char* digitizedOptions[] = {language["STR_NONE"], language["STR_SB"] };
	const char* musicOptions[] = { language["STR_NONE"], language["STR_ALSB"] };
	if(!AdLibPresent && !SoundBlasterPresent)
	{
		soundEffectsOptions[2] = NULL;
		musicOptions[1] = NULL;
	}
	if(!SoundBlasterPresent)
		digitizedOptions[1] = NULL;
	int soundEffectsMode = 0;
	int digitizedMode = 0;
	int musicMode = 0;
	switch(SoundMode)
	{
		default: soundEffectsMode = 0; break;
		case sdm_PC: soundEffectsMode = 1; break;
		case sdm_AdLib: soundEffectsMode = 2; break;
	}
	switch(DigiMode)
	{
		default: digitizedMode = 0; break;
		case sds_SoundBlaster: digitizedMode = 1; break;
	}
	switch(MusicMode)
	{
		default: musicMode = 0; break;
		case smm_AdLib: musicMode = 1; break;
	}
	soundBase.setHeadText(language["STR_SOUNDCONFIG"]);
	soundBase.addItem(new LabelMenuItem(language["STR_DIGITALDEVICE"]));
	soundBase.addItem(new MultipleChoiceMenuItem(SetDigitalSound, digitizedOptions, 2, digitizedMode));
	soundBase.addItem(new SliderMenuItem(SoundVolume, 150, MAX_VOLUME, language["STR_SOFT"], language["STR_LOUD"]));
	soundBase.addItem(new LabelMenuItem(language["STR_ADLIBDEVICE"]));
	soundBase.addItem(new MultipleChoiceMenuItem(SetSoundEffects, soundEffectsOptions, 3, soundEffectsMode));
	soundBase.addItem(new SliderMenuItem(AdlibVolume, 150, MAX_VOLUME, language["STR_SOFT"], language["STR_LOUD"]));
	soundBase.addItem(new LabelMenuItem(language["STR_MUSICDEVICE"]));
	soundBase.addItem(new MultipleChoiceMenuItem(SetMusic, musicOptions, 2, musicMode));
	soundBase.addItem(new SliderMenuItem(MusicVolume, 150, MAX_VOLUME, language["STR_SOFT"], language["STR_LOUD"]));

	controlBase.setHeadPicture("M_CONTRL");
	controlBase.addItem(new BooleanMenuItem(language["STR_ALWAYSRUN"], alwaysrun, EnterControlBase));
	controlBase.addItem(new BooleanMenuItem(language["STR_MOUSEEN"], mouseenabled, EnterControlBase));
	controlBase.addItem(new BooleanMenuItem(language["STR_WINDOWEDMOUSE"], forcegrabmouse, EnterControlBase));
	controlBase.addItem(new BooleanMenuItem(language["STR_DISABLEYAXIS"], mouseyaxisdisabled, EnterControlBase));
	controlBase.addItem(new MenuSwitcherMenuItem(language["STR_SENS"], mouseSensitivity));
	controlBase.addItem(new BooleanMenuItem(language["STR_JOYEN"], joystickenabled, EnterControlBase));
	controlBase.addItem(new MenuSwitcherMenuItem(language["STR_CUSTOM"], controls));

	const char* aspectOptions[] = {"Aspect: Auto", "Aspect: 16:9", "Aspect: 16:10", "Aspect: 17:10", "Aspect: 4:3", "Aspect: 5:4"};
	displayMenu.setHeadText(language["STR_DISPLAY"]);
	displayMenu.addItem(new BooleanMenuItem(language["STR_FULLSCREEN"], vid_fullscreen, ToggleFullscreen));
	displayMenu.addItem(new BooleanMenuItem(language["STR_DEPTHFOG"], r_depthfog));
	displayMenu.addItem(new MultipleChoiceMenuItem(SetAspectRatio, aspectOptions, 6, vid_aspect));
	displayMenu.addItem(new MenuSwitcherMenuItem(language["STR_SELECTRES"], resolutionMenu, EnterResolutionSelection));

	resolutionMenu.setHeadText(language["STR_SELECTRES"]);

	mouseSensitivity.addItem(new LabelMenuItem(language["STR_MOUSEADJ"]));
	mouseSensitivity.addItem(new SliderMenuItem(mouseadjustment, 200, 20, language["STR_SLOW"], language["STR_FAST"]));

	controls.setHeadPicture("M_CUSTOM");
	controls.showControlHeaders(true);
	for(int i = 0;controlScheme[i].button != bt_nobutton;i++)
	{
		controls.addItem(new ControlMenuItem(controlScheme[i]));
	}
}

static int SoundStatus = 1;
static int pickquick;
static char SaveName[13] = "savegam?.";

////////////////////////////////////////////////////////////////////
//
// Wolfenstein Control Panel!  Ta Da!
//
////////////////////////////////////////////////////////////////////
void US_ControlPanel (ScanCode scancode)
{
	int which;
	bool idEasterEgg = Wads.CheckNumForName("IDGUYPAL") != -1;

	if (ingame)
	{
		if (CP_CheckQuick (scancode))
			return;
		lastgamemusicoffset = StartCPMusic (gameinfo.MenuMusic);
	}
	else
		StartCPMusic (gameinfo.MenuMusic);
	SetupControlPanel ();

	//
	// F-KEYS FROM WITHIN GAME
	//
	Menu::closeMenus(false);
	switch (scancode)
	{
		case sc_F1:
			HelpScreens ();
			goto finishup;

		case sc_F2:
			GameSave::GetSaveMenu().show();
			goto finishup;

		case sc_F3:
			GameSave::GetLoadMenu().show();
			goto finishup;

		case sc_F4:
			soundBase.show();
			goto finishup;

		case sc_F6:
			controlBase.show ();
			goto finishup;

		finishup:
			CleanupControlPanel ();
			return;

		default:
			break;
	}

	if(ingame)
	{
		mainMenu[mainMenu.countItems()-3]->setText(language["STR_EG"]);
		mainMenu[mainMenu.countItems()-2]->setText(language["STR_BG"]);
		mainMenu[mainMenu.countItems()-2]->setHighlighted(true);
		mainMenu[3]->setEnabled(true);
	}
	else
	{
		mainMenu[mainMenu.countItems()-3]->setText(language["STR_VS"]);
		mainMenu[mainMenu.countItems()-2]->setText(language["STR_BD"]);
		mainMenu[mainMenu.countItems()-2]->setHighlighted(false);
		mainMenu[3]->setEnabled(false);
	}
	mainMenu.draw();
	MenuFadeIn ();
	Menu::closeMenus(false);

	//
	// MAIN MENU LOOP
	//
	do
	{
		which = mainMenu.handle();

		if(idEasterEgg)
		{
			IN_ProcessEvents();
	
			//
			// EASTER EGG FOR SPEAR OF DESTINY!
			//
			if (Keyboard[sc_I] && Keyboard[sc_D])
			{
				MenuFadeOut ();
				StartCPMusic ("XJAZNAZI");
				ClearMemory ();
				InitPalette("IDGUYPAL");

				CA_CacheScreen(TexMan("IDGUYS"));

				VW_UpdateScreen ();

				VW_FadeIn();

				while (Keyboard[sc_I] || Keyboard[sc_D])
					IN_WaitAndProcessEvents();
				IN_ClearKeysDown ();
				IN_Ack ();
	
				VW_FadeOut ();
				InitPalette(gameinfo.GamePalette);

				mainMenu.draw();
				StartCPMusic (gameinfo.MenuMusic);
				MenuFadeIn ();
			}
		}

		switch (which)
		{
			case -1:
				if(!ingame)
					QuitGame(0);
				else
					PlayDemosOrReturnToGame(0);
				break;
			default:
				break;
		}

		//
		// "EXIT OPTIONS" OR "NEW GAME" EXITS
		//
	}
	while (!Menu::areMenusClosed());

	//
	// DEALLOCATE EVERYTHING
	//
	CleanupControlPanel ();

	// RETURN/START GAME EXECUTION
}

////////////////////////////////////////////////////////////////////
//
// CHECK QUICK-KEYS & QUIT (WHILE IN A GAME)
//
////////////////////////////////////////////////////////////////////
int CP_CheckQuick (ScanCode scancode)
{
	switch (scancode)
	{
		//
		// END GAME
		//
		case sc_F7:
			WindowH = 160;
			if (Confirm (language["ENDGAMESTR"]))
			{
				playstate = ex_died;
				pickquick = players[0].lives = 0;
				players[0].killerobj = NULL;
			}

			WindowH = 200;
			return 1;

		//
		// QUICKSAVE
		//
		case sc_F8:
			GameSave::QuickSave();
			return 1;

		//
		// QUICKLOAD
		//
		case sc_F9:
			GameSave::QuickLoad();
			return 1;

		//
		// QUIT
		//
		case sc_F10:
			WindowX = WindowY = 0;
			WindowW = 320;
			WindowH = 160;
			QuitGame(-1);

			DrawPlayScreen ();
			WindowH = 200;
			return 1;
	}

	return 0;
}


////////////////////////////////////////////////////////////////////
//
// END THE CURRENT GAME
//
////////////////////////////////////////////////////////////////////
int CP_EndGame (int)
{
	int res;
	res = Confirm (language["ENDGAMESTR"]);
	mainMenu.draw();
	if(!res) return 0;

	pickquick = players[0].lives = 0;
	playstate = ex_died;
	players[0].killerobj = NULL;

	return 1;
}

////////////////////////////////////////////////////////////////////
//
// HANDLE INTRO SCREEN (SYSTEM CONFIG)
//
////////////////////////////////////////////////////////////////////
static void IntroFill (int color, double x, double y, double w, double h)
{
	VirtualToRealCoords(x, y, w, h, 320, 200, false, true);
	VWB_Clear (color, x, y, x+w, y+h);
}
static void MemeoryFill (int color, int basex, int basey)
{
	// To make color picking simple, pick the darkest color and then fill the
	// value component of the color.  This isn't exactly equivalent, but it's
	// good enough I think.

	float r = RPART(color), g = GPART(color), b = BPART(color);
	float h, s, v;

	RGBtoHSV(r, g, b, &h, &s, &v);
	float vstep = (255.0f - v)/10.0f;

	for (int i = 0; i < 10; ++i)
	{
		HSVtoRGB(&r, &g, &b, h, s, v);
		int pal = ColorMatcher.Pick(r, g, b);
		IntroFill(pal, basex, basey - 8 * i, 6, 5);
		v += vstep;
	}
}
void IntroScreen (void)
{
	const int FILLCOLOR =
		ColorMatcher.Pick(RPART(gameinfo.SignonColors[0]), GPART(gameinfo.SignonColors[0]), BPART(gameinfo.SignonColors[0]));

	//
	// DRAW MAIN MEMORY
	//
	MemeoryFill(gameinfo.SignonColors[1], 49, 163);
	MemeoryFill(gameinfo.SignonColors[2], 89, 163);
	MemeoryFill(gameinfo.SignonColors[3], 129, 163);


	//
	// FILL BOXES
	//
	if (MousePresent)
		IntroFill(FILLCOLOR, 164, 82, 12, 2);

	if (IN_JoyPresent())
		IntroFill(FILLCOLOR, 164, 105, 12, 2);

	if (AdLibPresent && !SoundBlasterPresent)
		IntroFill(FILLCOLOR, 164, 128, 12, 2);

	if (SoundBlasterPresent)
		IntroFill(FILLCOLOR, 164, 151, 12, 2);

//    if (SoundSourcePresent)
//        VWB_Bar (164, 174, 12, 2, FILLCOLOR);
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
	unsigned int wx = x, wy = y, ww = w, wh = h;
	MenuToRealCoords(wx, wy, ww, wh, MENU_CENTER);

	VWB_Clear (wcolor, wx, wy, wx+ww, wy+wh);
	DrawOutline (x, y, w, h, color1, color2);
}

void DrawOutline (int x, int y, int w, int h, int color1, int color2)
{
	MenuToRealCoords(x, y, w, h, MENU_CENTER);

	VWB_Clear(color2, x-scaleFactor, y, x+w+scaleFactor, y+scaleFactor);
	VWB_Clear(color2, x-scaleFactor, y, x, y+h);
	VWB_Clear(color1, x-scaleFactor, y+h, x+w+scaleFactor, y+h+scaleFactor);
	VWB_Clear(color1, x+w, y, x+w+scaleFactor, y+h);
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
// DELAY FOR AN AMOUNT OF TICS OR UNTIL CONTROLS ARE INACTIVE
//
////////////////////////////////////////////////////////////////////
void TicDelay (int count)
{
	ControlInfo ci;

	int32_t startTime = GetTimeCount ();
	do
	{
		SDL_Delay(5);
		ReadAnyControl (&ci);
	}
	while ((int32_t) GetTimeCount () - startTime < count && ci.dir != dir_None);
}

////////////////////////////////////////////////////////////////////
//
// WAIT FOR CTRLKEY-UP OR BUTTON-UP
//
////////////////////////////////////////////////////////////////////
void WaitKeyUp (void)
{
	ControlInfo ci;
	while (ReadAnyControl (&ci), ci.button0 |
		ci.button1 |
		ci.button2 | ci.button3 | Keyboard[sc_Space] | Keyboard[sc_Enter] | Keyboard[sc_Escape])
	{
		IN_WaitAndProcessEvents();
	}
}


////////////////////////////////////////////////////////////////////
//
// READ KEYBOARD, JOYSTICK AND MOUSE FOR INPUT
//
////////////////////////////////////////////////////////////////////
void ReadAnyControl (ControlInfo * ci)
{
	int mouseactive = 0;

	IN_ReadControl (0, ci);

	if (mouseenabled && IN_IsInputGrabbed())
	{
		int mousex, mousey, buttons;
		buttons = SDL_GetMouseState(&mousex, &mousey);
		int middlePressed = buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE);
		int rightPressed = buttons & SDL_BUTTON(SDL_BUTTON_RIGHT);
		buttons &= ~(SDL_BUTTON(SDL_BUTTON_MIDDLE) | SDL_BUTTON(SDL_BUTTON_RIGHT));
		if(middlePressed) buttons |= 1 << 2;
		if(rightPressed) buttons |= 1 << 1;

		if(mousey - CENTERY < -SENSITIVE)
		{
			ci->dir = dir_North;
			mouseactive = 1;
		}
		else if(mousey - CENTERY > SENSITIVE)
		{
			ci->dir = dir_South;
			mouseactive = 1;
		}

		if(mousex - CENTERX < -SENSITIVE)
		{
			ci->dir = dir_West;
			mouseactive = 1;
		}
		else if(mousex - CENTERX > SENSITIVE)
		{
			ci->dir = dir_East;
			mouseactive = 1;
		}

		if(mouseactive)
			IN_CenterMouse();

		if (buttons)
		{
			ci->button0 = buttons & 1;
			ci->button1 = buttons & 2;
			ci->button2 = buttons & 4;
			ci->button3 = false;
			mouseactive = 1;
		}
	}

	if (joystickenabled && !mouseactive)
	{
		int jx, jy, jb;

		IN_GetJoyDelta (&jx, &jy);
		if (jy < -SENSITIVE)
			ci->dir = dir_North;
		else if (jy > SENSITIVE)
			ci->dir = dir_South;

		if (jx < -SENSITIVE)
			ci->dir = dir_West;
		else if (jx > SENSITIVE)
			ci->dir = dir_East;

		jb = IN_JoyButtons ();
		if (jb)
		{
			ci->button0 = jb & 1;
			ci->button1 = jb & 2;
			ci->button2 = jb & 4;
			ci->button3 = jb & 8;
		}
	}
}


////////////////////////////////////////////////////////////////////
//
// DRAW DIALOG AND CONFIRM YES OR NO TO QUESTION
//
////////////////////////////////////////////////////////////////////
int Confirm (const char *string)
{
	int xit = 0, x, y, tick = 0, lastBlinkTime;
	const char* whichsnd[2] = { "menu/escape", "menu/activate" };
	ControlInfo ci;

	Message (string);
	IN_ClearKeysDown ();
	WaitKeyUp ();

	//
	// BLINK CURSOR
	//
	x = PrintX;
	y = PrintY;
	lastBlinkTime = GetTimeCount();

	do
	{
		ReadAnyControl(&ci);

		if (GetTimeCount() - lastBlinkTime >= 10)
		{
			switch (tick)
			{
				case 0:
				{
					double dx = x;
					double dy = y;
					double dw = 8;
					double dh = 13;
					MenuToRealCoords(dx, dy, dw, dh, MENU_CENTER);
					VWB_Clear(TEXTCOLOR, dx, dy, dx+dw, dy+dh);
					break;
				}
				case 1:
					PrintX = x;
					PrintY = y;
					US_Print (BigFont, "_");
			}
			VW_UpdateScreen ();
			tick ^= 1;
			lastBlinkTime = GetTimeCount();
		}
		else SDL_Delay(5);

	}
	while (!Keyboard[sc_Y] && !Keyboard[sc_S] && !Keyboard[sc_N] && !Keyboard[sc_Escape] && !ci.button0 && !ci.button1);

	if (Keyboard[sc_S] || Keyboard[sc_Y] || ci.button0)
	{
		xit = 1;
		ShootSnd ();
	}

	IN_ClearKeysDown ();
	WaitKeyUp ();

	SD_PlaySound (whichsnd[xit]);

	return xit;
}

////////////////////////////////////////////////////////////////////
//
// PRINT A MESSAGE IN A WINDOW
//
////////////////////////////////////////////////////////////////////
void Message (const char *string)
{
	int i, len = (int) strlen(string);
	FFont *font = BigFont;
	word width, height;

	FString measureString;
	measureString.Format("%s_", string);
	VW_MeasurePropString(BigFont, measureString, width, height);
	width = MIN<int>(width, 320 - 10);
	height = MIN<int>(height, 200 - 10);

	PrintY = (WindowH / 2) - height / 2;
	PrintX = WindowX = 160 - width / 2;

	DrawWindow (WindowX - 5, PrintY - 5, width + 10, height + 10, TEXTCOLOR);
	DrawOutline (WindowX - 5, PrintY - 5, width + 10, height + 10, 0, HIGHLIGHT);
	US_Print (BigFont, string, CR_UNTRANSLATED);
	VW_UpdateScreen ();
}

////////////////////////////////////////////////////////////////////
//
// THIS MAY BE FIXED A LITTLE LATER...
//
////////////////////////////////////////////////////////////////////
static int lastmusic;

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
	if (Paused)
	{
		switch (SoundStatus)
		{
			case 0:
				SD_MusicOn ();
				break;
			case 1:
				SD_MusicOff ();
				break;
		}

		SoundStatus ^= 1;
		VW_WaitVBL (3);
		IN_ClearKeysDown ();
		Paused = false;
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
	if(calcStripes != scaleFactor)
	{
		unsigned int dummyx = 0, dummyw = 320;
		sy = y;
		sh = 24;
		ly = y+22;
		lh = 1;
		calcStripes = scaleFactor;

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

	VL_FadeIn(0, 255, gamepal, 10);
}

void ShowMenu(Menu &menu)
{
	VW_FadeOut ();
	if(screenHeight % 200 != 0)
		VL_ClearScreen(0);

	lastgamemusicoffset = StartCPMusic (gameinfo.MenuMusic);
	Menu::closeMenus(false);
	menu.show();

	IN_ClearKeysDown ();
	VW_FadeOut();
	if(viewsize != 21)
		DrawPlayScreen ();

	if (!startgame && !loadedgame)
		ContinueMusic (lastgamemusicoffset);

	if (loadedgame)
		playstate = ex_abort;

	lasttimecount = GetTimeCount ();

	if (MousePresent && IN_IsInputGrabbed())
		IN_CenterMouse();     // Clear accumulated mouse movement
}
