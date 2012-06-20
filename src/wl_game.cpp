// WL_GAME.C

#ifdef WINDOWS
#include <io.h>
#endif
#include <unistd.h>

#include <math.h>
#include "wl_def.h"
#include "wl_menu.h"
#include "id_ca.h"
#include "id_sd.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#include "w_wad.h"
#include "thinker.h"
#include "actor.h"
#include <SDL_mixer.h>
#include "wl_agent.h"
#include "g_mapinfo.h"
#include "wl_inter.h"
#include "wl_draw.h"
#include "wl_play.h"
#include "wl_game.h"
#include "a_inventory.h"

#ifdef MYPROFILE
#include <TIME.H>
#endif


/*
=============================================================================

							LOCAL CONSTANTS

=============================================================================
*/


/*
=============================================================================

							GLOBAL VARIABLES

=============================================================================
*/

AActor			*killerobj;
bool			ingame,fizzlein;
gametype        gamestate;
byte            bordercol=VIEWCOLOR;        // color of the Change View/Ingame border

#ifdef SPEAR
int32_t         spearx,speary;
unsigned        spearangle;
bool            spearflag;
#endif

#ifdef USE_FEATUREFLAGS
int ffDataTopLeft, ffDataTopRight, ffDataBottomLeft, ffDataBottomRight;
#endif

//
// ELEVATOR BACK MAPS - REMEMBER (-1)!!
//
int ElevatorBackTo[]={1,1,7,3,5,3};

void SetupGameLevel (void);
void LoadLatchMem (void);
void GameLoop (void);

/*
=============================================================================

							LOCAL VARIABLES

=============================================================================
*/



//===========================================================================
//===========================================================================


/*
==========================
=
= SetSoundLoc - Given the location of an object (in terms of global
=       coordinates, held in globalsoundx and globalsoundy), munges the values
=       for an approximate distance from the left and right ear, and puts
=       those values into leftchannel and rightchannel.
=
= JAB
=
==========================
*/

int leftchannel, rightchannel;
#define ATABLEMAX 15
byte righttable[ATABLEMAX][ATABLEMAX * 2] = {
{ 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 6, 0, 0, 0, 0, 0, 1, 3, 5, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 7, 7, 6, 4, 0, 0, 0, 0, 0, 2, 4, 6, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 7, 6, 6, 4, 1, 0, 0, 0, 1, 2, 4, 6, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 7, 6, 5, 4, 2, 1, 0, 1, 2, 3, 5, 7, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 6, 5, 4, 3, 2, 2, 3, 3, 5, 6, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 6, 6, 5, 4, 4, 4, 4, 5, 6, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 6, 6, 5, 5, 5, 6, 6, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 6, 6, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8}
};
byte lefttable[ATABLEMAX][ATABLEMAX * 2] = {
{ 8, 8, 8, 8, 8, 8, 8, 8, 5, 3, 1, 0, 0, 0, 0, 0, 6, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 6, 4, 2, 0, 0, 0, 0, 0, 4, 6, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 6, 4, 2, 1, 0, 0, 0, 1, 4, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 7, 5, 3, 2, 1, 0, 1, 2, 4, 5, 6, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 6, 5, 3, 3, 2, 2, 3, 4, 5, 6, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 6, 5, 4, 4, 4, 4, 5, 6, 6, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 6, 6, 5, 5, 5, 6, 6, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 7, 6, 6, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8}
};

