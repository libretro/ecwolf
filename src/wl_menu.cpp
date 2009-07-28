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
#include <deque>
#include <vector>

#define LSA_X	96
#define LSA_Y	80
#define LSA_W	130
#define LSA_H	42

#include "file.h"
#include "m_classes.h"
#include "wl_def.h"
#include "wl_menu.h"
#include "id_sd.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
using namespace std;

struct SaveFile
{
	public:
		static deque<SaveFile>	files;

		char	name[32]; // Displayed on the menu.
		char	filename[15];
};
deque<SaveFile> SaveFile::files;

extern int	lastgamemusicoffset;
extern int	numEpisodesMissing;
int			episode = 0;
bool		quickSaveLoad = false;

MENU_LISTENER(EnterControlBase);
MENU_LISTENER(EnterLoadMenu);
MENU_LISTENER(EnterSaveMenu);

Menu loadGame(LSM_X, LSM_Y, LSM_W, 24, EnterLoadMenu);
Menu mainMenu(MENU_X, MENU_Y, MENU_W, 24);
Menu saveGame(LSM_X, LSM_Y, LSM_W, 24, EnterSaveMenu);
Menu soundBase(24, 45, 284, 24);
Menu controlBase(CTL_X, CTL_Y, CTL_W, 56, EnterControlBase);
Menu mouseSensitivity(10, 80, 300, 0);
Menu episodes(NE_X+4, NE_Y-1, NE_W+7, 83);
Menu skills(NM_X, NM_Y, NM_W, 24);
Menu controls(15, 75, 310, 24);

MENU_LISTENER(PlayDemosOrReturnToGame)
{
	Menu::closeMenus();
	if (!ingame)
		StartCPMusic(INTROSONG);
	VL_FadeOut(0, 255, 0, 0, 0, 10);
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
	
		fontnumber = 0;
	
#ifdef SPEAR
		StartCPMusic(XAWARD_MUS);
#else
		StartCPMusic(ROSTER_MUS);
#endif
	
		DrawHighScores();
		VW_UpdateScreen();
		MenuFadeIn();
		fontnumber = 1;
	
		IN_Ack();
	
		StartCPMusic(MENUSONG);
		MenuFadeOut();
		mainMenu.draw();
		MenuFadeIn ();
	}
	return true;
}
MENU_LISTENER(QuitGame)
{
#ifdef JAPAN
	if(GetYorN(7, 11, C_QUITMSGPIC))
#else

const char endStrings[9][80] = {
#ifndef SPEAR
    {"Dost thou wish to\nleave with such hasty\nabandon?"},
    {"Chickening out...\nalready?"},
    {"Press N for more carnage.\nPress Y to be a weenie."},
    {"So, you think you can\nquit this easily, huh?"},
    {"Press N to save the world.\nPress Y to abandon it in\nits hour of need."},
    {"Press N if you are brave.\nPress Y to cower in shame."},
    {"Heroes, press N.\nWimps, press Y."},
    {"You are at an intersection.\nA sign says, 'Press Y to quit.'\n>"},
    {"For guns and glory, press N.\nFor work and worry, press Y."}
#else
    ENDSTR1,
    ENDSTR2,
    ENDSTR3,
    ENDSTR4,
    ENDSTR5,
    ENDSTR6,
    ENDSTR7,
    ENDSTR8,
    ENDSTR9
#endif
};

#ifdef SPANISH
	if(Confirm(ENDGAMESTR))
#else
	if(Confirm(endStrings[US_RndT() & 0x7 + (US_RndT() & 1)]))
#endif

#endif
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
			StartCPMusic(MENUSONG);
	}
	return true;
}
MENU_LISTENER(EnterControlBase)
{
	controlBase[2]->setEnabled(mouseenabled);
	controlBase[3]->setEnabled(mouseenabled);
	controlBase[4]->setEnabled(IN_JoyPresent());
	controlBase.draw();
}
MENU_LISTENER(BeginEditSave)
{
	bool ret = Confirm(GAMESVD);
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
			sprintf(file.filename, "savegam%u.%s", i, extension);
			for(unsigned int j = 0;j < SaveFile::files.size();j++)
			{
				if(strcasecmp(file.filename, SaveFile::files[j].filename) == 0)
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

		SaveFile::files.push_front(file);

		saveGame[0]->setHighlighted(false);
		saveGame.setCurrentPosition(1);
		loadGame.setCurrentPosition(0);
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
		fontnumber = 1;
		Message (STR_SAVING "...");
		fontnumber = 0;
		SaveTheGame(fileh, 0, 0);
	}
	fclose(fileh);
#ifdef _arch_dreamcast
	DC_SaveToVMU(file.filename, 2);
#endif
	if(!quickSaveLoad)
		Menu::closeMenus(true);

	return true;
}
MENU_LISTENER(LoadSaveGame)
{
#ifdef _arch_dreamcast
	DC_LoadFromVMU(SaveFile::files[which].filename);
#endif
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

	for(unsigned int i = 0;i < SaveFile::files.size();i++)
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

	for(unsigned int i = 0;i < SaveFile::files.size();i++)
		saveGame.addItem(new TextInputMenuItem(SaveFile::files[i].name, 31, BeginEditSave, PerformSaveGame));

	return true;
}
MENU_LISTENER(SetEpisodeAndSwitchToSkill)
{
	if(which >= 6-numEpisodesMissing)
	{
		SD_PlaySound(NOWAYSND);
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
		if(!Confirm(CURGAME))
		{
			episodes.draw();
			return false;
		}
	}

	episode = which;
	return true;
}
MENU_LISTENER(StartNewGame)
{
	NewGame(which, episode);
	Menu::closeMenus();
	MenuFadeOut();

	//
	// CHANGE "READ THIS!" TO NORMAL COLOR
	//
#ifndef SPEAR
#ifndef GOODTIMES
	mainMenu[mainMenu.countItems()-3]->setHighlighted(false);
#endif
#endif

	return true;
}

