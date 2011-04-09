// WL_MAIN.C

#ifdef _WIN32
	#include <io.h>
#else
	#include <unistd.h>
#endif

#include "wl_def.h"
#include "wl_menu.h"
#include "id_pm.h"
#include "id_sd.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#pragma hdrstop
#include "wl_atmos.h"
#include "m_classes.h"
#include "config.hpp"
#include "w_wad.h"
#include "language.h"
#include "textures/textures.h"
#include "c_cvars.h"
#include "thingdef.h"
#include "v_palette.h"
#include "v_video.h"
#include <SDL_syswm.h>

// Wad Code Stuff
TArray<FString> wadfiles;
void WL_AddFile(const char *file)
{
	wadfiles.Push(file);
/*	wadlist_t *wad = (wadlist_t *)malloc(sizeof(*wad) + strlen(file));

	*wadtail = wad;
	wad->next = NULL;
	strcpy(wad->name, file);
	wadtail = &wad->next;*/
}


/*
=============================================================================

							WOLFENSTEIN 3-D

						An Id Software production

							by John Carmack

=============================================================================
*/

extern byte signon[];

/*
=============================================================================

							LOCAL CONSTANTS

=============================================================================
*/


#define FOCALLENGTH     (0x5700l)               // in global coordinates

#define VIEWWIDTH       256                     // size of view window
#define VIEWHEIGHT      144

/*
=============================================================================

							GLOBAL VARIABLES

=============================================================================
*/

char    str[80];
int     dirangle[9] = {0,ANGLES/8,2*ANGLES/8,3*ANGLES/8,4*ANGLES/8,
					5*ANGLES/8,6*ANGLES/8,7*ANGLES/8,ANGLES};

//
// proejection variables
//
fixed    focallength;
unsigned screenofs;
int      viewscreenx, viewscreeny;
int      viewwidth;
int      viewheight;
short    centerx;
int      shootdelta;           // pixels away from centerx a target can be
fixed    scale;
int32_t  heightnumerator;


void    Quit (const char *error,...);

boolean startgame;
boolean loadedgame;
int     mouseadjustment;

char    configname[13]="config.";

//
// Command line parameter variables
//
boolean param_debugmode = false;
boolean param_nowait = false;
int     param_difficulty = 1;           // default is "normal"
int     param_tedlevel = -1;            // default is not to start a level
int     param_joystickindex = 0;

#if defined(_arch_dreamcast)
int     param_joystickhat = 0;
int     param_samplerate = 11025;       // higher samplerates result in "out of memory"
int     param_audiobuffer = 4096 / (44100 / param_samplerate);
#elif defined(GP2X)
int     param_joystickhat = -1;
int     param_samplerate = 11025;       // higher samplerates result in "out of memory"
int     param_audiobuffer = 128;
#else
int     param_joystickhat = -1;
int     param_samplerate = 44100;
int     param_audiobuffer = 2048 / (44100 / param_samplerate);
#endif

int     param_mission = 1;
boolean param_goodtimes = false;
boolean param_ignorenumchunks = false;

/*
=============================================================================

							LOCAL VARIABLES

=============================================================================
*/





//===========================================================================

/*
=====================
=
= NewGame
=
= Set up new game to start from the beginning
=
=====================
*/

void NewGame (int difficulty,int episode)
{
	memset (&gamestate,0,sizeof(gamestate));
	gamestate.difficulty = difficulty;
	gamestate.weapon = gamestate.bestweapon
			= gamestate.chosenweapon = wp_pistol;
	gamestate.health = 100;
	gamestate.ammo = STARTAMMO;
	gamestate.lives = 3;
	gamestate.nextextra = EXTRAPOINTS;
	gamestate.episode=episode;

	startgame = true;
}

//===========================================================================

void DiskFlopAnim(int x,int y)
{
	static int8_t which=0;
	if (!x && !y)
		return;
	VWB_DrawPic(x,y,which == 0 ? "M_LDING1" : "M_LDING2");
	VW_UpdateScreen();
	which^=1;
}


int32_t DoChecksum(byte *source,unsigned size,int32_t checksum)
{
	unsigned i;

	for (i=0;i<size-1;i++)
	checksum += source[i]^source[i+1];

	return checksum;
}


/*
==================
=
= SaveTheGame
=
==================
*/

extern statetype s_grdstand;
extern statetype s_player;