void
SetSoundLoc(fixed gx,fixed gy)
{
	fixed   xt,yt;
	int     x,y;

//
// translate point to view centered coordinates
//
	gx -= viewx;
	gy -= viewy;

//
// calculate newx
//
	xt = FixedMul(gx,viewcos);
	yt = FixedMul(gy,viewsin);
	x = (xt - yt) >> TILESHIFT;

//
// calculate newy
//
	xt = FixedMul(gx,viewsin);
	yt = FixedMul(gy,viewcos);
	y = (yt + xt) >> TILESHIFT;

	if (y >= ATABLEMAX)
		y = ATABLEMAX - 1;
	else if (y <= -ATABLEMAX)
		y = -ATABLEMAX;
	if (x < 0)
		x = -x;
	if (x >= ATABLEMAX)
		x = ATABLEMAX - 1;
	leftchannel  =  lefttable[x][y + ATABLEMAX];
	rightchannel = righttable[x][y + ATABLEMAX];

#if 0
	CenterWindow(8,1);
	US_PrintSigned(leftchannel);
	US_Print(",");
	US_PrintSigned(rightchannel);
	VW_UpdateScreen();
#endif
}

/*
==========================
=
= SetSoundLocGlobal - Sets up globalsoundx & globalsoundy and then calls
=       UpdateSoundLoc() to transform that into relative channel volumes. Those
=       values are then passed to the Sound Manager so that they'll be used for
=       the next sound played (if possible).
=
= JAB
=
==========================
*/
void PlaySoundLocGlobal(const char* s,fixed gx,fixed gy,int chan)
{
	SetSoundLoc(gx, gy);
	SD_PositionSound(leftchannel, rightchannel);

	int channel = SD_PlaySound(s, static_cast<SoundChannel> (chan));
	if(channel)
	{
		channelSoundPos[channel - 1].globalsoundx = gx;
		channelSoundPos[channel - 1].globalsoundy = gy;
		channelSoundPos[channel - 1].valid = 1;
	}
}

void UpdateSoundLoc(void)
{
/*    if (SoundPositioned)
	{
		SetSoundLoc(globalsoundx,globalsoundy);
		SD_SetPosition(leftchannel,rightchannel);
	}*/

	for(int i = 0; i < MIX_CHANNELS; i++)
	{
		if(channelSoundPos[i].valid)
		{
			SetSoundLoc(channelSoundPos[i].globalsoundx,
				channelSoundPos[i].globalsoundy);
			SD_SetPosition(i, leftchannel, rightchannel);
		}
	}
}

/*
**      JAB End
*/

//==========================================================================

/*
==================
=
= SetupGameLevel
=
==================
*/

void SetupGameLevel (void)
{
	int  x,y;
	word tile;


	if (!loadedgame)
	{
		gamestate.TimeCount
			= gamestate.secrettotal
			= gamestate.killtotal
			= gamestate.treasuretotal
			= gamestate.secretcount
			= gamestate.killcount
			= gamestate.treasurecount
			= facetimes = 0;
		LastAttacker = NULL;
		killerobj = NULL;

		thinkerList->DestroyAll();
	}

	/*if (demoplayback || demorecord)
		US_InitRndT (false);
	else
		US_InitRndT (true);*/

//
// load the level
//
	CA_CacheMap (gamestate.mapname);

#ifdef USE_FEATUREFLAGS
	// Temporary definition to make things clearer
	#define MXX MAPSIZE - 1

	// Read feature flags data from map corners and overwrite corners with adjacent tiles
	ffDataTopLeft     = MAPSPOT(0,   0,   0); MAPSPOT(0,   0,   0) = MAPSPOT(1,       0,       0);
	ffDataTopRight    = MAPSPOT(MXX, 0,   0); MAPSPOT(MXX, 0,   0) = MAPSPOT(MXX,     1,       0);
	ffDataBottomRight = MAPSPOT(MXX, MXX, 0); MAPSPOT(MXX, MXX, 0) = MAPSPOT(MXX - 1, MXX,     0);
	ffDataBottomLeft  = MAPSPOT(0,   MXX, 0); MAPSPOT(0,   MXX, 0) = MAPSPOT(0,       MXX - 1, 0);

	#undef MXX
#endif

//
// spawn actors
//
	map->SpawnThings();
}


//==========================================================================


