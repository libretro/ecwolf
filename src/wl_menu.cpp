////////////////////////////////////////////////////////////////////
//
// WL_MENU.C
// by John Romero (C) 1992 Id Software, Inc.
//
////////////////////////////////////////////////////////////////////

#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
	#include <io.h>
#else
	#include <unistd.h>
#endif

#define LSA_X	96
#define LSA_Y	80
#define LSA_W	130
#define LSA_H	42

#include "file.h"
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

struct SaveFile
{
	public:
		static TArray<SaveFile>	files;

		char	name[32]; // Displayed on the menu.
		char	filename[15];
};
TArray<SaveFile> SaveFile::files;

extern int	lastgamemusicoffset;
EpisodeInfo	*episode = 0;
bool		quickSaveLoad = false;
int BORDCOLOR, BORD2COLOR, BORD3COLOR, BKGDCOLOR, STRIPE;
static MenuItem	*readThis;

MENU_LISTENER(EnterControlBase);
MENU_LISTENER(EnterLoadMenu);
MENU_LISTENER(EnterSaveMenu);

Menu loadGame(LSM_X, LSM_Y, LSM_W, 24, EnterLoadMenu);
Menu mainMenu(MENU_X, MENU_Y, MENU_W, 24);
Menu optionsMenu(80, 85, 180, 28);
Menu saveGame(LSM_X, LSM_Y, LSM_W, 24, EnterSaveMenu);
Menu soundBase(24, 45, 284, 24);
Menu controlBase(CTL_X, CTL_Y, CTL_W, 56, EnterControlBase);
Menu displayMenu(60, 95, 225, 56);
Menu mouseSensitivity(20, 80, 300, 0);
Menu episodes(NE_X+4, NE_Y-1, NE_W+7, 83);
Menu skills(NM_X, NM_Y, NM_W, 24);
Menu controls(15, 70, 310, 24);

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
const char* endStrings[9] = {
#ifndef SPEAR
	language["ENDSTR10"],
	language["ENDSTR11"],
	language["ENDSTR12"],
	language["ENDSTR13"],
	language["ENDSTR14"],
	language["ENDSTR15"],
	language["ENDSTR16"],
	language["ENDSTR17"],
	language["ENDSTR18"]
#else
	language["ENDSTR01"],
	language["ENDSTR02"],
	language["ENDSTR03"],
	language["ENDSTR04"],
	language["ENDSTR05"],
	language["ENDSTR06"],
	language["ENDSTR07"],
	language["ENDSTR08"],
	language["ENDSTR09"]
#endif
};

	if(Confirm(endStrings[(M_Random() & 0x7) + (M_Random() & 1)]))
	{
		VW_UpdateScreen();
		SD_MusicOff();
		SD_StopSound();
		MenuFadeOut();
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
	controlBase[4]->setEnabled(IN_JoyPresent());
	controlBase.draw();
	return true;
}
MENU_LISTENER(BeginEditSave)
{
	bool ret = Confirm(language["GAMESVD"]);
	saveGame.draw();
	return ret;
}
MENU_LISTENER(PerformSaveGame)
{
	SaveFile file;

	// Copy the name
	strcpy(file.name, static_cast<TextInputMenuItem *> (saveGame[which])->getValue());
	if(which == 0) // New
	{
		// Locate a available filename.  I don't want to assume savegamX.yza so this
		// might not be the fastest way to do things.
		bool nextSaveNumber = false;
		for(unsigned int i = 0;i < 10000;i++)
		{
			sprintf(file.filename, "savegam%u.ecs", i);
			for(unsigned int j = 0;j < SaveFile::files.Size();j++)
			{
				if(stricmp(file.filename, SaveFile::files[j].filename) == 0)
				{
					nextSaveNumber = true;
					continue;
				}
			}
			if(nextSaveNumber)
			{
				nextSaveNumber = false;
				continue;
			}
			break;
		}

		SaveFile::files.Push(file);

		saveGame[0]->setHighlighted(false);
		saveGame.setCurrentPosition(1);
		loadGame.setCurrentPosition(0);

		mainMenu[2]->setEnabled(true);
	}
	else
	{
		strcpy(file.filename, SaveFile::files[which-1].filename);
		SaveFile::files[which-1] = file;
		loadGame.setCurrentPosition(which-1);
	}

	FILE *fileh = fopen(file.filename, "wb");
	fwrite(file.name, 32, 1, fileh);
	fseek(fileh, 32, SEEK_SET);
	if(!quickSaveLoad)
	{
		DrawLSAction(1);
		SaveTheGame(fileh, LSA_X + 8, LSA_Y + 5);
	}
	else
	{
		Message (language["STR_SAVING"]);
		SaveTheGame(fileh, 0, 0);
	}
	fclose(fileh);
	if(!quickSaveLoad)
		Menu::closeMenus(true);

	return true;
}
MENU_LISTENER(LoadSaveGame)
{
	FILE *file = fopen(SaveFile::files[which].filename, "rb");
	fseek(file, 32, SEEK_SET);
	if(!quickSaveLoad)
		DrawLSAction(0);
	loadedgame = true;
	if(!quickSaveLoad)
		LoadTheGame(file, LSA_X + 8, LSA_Y + 5);
	else
		LoadTheGame(file, 0, 0);
	fclose(file);
	ShootSnd();
	if(!quickSaveLoad)
		Menu::closeMenus(true);

	saveGame.setCurrentPosition(which+1);
	return false;
}
MENU_LISTENER(EnterLoadMenu)
{
	if(saveGame.getNumItems() == 0)
		EnterSaveMenu(which); // This is needed so that there are no crashes on quick save/load

	loadGame.clear();

	for(unsigned int i = 0;i < SaveFile::files.Size();i++)
		loadGame.addItem(new TextInputMenuItem(SaveFile::files[i].name, 31, LoadSaveGame));

	return true;
}
MENU_LISTENER(EnterSaveMenu)
{
	// Create the menu now
	saveGame.clear();

	MenuItem *newSave = new TextInputMenuItem("    - NEW SAVE -", 31, NULL, PerformSaveGame, true);
	newSave->setHighlighted(true);
	saveGame.addItem(newSave);

	for(unsigned int i = 0;i < SaveFile::files.Size();i++)
		saveGame.addItem(new TextInputMenuItem(SaveFile::files[i].name, 31, BeginEditSave, PerformSaveGame));

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
	fullscreen = ~fullscreen;
	VL_SetVGAPlaneMode();
	displayMenu.draw();
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

	// Actually initialize the menus
	mainMenu.setHeadPicture("M_OPTION");

	if(EpisodeInfo::GetNumEpisodes() > 1)
		mainMenu.addItem(new MenuSwitcherMenuItem(language["STR_NG"], episodes));
	else
		mainMenu.addItem(new MenuSwitcherMenuItem(language["STR_NG"], skills));

	mainMenu.addItem(new MenuSwitcherMenuItem(language["STR_OPTIONS"], optionsMenu));
	MenuItem *lg = new MenuSwitcherMenuItem(language["STR_LG"], loadGame);
	lg->setEnabled(SaveFile::files.Size() > 0);
	mainMenu.addItem(lg);
	MenuItem *sg = new MenuSwitcherMenuItem(language["STR_SG"], saveGame);
	sg->setEnabled(false);
	mainMenu.addItem(sg);
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

	displayMenu.setHeadText(language["STR_DISPLAY"]);
	displayMenu.addItem(new BooleanMenuItem(language["STR_FULLSCREEN"], vid_fullscreen, ToggleFullscreen));
	displayMenu.addItem(new BooleanMenuItem(language["STR_DEPTHFOG"], r_depthfog));

	loadGame.setHeadPicture("M_LOADGM");
	saveGame.setHeadPicture("M_SAVEGM");

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
void
US_ControlPanel (ScanCode scancode)
{
	int which;
	bool idEasterEgg = Wads.CheckNumForName("IDGUYS1", ns_graphics) != -1;

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
			saveGame.show();
			goto finishup;

		case sc_F3:
			loadGame.show();
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

/*		if(idEasterEgg)
		{
			IN_ProcessEvents();
	
			//
			// EASTER EGG FOR SPEAR OF DESTINY!
			//
			if (Keyboard[sc_I] && Keyboard[sc_D])
			{
				VW_FadeOut ();
				StartCPMusic (XJAZNAZI_MUS);
				ClearMemory ();
	
	
				VWB_DrawPic (0, 0, "IDGUYS1");
				VWB_DrawPic (0, 80, "IDGUYS2");
	
				VW_UpdateScreen ();
	
				SDL_Color pal[256];
				VL_ConvertPalette("IDGUYPAL", pal, 256);
				VL_FadeIn (0, 255, pal, 30);
	
				while (Keyboard[sc_I] || Keyboard[sc_D])
					IN_WaitAndProcessEvents();
				IN_ClearKeysDown ();
				IN_Ack ();
	
				VW_FadeOut ();
	
				mainMenu.draw();
				StartCPMusic (gameinfo.MenuMusic);
				MenuFadeIn ();
			}
		}*/

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
			if(saveGame.getCurrentPosition() != 0)
			{
				quickSaveLoad = true;
				PerformSaveGame(saveGame.getCurrentPosition());
				quickSaveLoad = false;
			}
			else
			{
				VW_FadeOut ();
				if(screenHeight % 200 != 0)
					VL_ClearScreen(0);

				lastgamemusicoffset = StartCPMusic (gameinfo.MenuMusic);
				Menu::closeMenus(false);
				saveGame.show();

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
			return 1;

		//
		// QUICKLOAD
		//
		case sc_F9:
			if(saveGame.getCurrentPosition() != 0)
			{
				quickSaveLoad = true;
				char string[100];
				sprintf(string, "%s%s\"?", language["STR_LGC"], SaveFile::files[saveGame.getCurrentPosition()-1].name);
				if(Confirm(string))
					LoadSaveGame(saveGame.getCurrentPosition()-1);
				quickSaveLoad = false;
			}
			else
			{
				VW_FadeOut ();
				if(screenHeight % 200 != 0)
					VL_ClearScreen(0);

				lastgamemusicoffset = StartCPMusic (gameinfo.MenuMusic);
				Menu::closeMenus(false);
				loadGame.show();

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
			return 1;

		//
		// QUIT
		//
		case sc_F10:
			WindowX = WindowY = 0;
			WindowW = 320;
			WindowH = 160;
			QuitGame(-1);

			DrawPlayBorder ();
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

//
// DRAW LOAD/SAVE IN PROGRESS
//
void
DrawLSAction (int which)
{
	DrawWindow (LSA_X, LSA_Y, LSA_W, LSA_H, TEXTCOLOR);
	DrawOutline (LSA_X, LSA_Y, LSA_W, LSA_H, 0, HIGHLIGHT);
	VWB_DrawGraphic (TexMan("M_LDING1"), LSA_X + 8, LSA_Y + 5, MENU_CENTER);

	PrintX = LSA_X + 46;
	PrintY = LSA_Y + 13;

	if (!which)
		US_Print (BigFont, language["STR_LOADING"]);
	else
		US_Print (BigFont, language["STR_SAVING"]);

	VW_UpdateScreen ();
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
	if(Wads.CheckNumForName("BACKDROP", ns_graphics) == -1)
		VWB_Clear (BORDCOLOR, 0, 0, screenWidth, screenHeight);
	else
		CA_CacheScreen ("BACKDROP");
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
// SEE WHICH SAVE GAME FILES ARE AVAILABLE & READ STRING IN
//
////////////////////////////////////////////////////////////////////
void SetupSaveGames()
{
	char name[13];

	File saveDirectory("./");
	const TArray<FString> &files = saveDirectory.getFileList();
	for(unsigned int i = 0;i < files.Size();i++)
	{
		const FString &filename = files[i];
		if(filename.Len() <= 11 ||
			filename.Len() >= 15 ||
			filename.Mid(0, 7).Compare("savegam") != 0 ||
			filename.Mid(filename.Len()-3, 3).Compare("ecs") != 0)
			continue; // Too short or incorrect name

		const int handle = open(filename, O_RDONLY | O_BINARY);
		if(handle >= 0)
		{
			SaveFile sFile;
			read(handle, sFile.name, 32);
			close(handle);
			strcpy(sFile.filename, filename);
			SaveFile::files.Push(sFile);
		}
	}
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
	static bool calcStripes = true;
	static unsigned int sy = y, sh = 24;
	static unsigned int ly = y+22, lh = 1;
	if(calcStripes)
	{
		unsigned int dummyx = 0, dummyw = 320;
		calcStripes = false;

		MenuToRealCoords(dummyx, sy, dummyw, sh, MENU_TOP);
		MenuToRealCoords(dummyx, ly, dummyw, lh, MENU_TOP);
	}

	VWB_Clear(0, 0, sy, screenWidth, sy+sh);
	VWB_Clear(STRIPE, 0, ly, screenWidth, ly+lh);
}

void ShootSnd (void)
{
	SD_PlaySound ("menu/activate");
}

static bool menusAreFaded = true; 
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