boolean SaveTheGame(FILE *file,int x,int y)
{
#if 0
//    struct diskfree_t dfree;
//    int32_t avail,size,checksum;
	int checksum;
	objtype *ob;
	objtype nullobj;
	statobj_t nullstat;

/*    if (_dos_getdiskfree(0,&dfree))
		Quit("Error in _dos_getdiskfree call");

	avail = (int32_t)dfree.avail_clusters *
				dfree.bytes_per_sector *
				dfree.sectors_per_cluster;

	size = 0;
	for (ob = player; ob ; ob=ob->next)
		size += sizeof(*ob);
	size += sizeof(nullobj);

	size += sizeof(gamestate) +
			sizeof(LRstruct)*LRpack +
			sizeof(tilemap) +
			sizeof(actorat) +
			sizeof(laststatobj) +
			sizeof(statobjlist) +
			sizeof(doorposition) +
			sizeof(pwallstate) +
			sizeof(pwalltile) +
			sizeof(pwallx) +
			sizeof(pwally) +
			sizeof(pwalldir) +
			sizeof(pwallpos);

	if (avail < size)
	{
		Message(language["STR_NOSPACE"]);
		return false;
	}*/

	checksum = 0;

	DiskFlopAnim(x,y);
	fwrite(&gamestate,sizeof(gamestate),1,file);
	checksum = DoChecksum((byte *)&gamestate,sizeof(gamestate),checksum);

	DiskFlopAnim(x,y);
	fwrite(&LevelRatios[0],sizeof(LRstruct)*LRpack,1,file);
	checksum = DoChecksum((byte *)&LevelRatios[0],sizeof(LRstruct)*LRpack,checksum);

	DiskFlopAnim(x,y);
	fwrite(tilemap,sizeof(tilemap),1,file);
	checksum = DoChecksum((byte *)tilemap,sizeof(tilemap),checksum);
	DiskFlopAnim(x,y);

	int i;
	for(i=0;i<MAPSIZE;i++)
	{
		for(int j=0;j<MAPSIZE;j++)
		{
			word actnum;
			objtype *objptr=actorat[i][j];
			if(ISPOINTER(objptr))
				actnum=0x8000 | (word)(objptr-objlist);
			else
				actnum=(word)(uintptr_t)objptr;
			fwrite(&actnum,sizeof(actnum),1,file);
			checksum = DoChecksum((byte *)&actnum,sizeof(actnum),checksum);
		}
	}

	fwrite (areaconnect,sizeof(areaconnect),1,file);
	fwrite (areabyplayer,sizeof(areabyplayer),1,file);

	// player object needs special treatment as it's in WL_AGENT.CPP and not in
	// WL_ACT2.CPP which could cause problems for the relative addressing

	ob = player;
	DiskFlopAnim(x,y);
	memcpy(&nullobj,ob,sizeof(nullobj));
	nullobj.state=(statetype *) ((uintptr_t)nullobj.state-(uintptr_t)&s_player);
	fwrite(&nullobj,sizeof(nullobj),1,file);
	ob = ob->next;

	DiskFlopAnim(x,y);
	for (; ob ; ob=ob->next)
	{
		memcpy(&nullobj,ob,sizeof(nullobj));
		nullobj.state=(statetype *) ((uintptr_t)nullobj.state-(uintptr_t)&s_grdstand);
		fwrite(&nullobj,sizeof(nullobj),1,file);
	}
	nullobj.active = ac_badobject;          // end of file marker
	DiskFlopAnim(x,y);
	fwrite(&nullobj,sizeof(nullobj),1,file);

	DiskFlopAnim(x,y);
	word laststatobjnum=(word) (laststatobj-statobjlist);
	fwrite(&laststatobjnum,sizeof(laststatobjnum),1,file);
	checksum = DoChecksum((byte *)&laststatobjnum,sizeof(laststatobjnum),checksum);

	DiskFlopAnim(x,y);
	for(i=0;i<MAXSTATS;i++)
	{
		memcpy(&nullstat,statobjlist+i,sizeof(nullstat));
		nullstat.visspot=(byte *) ((uintptr_t) nullstat.visspot-(uintptr_t)spotvis);
		fwrite(&nullstat,sizeof(nullstat),1,file);
		checksum = DoChecksum((byte *)&nullstat,sizeof(nullstat),checksum);
	}

	DiskFlopAnim(x,y);
	fwrite (doorposition,sizeof(doorposition),1,file);
	checksum = DoChecksum((byte *)doorposition,sizeof(doorposition),checksum);
	DiskFlopAnim(x,y);
	fwrite (doorobjlist,sizeof(doorobjlist),1,file);
	checksum = DoChecksum((byte *)doorobjlist,sizeof(doorobjlist),checksum);

	DiskFlopAnim(x,y);
	fwrite (&pwallstate,sizeof(pwallstate),1,file);
	checksum = DoChecksum((byte *)&pwallstate,sizeof(pwallstate),checksum);
	fwrite (&pwalltile,sizeof(pwalltile),1,file);
	checksum = DoChecksum((byte *)&pwalltile,sizeof(pwalltile),checksum);
	fwrite (&pwallx,sizeof(pwallx),1,file);
	checksum = DoChecksum((byte *)&pwallx,sizeof(pwallx),checksum);
	fwrite (&pwally,sizeof(pwally),1,file);
	checksum = DoChecksum((byte *)&pwally,sizeof(pwally),checksum);
	fwrite (&pwalldir,sizeof(pwalldir),1,file);
	checksum = DoChecksum((byte *)&pwalldir,sizeof(pwalldir),checksum);
	fwrite (&pwallpos,sizeof(pwallpos),1,file);
	checksum = DoChecksum((byte *)&pwallpos,sizeof(pwallpos),checksum);

	//
	// WRITE OUT CHECKSUM
	//
	fwrite (&checksum,sizeof(checksum),1,file);

	fwrite (&lastgamemusicoffset,sizeof(lastgamemusicoffset),1,file);

	return(true);
#endif
}

//===========================================================================

/*
==================
=
= LoadTheGame
=
==================
*/