/*
===================
=
= DrawPlayBorderSides
=
= To fix window overwrites
=
===================
*/
void DrawPlayBorderSides(void)
{
	if(viewsize == 21) return;

	// Draw frame
	if(viewwidth != screenWidth)
	{
		VWB_Clear(0, viewscreenx-scaleFactor, viewscreeny-scaleFactor, viewscreenx+viewwidth+scaleFactor, viewscreeny);
		VWB_Clear(0, viewscreenx-scaleFactor, viewscreeny, viewscreenx, viewscreeny+viewheight);
		VWB_Clear(bordercol-scaleFactor, viewscreenx+viewwidth, viewscreeny, viewscreenx+viewwidth+scaleFactor, viewscreeny+viewheight);
		VWB_Clear(bordercol-scaleFactor, viewscreenx-scaleFactor, viewscreeny+viewheight, viewscreenx+viewwidth+scaleFactor, viewscreeny+viewheight+2);
	}
	else
		VWB_Clear(bordercol-scaleFactor, 0, viewscreeny+viewheight, screenWidth, viewscreeny+viewheight+scaleFactor);
}


/*
===================
=
= DrawStatusBorder
=
===================
*/

void DrawStatusBorder (byte color)
{
	int statusborderw = (screenWidth-scaleFactor*320)/2;

	VWB_BarScaledCoord (0,0,screenWidth,screenHeight-scaleFactor*(STATUSLINES-3),color);
	VWB_BarScaledCoord (0,screenHeight-scaleFactor*(STATUSLINES-3),
		statusborderw+scaleFactor*8,scaleFactor*(STATUSLINES-4),color);
	VWB_BarScaledCoord (0,screenHeight-scaleFactor*2,screenWidth,scaleFactor*2,color);
	VWB_BarScaledCoord (screenWidth-statusborderw-scaleFactor*8, screenHeight-scaleFactor*(STATUSLINES-3),
		statusborderw+scaleFactor*8,scaleFactor*(STATUSLINES-4),color);

	VWB_BarScaledCoord (statusborderw+scaleFactor*9, screenHeight-scaleFactor*3,
		scaleFactor*97, scaleFactor*1, color-1);
	VWB_BarScaledCoord (statusborderw+scaleFactor*106, screenHeight-scaleFactor*3,
		scaleFactor*161, scaleFactor*1, color-2);
	VWB_BarScaledCoord (statusborderw+scaleFactor*267, screenHeight-scaleFactor*3,
		scaleFactor*44, scaleFactor*1, color-3);
	VWB_BarScaledCoord (screenWidth-statusborderw-scaleFactor*9, screenHeight-scaleFactor*(STATUSLINES-4),
		scaleFactor*1, scaleFactor*20, color-2);
	VWB_BarScaledCoord (screenWidth-statusborderw-scaleFactor*9, screenHeight-scaleFactor*(STATUSLINES/2-4),
		scaleFactor*1, scaleFactor*14, color-3);
}


/*
===================
=
= DrawPlayBorder
=
===================
*/

void DrawPlayBorder (void)
{
	//TODO Highlight of bordercol-3 in lower left.
	VWB_Clear(bordercol, 0, 0, screenWidth, viewscreeny);
	VWB_Clear(bordercol, 0, viewscreeny, viewscreenx, viewheight + viewscreeny);
	VWB_Clear(bordercol, viewwidth + viewscreenx, viewscreeny, screenWidth, viewheight + viewscreeny);
	VWB_Clear(bordercol, 0, viewscreeny + viewheight, screenWidth, statusbary);
	if(statusbarx)
	{
		VWB_Clear(bordercol, 0, statusbary, statusbarx, screenHeight);
		VWB_Clear(bordercol, screenWidth-statusbarx, statusbary, screenWidth, screenHeight);
	}
	// Complete border
	VWB_Clear(bordercol, statusbarx, statusbary, screenWidth-statusbarx, screenHeight);

	DrawPlayBorderSides();
}


