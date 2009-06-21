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

#include "wl_def.h"
#include "wl_menu.h"
#pragma hdrstop
//new #includes
#include <vector>
using namespace std;

extern int lastgamemusicoffset;
extern int numEpisodesMissing;

Menu mainMenu(MENU_X, MENU_Y, MENU_W, 24);
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
////////////////////////////////////////////////////////////////////
//
// QUIT THIS INFERNAL GAME!
//
////////////////////////////////////////////////////////////////////
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
Menu soundBase(24, 45, 284, 24);
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
MENU_LISTENER(EnterControlBase);
Menu controlBase(CTL_X, CTL_Y, CTL_W, 56, EnterControlBase);
MENU_LISTENER(EnterControlBase)
{
	controlBase[1]->setEnabled(mouseenabled);
	controlBase[2]->setEnabled(mouseenabled);
	controlBase[3]->setEnabled(IN_JoyPresent());
	controlBase.draw();
}
Menu mouseSensitivity(10, 80, 300, 0);
Menu episodes(NE_X+4, NE_Y-1, NE_W+7, 83);
Menu skills(NM_X, NM_Y, NM_W, 24);
int episode = 0;
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
	mainMenu.addItem(new FunctionMenuItem(STR_LG, CP_LoadGame));
	MenuItem *sg = new FunctionMenuItem(STR_SG, CP_SaveGame);
	sg->setEnabled(false);
	mainMenu.addItem(sg);
	mainMenu.addItem(new FunctionMenuItem(STR_CV, CP_ChangeView));
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
	controlBase.addItem(new BooleanMenuItem(STR_MOUSEEN, mouseenabled, EnterControlBase));
	controlBase.addItem(new BooleanMenuItem(STR_DISABLEYAXIS, mouseyaxisdisabled, EnterControlBase));
	controlBase.addItem(new MenuSwitcherMenuItem(STR_SENS, mouseSensitivity));
	controlBase.addItem(new BooleanMenuItem(STR_JOYEN, joystickenabled, EnterControlBase));
	controlBase.addItem(new FunctionMenuItem(STR_CUSTOM, CustomControls));
	//HandleControlBase(static_cast<unsigned> (-1)); // Enabled/Disable options

	mouseSensitivity.addItem(new LabelMenuItem(STR_MOUSEADJ));
	mouseSensitivity.addItem(new SliderMenuItem(mouseadjustment, 200, 10, STR_SLOW, STR_FAST));
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

CP_itemtype LSMenu[] = {
    {1, "", 0},
    {1, "", 0},
    {1, "", 0},
    {1, "", 0},
    {1, "", 0},
    {1, "", 0},
    {1, "", 0},
    {1, "", 0},
    {1, "", 0},
    {1, "", 0}
};

CP_itemtype CusMenu[] = {
    {1, "", 0},
    {0, "", 0},
    {0, "", 0},
    {1, "", 0},
    {0, "", 0},
    {0, "", 0},
    {1, "", 0},
    {0, "", 0},
    {1, "", 0}
};

// CP_iteminfo struct format: short x, y, amount, curpos, indent;
CP_iteminfo LSItems   = { LSM_X, LSM_Y, lengthof(LSMenu), 0, 24 },
            CusItems  = { 8, CST_Y + 13 * 2, lengthof(CusMenu), -1, 0};

int EpisodeSelect[6] = { 1 };


static int SaveGamesAvail[10];
static int SoundStatus = 1;
static int pickquick;
static char SaveGameNames[10][32];
static char SaveName[13] = "savegam?.";


////////////////////////////////////////////////////////////////////
//
// INPUT MANAGER SCANCODE TABLES
//
////////////////////////////////////////////////////////////////////

#if 0
static const char *ScanNames[] =      // Scan code names with single chars
{
    "?", "?", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "+", "?", "?",
    "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "|", "?", "A", "S",
    "D", "F", "G", "H", "J", "K", "L", ";", "\"", "?", "?", "?", "Z", "X", "C", "V",
    "B", "N", "M", ",", ".", "/", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
    "?", "?", "?", "?", "?", "?", "?", "?", "\xf", "?", "-", "\x15", "5", "\x11", "+", "?",
    "\x13", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
    "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
    "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?"
};                              // DEBUG - consolidate these
static ScanCode ExtScanCodes[] =        // Scan codes with >1 char names
{
    1, 0xe, 0xf, 0x1d, 0x2a, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e,
    0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x57, 0x59, 0x46, 0x1c, 0x36,
    0x37, 0x38, 0x47, 0x49, 0x4f, 0x51, 0x52, 0x53, 0x45, 0x48,
    0x50, 0x4b, 0x4d, 0x00
};
static const char *ExtScanNames[] =   // Names corresponding to ExtScanCodes
{
    "Esc", "BkSp", "Tab", "Ctrl", "LShft", "Space", "CapsLk", "F1", "F2", "F3", "F4",
    "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "ScrlLk", "Enter", "RShft",
    "PrtSc", "Alt", "Home", "PgUp", "End", "PgDn", "Ins", "Del", "NumLk", "Up",
    "Down", "Left", "Right", ""
};

/*#pragma warning 737 9
static byte
                                        *ScanNames[] =          // Scan code names with single chars
                                        {
        "?","?","1","2","3","4","5","6","7","8","9","0","-","+","?","?",
        "Q","W","E","R","T","Y","U","I","O","P","[","]","|","?","A","S",
        "D","F","G","H","J","K","L",";","\"","?","?","?","Z","X","C","V",
        "B","N","M",",",".","/","?","?","?","?","?","?","?","?","?","?",
        "?","?","?","?","?","?","?","?","\xf","?","-","\x15","5","\x11","+","?",
        "\x13","?","?","?","?","?","?","?","?","?","?","?","?","?","?","?",
        "?","?","?","?","?","?","?","?","?","?","?","?","?","?","?","?",
        "?","?","?","?","?","?","?","?","?","?","?","?","?","?","?","?"
                                        };      // DEBUG - consolidate these
static byte ExtScanCodes[] =    // Scan codes with >1 char names
                                        {
        1,0xe,0xf,0x1d,0x2a,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,
        0x3f,0x40,0x41,0x42,0x43,0x44,0x57,0x59,0x46,0x1c,0x36,
        0x37,0x38,0x47,0x49,0x4f,0x51,0x52,0x53,0x45,0x48,
        0x50,0x4b,0x4d,0x00
                                        };
static byte *ExtScanNames[] =   // Names corresponding to ExtScanCodes
                                        {
        "Esc","BkSp","Tab","Ctrl","LShft","Space","CapsLk","F1","F2","F3","F4",
        "F5","F6","F7","F8","F9","F10","F11","F12","ScrlLk","Enter","RShft",
        "PrtSc","Alt","Home","PgUp","End","PgDn","Ins","Del","NumLk","Up",
        "Down","Left","Right",""
                                        };*/