boolean LoadTheGame(FILE *file,int x,int y)
{
#if 0
	int32_t checksum,oldchecksum;
	objtype nullobj;
	statobj_t nullstat;

	checksum = 0;

	DiskFlopAnim(x,y);
	fread (&gamestate,sizeof(gamestate),1,file);
	checksum = DoChecksum((byte *)&gamestate,sizeof(gamestate),checksum);

	DiskFlopAnim(x,y);
	fread (&LevelRatios[0],sizeof(LRstruct)*LRpack,1,file);
	checksum = DoChecksum((byte *)&LevelRatios[0],sizeof(LRstruct)*LRpack,checksum);

	DiskFlopAnim(x,y);
	SetupGameLevel ();

	DiskFlopAnim(x,y);
	fread (tilemap,sizeof(tilemap),1,file);
	checksum = DoChecksum((byte *)tilemap,sizeof(tilemap),checksum);

	DiskFlopAnim(x,y);

	int actnum=0, i;
	for(i=0;i<MAPSIZE;i++)
	{
		for(int j=0;j<MAPSIZE;j++)
		{
			fread (&actnum,sizeof(word),1,file);
			checksum = DoChecksum((byte *) &actnum,sizeof(word),checksum);
			if(actnum&0x8000)
				actorat[i][j]=objlist+(actnum&0x7fff);
			else
				actorat[i][j]=(objtype *)(uintptr_t) actnum;
		}
	}

	fread (areaconnect,sizeof(areaconnect),1,file);
	fread (areabyplayer,sizeof(areabyplayer),1,file);

	InitActorList ();
	DiskFlopAnim(x,y);
	fread (player,sizeof(*player),1,file);
	player->state=(statetype *) ((uintptr_t)player->state+(uintptr_t)&s_player);

	while (1)
	{
		DiskFlopAnim(x,y);
		fread (&nullobj,sizeof(nullobj),1,file);
		if (nullobj.active == ac_badobject)
			break;
		GetNewActor ();
		nullobj.state=(statetype *) ((uintptr_t)nullobj.state+(uintptr_t)&s_grdstand);
		// don't copy over the links
		memcpy (newobj,&nullobj,sizeof(nullobj)-8);
	}

	DiskFlopAnim(x,y);
	word laststatobjnum;
	fread (&laststatobjnum,sizeof(laststatobjnum),1,file);
	laststatobj=statobjlist+laststatobjnum;
	checksum = DoChecksum((byte *)&laststatobjnum,sizeof(laststatobjnum),checksum);

	DiskFlopAnim(x,y);
	for(i=0;i<MAXSTATS;i++)
	{
		fread(&nullstat,sizeof(nullstat),1,file);
		checksum = DoChecksum((byte *)&nullstat,sizeof(nullstat),checksum);
		nullstat.visspot=(byte *) ((uintptr_t)nullstat.visspot+(uintptr_t)spotvis);
		memcpy(statobjlist+i,&nullstat,sizeof(nullstat));
	}

	DiskFlopAnim(x,y);
	fread (doorposition,sizeof(doorposition),1,file);
	checksum = DoChecksum((byte *)doorposition,sizeof(doorposition),checksum);
	DiskFlopAnim(x,y);
	fread (doorobjlist,sizeof(doorobjlist),1,file);
	checksum = DoChecksum((byte *)doorobjlist,sizeof(doorobjlist),checksum);

	DiskFlopAnim(x,y);
	fread (&pwallstate,sizeof(pwallstate),1,file);
	checksum = DoChecksum((byte *)&pwallstate,sizeof(pwallstate),checksum);
	fread (&pwalltile,sizeof(pwalltile),1,file);
	checksum = DoChecksum((byte *)&pwalltile,sizeof(pwalltile),checksum);
	fread (&pwallx,sizeof(pwallx),1,file);
	checksum = DoChecksum((byte *)&pwallx,sizeof(pwallx),checksum);
	fread (&pwally,sizeof(pwally),1,file);
	checksum = DoChecksum((byte *)&pwally,sizeof(pwally),checksum);
	fread (&pwalldir,sizeof(pwalldir),1,file);
	checksum = DoChecksum((byte *)&pwalldir,sizeof(pwalldir),checksum);
	fread (&pwallpos,sizeof(pwallpos),1,file);
	checksum = DoChecksum((byte *)&pwallpos,sizeof(pwallpos),checksum);

	if (gamestate.secretcount)      // assign valid floorcodes under moved pushwalls
	{
		word *map, *obj; word tile, sprite;
		map = mapsegs[0]; obj = mapsegs[1];
		for (y=0;y<mapheight;y++)
			for (x=0;x<mapwidth;x++)
			{
				tile = *map++; sprite = *obj++;
				if (sprite == PUSHABLETILE && !tilemap[x][y]
					&& (tile < AREATILE || tile >= (AREATILE+NUMMAPS)))
				{
					if (*map >= AREATILE)
						tile = *map;
					if (*(map-1-mapwidth) >= AREATILE)
						tile = *(map-1-mapwidth);
					if (*(map-1+mapwidth) >= AREATILE)
						tile = *(map-1+mapwidth);
					if ( *(map-2) >= AREATILE)
						tile = *(map-2);

					*(map-1) = tile; *(obj-1) = 0;
				}
			}
	}

	Thrust(0,0);    // set player->areanumber to the floortile you're standing on

	fread (&oldchecksum,sizeof(oldchecksum),1,file);

	fread (&lastgamemusicoffset,sizeof(lastgamemusicoffset),1,file);
	if(lastgamemusicoffset<0) lastgamemusicoffset=0;


	if (oldchecksum != checksum)
	{
		Message(language["STR_SAVECHT"]);

		IN_ClearKeysDown();
		IN_Ack();

		gamestate.oldscore = gamestate.score = 0;
		gamestate.lives = 1;
		gamestate.weapon =
			gamestate.chosenweapon =
			gamestate.bestweapon = wp_pistol;
		gamestate.ammo = 8;
	}

	return true;
#endif
}

//===========================================================================

/*
==========================
=
= ShutdownId
=
= Shuts down all ID_?? managers
=
==========================
*/

void ShutdownId (void)
{
	US_Shutdown ();         // This line is completely useless...
	SD_Shutdown ();
	PM_Shutdown ();
	IN_Shutdown ();
	VW_Shutdown ();
	CA_Shutdown ();
#if defined(GP2X)
	GP2X_Shutdown();
#endif
}


//===========================================================================

/*
==================
=
= BuildTables
=
= Calculates:
=
= scale                 projection constant
= sintable/costable     overlapping fractional tables
=
==================
*/