void CreateMenus()
{
	mainMenu.setHeadPicture(C_OPTIONSPIC);
#ifndef SPEAR
	mainMenu.addItem(new MenuSwitcherMenuItem(STR_NG, episodes));
#else
	mainMenu.addItem(new MenuSwitcherMenuItem(STR_NG, skills));
#endif
	mainMenu.addItem(new MenuSwitcherMenuItem(STR_SD, soundBase));
	mainMenu.addItem(new MenuSwitcherMenuItem(STR_CL, controlBase));
	MenuItem *lg = new MenuSwitcherMenuItem(STR_LG, loadGame);
	lg->setEnabled(SaveFile::files.size() > 0);
	mainMenu.addItem(lg);
	MenuItem *sg = new MenuSwitcherMenuItem(STR_SG, saveGame);
	sg->setEnabled(false);
	mainMenu.addItem(sg);
	MenuItem *rt = new FunctionMenuItem("Read This!", 0);
#if defined(SPEAR) || defined(GOODTIMES)
	rt->setVisible(false);
#else
	rt->setVisible(true);
#endif
	rt->setHighlighted(true);
	mainMenu.addItem(rt);
	mainMenu.addItem(new MenuItem(STR_VS, ViewScoresOrEndGame));
	mainMenu.addItem(new MenuItem(STR_BD, PlayDemosOrReturnToGame));
	mainMenu.addItem(new MenuItem(STR_QT, QuitGame));

#ifndef SPEAR
	episodes.setHeadText("Which episode to play?");
	const char* episodeText[6] =
	{
		"Episode 1\nEscape from Wolfenstein",
		"Episode 2\nOperation: Eisenfaust",
		"Episode 3\nDie, Fuhrer, Die!",
		"Episode 4\nA Dark Secret",
		"Episode 5\nTrail of the Madman",
		"Episode 6\nConfrontation"
	};
	int episodePicture[6] = { C_EPISODE1PIC, C_EPISODE2PIC, C_EPISODE3PIC, C_EPISODE4PIC, C_EPISODE5PIC, C_EPISODE6PIC };
	for(unsigned int i = 0;i < 6;i++)
	{
		MenuItem *tmp = new MenuSwitcherMenuItem(episodeText[i], skills, SetEpisodeAndSwitchToSkill);
		tmp->setPicture(episodePicture[i]);
		if(i >= 6-numEpisodesMissing)
			tmp->setEnabled(false);
		episodes.addItem(tmp);
	}
#endif

	skills.setHeadText("How tough are you?");
	const char* skillText[4] =
	{
		STR_DADDY,
		STR_HURTME,
		STR_BRINGEM,
		STR_DEATH
	};
	int skillPicture[4] = { C_BABYMODEPIC, C_EASYPIC, C_NORMALPIC, C_HARDPIC };
	for(unsigned int i = 0;i < 4;i++)
	{
		MenuItem *tmp = new MenuItem(skillText[i], StartNewGame);
		tmp->setPicture(skillPicture[i], NM_X + 185, NM_Y + 7);
		skills.addItem(tmp);
	}

	// Collect options and defaults
	const char* soundEffectsOptions[] = {STR_NONE, STR_PC, STR_ALSB };
	soundEffectsOptions[1] = NULL;
	const char* digitizedOptions[] = {STR_NONE, STR_SB };
	const char* musicOptions[] = { STR_NONE, STR_ALSB };
	if(!AdLibPresent && !SoundBlasterPresent)
	{
		soundEffectsOptions[2] = NULL;
		musicOptions[2] = NULL;
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
	soundBase.setHeadText("Sound Configuration");
	soundBase.addItem(new LabelMenuItem("Digital Device & Volume"));
	soundBase.addItem(new MultipleChoiceMenuItem(SetDigitalSound, digitizedOptions, 2, digitizedMode));
	soundBase.addItem(new SliderMenuItem(SoundVolume, 150, MAX_VOLUME, "Soft", "Loud"));
	soundBase.addItem(new LabelMenuItem("Adlib Device & Volume"));
	soundBase.addItem(new MultipleChoiceMenuItem(SetSoundEffects, soundEffectsOptions, 3, soundEffectsMode));
	soundBase.addItem(new SliderMenuItem(AdlibVolume, 150, MAX_VOLUME, "Soft", "Loud"));
	soundBase.addItem(new LabelMenuItem("Music Device & Volume"));
	soundBase.addItem(new MultipleChoiceMenuItem(SetMusic, musicOptions, 2, musicMode));
	soundBase.addItem(new SliderMenuItem(MusicVolume, 150, MAX_VOLUME, "Soft", "Loud"));

	controlBase.setHeadPicture(C_CONTROLPIC);
	controlBase.addItem(new BooleanMenuItem("Always Run", alwaysrun, EnterControlBase));
	controlBase.addItem(new BooleanMenuItem(STR_MOUSEEN, mouseenabled, EnterControlBase));
	controlBase.addItem(new BooleanMenuItem(STR_DISABLEYAXIS, mouseyaxisdisabled, EnterControlBase));
	controlBase.addItem(new MenuSwitcherMenuItem(STR_SENS, mouseSensitivity));
	controlBase.addItem(new BooleanMenuItem(STR_JOYEN, joystickenabled, EnterControlBase));
	controlBase.addItem(new MenuSwitcherMenuItem(STR_CUSTOM, controls));

	loadGame.setHeadPicture(C_LOADGAMEPIC);
	saveGame.setHeadPicture(C_SAVEGAMEPIC);

	mouseSensitivity.addItem(new LabelMenuItem(STR_MOUSEADJ));
	mouseSensitivity.addItem(new SliderMenuItem(mouseadjustment, 200, 10, STR_SLOW, STR_FAST));

	controls.setHeadPicture(C_CUSTOMIZEPIC);
	controls.showControlHeaders(true);
	for(int i = 0;controlScheme[i].button != bt_nobutton;i++)
	{
		controls.addItem(new ControlMenuItem(controlScheme[i]));
	}
}

static const int color_hlite[] = {
    DEACTIVE,
    HIGHLIGHT,
    READHCOLOR,
    0x67
};

static const int color_norml[] = {
    DEACTIVE,
    TEXTCOLOR,
    READCOLOR,
    0x6b
};

//
// PRIVATE PROTOTYPES
//
int CP_ReadThis (int);

#ifdef SPEAR
#define STARTITEM       newgame

#else
#ifdef GOODTIMES
#define STARTITEM       newgame

#else
#define STARTITEM       readthis
#endif
#endif

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

    if (ingame)
    {
        if (CP_CheckQuick (scancode))
            return;
        lastgamemusicoffset = StartCPMusic (MENUSONG);
    }
    else
        StartCPMusic (MENUSONG);
    SetupControlPanel ();

    //
    // F-KEYS FROM WITHIN GAME
    //
	Menu::closeMenus(false);
    switch (scancode)
    {
        case sc_F1:
#ifdef SPEAR
            BossKey ();
#else
#ifdef GOODTIMES
            BossKey ();
#else
            HelpScreens ();
#endif
#endif
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
		mainMenu[mainMenu.countItems()-3]->setText(STR_EG);
		mainMenu[mainMenu.countItems()-2]->setText("Back to Game");
		mainMenu[mainMenu.countItems()-2]->setHighlighted(true);
		mainMenu[4]->setEnabled(true);
	}
	else
	{
		mainMenu[mainMenu.countItems()-3]->setText(STR_VS);
		mainMenu[mainMenu.countItems()-2]->setText(STR_BD);
		mainMenu[mainMenu.countItems()-2]->setHighlighted(false);
		mainMenu[4]->setEnabled(false);
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

#ifdef SPEAR
#ifndef SPEARDEMO
        IN_ProcessEvents();

        //
        // EASTER EGG FOR SPEAR OF DESTINY!
        //
        if (Keyboard[sc_I] && Keyboard[sc_D])
        {
            VW_FadeOut ();
            StartCPMusic (XJAZNAZI_MUS);
            UnCacheLump (OPTIONS_LUMP_START, OPTIONS_LUMP_END);
            UnCacheLump (BACKDROP_LUMP_START, BACKDROP_LUMP_END);
            ClearMemory ();


            CA_CacheGrChunk (IDGUYS1PIC);
            VWB_DrawPic (0, 0, IDGUYS1PIC);
            UNCACHEGRCHUNK (IDGUYS1PIC);

            CA_CacheGrChunk (IDGUYS2PIC);
            VWB_DrawPic (0, 80, IDGUYS2PIC);
            UNCACHEGRCHUNK (IDGUYS2PIC);

            VW_UpdateScreen ();

            SDL_Color pal[256];
            CA_CacheGrChunk (IDGUYSPALETTE);
            VL_ConvertPalette(grsegs[IDGUYSPALETTE], pal, 256);
            VL_FadeIn (0, 255, pal, 30);
            UNCACHEGRCHUNK (IDGUYSPALETTE);

            while (Keyboard[sc_I] || Keyboard[sc_D])
                IN_WaitAndProcessEvents();
            IN_ClearKeysDown ();
            IN_Ack ();

            VW_FadeOut ();

            CacheLump (BACKDROP_LUMP_START, BACKDROP_LUMP_END);
            CacheLump (OPTIONS_LUMP_START, OPTIONS_LUMP_END);
            mainMenu.draw();
            StartCPMusic (MENUSONG);
            MenuFadeIn ();
        }
#endif
#endif

        switch (which)
        {
            case -1:
                QuitGame(0);
                break;

            default:
                if (!Menu::areMenusClosed())
                {
                    mainMenu.draw();
                    MenuFadeIn ();
                }
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

#ifndef GOODTIMES
#ifndef SPEAR
////////////////////////////////////////////////////////////////////
//
// READ THIS!
//
////////////////////////////////////////////////////////////////////
int
CP_ReadThis (int)
{
    StartCPMusic (CORNER_MUS);
    HelpScreens ();
    StartCPMusic (MENUSONG);
    return true;
}
#endif
#endif


#ifdef GOODTIMES
////////////////////////////////////////////////////////////////////
//
// BOSS KEY
//
////////////////////////////////////////////////////////////////////
void
BossKey (void)
{
#ifdef NOTYET
    byte palette1[256][3];
    SD_MusicOff ();
/*       _AX = 3;
        geninterrupt(0x10); */
    _asm
    {
    mov eax, 3 int 0x10}
    puts ("C>");
    SetTextCursor (2, 0);
//      while (!Keyboard[sc_Escape])
    IN_Ack ();
    IN_ClearKeysDown ();

    SD_MusicOn ();
    VL_SetVGAPlaneMode ();
    for (int i = 0; i < 768; i++)
        palette1[0][i] = 0;

    VL_SetPalette (&palette1[0][0]);
    LoadLatchMem ();
#endif
}
#else
#ifdef SPEAR
void
BossKey (void)
{
#ifdef NOTYET
    byte palette1[256][3];
    SD_MusicOff ();
/*       _AX = 3;
        geninterrupt(0x10); */
    _asm
    {
    mov eax, 3 int 0x10}
    puts ("C>");
    SetTextCursor (2, 0);
//      while (!Keyboard[sc_Escape])
    IN_Ack ();
    IN_ClearKeysDown ();

    SD_MusicOn ();
    VL_SetVGAPlaneMode ();
    for (int i = 0; i < 768; i++)
        palette1[0][i] = 0;

    VL_SetPalette (&palette1[0][0]);
    LoadLatchMem ();
#endif
}
#endif
#endif


////////////////////////////////////////////////////////////////////
//
// CHECK QUICK-KEYS & QUIT (WHILE IN A GAME)
//
////////////////////////////////////////////////////////////////////
int
CP_CheckQuick (ScanCode scancode)
{
    switch (scancode)
    {
        //
        // END GAME
        //
        case sc_F7:
            CA_CacheGrChunk (STARTFONT + 1);

            WindowH = 160;
#ifdef JAPAN
            if (GetYorN (7, 8, C_JAPQUITPIC))
#else
            if (Confirm (ENDGAMESTR))
#endif
            {
                playstate = ex_died;
                killerobj = NULL;
                pickquick = gamestate.lives = 0;
            }

            WindowH = 200;
            fontnumber = 0;
            return 1;

        //
        // QUICKSAVE
        //
        case sc_F8:
			if(saveGame.getCurrentPosition() != 0)
            {
				quickSaveLoad = true;
				CA_CacheGrChunk(STARTFONT + 1);
				PerformSaveGame(saveGame.getCurrentPosition());
				quickSaveLoad = false;
            }
            else
            {
#ifndef SPEAR
                CA_CacheGrChunk (STARTFONT + 1);
                CA_CacheGrChunk (C_CURSOR1PIC);
                CA_CacheGrChunk (C_CURSOR2PIC);
                CA_CacheGrChunk (C_DISKLOADING1PIC);
                CA_CacheGrChunk (C_DISKLOADING2PIC);
                CA_CacheGrChunk (C_SAVEGAMEPIC);
                CA_CacheGrChunk (C_MOUSELBACKPIC);
#else
                CacheLump (BACKDROP_LUMP_START, BACKDROP_LUMP_END);
                CA_CacheGrChunk (C_CURSOR1PIC);
#endif

                VW_FadeOut ();
                if(screenHeight % 200 != 0)
                    VL_ClearScreen(0);

                lastgamemusicoffset = StartCPMusic (MENUSONG);
				Menu::closeMenus(false);
                saveGame.show();

                SETFONTCOLOR (0, 15);
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

#ifndef SPEAR
                UNCACHEGRCHUNK (C_CURSOR1PIC);
                UNCACHEGRCHUNK (C_CURSOR2PIC);
                UNCACHEGRCHUNK (C_DISKLOADING1PIC);
                UNCACHEGRCHUNK (C_DISKLOADING2PIC);
                UNCACHEGRCHUNK (C_SAVEGAMEPIC);
                UNCACHEGRCHUNK (C_MOUSELBACKPIC);
#else
                UnCacheLump (BACKDROP_LUMP_START, BACKDROP_LUMP_END);
#endif
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
				sprintf(string, STR_LGC "%s\"?", SaveFile::files[saveGame.getCurrentPosition()-1].name);
				CA_CacheGrChunk(STARTFONT + 1);
				fontnumber = 1;
				if(Confirm(string))
					LoadSaveGame(saveGame.getCurrentPosition()-1);
				fontnumber = 0;
				quickSaveLoad = false;
            }
            else
            {
#ifndef SPEAR
                CA_CacheGrChunk (STARTFONT + 1);
                CA_CacheGrChunk (C_CURSOR1PIC);
                CA_CacheGrChunk (C_CURSOR2PIC);
                CA_CacheGrChunk (C_DISKLOADING1PIC);
                CA_CacheGrChunk (C_DISKLOADING2PIC);
                CA_CacheGrChunk (C_LOADGAMEPIC);
                CA_CacheGrChunk (C_MOUSELBACKPIC);
#else
                CA_CacheGrChunk (C_CURSOR1PIC);
                CacheLump (BACKDROP_LUMP_START, BACKDROP_LUMP_END);
#endif

                VW_FadeOut ();
                if(screenHeight % 200 != 0)
                    VL_ClearScreen(0);

                lastgamemusicoffset = StartCPMusic (MENUSONG);
				Menu::closeMenus(false);
                loadGame.show();

                SETFONTCOLOR (0, 15);
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

#ifndef SPEAR
                UNCACHEGRCHUNK (C_CURSOR1PIC);
                UNCACHEGRCHUNK (C_CURSOR2PIC);
                UNCACHEGRCHUNK (C_DISKLOADING1PIC);
                UNCACHEGRCHUNK (C_DISKLOADING2PIC);
                UNCACHEGRCHUNK (C_LOADGAMEPIC);
                UNCACHEGRCHUNK (C_MOUSELBACKPIC);
#else
                UnCacheLump (BACKDROP_LUMP_START, BACKDROP_LUMP_END);
#endif
            }
            return 1;

        //
        // QUIT
        //
        case sc_F10:
            CA_CacheGrChunk (STARTFONT + 1);

            WindowX = WindowY = 0;
            WindowW = 320;
            WindowH = 160;
			QuitGame(-1);

            DrawPlayBorder ();
            WindowH = 200;
            fontnumber = 0;
            return 1;
    }

    return 0;
}


////////////////////////////////////////////////////////////////////
//
// END THE CURRENT GAME
//
////////////////////////////////////////////////////////////////////
int
CP_EndGame (int)
{
    int res;
#ifdef JAPAN
    res = GetYorN (7, 8, C_JAPQUITPIC);
#else
    res = Confirm (ENDGAMESTR);
#endif
    mainMenu.draw();
    if(!res) return 0;

    pickquick = gamestate.lives = 0;
    playstate = ex_died;
    killerobj = NULL;

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
    VWB_DrawPic (LSA_X + 8, LSA_Y + 5, C_DISKLOADING1PIC);

    fontnumber = 1;
    SETFONTCOLOR (0, TEXTCOLOR);
    PrintX = LSA_X + 46;
    PrintY = LSA_Y + 13;

    if (!which)
        US_Print (STR_LOADING "...");
    else
        US_Print (STR_SAVING "...");

    VW_UpdateScreen ();
}

////////////////////////////////////////////////////////////////////
//
// HANDLE INTRO SCREEN (SYSTEM CONFIG)
//
////////////////////////////////////////////////////////////////////
void
IntroScreen (void)
{
#ifdef SPEAR

#define MAINCOLOR       0x4f
#define EMSCOLOR        0x4f
#define XMSCOLOR        0x4f

#else

#define MAINCOLOR       0x6c
#define EMSCOLOR        0x6c    // 0x4f
#define XMSCOLOR        0x6c    // 0x7f

#endif
#define FILLCOLOR       14

//      long memory;
//      long emshere,xmshere;
    int i;
/*      int ems[10]={100,200,300,400,500,600,700,800,900,1000},
                xms[10]={100,200,300,400,500,600,700,800,900,1000};
        int main[10]={32,64,96,128,160,192,224,256,288,320};*/


    //
    // DRAW MAIN MEMORY
    //
#ifdef ABCAUS
    memory = (1023l + mminfo.nearheap + mminfo.farheap) / 1024l;
    for (i = 0; i < 10; i++)
        if (memory >= main[i])
            VWB_Bar (49, 163 - 8 * i, 6, 5, MAINCOLOR - i);

    //
    // DRAW EMS MEMORY
    //
    if (EMSPresent)
    {
        emshere = 4l * EMSPagesAvail;
        for (i = 0; i < 10; i++)
            if (emshere >= ems[i])
                VWB_Bar (89, 163 - 8 * i, 6, 5, EMSCOLOR - i);
    }

    //
    // DRAW XMS MEMORY
    //
    if (XMSPresent)
    {
        xmshere = 4l * XMSPagesAvail;
        for (i = 0; i < 10; i++)
            if (xmshere >= xms[i])
                VWB_Bar (129, 163 - 8 * i, 6, 5, XMSCOLOR - i);
    }
#else
    for (i = 0; i < 10; i++)
        VWB_Bar (49, 163 - 8 * i, 6, 5, MAINCOLOR - i);
    for (i = 0; i < 10; i++)
        VWB_Bar (89, 163 - 8 * i, 6, 5, EMSCOLOR - i);
    for (i = 0; i < 10; i++)
        VWB_Bar (129, 163 - 8 * i, 6, 5, XMSCOLOR - i);
#endif


    //
    // FILL BOXES
    //
    if (MousePresent)
        VWB_Bar (164, 82, 12, 2, FILLCOLOR);

    if (IN_JoyPresent())
        VWB_Bar (164, 105, 12, 2, FILLCOLOR);

    if (AdLibPresent && !SoundBlasterPresent)
        VWB_Bar (164, 128, 12, 2, FILLCOLOR);

    if (SoundBlasterPresent)
        VWB_Bar (164, 151, 12, 2, FILLCOLOR);

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
void
ClearMScreen (void)
{
#ifndef SPEAR
    VWB_Bar (0, 0, 320, 200, BORDCOLOR);
#else
    VWB_DrawPic (0, 0, C_BACKDROPPIC);
#endif
}


////////////////////////////////////////////////////////////////////
//
// Un/Cache a LUMP of graphics
//
////////////////////////////////////////////////////////////////////
void
CacheLump (int lumpstart, int lumpend)
{
    int i;

    for (i = lumpstart; i <= lumpend; i++)
        CA_CacheGrChunk (i);
}


void
UnCacheLump (int lumpstart, int lumpend)
{
    int i;

    for (i = lumpstart; i <= lumpend; i++)
        if (grsegs[i])
            UNCACHEGRCHUNK (i);
}


////////////////////////////////////////////////////////////////////
//
// Draw a window for a menu
//
////////////////////////////////////////////////////////////////////
void
DrawWindow (int x, int y, int w, int h, int wcolor)
{
    VWB_Bar (x, y, w, h, wcolor);
    DrawOutline (x, y, w, h, BORD2COLOR, DEACTIVE);
}


void
DrawOutline (int x, int y, int w, int h, int color1, int color2)
{
    VWB_Hlin (x, x + w, y, color2);
    VWB_Vlin (y, y + h, x, color2);
    VWB_Hlin (x, x + w, y + h, color1);
    VWB_Vlin (y, y + h, x + w, color1);
}


////////////////////////////////////////////////////////////////////
//
// Setup Control Panel stuff - graphics, etc.
//
////////////////////////////////////////////////////////////////////
void
SetupControlPanel (void)
{
    //
    // CACHE GRAPHICS & SOUNDS
    //
    CA_CacheGrChunk (STARTFONT + 1);
//#ifndef SPEAR
    CacheLump (CONTROLS_LUMP_START, CONTROLS_LUMP_END);
//#else
//    CacheLump (BACKDROP_LUMP_START, BACKDROP_LUMP_END);
//#endif

    SETFONTCOLOR (TEXTCOLOR, BKGDCOLOR);
    fontnumber = 1;
    WindowH = 200;
    if(screenHeight % 200 != 0)
        VL_ClearScreen(0);

    if (!ingame)
        CA_LoadAllSounds ();

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

#ifdef _arch_dreamcast
    file_t dir;
    dirent_t *dirent;

    dir = fs_open("/vmu/a1", O_RDONLY | O_DIR);

    strcpy(name, SaveName);
    while((dirent = fs_readdir(dir)) && x < 10)
    {
		int filenameLen = strlen(dirent->name);
		if(filenameLen < 12)
			continue;
		char savegam[8];
		char extent[4];
		memcpy(savegam, dirent->name, 7);
		memcpy(extent, dirent->name+(filenameLen-3), 3);
		savegam[7] = 0;
		extent[3] = 0;
		// match pattern savegam%u.%s
		if(strcasecmp("savegam", savegam) == 0 && dirent->name[filenameLen-4] == '.' && strcasecmp(extension, extent) == 0)
		{
			if(DC_LoadFromVMU(dirent->name) != -1)
			{
				const int handle = open(name, O_RDONLY);
				if(handle >= 0)
				{
					SaveFile sFile;
					read(handle, sFile.name, 32);
					close(handle);
					strcpy(sFile.filename, dirent->name);
					SaveFile::files.push_back(sFile);
				}
				fs_unlink(name);
			}
		}
	}
    fs_close(dir);
#else
	File saveDirectory("./");
	const deque<string> &files = saveDirectory.getFileList();
	for(unsigned int i = 0;i < files.size();i++)
	{
		const string &filename = files[i];
		if(filename.length() <= 11 ||
			filename.length() >= 15 ||
			filename.substr(0, 7).compare("savegam") != 0 ||
			filename.substr(filename.length()-3, 3).compare(extension) != 0)
			continue; // Too short or incorrect name

		const int handle = open(filename.c_str(), O_RDONLY | O_BINARY);
		if(handle >= 0)
		{
			SaveFile sFile;
			read(handle, sFile.name, 32);
			close(handle);
			strcpy(sFile.filename, filename.c_str());
			SaveFile::files.push_back(sFile);
		}
	}
#endif
}

////////////////////////////////////////////////////////////////////
//
// Clean up all the Control Panel stuff
//
////////////////////////////////////////////////////////////////////
void
CleanupControlPanel (void)
{
//#ifndef SPEAR
    UnCacheLump (CONTROLS_LUMP_START, CONTROLS_LUMP_END);
//#else
//    UnCacheLump (BACKDROP_LUMP_START, BACKDROP_LUMP_END);
//#endif

    fontnumber = 0;
}

////////////////////////////////////////////////////////////////////
//
// DELAY FOR AN AMOUNT OF TICS OR UNTIL CONTROLS ARE INACTIVE
//
////////////////////////////////////////////////////////////////////
void
TicDelay (int count)
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
void
WaitKeyUp (void)
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
void
ReadAnyControl (ControlInfo * ci)
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
int
Confirm (const char *string)
{
    int xit = 0, x, y, tick = 0, lastBlinkTime;
    int whichsnd[2] = { ESCPRESSEDSND, SHOOTSND };
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
                    VWB_Bar (x, y, 8, 13, TEXTCOLOR);
                    break;
                case 1:
                    PrintX = x;
                    PrintY = y;
                    US_Print ("_");
            }
            VW_UpdateScreen ();
            tick ^= 1;
            lastBlinkTime = GetTimeCount();
        }
        else SDL_Delay(5);

#ifdef SPANISH
    }
    while (!Keyboard[sc_S] && !Keyboard[sc_N] && !Keyboard[sc_Escape]);
#else
    }
    while (!Keyboard[sc_Y] && !Keyboard[sc_N] && !Keyboard[sc_Escape] && !ci.button0 && !ci.button1);
#endif

#ifdef SPANISH
    if (Keyboard[sc_S] || ci.button0)
    {
        xit = 1;
        ShootSnd ();
    }
#else
    if (Keyboard[sc_Y] || ci.button0)
    {
        xit = 1;
        ShootSnd ();
    }
#endif

    IN_ClearKeysDown ();
    WaitKeyUp ();

    SD_PlaySound ((soundnames) whichsnd[xit]);

    return xit;
}