/*
===================
=
= DrawPlayScreen
=
===================
*/

void DrawPlayScreen (bool noborder)
{
	if(!noborder)
		DrawPlayBorder ();

	DrawStatusBar();
}

void ShowActStatus()
{
	ingame = false;
	DrawStatusBar();
	ingame = true;
}


//==========================================================================

/*
==================
=
= StartDemoRecord
=
==================
*/

char    demoname[13] = "DEMO?.";

#define MAXDEMOSIZE     8192

void StartDemoRecord (int levelnumber)
{
	demobuffer=malloc(MAXDEMOSIZE);
	CHECKMALLOCRESULT(demobuffer);
	demoptr = (int8_t *) demobuffer;
	lastdemoptr = demoptr+MAXDEMOSIZE;

	*demoptr = levelnumber;
	demoptr += 4;                           // leave space for length
	demorecord = true;
}


/*
==================
=
= FinishDemoRecord
=
==================
*/

static bool CA_WriteFile (const char *filename, void *ptr, int32_t length)
{
	const int handle = open(filename, O_CREAT | O_WRONLY | O_BINARY, 777);
	if (handle == -1)
		return false;

	if (!write (handle,ptr,length))
	{
		close (handle);
		return false;
	}
	close (handle);
	return true;
}

void FinishDemoRecord (void)
{
	int32_t    length,level;
	char str[80];

	demorecord = false;

	length = (int32_t) (demoptr - (int8_t *)demobuffer);

	demoptr = ((int8_t *)demobuffer)+1;
	demoptr[0] = (int8_t) length;
	demoptr[1] = (int8_t) (length >> 8);
	demoptr[2] = 0;

	VW_FadeIn();
	CenterWindow(24,3);
	PrintY+=6;
	US_Print(SmallFont, " Demo number (0-9): ");
	VW_UpdateScreen();

	if (US_LineInput (px,py,str,NULL,true,1,0))
	{
		level = atoi (str);
		if (level>=0 && level<=9)
		{
			demoname[4] = (char)('0'+level);
			CA_WriteFile (demoname,demobuffer,length);
		}
	}

	free(demobuffer);
}

//==========================================================================

/*
==================
=
= RecordDemo
=
= Fades the screen out, then starts a demo.  Exits with the screen faded
=
==================
*/

void RecordDemo (void)
{
	FString level;
	int levelnum, esc,maps;
	char str[80];

	CenterWindow(26,3);
	PrintY+=6;
	US_Print(SmallFont, "  Demo which level (#): ");
	VW_UpdateScreen();
	VW_FadeIn ();
	esc = !US_LineInput (px,py,str,NULL,true,2,0);
	if (esc)
		return;

	levelnum = atoi(str);
	level.Format("MAP%02d", levelnum);

	if (Wads.CheckNumForName(level) == -1)
		return;

	VW_FadeOut ();

	NewGame (gd_hard, level);

	StartDemoRecord (levelnum);

	DrawPlayScreen ();
	VW_FadeIn ();

	startgame = false;
	demorecord = true;

	SetupGameLevel ();
	StartMusic ();

	if(usedoublebuffering) VH_UpdateScreen();
	fizzlein = true;

	PlayLoop ();

	demoplayback = false;

	StopMusic ();
	VW_FadeOut ();
	ClearMemory ();

	FinishDemoRecord ();
}



//==========================================================================

/*
==================
=
= PlayDemo
=
= Fades the screen out, then starts a demo.  Exits with the screen unfaded
=
==================
*/