#else
static const char* const ScanNames[SDLK_LAST] =
    {
        "?","?","?","?","?","?","?","?",                                //   0
        "BkSp","Tab","?","?","?","Return","?","?",                      //   8
        "?","?","?","Pause","?","?","?","?",                            //  16
        "?","?","?","Esc","?","?","?","?",                              //  24
        "Space","!","\"","#","$","?","&","'",                           //  32
        "(",")","*","+",",","-",".","/",                                //  40
        "0","1","2","3","4","5","6","7",                                //  48
        "8","9",":",";","<","=",">","?",                                //  56
        "@","A","B","C","D","E","F","G",                                //  64
        "H","I","J","K","L","M","N","O",                                //  72
        "P","Q","R","S","T","U","V","W",                                //  80
        "X","Y","Z","[","\\","]","^","_",                               //  88
        "`","a","b","c","d","e","f","h",                                //  96
        "h","i","j","k","l","m","n","o",                                // 104
        "p","q","r","s","t","u","v","w",                                // 112
        "x","y","z","{","|","}","~","?",                                // 120
        "?","?","?","?","?","?","?","?",                                // 128
        "?","?","?","?","?","?","?","?",                                // 136
        "?","?","?","?","?","?","?","?",                                // 144
        "?","?","?","?","?","?","?","?",                                // 152
        "?","?","?","?","?","?","?","?",                                // 160
        "?","?","?","?","?","?","?","?",                                // 168
        "?","?","?","?","?","?","?","?",                                // 176
        "?","?","?","?","?","?","?","?",                                // 184
        "?","?","?","?","?","?","?","?",                                // 192
        "?","?","?","?","?","?","?","?",                                // 200
        "?","?","?","?","?","?","?","?",                                // 208
        "?","?","?","?","?","?","?","?",                                // 216
        "?","?","?","?","?","?","?","?",                                // 224
        "?","?","?","?","?","?","?","?",                                // 232
        "?","?","?","?","?","?","?","?",                                // 240
        "?","?","?","?","?","?","?","?",                                // 248
        "?","?","?","?","?","?","?","?",                                // 256
        "?","?","?","?","?","?","?","Enter",                            // 264
        "?","Up","Down","Right","Left","Ins","Home","End",              // 272
        "PgUp","PgDn","F1","F2","F3","F4","F5","F6",                    // 280
        "F7","F8","F9","F10","F11","F12","?","?",                       // 288
        "?","?","?","?","NumLk","CapsLk","ScrlLk","RShft",              // 296
        "Shift","RCtrl","Ctrl","RAlt","Alt","?","?","?",                // 304
        "?","?","?","?","PrtSc","?","?","?",                            // 312
        "?","?"                                                         // 320
    };