#ifdef JAPAN
////////////////////////////////////////////////////////////////////
//
// DRAW MESSAGE & GET Y OR N
//
////////////////////////////////////////////////////////////////////
int
GetYorN (int x, int y, int pic)
{
    int xit = 0, whichsnd[2] = { ESCPRESSEDSND, SHOOTSND };


    CA_CacheGrChunk (pic);
    VWB_DrawPic (x * 8, y * 8, pic);
    UNCACHEGRCHUNK (pic);
    VW_UpdateScreen ();
    IN_ClearKeysDown ();

    do
    {
        IN_WaitAndProcessEvents();
#ifndef SPEAR
        if (Keyboard[sc_Tab] && Keyboard[sc_P] && param_debugmode)
            PicturePause ();
#endif

#ifdef SPANISH
    }
    while (!Keyboard[sc_S] && !Keyboard[sc_N] && !Keyboard[sc_Escape]);
#else
    }
    while (!Keyboard[sc_Y] && !Keyboard[sc_N] && !Keyboard[sc_Escape]);
#endif

#ifdef SPANISH
    if (Keyboard[sc_S])
    {
        xit = 1;
        ShootSnd ();
    }

    while (Keyboard[sc_S] || Keyboard[sc_N] || Keyboard[sc_Escape])
        IN_WaitAndProcessEvents();