void PlayDemo (int demonumber)
{
	int length;
	char demoName[9];
	sprintf(demoName, "DEMO%d", demonumber);
	int lumpNum = Wads.GetNumForName(demoName);
	if(lumpNum == -1)
		return;
	FWadLump lump = Wads.OpenLumpNum(lumpNum);
	demoptr = new int8_t[Wads.LumpLength(lumpNum)];
	int8_t* demoptr_freeme = demoptr; // Since I can't delete[] demoptr when the time comes.
	lump.Read(demoptr, Wads.LumpLength(lumpNum));

	int mapon = *demoptr++;
	FString level;
	level.Format("MAP%02d", mapon);
	NewGame (1,level);
	gamestate.difficulty = gd_hard;
	length = READWORD(*(uint8_t **)&demoptr);
	// TODO: Seems like the original demo format supports 16 MB demos
	//       But T_DEM00 and T_DEM01 of Wolf have a 0xd8 as third length size...
	demoptr++;
	lastdemoptr = demoptr-4+length;

	VW_FadeOut ();

	DrawPlayScreen ();

	startgame = false;
	demoplayback = true;

	SetupGameLevel ();
	StartMusic ();

	PlayLoop ();

	delete[] demoptr_freeme;

	demoplayback = false;

	StopMusic ();
	ClearMemory ();
}

//==========================================================================

/*
==================
=
= Died
=
==================
*/

#define DEATHROTATE (ANGLE_1*2)

void Died (void)
{
	float   fangle;
	int32_t dx,dy;
	angle_t iangle,curangle,clockwise,counter,change;

	if (screenfaded)
	{
		ThreeDRefresh ();
		VW_FadeIn ();
	}

	SD_PlaySound ("player/death");

	//
	// swing around to face attacker
	//
	if(killerobj)
	{
		dx = killerobj->x - players[0].mo->x;
		dy = players[0].mo->y - killerobj->y;

		fangle = (float) atan2((float) dy, (float) dx);     // returns -pi to pi
		if (fangle<0)
			fangle = (float) (M_PI*2+fangle);

		iangle = (angle_t) (fangle*ANGLE_180/M_PI);
	}
	else
	{
		iangle = players[0].mo->angle + ANGLE_180;
	}

	if (players[0].mo->angle > iangle)
	{
		counter = players[0].mo->angle - iangle;
		clockwise = players[0].mo->angle + iangle;
	}
	else
	{
		clockwise = iangle - players[0].mo->angle;
		counter = players[0].mo->angle - iangle;
	}

	curangle = players[0].mo->angle;

	if (clockwise<counter)
	{
		//
		// rotate clockwise
		//
		do
		{
			change = tics*DEATHROTATE;
			if (curangle + change > iangle)
				change = iangle-curangle;

			curangle += change;
			players[0].mo->angle += change;

			ThreeDRefresh ();
			CalcTics ();
		} while (curangle != iangle);
	}
	else
	{
		//
		// rotate counterclockwise
		//
		do
		{
			change = tics*DEATHROTATE;
			if (curangle - change < iangle)
				change = curangle - iangle;

			curangle -= change;
			players[0].mo->angle -= change;

			ThreeDRefresh ();
			CalcTics ();
		} while (curangle != iangle);
	}

	//
	// fade to red
	//
	FinishPaletteShifts ();

	if(usedoublebuffering) VH_UpdateScreen();

	VL_BarScaledCoord (viewscreenx,viewscreeny,viewwidth,viewheight,4);

	IN_ClearKeysDown ();

	FizzleFade(screenBuffer,viewscreenx,viewscreeny,viewwidth,viewheight,70,false);

	IN_UserInput(100);
	SD_WaitSoundDone ();
	ClearMemory();

	players[0].lives--;

	if (players[0].lives > -1)
	{
		DrawStatusBar();

		players[0].state = player_t::PST_REBORN;
		gamestate.attackframe = gamestate.attackcount =
			gamestate.weaponframe = 0;
		thinkerList->DestroyAll();
	}
}

//==========================================================================

/*
===================
=
= GameLoop
=
===================
*/