const float radtoint = (float)(FINEANGLES/2/PI);

void BuildTables (void)
{
	//
	// calculate fine tangents
	//

	int i;
	for(i=0;i<FINEANGLES/8;i++)
	{
		double tang=tan((i+0.5)/radtoint);
		finetangent[i]=(int32_t)(tang*GLOBAL1);
		finetangent[FINEANGLES/4-1-i]=(int32_t)((1/tang)*GLOBAL1);
	}

	//
	// costable overlays sintable with a quarter phase shift
	// ANGLES is assumed to be divisable by four
	//

	float angle=0;
	float anglestep=(float)(PI/2/ANGLEQUAD);
	for(i=0; i<ANGLEQUAD; i++)
	{
		fixed value=(int32_t)(GLOBAL1*sin(angle));
		sintable[i]=sintable[i+ANGLES]=sintable[ANGLES/2-i]=value;
		sintable[ANGLES-i]=sintable[ANGLES/2+i]=-value;
		angle+=anglestep;
	}
	angle = 0;
	anglestep = (float)(PI/2/ANG90);
	for(i=0; i<FINEANGLES; i++)
	{
		finesine[i]=fixed(GLOBAL1*sin(angle));
		angle+=anglestep;
	}
	sintable[ANGLEQUAD] = 65536;
	sintable[3*ANGLEQUAD] = -65536;

#if defined(USE_STARSKY) || defined(USE_RAIN) || defined(USE_SNOW)
	Init3DPoints();
#endif
}

//===========================================================================


/*
====================
=
= CalcProjection
=
= Uses focallength
=
====================
*/

void CalcProjection (int32_t focal)
{
	int     i;
	int    intang;
	float   angle;
	double  tang;
	int     halfview;
	double  facedist;

	focallength = focal;
	facedist = focal+MINDIST;
	halfview = viewwidth/2;                                 // half view in pixels

	//
	// calculate scale value for vertical height calculations
	// and sprite x calculations
	//
	scale = (fixed) (halfview*facedist/(AspectCorrection[vid_aspect].viewGlobal/2));

	//
	// divide heightnumerator by a posts distance to get the posts height for
	// the heightbuffer.  The pixel height is height>>2
	//
	heightnumerator = CorrectHeightFactor((TILEGLOBAL*scale)>>6);

	//
	// calculate the angle offset from view angle of each pixel's ray
	//

	for (i=0;i<halfview;i++)
	{
		// start 1/2 pixel over, so viewangle bisects two middle pixels
		tang = (int32_t)i*AspectCorrection[vid_aspect].viewGlobal/viewwidth/facedist;
		angle = (float) atan(tang);
		intang = (int) (angle*radtoint);
		pixelangle[halfview-1-i] = intang;
		pixelangle[halfview+i] = -intang;
	}
}



//===========================================================================

/*
===================
=
= SetupWalls
=
= Map tile values to scaled pics
=
===================
*/

void SetupWalls (void)
{
	int     i;

	horizwall[0]=0;
	vertwall[0]=0;

	for (i=1;i<MAXWALLTILES;i++)
	{
		horizwall[i]=(i-1)*2;
		vertwall[i]=(i-1)*2+1;
	}
}

//===========================================================================

/*
==========================
=
= SignonScreen
=
==========================
*/

void SignonScreen (void)                        // VGA version
{
	VL_SetVGAPlaneMode ();
	CA_CacheScreen("WLFSGNON");
}


/*
==========================
=
= FinishSignon
=
==========================
*/

void FinishSignon (void)
{
#ifndef SPEAR
	VW_Bar (0,189,300,11,VL_GetPixel(0,0));
	WindowX = 0;
	WindowW = 320;
	PrintY = 190;

	#ifndef JAPAN
	SETFONTCOLOR(14,4);

	#ifdef SPANISH
	US_CPrint ("Oprima una tecla");
	#else
	US_CPrint ("Press a key");
	#endif

	#endif

	VH_UpdateScreen();

	if (!param_nowait)
		IN_Ack ();

	#ifndef JAPAN
	VW_Bar (0,189,300,11,VL_GetPixel(0,0));

	PrintY = 190;
	SETFONTCOLOR(10,4);

	#ifdef SPANISH
	US_CPrint ("pensando...");
	#else
	US_CPrint ("Working...");
	#endif

	VH_UpdateScreen();
	#endif

	SETFONTCOLOR(0,15);
#else
	VH_UpdateScreen();

	if (!param_nowait)
		VW_WaitVBL(3*70);
#endif
}

//===========================================================================