#endif

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
            CP_SaveGame (0);
            goto finishup;

        case sc_F3:
            CP_LoadGame (0);
            goto finishup;

        case sc_F4:
            soundBase.show();
            goto finishup;

        case sc_F5:
            CP_ChangeView (0);
            goto finishup;

        case sc_F6:
            controlBase.show ();
            goto finishup;

        finishup:
            CleanupControlPanel ();
            return;
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
            if (SaveGamesAvail[LSItems.curpos] && pickquick)
            {
                CA_CacheGrChunk (STARTFONT + 1);
                fontnumber = 1;
                Message (STR_SAVING "...");
                CP_SaveGame (1);
                fontnumber = 0;
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
                pickquick = CP_SaveGame (0);

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
            if (SaveGamesAvail[LSItems.curpos] && pickquick)
            {
                char string[100] = STR_LGC;


                CA_CacheGrChunk (STARTFONT + 1);
                fontnumber = 1;

                strcat (string, SaveGameNames[LSItems.curpos]);
                strcat (string, "\"?");

                if (Confirm (string))
                    CP_LoadGame (1);

                fontnumber = 0;
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
                pickquick = CP_LoadGame (0);    // loads lastgamemusicoffs

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
#define LSA_X   96
#define LSA_Y   80
#define LSA_W   130
#define LSA_H   42

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
// LOAD SAVED GAMES
//
////////////////////////////////////////////////////////////////////
int
CP_LoadGame (int quick)
{
    FILE *file;
    int which, exit = 0;
    char name[13];


    strcpy (name, SaveName);

    //
    // QUICKLOAD?
    //
    if (quick)
    {
        which = LSItems.curpos;

        if (SaveGamesAvail[which])
        {
            name[7] = which + '0';
#ifdef _arch_dreamcast
            DC_LoadFromVMU(name);
#endif
            file = fopen (name, "rb");
            fseek (file, 32, SEEK_SET);
            loadedgame = true;
            LoadTheGame (file, 0, 0);
            loadedgame = false;
            fclose (file);

            DrawFace ();
            DrawHealth ();
            DrawLives ();
            DrawLevel ();
            DrawAmmo ();
            DrawKeys ();
            DrawWeapon ();
            DrawScore ();
            ContinueMusic (lastgamemusicoffset);
            return 1;
        }
    }

    DrawLoadSaveScreen (0);

    do
    {
        which = HandleMenu (&LSItems, &LSMenu[0], TrackWhichGame);
        if (which >= 0 && SaveGamesAvail[which])
        {
            ShootSnd ();
            name[7] = which + '0';

#ifdef _arch_dreamcast
            DC_LoadFromVMU(name);
#endif
            file = fopen (name, "rb");
            fseek (file, 32, SEEK_SET);

            DrawLSAction (0);
            loadedgame = true;

            LoadTheGame (file, LSA_X + 8, LSA_Y + 5);
            fclose (file);

			Menu::closeMenus();
            ShootSnd ();
            //
            // CHANGE "READ THIS!" TO NORMAL COLOR
            //

#ifndef SPEAR
#ifndef GOODTIMES
            mainMenu[mainMenu.countItems()-3]->setHighlighted(false);
#endif
#endif

            exit = 1;
            break;
        }

    }
    while (which >= 0);

    MenuFadeOut ();

    return exit;
}


///////////////////////////////////
//
// HIGHLIGHT CURRENT SELECTED ENTRY
//
void
TrackWhichGame (int w)
{
    static int lastgameon = 0;

    PrintLSEntry (lastgameon, TEXTCOLOR);
    PrintLSEntry (w, HIGHLIGHT);

    lastgameon = w;
}


////////////////////////////
//
// DRAW THE LOAD/SAVE SCREEN
//
void
DrawLoadSaveScreen (int loadsave)
{
#define DISKX   100
#define DISKY   0

    int i;


    ClearMScreen ();
    fontnumber = 1;
    VWB_DrawPic (112, 184, C_MOUSELBACKPIC);
    DrawWindow (LSM_X - 10, LSM_Y - 5, LSM_W, LSM_H, BKGDCOLOR);
    DrawStripes (10);

    if (!loadsave)
        VWB_DrawPic (60, 0, C_LOADGAMEPIC);
    else
        VWB_DrawPic (60, 0, C_SAVEGAMEPIC);

    for (i = 0; i < 10; i++)
        PrintLSEntry (i, TEXTCOLOR);

    DrawMenu (&LSItems, &LSMenu[0]);
    VW_UpdateScreen ();
    MenuFadeIn ();
    WaitKeyUp ();
}


///////////////////////////////////////////
//
// PRINT LOAD/SAVE GAME ENTRY W/BOX OUTLINE
//
void
PrintLSEntry (int w, int color)
{
    SETFONTCOLOR (color, BKGDCOLOR);
    DrawOutline (LSM_X + LSItems.indent, LSM_Y + w * 13, LSM_W - LSItems.indent - 15, 11, color,
                 color);
    PrintX = LSM_X + LSItems.indent + 2;
    PrintY = LSM_Y + w * 13 + 1;
    fontnumber = 0;

    if (SaveGamesAvail[w])
        US_Print (SaveGameNames[w]);
    else
        US_Print ("      - " STR_EMPTY " -");

    fontnumber = 1;
}


////////////////////////////////////////////////////////////////////
//
// SAVE CURRENT GAME
//
////////////////////////////////////////////////////////////////////
int
CP_SaveGame (int quick)
{
    int which, exit = 0;
    FILE *file;
    char name[13];
    char input[32];


    strcpy (name, SaveName);

    //
    // QUICKSAVE?
    //
    if (quick)
    {
        which = LSItems.curpos;

        if (SaveGamesAvail[which])
        {
            name[7] = which + '0';
            unlink (name);
            file = fopen (name, "wb");

            strcpy (input, &SaveGameNames[which][0]);

//                      _dos_write(handle,(void far *)input,32,&nwritten);
            fwrite (input, 1, 32, file);
            fseek (file, 32, SEEK_SET);
            SaveTheGame (file, 0, 0);
            fclose (file);

#ifdef _arch_dreamcast
            DC_SaveToVMU (name, 2);
#endif

            return 1;
        }
    }

    DrawLoadSaveScreen (1);

    do
    {
        which = HandleMenu (&LSItems, &LSMenu[0], TrackWhichGame);
        if (which >= 0)
        {
            //
            // OVERWRITE EXISTING SAVEGAME?
            //
            if (SaveGamesAvail[which])
#ifdef JAPAN
                if (!GetYorN (7, 8, C_JAPSAVEOVERPIC))
#else
                if (!Confirm (GAMESVD))
#endif
                {
                    DrawLoadSaveScreen (1);
                    continue;
                }
                else
                {
                    DrawLoadSaveScreen (1);
                    PrintLSEntry (which, HIGHLIGHT);
                    VW_UpdateScreen ();
                }

            ShootSnd ();

            strcpy (input, &SaveGameNames[which][0]);
            name[7] = which + '0';

            fontnumber = 0;
            if (!SaveGamesAvail[which])
                VWB_Bar (LSM_X + LSItems.indent + 1, LSM_Y + which * 13 + 1,
                         LSM_W - LSItems.indent - 16, 10, BKGDCOLOR);
            VW_UpdateScreen ();

            if (US_LineInput
                (LSM_X + LSItems.indent + 2, LSM_Y + which * 13 + 1, input, input, true, 31,
                 LSM_W - LSItems.indent - 30))
            {
                SaveGamesAvail[which] = 1;
                strcpy (&SaveGameNames[which][0], input);

                unlink (name);
                file = fopen (name, "wb");
//                              _dos_write(handle,(void far *)input,32,&nwritten);
                fwrite (input, 32, 1, file);
                fseek (file, 32, SEEK_SET);

                DrawLSAction (1);
                SaveTheGame (file, LSA_X + 8, LSA_Y + 5);

                fclose (file);

#ifdef _arch_dreamcast
                DC_SaveToVMU (name, 2);
#endif

                ShootSnd ();
                exit = 1;
            }
            else
            {
                VWB_Bar (LSM_X + LSItems.indent + 1, LSM_Y + which * 13 + 1,
                         LSM_W - LSItems.indent - 16, 10, BKGDCOLOR);
                PrintLSEntry (which, HIGHLIGHT);
                VW_UpdateScreen ();
                SD_PlaySound (ESCPRESSEDSND);
                continue;
            }

            fontnumber = 1;
            break;
        }

    }
    while (which >= 0);

    MenuFadeOut ();

    return exit;
}

////////////////////////////////////////////////////////////////////
//
// CUSTOMIZE CONTROLS
//
////////////////////////////////////////////////////////////////////
enum
{ FIRE, STRAFE, RUN, OPEN };
char mbarray[4][3] = { "b0", "b1", "b2", "b3" };
int8_t order[4] = { RUN, OPEN, FIRE, STRAFE };


int
CustomControls (int)
{
    int which;

    DrawCustomScreen ();
    do
    {
        which = HandleMenu (&CusItems, &CusMenu[0], FixupCustom);
        switch (which)
        {
            case 0:
                DefineMouseBtns ();
                DrawCustMouse (1);
                break;
            case 3:
                DefineJoyBtns ();
                DrawCustJoy (0);
                break;
            case 6:
                DefineKeyBtns ();
                DrawCustKeybd (0);
                break;
            case 8:
                DefineKeyMove ();
                DrawCustKeys (0);
        }
    }
    while (which >= 0);

    MenuFadeOut ();

    return 0;
}


////////////////////////
//
// DEFINE THE MOUSE BUTTONS
//
void
DefineMouseBtns (void)
{
    CustomCtrls mouseallowed = { 0, 1, 1, 1 };
    EnterCtrlData (2, &mouseallowed, DrawCustMouse, PrintCustMouse, MOUSE);
}


////////////////////////
//
// DEFINE THE JOYSTICK BUTTONS
//
void
DefineJoyBtns (void)
{
    CustomCtrls joyallowed = { 1, 1, 1, 1 };
    EnterCtrlData (5, &joyallowed, DrawCustJoy, PrintCustJoy, JOYSTICK);
}


////////////////////////
//
// DEFINE THE KEYBOARD BUTTONS
//
void
DefineKeyBtns (void)
{
    CustomCtrls keyallowed = { 1, 1, 1, 1 };
    EnterCtrlData (8, &keyallowed, DrawCustKeybd, PrintCustKeybd, KEYBOARDBTNS);
}


////////////////////////
//
// DEFINE THE KEYBOARD BUTTONS
//
void
DefineKeyMove (void)
{
    CustomCtrls keyallowed = { 1, 1, 1, 1 };
    EnterCtrlData (10, &keyallowed, DrawCustKeys, PrintCustKeys, KEYBOARDMOVE);
}


////////////////////////
//
// ENTER CONTROL DATA FOR ANY TYPE OF CONTROL
//
enum
{ FWRD, RIGHT, BKWD, LEFT };
int moveorder[4] = { LEFT, RIGHT, FWRD, BKWD };

void
EnterCtrlData (int index, CustomCtrls * cust, void (*DrawRtn) (int), void (*PrintRtn) (int),
               int type)
{
    int j, exit, tick, redraw, which, x, picked, lastFlashTime;
    ControlInfo ci;


    ShootSnd ();
    PrintY = CST_Y + 13 * index;
    IN_ClearKeysDown ();
    exit = 0;
    redraw = 1;
    //
    // FIND FIRST SPOT IN ALLOWED ARRAY
    //
    for (j = 0; j < 4; j++)
        if (cust->allowed[j])
        {
            which = j;
            break;
        }

    do
    {
        if (redraw)
        {
            x = CST_START + CST_SPC * which;
            DrawWindow (5, PrintY - 1, 310, 13, BKGDCOLOR);

            DrawRtn (1);
            DrawWindow (x - 2, PrintY, CST_SPC, 11, TEXTCOLOR);
            DrawOutline (x - 2, PrintY, CST_SPC, 11, 0, HIGHLIGHT);
            SETFONTCOLOR (0, TEXTCOLOR);
            PrintRtn (which);
            PrintX = x;
            SETFONTCOLOR (TEXTCOLOR, BKGDCOLOR);
            VW_UpdateScreen ();
            WaitKeyUp ();
            redraw = 0;
        }

        SDL_Delay(5);
        ReadAnyControl (&ci);

        if (type == MOUSE || type == JOYSTICK)
            if (IN_KeyDown (sc_Enter) || IN_KeyDown (sc_Control) || IN_KeyDown (sc_Alt))
            {
                IN_ClearKeysDown ();
                ci.button0 = ci.button1 = false;
            }

        //
        // CHANGE BUTTON VALUE?
        //
        if ((type != KEYBOARDBTNS && type != KEYBOARDMOVE) && (ci.button0 | ci.button1 | ci.button2 | ci.button3) ||
            ((type == KEYBOARDBTNS || type == KEYBOARDMOVE) && LastScan == sc_Enter))
        {
            lastFlashTime = GetTimeCount();
            tick = picked = 0;
            SETFONTCOLOR (0, TEXTCOLOR);

            if (type == KEYBOARDBTNS || type == KEYBOARDMOVE)
                IN_ClearKeysDown ();

            while(1)
            {
                int button, result = 0;

                //
                // FLASH CURSOR
                //
                if (GetTimeCount() - lastFlashTime > 10)
                {
                    switch (tick)
                    {
                        case 0:
                            VWB_Bar (x, PrintY + 1, CST_SPC - 2, 10, TEXTCOLOR);
                            break;
                        case 1:
                            PrintX = x;
                            US_Print ("?");
                            SD_PlaySound (HITWALLSND);
                    }
                    tick ^= 1;
                    lastFlashTime = GetTimeCount();
                    VW_UpdateScreen ();
                }
                else SDL_Delay(5);

                //
                // WHICH TYPE OF INPUT DO WE PROCESS?
                //
                switch (type)
                {
                    case MOUSE:
                        button = IN_MouseButtons();
                        switch (button)
                        {
                            case 1:
                                result = 1;
                                break;
                            case 2:
                                result = 2;
                                break;
                            case 4:
                                result = 3;
                                break;
                        }

                        if (result)
                        {
                            for (int z = 0; z < 4; z++)
                                if (order[which] == buttonmouse[z])
                                {
                                    buttonmouse[z] = bt_nobutton;
                                    break;
                                }

                            buttonmouse[result - 1] = order[which];
                            picked = 1;
                            SD_PlaySound (SHOOTDOORSND);
                        }
                        break;

                    case JOYSTICK:
                        if (ci.button0)
                            result = 1;
                        else if (ci.button1)
                            result = 2;
                        else if (ci.button2)
                            result = 3;
                        else if (ci.button3)
                            result = 4;

                        if (result)
                        {
                            for (int z = 0; z < 4; z++)
                            {
                                if (order[which] == buttonjoy[z])
                                {
                                    buttonjoy[z] = bt_nobutton;
                                    break;
                                }
                            }

                            buttonjoy[result - 1] = order[which];
                            picked = 1;
                            SD_PlaySound (SHOOTDOORSND);
                        }
                        break;

                    case KEYBOARDBTNS:
                        if (LastScan && LastScan != sc_Escape)
                        {
                            buttonscan[order[which]] = LastScan;
                            picked = 1;
                            ShootSnd ();
                            IN_ClearKeysDown ();
                        }
                        break;

                    case KEYBOARDMOVE:
                        if (LastScan && LastScan != sc_Escape)
                        {
                            dirscan[moveorder[which]] = LastScan;
                            picked = 1;
                            ShootSnd ();
                            IN_ClearKeysDown ();
                        }
                        break;
                }

                //
                // EXIT INPUT?
                //
                if (IN_KeyDown (sc_Escape) || type != JOYSTICK && ci.button1)
                {
                    picked = 1;
                    SD_PlaySound (ESCPRESSEDSND);
                }

                if(picked) break;

                ReadAnyControl (&ci);
            }

            SETFONTCOLOR (TEXTCOLOR, BKGDCOLOR);
            redraw = 1;
            WaitKeyUp ();
            continue;
        }

        if (ci.button1 || IN_KeyDown (sc_Escape))
            exit = 1;

        //
        // MOVE TO ANOTHER SPOT?
        //
        switch (ci.dir)
        {
            case dir_West:
                do
                {
                    which--;
                    if (which < 0)
                        which = 3;
                }
                while (!cust->allowed[which]);
                redraw = 1;
                SD_PlaySound (MOVEGUN1SND);
                while (ReadAnyControl (&ci), ci.dir != dir_None) SDL_Delay(5);
                IN_ClearKeysDown ();
                break;

            case dir_East:
                do
                {
                    which++;
                    if (which > 3)
                        which = 0;
                }
                while (!cust->allowed[which]);
                redraw = 1;
                SD_PlaySound (MOVEGUN1SND);
                while (ReadAnyControl (&ci), ci.dir != dir_None) SDL_Delay(5);
                IN_ClearKeysDown ();
                break;
            case dir_North:
            case dir_South:
                exit = 1;
        }
    }
    while (!exit);

    SD_PlaySound (ESCPRESSEDSND);
    WaitKeyUp ();
    DrawWindow (5, PrintY - 1, 310, 13, BKGDCOLOR);
}


////////////////////////
//
// FIXUP GUN CURSOR OVERDRAW SHIT
//
void
FixupCustom (int w)
{
    static int lastwhich = -1;
    int y = CST_Y + 26 + w * 13;


    VWB_Hlin (7, 32, y - 1, DEACTIVE);
    VWB_Hlin (7, 32, y + 12, BORD2COLOR);
#ifndef SPEAR
    VWB_Hlin (7, 32, y - 2, BORDCOLOR);
    VWB_Hlin (7, 32, y + 13, BORDCOLOR);
#else
    VWB_Hlin (7, 32, y - 2, BORD2COLOR);
    VWB_Hlin (7, 32, y + 13, BORD2COLOR);
#endif

    switch (w)
    {
        case 0:
            DrawCustMouse (1);
            break;
        case 3:
            DrawCustJoy (1);
            break;
        case 6:
            DrawCustKeybd (1);
            break;
        case 8:
            DrawCustKeys (1);
    }


    if (lastwhich >= 0)
    {
        y = CST_Y + 26 + lastwhich * 13;
        VWB_Hlin (7, 32, y - 1, DEACTIVE);
        VWB_Hlin (7, 32, y + 12, BORD2COLOR);
#ifndef SPEAR
        VWB_Hlin (7, 32, y - 2, BORDCOLOR);
        VWB_Hlin (7, 32, y + 13, BORDCOLOR);
#else
        VWB_Hlin (7, 32, y - 2, BORD2COLOR);
        VWB_Hlin (7, 32, y + 13, BORD2COLOR);
#endif

        if (lastwhich != w)
            switch (lastwhich)
            {
                case 0:
                    DrawCustMouse (0);
                    break;
                case 3:
                    DrawCustJoy (0);
                    break;
                case 6:
                    DrawCustKeybd (0);
                    break;
                case 8:
                    DrawCustKeys (0);
            }
    }

    lastwhich = w;
}


////////////////////////
//
// DRAW CUSTOMIZE SCREEN
//
void
DrawCustomScreen (void)
{
    int i;


#ifdef JAPAN
    CA_CacheScreen (S_CUSTOMPIC);
    fontnumber = 1;

    PrintX = CST_START;
    PrintY = CST_Y + 26;
    DrawCustMouse (0);

    PrintX = CST_START;
    US_Print ("\n\n\n");
    DrawCustJoy (0);

    PrintX = CST_START;
    US_Print ("\n\n\n");
    DrawCustKeybd (0);

    PrintX = CST_START;
    US_Print ("\n\n\n");
    DrawCustKeys (0);
#else
    ClearMScreen ();
    WindowX = 0;
    WindowW = 320;
    VWB_DrawPic (112, 184, C_MOUSELBACKPIC);
    DrawStripes (10);
    VWB_DrawPic (80, 0, C_CUSTOMIZEPIC);

    //
    // MOUSE
    //
    SETFONTCOLOR (READCOLOR, BKGDCOLOR);
    WindowX = 0;
    WindowW = 320;

#ifndef SPEAR
    PrintY = CST_Y;
    US_CPrint ("Mouse\n");
#else
    PrintY = CST_Y + 13;
    VWB_DrawPic (128, 48, C_MOUSEPIC);
#endif

    SETFONTCOLOR (TEXTCOLOR, BKGDCOLOR);
#ifdef SPANISH
    PrintX = CST_START - 16;
    US_Print (STR_CRUN);
    PrintX = CST_START - 16 + CST_SPC * 1;
    US_Print (STR_COPEN);
    PrintX = CST_START - 16 + CST_SPC * 2;
    US_Print (STR_CFIRE);
    PrintX = CST_START - 16 + CST_SPC * 3;
    US_Print (STR_CSTRAFE "\n");
#else
    PrintX = CST_START;
    US_Print (STR_CRUN);
    PrintX = CST_START + CST_SPC * 1;
    US_Print (STR_COPEN);
    PrintX = CST_START + CST_SPC * 2;
    US_Print (STR_CFIRE);
    PrintX = CST_START + CST_SPC * 3;
    US_Print (STR_CSTRAFE "\n");
#endif

    DrawWindow (5, PrintY - 1, 310, 13, BKGDCOLOR);
    DrawCustMouse (0);
    US_Print ("\n");


    //
    // JOYSTICK/PAD
    //
#ifndef SPEAR
    SETFONTCOLOR (READCOLOR, BKGDCOLOR);
    US_CPrint ("Joystick/Gravis GamePad\n");
#else
    PrintY += 13;
    VWB_DrawPic (40, 88, C_JOYSTICKPIC);
#endif

#ifdef SPEAR
    VWB_DrawPic (112, 120, C_KEYBOARDPIC);
#endif

    SETFONTCOLOR (TEXTCOLOR, BKGDCOLOR);
#ifdef SPANISH
    PrintX = CST_START - 16;
    US_Print (STR_CRUN);
    PrintX = CST_START - 16 + CST_SPC * 1;
    US_Print (STR_COPEN);
    PrintX = CST_START - 16 + CST_SPC * 2;
    US_Print (STR_CFIRE);
    PrintX = CST_START - 16 + CST_SPC * 3;
    US_Print (STR_CSTRAFE "\n");
#else
    PrintX = CST_START;
    US_Print (STR_CRUN);
    PrintX = CST_START + CST_SPC * 1;
    US_Print (STR_COPEN);
    PrintX = CST_START + CST_SPC * 2;
    US_Print (STR_CFIRE);
    PrintX = CST_START + CST_SPC * 3;
    US_Print (STR_CSTRAFE "\n");
#endif
    DrawWindow (5, PrintY - 1, 310, 13, BKGDCOLOR);
    DrawCustJoy (0);
    US_Print ("\n");


    //
    // KEYBOARD
    //
#ifndef SPEAR
    SETFONTCOLOR (READCOLOR, BKGDCOLOR);
    US_CPrint ("Keyboard\n");
#else
    PrintY += 13;
#endif
    SETFONTCOLOR (TEXTCOLOR, BKGDCOLOR);
#ifdef SPANISH
    PrintX = CST_START - 16;
    US_Print (STR_CRUN);
    PrintX = CST_START - 16 + CST_SPC * 1;
    US_Print (STR_COPEN);
    PrintX = CST_START - 16 + CST_SPC * 2;
    US_Print (STR_CFIRE);
    PrintX = CST_START - 16 + CST_SPC * 3;
    US_Print (STR_CSTRAFE "\n");
#else
    PrintX = CST_START;
    US_Print (STR_CRUN);
    PrintX = CST_START + CST_SPC * 1;
    US_Print (STR_COPEN);
    PrintX = CST_START + CST_SPC * 2;
    US_Print (STR_CFIRE);
    PrintX = CST_START + CST_SPC * 3;
    US_Print (STR_CSTRAFE "\n");
#endif
    DrawWindow (5, PrintY - 1, 310, 13, BKGDCOLOR);
    DrawCustKeybd (0);
    US_Print ("\n");


    //
    // KEYBOARD MOVE KEYS
    //
    SETFONTCOLOR (TEXTCOLOR, BKGDCOLOR);
#ifdef SPANISH
    PrintX = 4;
    US_Print (STR_LEFT);
    US_Print ("/");
    US_Print (STR_RIGHT);
    US_Print ("/");
    US_Print (STR_FRWD);
    US_Print ("/");
    US_Print (STR_BKWD "\n");
#else
    PrintX = CST_START;
    US_Print (STR_LEFT);
    PrintX = CST_START + CST_SPC * 1;
    US_Print (STR_RIGHT);
    PrintX = CST_START + CST_SPC * 2;
    US_Print (STR_FRWD);
    PrintX = CST_START + CST_SPC * 3;
    US_Print (STR_BKWD "\n");
#endif
    DrawWindow (5, PrintY - 1, 310, 13, BKGDCOLOR);
    DrawCustKeys (0);
#endif
    //
    // PICK STARTING POINT IN MENU
    //
    if (CusItems.curpos < 0)
        for (i = 0; i < CusItems.amount; i++)
            if (CusMenu[i].active)
            {
                CusItems.curpos = i;
                break;
            }


    VW_UpdateScreen ();
    MenuFadeIn ();
}


void
PrintCustMouse (int i)
{
    int j;

    for (j = 0; j < 4; j++)
        if (order[i] == buttonmouse[j])
        {
            PrintX = CST_START + CST_SPC * i;
            US_Print (mbarray[j]);
            break;
        }
}

void
DrawCustMouse (int hilight)
{
    int i, color;


    color = TEXTCOLOR;
    if (hilight)
        color = HIGHLIGHT;
    SETFONTCOLOR (color, BKGDCOLOR);

    if (!mouseenabled)
    {
        SETFONTCOLOR (DEACTIVE, BKGDCOLOR);
        CusMenu[0].active = 0;
    }
    else
        CusMenu[0].active = 1;

    PrintY = CST_Y + 13 * 2;
    for (i = 0; i < 4; i++)
        PrintCustMouse (i);
}

void
PrintCustJoy (int i)
{
    for (int j = 0; j < 4; j++)
    {
        if (order[i] == buttonjoy[j])
        {
            PrintX = CST_START + CST_SPC * i;
            US_Print (mbarray[j]);
            break;
        }
    }
}

void
DrawCustJoy (int hilight)
{
    int i, color;

    color = TEXTCOLOR;
    if (hilight)
        color = HIGHLIGHT;
    SETFONTCOLOR (color, BKGDCOLOR);

    if (!joystickenabled)
    {
        SETFONTCOLOR (DEACTIVE, BKGDCOLOR);
        CusMenu[3].active = 0;
    }
    else
        CusMenu[3].active = 1;

    PrintY = CST_Y + 13 * 5;
    for (i = 0; i < 4; i++)
        PrintCustJoy (i);
}


void
PrintCustKeybd (int i)
{
    PrintX = CST_START + CST_SPC * i;
    US_Print ((const char *) IN_GetScanName (buttonscan[order[i]]));
}

void
DrawCustKeybd (int hilight)
{
    int i, color;


    color = TEXTCOLOR;
    if (hilight)
        color = HIGHLIGHT;
    SETFONTCOLOR (color, BKGDCOLOR);

    PrintY = CST_Y + 13 * 8;
    for (i = 0; i < 4; i++)
        PrintCustKeybd (i);
}

void
PrintCustKeys (int i)
{
    PrintX = CST_START + CST_SPC * i;
    US_Print ((const char *) IN_GetScanName (dirscan[moveorder[i]]));
}

void
DrawCustKeys (int hilight)
{
    int i, color;


    color = TEXTCOLOR;
    if (hilight)
        color = HIGHLIGHT;
    SETFONTCOLOR (color, BKGDCOLOR);

    PrintY = CST_Y + 13 * 10;
    for (i = 0; i < 4; i++)
        PrintCustKeys (i);
}


////////////////////////////////////////////////////////////////////
//
// CHANGE SCREEN VIEWING SIZE
//
////////////////////////////////////////////////////////////////////
int
CP_ChangeView (int)
{
    int exit = 0, oldview, newview;
    ControlInfo ci;

    WindowX = WindowY = 0;
    WindowW = 320;
    WindowH = 200;
    newview = oldview = viewsize;
    DrawChangeView (oldview);
    MenuFadeIn ();

    do
    {
        CheckPause ();
        SDL_Delay(5);
        ReadAnyControl (&ci);
        switch (ci.dir)
        {
            case dir_South:
            case dir_West:
                newview--;
                if (newview < 4)
                    newview = 4;
                if(newview >= 19) DrawChangeView(newview);
                else ShowViewSize (newview);
                VW_UpdateScreen ();
                SD_PlaySound (HITWALLSND);
                TicDelay (10);
                break;

            case dir_North:
            case dir_East:
                newview++;
                if (newview >= 21)
                {
                    newview = 21;
                    DrawChangeView(newview);
                }
                else ShowViewSize (newview);
                VW_UpdateScreen ();
                SD_PlaySound (HITWALLSND);
                TicDelay (10);
                break;
        }

        if (ci.button0 || Keyboard[sc_Enter])
            exit = 1;
        else if (ci.button1 || Keyboard[sc_Escape])
        {
            SD_PlaySound (ESCPRESSEDSND);
            MenuFadeOut ();
            if(screenHeight % 200 != 0)
                VL_ClearScreen(0);
            return 0;
        }
    }
    while (!exit);

    if (oldview != newview)
    {
        SD_PlaySound (SHOOTSND);
        Message (STR_THINK "...");
        NewViewSize (newview);
    }

    ShootSnd ();
    MenuFadeOut ();
    if(screenHeight % 200 != 0)
        VL_ClearScreen(0);

    return 0;
}


/////////////////////////////
//
// DRAW THE CHANGEVIEW SCREEN
//
void
DrawChangeView (int view)
{
    int rescaledHeight = screenHeight / scaleFactor;
    if(view != 21) VWB_Bar (0, rescaledHeight - 40, 320, 40, bordercol);

#ifdef JAPAN
    CA_CacheScreen (S_CHANGEPIC);

    ShowViewSize (view);
#else
    ShowViewSize (view);

    PrintY = (screenHeight / scaleFactor) - 39;
    WindowX = 0;
    WindowY = 320;                                  // TODO: Check this!
    SETFONTCOLOR (HIGHLIGHT, BKGDCOLOR);

    US_CPrint (STR_SIZE1 "\n");
    US_CPrint (STR_SIZE2 "\n");
    US_CPrint (STR_SIZE3);
#endif
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
    int x;

    dir = fs_open("/vmu/a1", O_RDONLY | O_DIR);
    x = 0;

    strcpy(name, SaveName);
    while((dirent = fs_readdir(dir)) && x < 10)
    {
        for(int i=0; i<10; i++)
        {
            name[7] = '0' + i;
            if(!strcmp(name, dirent->name))
            {
                if(DC_LoadFromVMU(name) != -1)
                {
                    const int handle = open(name, O_RDONLY);
                    if (handle >= 0)
                    {
                        char temp[32];

                        SaveGamesAvail[i] = 1;
                        read(handle, temp, 32);
                        close(handle);
                        strcpy(&SaveGameNames[i][0], temp);
                        x++;
                    }
                    fs_unlink(name);
                }
            }
        }
    }

    fs_close(dir);
#else
    strcpy(name, SaveName);
    for(int i=0; i<10; i++)
    {
        name[7] = '0' + i;
        const int handle = open(name, O_RDONLY | O_BINARY);
        if (handle >= 0)
        {
            char temp[32];

            SaveGamesAvail[i] = 1;
            read(handle, temp, 32);
            close(handle);
            strcpy(&SaveGameNames[i][0], temp);
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
// Handle moving gun around a menu
//
////////////////////////////////////////////////////////////////////
int
HandleMenu (CP_iteminfo * item_i, CP_itemtype * items, void (*routine) (int w))
{
    char key;
    static int redrawitem = 1, lastitem = -1;
    int i, x, y, basey, exit, which, shape;
    int32_t lastBlinkTime, timer;
    ControlInfo ci;


    which = item_i->curpos;
    x = item_i->x & -8;
    basey = item_i->y - 2;
    y = basey + which * 13;

    VWB_DrawPic (x, y, C_CURSOR1PIC);
    SetTextColor (items + which, 1);
    if (redrawitem)
    {
        PrintX = item_i->x + item_i->indent;
        PrintY = item_i->y + which * 13;
        US_Print ((items + which)->string);
    }
    //
    // CALL CUSTOM ROUTINE IF IT IS NEEDED
    //
    if (routine)
        routine (which);
    VW_UpdateScreen ();

    shape = C_CURSOR1PIC;
    timer = 8;
    exit = 0;
    lastBlinkTime = GetTimeCount ();
    IN_ClearKeysDown ();


    do
    {
        //
        // CHANGE GUN SHAPE
        //
        if ((int32_t)GetTimeCount () - lastBlinkTime > timer)
        {
            lastBlinkTime = GetTimeCount ();
            if (shape == C_CURSOR1PIC)
            {
                shape = C_CURSOR2PIC;
                timer = 8;
            }
            else
            {
                shape = C_CURSOR1PIC;
                timer = 70;
            }
            VWB_DrawPic (x, y, shape);
            if (routine)
                routine (which);
            VW_UpdateScreen ();
        }
        else SDL_Delay(5);

        CheckPause ();

        //
        // SEE IF ANY KEYS ARE PRESSED FOR INITIAL CHAR FINDING
        //
        key = LastASCII;
        if (key)
        {
            int ok = 0;

            if (key >= 'a')
                key -= 'a' - 'A';

            for (i = which + 1; i < item_i->amount; i++)
                if ((items + i)->active && (items + i)->string[0] == key)
                {
                    EraseGun (item_i, items, x, y, which);
                    which = i;
                    DrawGun (item_i, items, x, &y, which, basey, routine);
                    ok = 1;
                    IN_ClearKeysDown ();
                    break;
                }

            //
            // DIDN'T FIND A MATCH FIRST TIME THRU. CHECK AGAIN.
            //
            if (!ok)
            {
                for (i = 0; i < which; i++)
                    if ((items + i)->active && (items + i)->string[0] == key)
                    {
                        EraseGun (item_i, items, x, y, which);
                        which = i;
                        DrawGun (item_i, items, x, &y, which, basey, routine);
                        IN_ClearKeysDown ();
                        break;
                    }
            }
        }

        //
        // GET INPUT
        //
        ReadAnyControl (&ci);
        switch (ci.dir)
        {
                ////////////////////////////////////////////////
                //
                // MOVE UP
                //
            case dir_North:

                EraseGun (item_i, items, x, y, which);

                //
                // ANIMATE HALF-STEP
                //
                if (which && (items + which - 1)->active)
                {
                    y -= 6;
                    DrawHalfStep (x, y);
                }

                //
                // MOVE TO NEXT AVAILABLE SPOT
                //
                do
                {
                    if (!which)
                        which = item_i->amount - 1;
                    else
                        which--;
                }
                while (!(items + which)->active);

                DrawGun (item_i, items, x, &y, which, basey, routine);
                //
                // WAIT FOR BUTTON-UP OR DELAY NEXT MOVE
                //
                TicDelay (20);
                break;

                ////////////////////////////////////////////////
                //
                // MOVE DOWN
                //
            case dir_South:

                EraseGun (item_i, items, x, y, which);
                //
                // ANIMATE HALF-STEP
                //
                if (which != item_i->amount - 1 && (items + which + 1)->active)
                {
                    y += 6;
                    DrawHalfStep (x, y);
                }

                do
                {
                    if (which == item_i->amount - 1)
                        which = 0;
                    else
                        which++;
                }
                while (!(items + which)->active);

                DrawGun (item_i, items, x, &y, which, basey, routine);

                //
                // WAIT FOR BUTTON-UP OR DELAY NEXT MOVE
                //
                TicDelay (20);
                break;
        }

        if (ci.button0 || Keyboard[sc_Space] || Keyboard[sc_Enter])
            exit = 1;

        if (ci.button1 && !Keyboard[sc_Alt] || Keyboard[sc_Escape])
            exit = 2;

    }
    while (!exit);


    IN_ClearKeysDown ();

    //
    // ERASE EVERYTHING
    //
    if (lastitem != which)
    {
        VWB_Bar (x - 1, y, 25, 16, BKGDCOLOR);
        PrintX = item_i->x + item_i->indent;
        PrintY = item_i->y + which * 13;
        US_Print ((items + which)->string);
        redrawitem = 1;
    }
    else
        redrawitem = 0;

    if (routine)
        routine (which);
    VW_UpdateScreen ();

    item_i->curpos = which;

    lastitem = which;
    switch (exit)
    {
        case 1:
            //
            // CALL THE ROUTINE
            //
            if ((items + which)->routine != NULL)
            {
                ShootSnd ();
                MenuFadeOut ();
                (items + which)->routine (0);
            }
            return which;

        case 2:
            SD_PlaySound (ESCPRESSEDSND);
            return -1;
    }

    return 0;                   // JUST TO SHUT UP THE ERROR MESSAGES!
}


//
// ERASE GUN & DE-HIGHLIGHT STRING
//
void
EraseGun (CP_iteminfo * item_i, CP_itemtype * items, int x, int y, int which)
{
    VWB_Bar (x - 1, y, 25, 16, BKGDCOLOR);
    SetTextColor (items + which, 0);

    PrintX = item_i->x + item_i->indent;
    PrintY = item_i->y + which * 13;
    US_Print ((items + which)->string);
    VW_UpdateScreen ();
}


//
// DRAW HALF STEP OF GUN TO NEXT POSITION
//
void
DrawHalfStep (int x, int y)
{
    VWB_DrawPic (x, y, C_CURSOR1PIC);
    VW_UpdateScreen ();
    SD_PlaySound (MOVEGUN1SND);
    SDL_Delay (8 * 100 / 7);
}


//
// DRAW GUN AT NEW POSITION
//
void
DrawGun (CP_iteminfo * item_i, CP_itemtype * items, int x, int *y, int which, int basey,
         void (*routine) (int w))
{
    VWB_Bar (x - 1, *y, 25, 16, BKGDCOLOR);
    *y = basey + which * 13;
    VWB_DrawPic (x, *y, C_CURSOR1PIC);
    SetTextColor (items + which, 1);

    PrintX = item_i->x + item_i->indent;
    PrintY = item_i->y + which * 13;
    US_Print ((items + which)->string);

    //
    // CALL CUSTOM ROUTINE IF IT IS NEEDED
    //
    if (routine)
        routine (which);
    VW_UpdateScreen ();
    SD_PlaySound (MOVEGUN2SND);
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
// Draw a menu
//
////////////////////////////////////////////////////////////////////
void
DrawMenu (CP_iteminfo * item_i, CP_itemtype * items)
{
    int i, which = item_i->curpos;


    WindowX = PrintX = item_i->x + item_i->indent;
    WindowY = PrintY = item_i->y;
    WindowW = 320;
    WindowH = 200;

    for (i = 0; i < item_i->amount; i++)
    {
        SetTextColor (items + i, which == i);

        PrintY = item_i->y + i * 13;
        if ((items + i)->active)
            US_Print ((items + i)->string);
        else
        {
            SETFONTCOLOR (DEACTIVE, BKGDCOLOR);
            US_Print ((items + i)->string);
            SETFONTCOLOR (TEXTCOLOR, BKGDCOLOR);
        }

        US_Print ("\n");
    }
}


////////////////////////////////////////////////////////////////////
//
// SET TEXT COLOR (HIGHLIGHT OR NO)
//
////////////////////////////////////////////////////////////////////
void
SetTextColor (CP_itemtype * items, int hlight)
{
    if (hlight)
    {
        SETFONTCOLOR (color_hlite[items->active], BKGDCOLOR);
    }
    else
    {
        SETFONTCOLOR (color_norml[items->active], BKGDCOLOR);
    }
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
//      IN_GetScanName() - Returns a string containing the name of the
//              specified scan code
//
///////////////////////////////////////////////////////////////////////////
const char *
IN_GetScanName (ScanCode scan)
{
/*    const char **p;
    ScanCode *s;

    for (s = ExtScanCodes, p = ExtScanNames; *s; p++, s++)
        if (*s == scan)
            return (*p);*/

    return (ScanNames[scan]);
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
// DRAW GUN CURSOR AT CORRECT POSITION IN MENU
//
///////////////////////////////////////////////////////////////////////////
void
DrawMenuGun (CP_iteminfo * iteminfo)
{
    int x, y;


    x = iteminfo->x;
    y = iteminfo->y + iteminfo->curpos * 13 - 2;
    VWB_DrawPic (x, y, C_CURSOR1PIC);
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