void StartTravel ();
void FinishTravel ();
void GameLoop (void)
{
	bool died;
#ifdef MYPROFILE
	clock_t start,end;
#endif

restartgame:
	ClearMemory ();
	VW_FadeOut();
	DrawPlayScreen ();
	died = false;
	do
	{
		if (!loadedgame)
			players[0].score = players[0].oldscore;

		startgame = false;
		if (!loadedgame)
		{
			SetupGameLevel ();
			if(playstate != ex_warped)
				FinishTravel ();
		}

		ingame = true;
		if(loadedgame)
		{
			ContinueMusic(lastgamemusicoffset);
			loadedgame = false;
		}
		else StartMusic ();

		if (!died)
			PreloadGraphics ();             // TODO: Let this do something useful!
		else
		{
			died = false;
			fizzlein = true;
		}

		DrawStatusBar();

#ifdef SPEAR
startplayloop:
#endif
		PlayLoop ();

#ifdef SPEAR
		if (spearflag)
		{
			SD_StopSound();
			SD_PlaySound("misc/spear_pickup");
			if (DigiMode != sds_Off)
			{
				Delay(150);
			}
			else
				SD_WaitSoundDone();

			ClearMemory ();
			players[0].oldscore = players[0].score;
			gamestate.mapon = 20;
			SetupGameLevel ();
			StartMusic ();
			players[0].mo->x = spearx;
			players[0].mo->y = speary;
			players[0].mo->angle = (short)spearangle;
			spearflag = false;
			Thrust (0,0);
			goto startplayloop;
		}
#endif

		StopMusic ();
		ingame = false;

		if (demorecord && playstate != ex_warped)
			FinishDemoRecord ();

		if (startgame || loadedgame)
			goto restartgame;

		switch (playstate)
		{
			case ex_completed:
			case ex_secretlevel:
				if(viewsize == 21) DrawPlayScreen();

				// Remove inventory items that don't transfer (keys for example)
				for(AInventory *inv = players[0].mo->inventory;inv;inv = inv->inventory)
				{
					if(inv->interhubamount < 1)
						inv->Destroy();
					else if((inv->itemFlags & IF_INVBAR) && inv->amount > inv->interhubamount)
						inv->amount = inv->interhubamount;
				}

				DrawStatusBar();
				VW_FadeOut ();

				ClearMemory ();

				StartTravel ();
				LevelCompleted ();              // do the intermission
				if(viewsize == 21) DrawPlayScreen();

#ifdef SPEARDEMO
				if (gamestate.mapon == 1)
				{
					died = true;                    // don't "get psyched!"

					VW_FadeOut ();

					ClearMemory ();

					CheckHighScore (players[0].score,gamestate.mapon+1);
					return;
				}
#endif

				players[0].oldscore = players[0].score;

				if(playstate == ex_secretlevel)
					strncpy(gamestate.mapname, levelInfo->NextSecret, 8);
				else
					strncpy(gamestate.mapname, levelInfo->NextMap, 8);
				gamestate.mapname[8] = 0;
				break;

			case ex_died:
				Died ();
				died = true;                    // don't "get psyched!"

				if (players[0].lives > -1)
					break;                          // more lives left

				VW_FadeOut ();
				if(screenHeight % 200 != 0)
					VL_ClearScreen(0);

				ClearMemory ();

				CheckHighScore (players[0].score,gamestate.mapon+1);
				return;

			case ex_victorious:
				if(viewsize == 21) DrawPlayScreen();
#ifndef SPEAR
				VW_FadeOut ();
#else
				VL_FadeOut (0,255,0,68,68,300);
#endif
				ClearMemory ();

				Victory ();

				ClearMemory ();

				CheckHighScore (players[0].score,gamestate.mapon+1);
				return;

			case ex_warped:
				players[0].state = player_t::PST_ENTER;
				break;

			default:
				if(viewsize == 21) DrawPlayScreen();
				ClearMemory ();
				break;
		}
	} while (1);
}