Menu musicMenu(CTL_X, CTL_Y-6, 280, 32);
static struct Song
{
	const char*		name;
	const char*		track;
} songList[] =
{
#ifndef SPEAR
	{"Get Them!",			"GETTHEM"},
	{"Searching",			"SEARCHN"},
	{"P.O.W.",				"POW"},
	{"Suspense",			"SUSPENSE"},
	{"War March",			"WARMARCH"},
	{"Around The Corner!",	"CORNER"},
	{"Nazi Anthem (Title)",	"NAZI_NOR"},
	{"Nazi Anthem",			"NAZI_OMI"},
	{"Lurking...",			"PREGNANT"},
	{"Going After Hitler",	"GOINGAFT"},
	{"Pounding Headache",	"HEADACHE"},
	{"Into the Dungeons",	"DUNGEON"},
	{"Kill the S.O.B.",		"INTROCW3"},
	{"The Nazi Rap",		"NAZI_RAP"},
	{"Twelfth Hour",		"TWELFTH"},
	{"Zero Hour",			"ZEROHOUR"},
	{"Ultimate Conquest",	"ULTIMATE"},
	{"Wolfpack",			"PACMAN"},
	{"Hitler Waltz",		"HITLWLTZ"},
	{"Salute",				"SALUTE"},
	{"Victors",				"VICTORS"},
	{"Wondering About My...",	"WONDERIN"},
	{"Funk You",			"FUNKYOU"},
	{"Intermission",		"ENDLEVEL"},
	{"Roster",				"ROSTER"},
	{"You're a Hero",		"URAHERO"},
	{"Victory March",		"VICMARCH"},
#else
	{"Funky Colonel Bill",		"XFUNKIE"},
	{"Death To The Nazis",		"XDEATH"},
	{"Tiptoeing Around",		"XTIPTOE"},
	{"Is This THE END?",		"XTHEEND"},
	{"Evil Incarnate",			"XEVIL"},
	{"Jazzin' Them Nazis",		"XJAZNAZI"},
	{"Puttin' It To The Enemy",	"XPUTIT"},
	{"The SS Gonna Get You",	"XGETYOU"},
	{"Towering Above",			"XTOWER2"},
#endif

	// End of list
	{ NULL, NULL }
};
MENU_LISTENER(ChangeMusic)
{
	StartCPMusic(songList[which].track);
	for(unsigned int i = 0;songList[i].name != NULL;i++)
	{
		musicMenu[i]->setHighlighted(i == which);
	}
	musicMenu.draw();
}

#ifndef SPEARDEMO
void DoJukebox(void)
{
	IN_ClearKeysDown();
	if (!AdLibPresent && !SoundBlasterPresent)
		return;

	MenuFadeOut();

	fontnumber=1;
	ClearMScreen ();
	musicMenu.setHeadText("Robert's Jukebox", true);
	for(unsigned int i = 0;songList[i].name != NULL;i++)
		musicMenu.addItem(new MenuItem(songList[i].name, ChangeMusic));
	musicMenu.show();
	return;
}
#endif

/*
==========================
=
= InitGame
=
= Load a few things right away
=
==========================
*/

