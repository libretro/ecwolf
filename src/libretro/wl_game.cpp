// WL_GAME.C

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

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
#include "wl_agent.h"
#include "g_intermission.h"
#include "g_mapinfo.h"
#include "r_sprites.h"
#include "wl_inter.h"
#include "wl_draw.h"
#include "wl_play.h"
#include "state_machine.h"
#include "wl_game.h"
#include "wl_text.h"
#include "a_inventory.h"
#include "colormatcher.h"
#include "thingdef/thingdef.h"
#include "doomerrors.h"

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

bool			ingame,fizzlein;
gametype        gamestate;

NewMap_t NewMap;

#ifdef USE_FEATUREFLAGS
int ffDataTopLeft, ffDataTopRight, ffDataBottomLeft, ffDataBottomRight;
#endif

//
// ELEVATOR BACK MAPS - REMEMBER (-1)!!
//
int ElevatorBackTo[]={1,1,7,3,5,3};

void SetupGameLevel (void);
bool GameLoop (void);

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
const byte righttable[ATABLEMAX][ATABLEMAX * 2] = {
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
const byte lefttable[ATABLEMAX][ATABLEMAX * 2] = {
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

#ifdef TODO
	US_CenterWindow(8,1);
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
	if (!loadedgame)
	{
		gamestate.victoryflag = false;
		gamestate.fullmap = false;
		gamestate.TimeCount
			= gamestate.secrettotal
			= gamestate.killtotal
			= gamestate.treasuretotal
			= gamestate.secretcount
			= gamestate.killcount
			= gamestate.treasurecount = 0;
		LastAttacker = NULL;
		players[0].killerobj = NULL;
	}

	gamestate.faceframe.SetInvalid();

//
// load the level
//
	CA_CacheMap (gamestate.mapname, loadedgame);
	if (!loadedgame)
		StartMusic ();

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
	if(!loadedgame)
	{
		map->SpawnThings();

		// Check to see if a player spawned
		if(players[0].mo == NULL)
			throw CRecoverableError("No player 1 start!");
	}
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
	if((unsigned)viewheight == screenHeight) return;

	if(!gameinfo.Border.issolid)
	{
		static FTexture * const BorderTextures[8] =
		{
			TexMan(gameinfo.Border.tl), TexMan(gameinfo.Border.t), TexMan(gameinfo.Border.tr),
			TexMan(gameinfo.Border.l), TexMan(gameinfo.Border.r),
			TexMan(gameinfo.Border.bl), TexMan(gameinfo.Border.b), TexMan(gameinfo.Border.br)
		};
		const int offset = gameinfo.Border.offset;

		// Draw frame
		if((unsigned)viewwidth != screenWidth)
		{
			VWB_DrawFill(BorderTextures[0], viewscreenx-offset, viewscreeny-offset, viewscreenx, viewscreeny, true);
			VWB_DrawFill(BorderTextures[1], viewscreenx, viewscreeny-BorderTextures[1]->GetHeight(), viewscreenx+viewwidth, viewscreeny, true);
			VWB_DrawFill(BorderTextures[2], viewscreenx, viewscreeny-offset, viewscreenx+viewwidth+offset, viewscreeny, true);
			VWB_DrawFill(BorderTextures[3], viewscreenx-BorderTextures[3]->GetWidth(), viewscreeny, viewscreenx, viewscreeny+viewheight, true);
			VWB_DrawFill(BorderTextures[4], viewscreenx+viewwidth, viewscreeny, viewscreenx+viewwidth+BorderTextures[4]->GetWidth(), viewscreeny+viewheight, true);
			VWB_DrawFill(BorderTextures[5], viewscreenx-offset, viewscreeny+viewheight, viewscreenx, viewscreeny+viewheight+offset, true);
			VWB_DrawFill(BorderTextures[6], viewscreenx, viewscreeny+viewheight, viewscreenx+viewwidth, viewscreeny+viewheight+BorderTextures[3]->GetHeight(), true);
			VWB_DrawFill(BorderTextures[7], viewscreenx+viewwidth, viewscreeny+viewheight, viewscreenx+viewwidth+offset, viewscreeny+viewheight+offset, true);
		}
		else
			VWB_DrawFill(BorderTextures[6], 0, viewscreeny+viewheight, screenWidth, viewscreeny+viewheight+BorderTextures[6]->GetHeight(), true);
	}
	else
	{
		byte colors[3] =
		{
			ColorMatcher.Pick(RPART(gameinfo.Border.topcolor), GPART(gameinfo.Border.topcolor), BPART(gameinfo.Border.topcolor)),
			ColorMatcher.Pick(RPART(gameinfo.Border.bottomcolor), GPART(gameinfo.Border.bottomcolor), BPART(gameinfo.Border.bottomcolor)),
			ColorMatcher.Pick(RPART(gameinfo.Border.highlightcolor), GPART(gameinfo.Border.highlightcolor), BPART(gameinfo.Border.highlightcolor))
		};

		if((unsigned)viewwidth != screenWidth)
		{
			VWB_Clear(colors[0], viewscreenx-scaleFactorX, viewscreeny-scaleFactorY, viewscreenx+viewwidth, viewscreeny);
			VWB_Clear(colors[0], viewscreenx-scaleFactorX, viewscreeny, viewscreenx, viewscreeny+viewheight);
			VWB_Clear(colors[1], viewscreenx, viewscreeny+viewheight, viewscreenx+viewwidth+scaleFactorX, viewscreeny+viewheight+scaleFactorY);
			VWB_Clear(colors[1], viewscreenx+viewwidth, viewscreeny-scaleFactorY, viewscreenx+viewwidth+scaleFactorX, viewscreeny+viewheight);
			VWB_Clear(colors[2], viewscreenx-scaleFactorX, viewscreeny+viewheight, viewscreenx, viewscreeny+viewheight+scaleFactorY);
		}
		else
		{
			if(statusbary1)
				VWB_Clear(colors[0], 0, viewscreeny-scaleFactorY, screenWidth, viewscreeny);
			VWB_Clear(colors[1], 0, viewscreeny+viewheight, screenWidth, viewscreeny+viewheight+scaleFactorY);
		}
	}
}

/*
===================
=
= DrawPlayBorder
=
===================
*/

void DBaseStatusBar::RefreshBackground (bool noborder)
{
	FTexture *borderTex = TexMan(levelInfo->GetBorderTexture());

	if(viewscreeny > statusbary1)
		VWB_DrawFill(borderTex, 0, statusbary1, screenWidth, viewscreeny);
	VWB_DrawFill(borderTex, 0, viewscreeny, viewscreenx, viewheight + viewscreeny);
	VWB_DrawFill(borderTex, viewwidth + viewscreenx, viewscreeny, screenWidth, viewheight + viewscreeny);
	VWB_DrawFill(borderTex, 0, viewscreeny + viewheight, screenWidth, statusbary2);
	if(statusbarx)
	{
		VWB_DrawFill(borderTex, 0, 0, statusbarx, statusbary1);
		VWB_DrawFill(borderTex, screenWidth-statusbarx, 0, screenWidth, statusbary1);
		VWB_DrawFill(borderTex, 0, statusbary2, statusbarx, screenHeight);
		VWB_DrawFill(borderTex, screenWidth-statusbarx, statusbary2, screenWidth, screenHeight);
	}
	// Complete border
	if(statusbary1)
		VWB_DrawFill(borderTex, statusbarx, 0, screenWidth-statusbarx, statusbary1);
	VWB_DrawFill(borderTex, statusbarx, statusbary2, screenWidth-statusbarx, screenHeight);

	if(noborder)
		return;
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
	StatusBar->RefreshBackground(noborder);
	StatusBar->DrawStatusBar();
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
	US_CenterWindow(24,3);
	PrintY+=6;
	US_Print(SmallFont, " Demo number (0-9): ");
	VW_UpdateScreen();

	if (US_LineInput (SmallFont,px,py,str,NULL,true,1,0,GPalette.WhiteIndex))
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

#ifdef TODO
void RecordDemo (void)
{
	FString level;
	int levelnum, esc;
	char str[80];

	US_CenterWindow(26,3);
	PrintY+=6;
	US_Print(SmallFont, "  Demo which level (#): ");
	VW_UpdateScreen();
	VW_FadeIn ();
	esc = !US_LineInput (SmallFont,px,py,str,NULL,true,2,0,GPalette.WhiteIndex);
	if (esc)
		return;

	levelnum = atoi(str);
	level.Format("MAP%02d", levelnum);

	if (Wads.CheckNumForName(level) == -1)
		return;

	VW_FadeOut ();

	NewGame (gd_hard, level, false);

	StartDemoRecord (levelnum);

	DrawPlayScreen ();
	VW_FadeIn ();

	startgame = false;
	demorecord = true;

	SetupGameLevel ();

	if(usedoublebuffering) VH_UpdateScreen();
	fizzlein = true;

	PlayLoop ();

	demoplayback = false;

	StopMusic ();
	VW_FadeOut ();

	FinishDemoRecord ();
}
#endif


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
#ifdef TODO
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
	NewGame (1,level,false);
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

	PlayLoop ();

	delete[] demoptr_freeme;

	demoplayback = false;

	StopMusic ();
#endif
}

//==========================================================================

/*
==================
=
= Died
=
==================
*/

bool Died1(wl_state_t *state)
{
	if (screenfaded)
	{
		ThreeDRefresh ();
		State_FadeIn (state);
	}

	SD_PlaySound ("player/death");
	state->stage = DIED2;
	return false;
}

bool Died2 (wl_state_t *state)
{
	float   fangle;
	int32_t dx,dy;
	angle_t iangle;

	//
	// swing around to face attacker
	//
	if(players[0].killerobj)
	{
		dx = players[0].killerobj->x - players[0].mo->x;
		dy = players[0].mo->y - players[0].killerobj->y;

		fangle = (float) atan2((float) dy, (float) dx);     // returns -pi to pi
		if (fangle<0)
			fangle = (float) (M_PI*2+fangle);

		iangle = (angle_t) (fangle*ANGLE_180/M_PI);
	}
	else
	{
		iangle = players[0].mo->angle;
	}

	state->iangle = iangle;
	state->stage = DEATH_ROTATE;
	return false;
}

bool DeathRotate(wl_state_t *state)
{
	static const angle_t DEATHROTATE = ANGLE_1*2;
	angle_t &curangle = players[0].mo->angle;
	const int rotate = curangle - state->iangle > ANGLE_180 ? 1 : -1;

	if (curangle == state->iangle) {
		state->stage = DEATH_WEAPON_DROP;
		return false;
	}

	for(unsigned int t = tics;t-- > 0;)
	{
		players[0].mo->Tick();
		
		if (curangle - state->iangle < DEATHROTATE)
			curangle = state->iangle;
		else
			curangle += rotate*DEATHROTATE;
	}

	ThreeDRefresh ();
	VH_UpdateScreen();

	return true;
}

bool DeathDropWeapon (wl_state_t *state)
{

	// Wait for weapon to drop
	if(!players[0].psprite[player_t::ps_weapon].frame)
	{
		state->stage = DIED3;
		return false;
	}

	for(unsigned int t = tics;t-- > 0;)
		players[0].mo->Tick();

	ThreeDRefresh();
	VH_UpdateScreen();

	return true;
}

bool Died3(wl_state_t *state)
{
	//
	// fade to red
	//
	FinishPaletteShifts ();
	state->stage = DIED4;

	return true;
}

bool Died4(wl_state_t *state)
{
	if (gamestate.difficulty->LivesCount < 0) {
		state->stage = DIED5;
		return false;
	}
		
	--players[0].lives;

	if (gameinfo.GameOverPic.IsEmpty() || players[0].lives != -1) {
		state->stage = DIED5;
		return false;
	}

	FTextureID texID = TexMan.CheckForTexture(gameinfo.GameOverPic, FTexture::TEX_Any);
	if(!texID.isValid()) {
		state->stage = DIED5;
		return false;
	}
	R_DrawZoomer(texID);
	state->stage = DIED5;
	return true;
}

bool Died5(wl_state_t *state)
{
	if(gameinfo.DeathTransition != GameInfo::TRANSITION_Fizzle)
	{
		// If we get a game over we will fade out any way
		if((players[0].lives > -1) || (gamestate.difficulty->LivesCount < 0))
			State_FadeOut(state, 0, 255, 64);
		state->stage = DIED6;
		return false;
	}

	state->stage = DEATH_FIZZLE;
	state->rndval = 0;

	return false;
}

void FizzleColorFinish(wl_state_t *state, int x1, int y1,
	unsigned width, unsigned height, unsigned frames, byte color)
{
	unsigned y;

	for (y = y1; y < (y1 + height); ++y)
	{
		memset(screen->GetBuffer() + (y * SCREENPITCH) + x1, color,
		       width); 
	}
	VH_UpdateScreen();
}

extern unsigned int rndbits_y;
extern unsigned int rndmask;

bool FizzleColorStep(wl_state_t *state, int x1, int y1,
	unsigned width, unsigned height, unsigned frames, byte color)
{
	unsigned pixperframe;

	pixperframe = width * height / frames;
	
	for(unsigned p = 0; p < pixperframe * tics; p++)
	{
		//
		// seperate random value into x/y pair
		//

		unsigned x = state->rndval >> rndbits_y;
		unsigned y = state->rndval & ((1 << rndbits_y) - 1);

		//
		// advance to next random element
		//

		state->rndval = (state->rndval >> 1) ^ (state->rndval & 1 ? 0 : rndmask);

		if(x >= width || y >= height)
		{
			if(state->rndval == 0) {    // entire sequence has been completed
				FizzleColorFinish(state, x1, y1,
						  width, height, frames, color);
				return true;
			}
			p--;
			continue;
		}

		//
		// copy one pixel
		//
		*(screen->GetBuffer() + (y1 + y) * SCREENPITCH + x1 + x)
			= color;

		if(state->rndval == 0) {		// entire sequence has been completed
			FizzleColorFinish(state, x1, y1,
					  width, height, frames, color);
			return true;
		}
	}

	VH_UpdateScreen();
	return false;
}

bool DeathFizzle(wl_state_t *state) {
	// Fizzle fade used a slightly darker shade of red.
	byte fr = RPART(players[0].mo->damagecolor)*2/3;
	byte fg = GPART(players[0].mo->damagecolor)*2/3;
	byte fb = BPART(players[0].mo->damagecolor)*2/3;
	if (!FizzleColorStep(state, viewscreenx,viewscreeny,viewwidth,viewheight,70,ColorMatcher.Pick(fr,fg,fb)))
		return true;

	state->stage = DIED6;
	State_UserInput(state, 100);
	return true;
}

bool Died6(wl_state_t *state)
{
	SD_WaitSoundDone ();
	
	if ((players[0].lives > -1) || (gamestate.difficulty->LivesCount < 0))
		players[0].state = player_t::PST_REBORN;
	if ((players[0].lives > -1) || (gamestate.difficulty->LivesCount < 0))
	{
		state->stage = GAME_LOAD_MAP;
		return false;
	}

	VW_FadeOut ();
	if(screenHeight % 200 != 0)
		VL_ClearScreen(0);

	CheckHighScore (players[0].score,levelInfo);
	state->stage = START_DEMO_INTERMISSION;
	return false;
}

//==========================================================================

static void StripInventory(AActor *actor)
{
	// Remove inventory items that don't transfer (keys for example)
	for(AInventory *inv = actor->inventory;inv;)
	{
		if(inv->interhubamount < 1)
		{
			// Remove the inventory item and clean it up
			AInventory *removeMe = inv;
			inv = inv->inventory;
			players[0].mo->RemoveInventory(removeMe);
			removeMe->Destroy();
			continue;
		}
		else if((inv->itemFlags & IF_INVBAR) && inv->amount > inv->interhubamount)
			inv->amount = inv->interhubamount;

		inv = inv->inventory;
	}
}

/*
===================
=
= GameLoop
=
===================
*/

void StartTravel ();
void FinishTravel ();

bool GameLoopInit (wl_state_t *state)
{
	param_tedlevel = NULL;
	state->died = false;
	state->dointermission = true;

	const FString &map = EpisodeInfo::GetEpisode(state->episode_num).StartMap;
	int difficulty = state->skill_num;
	const ClassDef *playerClass = ClassDef::FindClass(gameinfo.PlayerClasses[0]);

	// void cast can be removed when we move to C++11
	memset ((void*)&gamestate,0,sizeof(gamestate));
	gamestate.difficulty = &SkillInfo::GetSkill(difficulty);
	strncpy(gamestate.mapname, map, 8);
	gamestate.mapname[8] = 0;
	gamestate.playerClass = playerClass;
	levelInfo = &LevelInfo::Find(map);

	// Clear LevelRatios
	LevelRatios.killratio = LevelRatios.secretsratio = LevelRatios.treasureratio =
		LevelRatios.numLevels = LevelRatios.time = 0;

	players[0].state = player_t::PST_ENTER;

	Dialog::ClearConversations();

	startgame = true;

	state->stage = LEVEL_ENTER_TEXT;
	state->stageAfterIntermission = GAME_DRAW_PLAY_SCREEN;
	State_FadeOut(state);
	return false;
}

bool LevelEnterText(wl_state_t *state)
{
	return TransitionText(state, -1, levelInfo->Cluster);
}

void drawPsych ()
{
	ClearSplitVWB ();           // set up for double buffering in split screen

	VWB_DrawFill(TexMan(levelInfo->GetBorderTexture()), 0, 0, screenWidth, screenHeight);

	const bool oldingame = ingame;
	ingame = false;
	DrawPlayScreen(true);
	ingame = oldingame;

	VWB_DrawGraphic(TexMan("GETPSYCH"), 48, 56);

	WindowX = (screenWidth - scaleFactorX*224)/2;
	WindowY = (screenHeight - scaleFactorY*(StatusBar->GetHeight(false)+48))/2;
	WindowW = scaleFactorX * 28 * 8;
	WindowH = scaleFactorY * 48;
}

bool GameMapStart (wl_state_t *state)
{
	if (!loadedgame)
		players[0].score = players[0].oldscore;

	startgame = false;
	if (!loadedgame)
	{
		SetupGameLevel ();
		if(playstate != ex_warped)
		{
			FinishTravel ();
			if(playstate == ex_newmap)
			{
				if(NewMap.flags & NEWMAP_KEEPPOSITION)
				{
					players[0].mo->x = NewMap.x;
					players[0].mo->y = NewMap.y;
				}
				if(NewMap.flags & NEWMAP_KEEPFACING)
					players[0].mo->angle = NewMap.angle;
			}
		}

		if(levelInfo->ResetHealth)
			players[0].health = players[0].mo->health = players[0].mo->SpawnHealth();

		if(levelInfo->ResetInventory)
		{
			players[0].mo->ClearInventory();
			players[0].mo->GiveStartingInventory();
		}

		if(levelInfo->EnsureInventory.Size() > 0)
		{
			for(unsigned int i = 0;i < levelInfo->EnsureInventory.Size();++i)
			{
				const ClassDef *ensure = levelInfo->EnsureInventory[i];
				AInventory *holding = players[0].mo->FindInventory(ensure);

				if(ensure->IsDescendantOf(NATIVE_CLASS(Ammo)))
				{
					// For ammo ensure we have the proper amount
					AAmmo *ammo = static_cast<AAmmo*>(AActor::Spawn(ensure, 0, 0, 0, 0));
					ammo->RemoveFromWorld();

					if(!holding)
						holding = players[0].mo->FindInventory(ammo->GetAmmoType());

					if(holding && holding->amount < ammo->amount)
						ammo->amount -= holding->amount;
					else if(holding && holding->amount >= ammo->amount)
					{
						ammo->Destroy();
						ammo = NULL;
					}

					if(ammo && !ammo->CallTryPickup(players[0].mo))
						ammo->Destroy();
					continue;
				}

				if(holding)
					continue;

				AInventory *item = static_cast<AInventory*>(AActor::Spawn(ensure, 0, 0, 0, 0));
				item->RemoveFromWorld();
				if(!item->CallTryPickup(players[0].mo))
					item->Destroy();
			}
		}
	}
	else
	{
		loadedgame = false;
		StartMusic ();
	}

	ingame = true;


	if (state->died)
	{
		state->died = false;
		fizzlein = true;
		StatusBar->DrawStatusBar();

		state->dointermission = true;
		state->stage = PLAY_START;
		return false;
	}

	if (!state->dointermission) {
		PreloadGraphics(false);
	  	StatusBar->DrawStatusBar();

		state->dointermission = true;
		state->stage = PLAY_START;
		return false;
	}

	drawPsych();
	VW_UpdateScreen ();
	State_FadeIn(state);
	state->stage = PSYCH_1;
	return true;
}

bool updatePsych4(wl_state_t *state)
{
	DrawPlayScreen ();
	StatusBar->DrawStatusBar();
	VW_UpdateScreen ();

	state->dointermission = true;
	state->stage = PLAY_START;
	return true;
}

bool MapChange1(wl_state_t *state)
{
	StripInventory(players[0].mo);

	if(state->dointermission)
		State_Fade(state, 0, 255, RPART(levelInfo->ExitFadeColor), GPART(levelInfo->ExitFadeColor), BPART(levelInfo->ExitFadeColor), levelInfo->ExitFadeDuration);
	state->stage = MAP_CHANGE_2;
	return false;
}

bool MapChange2(wl_state_t *state)
{
	StartTravel ();
	if(state->dointermission) {
		return LevelCompletedState1 (state);
	}
	state->stage = MAP_CHANGE_3;
	State_FadeIn(state);
	return state->dointermission;
}

bool MapChange3(wl_state_t *state)
{
	LevelInfo &nextLevel = LevelInfo::Find(state->nextMap);
	state->stageAfterIntermission = MAP_CHANGE_4;
	if(nextLevel.Cluster != levelInfo->Cluster)
	  return TransitionText(state, levelInfo->Cluster, nextLevel.Cluster);
	
	state->stage = MAP_CHANGE_4;
	return false;
}

bool MapChange4(wl_state_t *state)
{
	if(state->dointermission)
		State_FadeOut (state);
	state->stage = MAP_CHANGE_5;
	return false;
}

bool MapChange5(wl_state_t *state)
{
	FString &next = state->nextMap;
	if(viewsize == 21) DrawPlayScreen();

	if(next.CompareNoCase("EndDemo") == 0)
	{
		CheckHighScore (players[0].score,levelInfo);
		state->stage = START_DEMO_INTERMISSION;
		return true;
	}

	players[0].oldscore = players[0].score;
	
	strncpy(gamestate.mapname, next, 8);
	gamestate.mapname[8] = 0;
	state->stage = GAME_LOAD_MAP;
	return false;
}

bool VictoryZoomerStart (wl_state_t *state)
{
	if(gameinfo.VictoryPic.IsNotEmpty()) {
		state->stage = GAME_END_MAP;
		return false;
	}
	
	FTextureID ywin = TexMan.CheckForTexture(gameinfo.VictoryPic, FTexture::TEX_Any);
	if(!ywin.isValid()) {
		state->stage = GAME_END_MAP;
		return false;
	}

	TObjPtr<SpriteZoomer> zoomer = new SpriteZoomer(ywin, 192);
	if(!zoomer) {
		state->stage = GAME_END_MAP;
		return false;
	}

	state->zoomer = zoomer;
	state->stage = VICTORY_ZOOMER_STEP;
	return false;
}

bool VictoryZoomerStep (wl_state_t *state)
{
	for(unsigned int t = tics;state->zoomer && t-- > 0;)
		state->zoomer->Tick();
	if(!state->zoomer) {
		state->stage = GAME_END_MAP;
		return false;
	}
		
	ThreeDRefresh();
	state->zoomer->Draw();
	VH_UpdateScreen();
	return true;
}

bool GameMapEnd (wl_state_t *state)
{
	StopMusic ();
	ingame = false;

	if (demorecord && playstate != ex_warped)
		FinishDemoRecord ();

	if (startgame || loadedgame) {
		state->stage = START_GAME;
		return false;
	}

	switch (playstate)
	{
	case ex_completed:
	case ex_secretlevel:
	case ex_newmap:
	case ex_victorious:
	{
		state->dointermission = !levelInfo->NoIntermission;

		FString next;
		if(playstate != ex_newmap)
		{
			switch(playstate)
			{
			case ex_secretlevel:
				next = levelInfo->NextSecret;
				break;
			case ex_victorious:
				if(!levelInfo->NextVictory.IsEmpty())
				{
					next = levelInfo->NextVictory;
					break;
				}
			default:
				next = levelInfo->NextMap;
			}

			state->nextMap = next;

			state->stage = MAP_CHANGE_1;

			if(next.IndexOf("EndSequence:") == 0 || next.CompareNoCase("EndTitle") == 0)
			{
				state->stage = END_SEQUENCE_1;
				return false;
			}

			return false;
		}
		else
		{
			NewMap.x = players[0].mo->x;
			NewMap.y = players[0].mo->y;
			NewMap.angle = players[0].mo->angle;

			LevelInfo &teleportMap = LevelInfo::FindByNumber(NewMap.newmap);
			if(teleportMap.MapName[0] == 0)
				Quit("Tried to teleport to unkown map.");
			next = teleportMap.MapName;
		}

		state->nextMap = next;
		state->stage = MAP_CHANGE_1;
		return false;
	}

	case ex_died:
		state->stage = DIED1;
		state->died = true;                    // don't "get psyched!"
		return false;

	case ex_warped:
		players[0].state = player_t::PST_ENTER;
		break;

	default:
		if(viewsize == 21) DrawPlayScreen();
		break;
	}
	state->stage = GAME_LOAD_MAP;
	return true;
}