#else

    if (Keyboard[sc_Y])
    {
        xit = 1;
        ShootSnd ();
    }

    while (Keyboard[sc_Y] || Keyboard[sc_N] || Keyboard[sc_Escape])
        IN_WaitAndProcessEvents();
#endif

    IN_ClearKeysDown ();
    SD_PlaySound (whichsnd[xit]);
    return xit;
}
#endif


////////////////////////////////////////////////////////////////////
//
// PRINT A MESSAGE IN A WINDOW
//
////////////////////////////////////////////////////////////////////
void
Message (const char *string)
{
    int h = 0, w = 0, mw = 0, i, len = (int) strlen(string);
    fontstruct *font;


    CA_CacheGrChunk (STARTFONT + 1);
    fontnumber = 1;
    font = (fontstruct *) grsegs[STARTFONT + fontnumber];
    h = font->height;
    for (i = 0; i < len; i++)
    {
        if (string[i] == '\n')
        {
            if (w > mw)
                mw = w;
            w = 0;
            h += font->height;
        }
        else
            w += font->width[string[i]];
    }

    if (w + 10 > mw)
        mw = w + 10;

    PrintY = (WindowH / 2) - h / 2;
    PrintX = WindowX = 160 - mw / 2;

    DrawWindow (WindowX - 5, PrintY - 5, mw + 10, h + 10, TEXTCOLOR);
    DrawOutline (WindowX - 5, PrintY - 5, mw + 10, h + 10, 0, HIGHLIGHT);
    SETFONTCOLOR (0, TEXTCOLOR);
    US_Print (string);
    VW_UpdateScreen ();
}