static void InitGame()
{
#ifndef SPEARDEMO
	boolean didjukebox=false;
#endif

	// initialize SDL
#if defined _WIN32
	putenv("SDL_VIDEODRIVER=directx");
#endif
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
	{
		printf("Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	int numJoysticks = SDL_NumJoysticks();
	if(param_joystickindex && (param_joystickindex < -1 || param_joystickindex >= numJoysticks))
	{
		if(!numJoysticks)
			printf("No joysticks are available to SDL!\n");
		else
			printf("The joystick index must be between -1 and %i!\n", numJoysticks - 1);
		exit(1);
	}

	SignonScreen ();

#if defined _WIN32
	if(!fullscreen)
	{
		struct SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);

		if(SDL_GetWMInfo(&wmInfo) != -1)
		{
			HWND hwndSDL = wmInfo.window;
			DWORD style = GetWindowLong(hwndSDL, GWL_STYLE) & ~WS_SYSMENU;
			SetWindowLong(hwndSDL, GWL_STYLE, style);
			SetWindowPos(hwndSDL, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		}
	}
#endif

	GenerateLookupTables();
	VH_Startup ();
	IN_Startup ();
	PM_Startup ();
	SD_Startup ();
	printf("CA_Startup: Starting the Cache Manager.\n");
	CA_Startup ();
	printf("US_Startup: Starting the User Manager.\n");
	US_Startup ();

	// TODO: Will any memory checking be needed someday??
#ifdef NOTYET
#ifndef SPEAR
	if (mminfo.mainmem < 235000L)
#else
	if (mminfo.mainmem < 257000L && !MS_CheckParm("debugmode"))
#endif
	{
		byte *screen;

		CA_CacheGrChunk (ERRORSCREEN);
		screen = grsegs[ERRORSCREEN];
		ShutdownId();
/*        memcpy((byte *)0xb8000,screen+7+7*160,17*160);
		gotoxy (1,23);*/
		exit(1);
	}
#endif


//
// build some tables
//

	printf("SetupSaveGames: Looking for saves.\n");
	SetupSaveGames();

//
// Load Actors
//

	ClassDef::LoadActors();

//
// Init texture manager
//

	TexMan.Init();

//
// draw intro screen stuff
//
	IntroScreen ();

//
// load in and lock down some basic chunks
//

	LoadLatchMem ();
	BuildTables ();          // trig tables
	SetupWalls ();

	NewViewSize (viewsize);

//
// initialize variables
//
	InitRedShifts ();
#ifndef SPEARDEMO
	if(!didjukebox)
#endif
		FinishSignon();

//
// HOLDING DOWN 'M' KEY?
//
#ifndef SPEARDEMO
	if (Keyboard[sc_M])
	{
		DoJukebox();
		didjukebox=true;
	}
#endif

#ifdef NOTYET
	vdisp = (byte *) (0xa0000+PAGE1START);
	vbuf = (byte *) (0xa0000+PAGE2START);
#endif
}

//===========================================================================

/*
==========================
=
= SetViewSize
=
==========================
*/

boolean SetViewSize (unsigned width, unsigned height)
{
	viewwidth = width&~15;                  // must be divisable by 16
	viewheight = height&~1;                 // must be even
	centerx = viewwidth/2-1;
	shootdelta = viewwidth/10;
	if((unsigned) viewheight == screenHeight)
		viewscreenx = viewscreeny = screenofs = 0;
	else
	{
		viewscreenx = (screenWidth-viewwidth) / 2;
		viewscreeny = (screenHeight-scaleFactor*STATUSLINES-viewheight)/2;
		screenofs = viewscreeny*screenWidth+viewscreenx;
	}

//
// calculate trace angles and projection constants
//
	CalcProjection (FOCALLENGTH);

	return true;
}


void ShowViewSize (int width)
{
	int oldwidth,oldheight;

	oldwidth = viewwidth;
	oldheight = viewheight;

	if(width == 21)
	{
		viewwidth = screenWidth;
		viewheight = screenHeight;
		VWB_BarScaledCoord (0, 0, screenWidth, screenHeight, 0);
	}
	else if(width == 20)
	{
		viewwidth = screenWidth;
		viewheight = screenHeight - scaleFactor*STATUSLINES;
		DrawPlayBorder ();
	}
	else
	{
		viewwidth = width*16*screenWidth/320;
		viewheight = (int) (width*16*HEIGHTRATIO*screenHeight/200);
		DrawPlayBorder ();
	}

	viewwidth = oldwidth;
	viewheight = oldheight;
}


void NewViewSize (int width)
{
	if(width < 4 || width > 21)
		return;

	viewsize = width;
	if(viewsize == 21)
		SetViewSize(screenWidth, screenHeight);
	else if(viewsize == 20)
		SetViewSize(screenWidth, screenHeight - scaleFactor * STATUSLINES);
	else
		SetViewSize(width*16*screenWidth/320, (unsigned) (width*16*HEIGHTRATIO*screenHeight/200));
}



//===========================================================================

/*
==========================
=
= Quit
=
==========================
*/

void Quit (const char *errorStr, ...)
{
#ifdef NOTYET
	byte *screen;
#endif
	char error[256];
	if(errorStr != NULL)
	{
		va_list vlist;
		va_start(vlist, errorStr);
		vsprintf(error, errorStr, vlist);
		va_end(vlist);
	}
	else error[0] = 0;

/*    if (!pictable)  // don't try to display the red box before it's loaded
	{
		ShutdownId();
		if (error && *error)
		{
#ifdef NOTYET
			SetTextCursor(0,0);
#endif
			puts(error);
#ifdef NOTYET
			SetTextCursor(0,2);
#endif
			VW_WaitVBL(100);
		}
		exit(1);
	}*/

	if (error[0] == 0)
	{
#ifdef NOTYET
		#ifndef JAPAN
		CA_CacheGrChunk (ORDERSCREEN);
		screen = grsegs[ORDERSCREEN];
		#endif
#endif

		WriteConfig ();
	}
#ifdef NOTYET
	else
	{
		CA_CacheGrChunk (ERRORSCREEN);
		screen = grsegs[ERRORSCREEN];
	}
#endif

	ShutdownId ();

	if (error[0] != 0)
	{
#ifdef NOTYET
		memcpy((byte *)0xb8000,screen+7,7*160);
		SetTextCursor(9,3);
#endif
		puts(error);
#ifdef NOTYET
		SetTextCursor(0,7);
#endif
		VW_WaitVBL(200);
		exit(1);
	}
	else
	if (error[0] == 0)
	{
#ifdef NOTYET
		#ifndef JAPAN
		memcpy((byte *)0xb8000,screen+7,24*160); // 24 for SPEAR/UPLOAD compatibility
		#endif
		SetTextCursor(0,23);
#endif
	}

	exit(0);
}

//===========================================================================



/*
=====================
=
= DemoLoop
=
=====================
*/


static void DemoLoop()
{
	int LastDemo = 0;

//
// check for launch from ted
//
	if (param_tedlevel != -1)
	{
		param_nowait = true;
		NewGame(param_difficulty,0);

#ifndef SPEAR
		gamestate.episode = param_tedlevel/10;
		gamestate.mapon = param_tedlevel%10;
#else
		gamestate.episode = 0;
		gamestate.mapon = param_tedlevel;
#endif
		GameLoop();
		Quit (NULL);
	}


//
// main game cycle
//

#ifndef DEMOTEST

	#ifndef UPLOAD

		#ifndef GOODTIMES
		#ifndef SPEAR
		#ifndef JAPAN
		if (!param_nowait)
			NonShareware();
		#endif
		#endif
		#endif
	#endif

	StartCPMusic(INTROSONG);

#ifndef JAPAN
	if (!param_nowait)
		PG13 ();
#endif

#endif

	while (1)
	{
		while (!param_nowait)
		{
//
// title page
//
#ifndef DEMOTEST

#ifdef SPEAR
			SDL_Color pal[256];
			VL_ConvertPalette("TITLEPAL", pal, 256);

			VWB_DrawPic (0,0,"TITLE1");

			VWB_DrawPic (0,80,"TITLE2");
			VW_UpdateScreen ();
			VL_FadeIn(0,255,pal,30);
#else
			CA_CacheScreen ("TITLEPIC");
			VW_UpdateScreen ();
			VW_FadeIn();
#endif
			if (IN_UserInput(TickBase*15))
				break;
			VW_FadeOut();
//
// credits page
//
			CA_CacheScreen ("CREDITS");
			VW_UpdateScreen();
			VW_FadeIn ();
			if (IN_UserInput(TickBase*10))
				break;
			VW_FadeOut ();
//
// high scores
//
			DrawHighScores ();
			VW_UpdateScreen ();
			VW_FadeIn ();

			if (IN_UserInput(TickBase*10))
				break;
#endif
//
// demo
//

			bool demoPlayed = false;
			do // This basically loops twice at most.  If the lump exists it plays the demo if not it goes to DEMO0.
			{  // PlayDemo will actually play the demo picked if it exists otherwise it will immediately return.
				char demoName[9];
				sprintf(demoName, "DEMO%d", LastDemo);
				if(Wads.CheckNumForName(demoName) == -1)
				{
					if(LastDemo == 0)
						break;
					else
						LastDemo = 0;
					continue;
				}
				else
				{
					demoPlayed = true;
					PlayDemo(LastDemo++);
					break;
				}
			}
			while(true);

			if (playstate == ex_abort)
				break;
			VW_FadeOut();
			if(screenHeight % 200 != 0)
				VL_ClearScreen(0);
			if(demoPlayed)
				StartCPMusic(INTROSONG);
		}

		VW_FadeOut ();

#ifdef DEBUGKEYS
		if (Keyboard[sc_Tab] && param_debugmode)
			RecordDemo ();
		else
			US_ControlPanel (0);
#else
		US_ControlPanel (0);
#endif

		if (startgame || loadedgame)
		{
			GameLoop ();
			if(!param_nowait)
			{
				VW_FadeOut();
				StartCPMusic(INTROSONG);
			}
		}
	}
}


//===========================================================================

// CheckRatio -- From ZDoom
//
// Tries to guess the physical dimensions of the screen based on the
// screen's pixel dimensions.
Aspect CheckRatio (int width, int height)//, int *trueratio)
{
	//int fakeratio = -1;
	Aspect ratio;

	/*if ((vid_aspect >=1) && (vid_aspect <=4))
	{
		// [SP] User wants to force aspect ratio; let them.
		fakeratio = vid_aspect == 3? 0: int(vid_aspect);
	}*/
	/*if (vid_nowidescreen)
	{
		if (!vid_tft)
		{
			fakeratio = 0;
		}
		else
		{
			fakeratio = (height * 5/4 == width) ? 4 : 0;
		}
	}*/
	// If the size is approximately 16:9, consider it so.
	if (abs (height * 16/9 - width) < 10)
	{
		ratio = ASPECT_16_9;
	}
	// 16:10 has more variance in the pixel dimensions. Grr.
	else if (abs (height * 16/10 - width) < 60)
	{
		// 320x200 and 640x400 are always 4:3, not 16:10
		if ((width == 320 && height == 200) || (width == 640 && height == 400))
		{
			ratio = ASPECT_NONE;
		}
		else
		{
			ratio = ASPECT_16_10;
		}
	}
	// Unless vid_tft is set, 1280x1024 is 4:3, not 5:4.
	else if (height * 5/4 == width)// && vid_tft)
	{
		ratio = ASPECT_5_4;
	}
	// Assume anything else is 4:3.
	else
	{
		ratio = ASPECT_4_3;
	}

	/*if (trueratio != NULL)
	{
		*trueratio = ratio;
	}*/
	return /*(fakeratio >= 0) ? fakeratio :*/ ratio;
}

#define IFARG(str) if(!strcmp(arg, (str)))

void CheckParameters(int argc, char *argv[])
{
	bool hasError = false, showHelp = false;
	bool sampleRateGiven = false, audioBufferGiven = false;
	int defaultSampleRate = param_samplerate;
	bool needRatio = true;

	fullscreen = vid_fullscreen;

	for(int i = 1; i < argc; i++)
	{
		char *arg = argv[i];
#ifndef SPEAR
		IFARG("--goobers")
#else
		IFARG("--debugmode")
#endif
			param_debugmode = true;
		else IFARG("--baby")
			param_difficulty = 0;
		else IFARG("--easy")
			param_difficulty = 1;
		else IFARG("--normal")
			param_difficulty = 2;
		else IFARG("--hard")
			param_difficulty = 3;
		else IFARG("--nowait")
			param_nowait = true;
		else IFARG("--tedlevel")
		{
			if(++i >= argc)
			{
				printf("The tedlevel option is missing the level argument!\n");
				hasError = true;
			}
			else param_tedlevel = atoi(argv[i]);
		}
		else IFARG("--windowed")
			fullscreen = false;
		else IFARG("--res")
		{
			if(i + 2 >= argc)
			{
				printf("The res option needs the width and/or the height argument!\n");
				hasError = true;
			}
			else
			{
				screenWidth = atoi(argv[++i]);
				screenHeight = atoi(argv[++i]);
				unsigned factor = screenWidth / 320;
				if(screenWidth % 320 || screenHeight != 200 * factor && screenHeight != 240 * factor)
					printf("Screen size must be a multiple of 320x200 or 320x240!\n"), hasError = true;
			}
		}
		else IFARG("--resf")
		{
			if(i + 2 >= argc)
			{
				printf("The resf option needs the width and/or the height argument!\n");
				hasError = true;
			}
			else
			{
				screenWidth = atoi(argv[++i]);
				screenHeight = atoi(argv[++i]);
				if(screenWidth < 320)
					printf("Screen width must be at least 320!\n"), hasError = true;
				if(screenHeight < 200)
					printf("Screen height must be at least 200!\n"), hasError = true;
			}
		}
		else IFARG("--aspect")
		{
			const char* ratio = argv[++i];
			if(strcmp(ratio, "4:3") == 0)
				vid_aspect = ASPECT_4_3;
			else if(strcmp(ratio, "16:10") == 0)
				vid_aspect = ASPECT_16_10;
			else if(strcmp(ratio, "16:9") == 0)
				vid_aspect = ASPECT_16_9;
			else if(strcmp(ratio, "5:4") == 0)
				vid_aspect = ASPECT_5_4;
			else
			{
				printf("Unknown aspect ratio %s!\n", ratio);
				hasError = true;
			}
			needRatio = false;
		}
		else IFARG("--bits")
		{
			if(++i >= argc)
			{
				printf("The bits option is missing the color depth argument!\n");
				hasError = true;
			}
			else
			{
				screenBits = atoi(argv[i]);
				switch(screenBits)
				{
					case 8:
					case 16:
					case 24:
					case 32:
						break;

					default:
						printf("Screen color depth must be 8, 16, 24, or 32!\n");
						hasError = true;
						break;
				}
			}
		}
		else IFARG("--joystick")
		{
			if(++i >= argc)
			{
				printf("The joystick option is missing the index argument!\n");
				hasError = true;
			}
			else param_joystickindex = atoi(argv[i]);   // index is checked in InitGame
		}
		else IFARG("--joystickhat")
		{
			if(++i >= argc)
			{
				printf("The joystickhat option is missing the index argument!\n");
				hasError = true;
			}
			else param_joystickhat = atoi(argv[i]);
		}
		else IFARG("--samplerate")
		{
			if(++i >= argc)
			{
				printf("The samplerate option is missing the rate argument!\n");
				hasError = true;
			}
			else param_samplerate = atoi(argv[i]);
			sampleRateGiven = true;
		}
		else IFARG("--audiobuffer")
		{
			if(++i >= argc)
			{
				printf("The audiobuffer option is missing the size argument!\n");
				hasError = true;
			}
			else param_audiobuffer = atoi(argv[i]);
			audioBufferGiven = true;
		}
		else IFARG("--mission")
		{
			if(++i >= argc)
			{
				printf("The mission option is missing the mission argument!\n");
				hasError = true;
			}
			else param_mission = atoi(argv[i]);
		}
		else IFARG("--goodtimes")
			param_goodtimes = true;
		else IFARG("--ignorenumchunks")
			param_ignorenumchunks = true;
		else IFARG("--help")
			showHelp = true;
		else
			WL_AddFile(argv[i]);
	}
	if(hasError || showHelp)
	{
		if(hasError) printf("\n");
		printf(
			"Wolf4SDL v1.6 ($Revision: 232 $)\n"
			"Ported by Chaos-Software (http://www.chaos-software.de.vu)\n"
			"Original Wolfenstein 3D by id Software\n\n"
			"Usage: Wolf4SDL [options]\n"
			"Options:\n"
			" --help                 This help page\n"
			" --tedlevel <level>     Starts the game in the given level\n"
			" --baby                 Sets the difficulty to baby for tedlevel\n"
			" --easy                 Sets the difficulty to easy for tedlevel\n"
			" --normal               Sets the difficulty to normal for tedlevel\n"
			" --hard                 Sets the difficulty to hard for tedlevel\n"
			" --nowait               Skips intro screens\n"
			" --windowed             Starts the game in a window\n"
			" --res <width> <height> Sets the screen resolution\n"
			"                        (must be multiple of 320x200 or 320x240)\n"
			" --resf <w> <h>         Sets any screen resolution >= 320x200\n"
			"                        (which may result in graphic errors)\n"
			" --aspect <aspect>      Sets the aspect ratio.\n"
			" --bits <b>             Sets the screen color depth\n"
			"                        (use this when you have palette/fading problems\n"
			"                        allowed: 8, 16, 24, 32, default: \"best\" depth)\n"
			" --joystick <index>     Use the index-th joystick if available\n"
			"                        (-1 to disable joystick, default: 0)\n"
			" --joystickhat <index>  Enables movement with the given coolie hat\n"
			" --samplerate <rate>    Sets the sound sample rate (given in Hz, default: %i)\n"
			" --audiobuffer <size>   Sets the size of the audio buffer (-> sound latency)\n"
#ifdef _arch_dreamcast
			"                        (given in bytes, default: 4096 / (44100 / samplerate))\n"
#else
			"                        (given in bytes, default: 2048 / (44100 / samplerate))\n"
#endif
			" --ignorenumchunks      Ignores the number of chunks in VGAHEAD.*\n"
			"                        (may be useful for some broken mods)\n"
#if defined(SPEAR) && !defined(SPEARDEMO)
			" --mission <mission>    Mission number to play (1-3)\n"
			" --goodtimes            Disable copy protection quiz\n"
#endif
			, defaultSampleRate
		);
		exit(1);
	}

	if(needRatio)
	{
		vid_aspect = CheckRatio(screenWidth, screenHeight);
	}

	if(sampleRateGiven && !audioBufferGiven)
#ifdef _arch_dreamcast
		param_audiobuffer = 4096 / (44100 / param_samplerate);
#else
		param_audiobuffer = 2048 / (44100 / param_samplerate);
#endif
}

/*
==========================
=
= main
=
==========================
*/

#include "lumpremap.h"
int main (int argc, char *argv[])
{
	printf("ReadConfig: Reading the Configuration.\n");
	config->LocateConfigFile(argc, argv);
	ReadConfig();

	WL_AddFile("ecwolf.pk3");

	CheckForEpisodes();
	WL_AddFile("audiot.wl6");
	WL_AddFile("vgagraph.wl6");
	WL_AddFile("vswap.wl6");

#if defined(_arch_dreamcast)
	DC_Main();
	DC_CheckParameters();
#elif defined(GP2X)
	GP2X_Init();
#else
	CheckParameters(argc, argv);
#endif

	CheckForEpisodes();

	printf("W_Init: Init WADfiles.\n");
	Wads.InitMultipleFiles(wadfiles);
	language.SetupStrings();
	LumpRemaper::RemapAll();

	printf("VL_ReadPalette: Setting up the Palette...\n");
	VL_ReadPalette();
	InitPalette("WOLFPAL");
	printf("InitGame: Setting up the game...\n");
	InitGame();
	FinalReadConfig();
	printf("CreateMenus: Preparing the menu system...\n");
	CreateMenus();

	printf("DemoLoop: Starting the game loop...\n");
	DemoLoop();

	Quit("Demo loop exited???");
	return 1;
}