////////////////////////////////////////////////////////////////////
//
// THIS MAY BE FIXED A LITTLE LATER...
//
////////////////////////////////////////////////////////////////////
static int lastmusic;

int
StartCPMusic (int song)
{
    int lastoffs;

    lastmusic = song;
    lastoffs = SD_MusicOff ();
    UNCACHEAUDIOCHUNK (STARTMUSIC + lastmusic);

    SD_StartMusic(STARTMUSIC + song);
    return lastoffs;
}

void
FreeMusic (void)
{
    UNCACHEAUDIOCHUNK (STARTMUSIC + lastmusic);
}

///////////////////////////////////////////////////////////////////////////
//
// CHECK FOR PAUSE KEY (FOR MUSIC ONLY)
//
///////////////////////////////////////////////////////////////////////////
void
CheckPause (void)
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
void
DrawStripes (int y)
{
#ifndef SPEAR
    VWB_Bar (0, y, 320, 24, 0);
    VWB_Hlin (0, 319, y + 22, STRIPE);
#else
    VWB_Bar (0, y, 320, 22, 0);
    VWB_Hlin (0, 319, y + 23, 0);
#endif
}

void
ShootSnd (void)
{
    SD_PlaySound (SHOOTSND);
}


///////////////////////////////////////////////////////////////////////////
//
// CHECK FOR EPISODES
//
///////////////////////////////////////////////////////////////////////////
void
CheckForEpisodes (void)
{
    struct stat statbuf;

//
// JAPANESE VERSION
//
#ifdef JAPAN
#ifdef JAPDEMO
    if(!stat("vswap.wj1", &statbuf))
    {
        strcpy (extension, "wj1");
        numEpisodesMissing = 5;
#else
    if(!stat("vswap.wj6", &statbuf))
    {
        strcpy (extension, "wj6");
#endif
        strcat (configname, extension);
        strcat (SaveName, extension);
        strcat (demoname, extension);
        EpisodeSelect[1] =
            EpisodeSelect[2] = EpisodeSelect[3] = EpisodeSelect[4] = EpisodeSelect[5] = 1;
    }
    else
        Quit ("NO JAPANESE WOLFENSTEIN 3-D DATA FILES to be found!");
#else

//
// ENGLISH
//
#ifdef UPLOAD
    if(!stat("vswap.wl1", &statbuf))
    {
        strcpy (extension, "wl1");
        numEpisodesMissing = 5;
    }
    else
        Quit ("NO WOLFENSTEIN 3-D DATA FILES to be found!");
#else
#ifndef SPEAR
    if(!stat("vswap.wl6", &statbuf))
    {
        strcpy (extension, "wl6");
    }
    else
    {
        if(!stat("vswap.wl3", &statbuf))
        {
            strcpy (extension, "wl3");
            numEpisodesMissing = 3;
        }
        else
        {
            if(!stat("vswap.wl1", &statbuf))
            {
                strcpy (extension, "wl1");
                numEpisodesMissing = 5;
            }
            else
                Quit ("NO WOLFENSTEIN 3-D DATA FILES to be found!");
        }
    }
#endif
#endif


#ifdef SPEAR
#ifndef SPEARDEMO
    if(param_mission == 1)
    {
        if(!stat("vswap.sod", &statbuf))
            strcpy (extension, "sod");
        else
            Quit ("NO SPEAR OF DESTINY DATA FILES TO BE FOUND!");
    }
    else if(param_mission == 2)
    {
        if(!stat("vswap.sd2", &statbuf))
            strcpy (extension, "sd2");
        else
            Quit ("NO SPEAR OF DESTINY DATA FILES TO BE FOUND!");
    }
    else if(param_mission == 3)
    {
        if(!stat("vswap.sd3", &statbuf))
            strcpy (extension, "sd3");
        else
            Quit ("NO SPEAR OF DESTINY DATA FILES TO BE FOUND!");
    }
    else
        Quit ("UNSUPPORTED MISSION!");
    strcpy (graphext, "sod");
    strcpy (audioext, "sod");
#else
    if(!stat("vswap.sdm", &statbuf))
    {
        strcpy (extension, "sdm");
    }
    else
        Quit ("NO SPEAR OF DESTINY DEMO DATA FILES TO BE FOUND!");
    strcpy (graphext, "sdm");
    strcpy (audioext, "sdm");
#endif
#else
    strcpy (graphext, extension);
    strcpy (audioext, extension);
#endif

    strcat (configname, extension);
    strcat (SaveName, extension);
    strcat (demoname, extension);

#ifndef SPEAR
#ifndef GOODTIMES
    strcat (helpfilename, extension);
#endif
    strcat (endfilename, extension);
#endif
#endif
}
