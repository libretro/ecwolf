// WL_DEBUG.C

#ifdef _WIN32
	#include <io.h>
#else
	#include <unistd.h>
#endif

#include "wl_def.h"
#include "wl_menu.h"
#include "id_ca.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#include "g_mapinfo.h"
#include "actor.h"
#include "wl_agent.h"
#include "wl_debug.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_inter.h"
#include "wl_play.h"
#include "w_wad.h"
#include "thingdef/thingdef.h"
#include "g_shared/a_keys.h"

#ifdef USE_CLOUDSKY
#include "wl_cloudsky.h"
#endif

/*
=============================================================================

												LOCAL CONSTANTS

=============================================================================
*/

#define VIEWTILEX       (viewwidth/16)
#define VIEWTILEY       (viewheight/16)

/*
=============================================================================

												GLOBAL VARIABLES

=============================================================================
*/


int DebugKeys (void);


// from WL_DRAW.C

void ScalePost();

/*
=============================================================================

												LOCAL VARIABLES

=============================================================================
*/

int     maporgx;
int     maporgy;
enum ViewType {mapview,tilemapview,actoratview,visview};
ViewType viewtype;

void ViewMap (void);

//===========================================================================

/*
===================
=
= PictureGrabber
=
===================
*/
void PictureGrabber (void)
{
	static char fname[] = "WSHOT000.BMP";

	for(int i = 0; i < 1000; i++)
	{
		fname[7] = i % 10 + '0';
		fname[6] = (i / 10) % 10 + '0';
		fname[5] = i / 100 + '0';
		int file = open(fname, O_RDONLY | O_BINARY);
		if(file == -1) break;       // file does not exist, so use that filename
		close(file);
	}

	// overwrites WSHOT999.BMP if all wshot files exist

	SDL_SaveBMP(curSurface, fname);

	US_CenterWindow (18,2);
	US_PrintCentered ("Screenshot taken");
	VW_UpdateScreen();
	IN_Ack();
}


//===========================================================================

/*
===================
=
= BasicOverhead
=
===================
*/

void BasicOverhead (void)
{
#if 0
	int x, y, z, offx, offy;

	z = 128/MAPSIZE; // zoom scale
	offx = 320/2;
	offy = (160-MAPSIZE*z)/2;

#ifdef MAPBORDER
	int temp = viewsize;
	NewViewSize(16);
	DrawPlayBorder();
#endif

	// right side (raw)

	for(x=0;x<MAPSIZE;x++)
		for(y=0;y<MAPSIZE;y++)
			VWB_Bar(x*z+offx, y*z+offy,z,z,(unsigned)(uintptr_t)actorat[x][y]);

	// left side (filtered)

	uintptr_t tile;
	int color;
	offx -= 128;

	for(x=0;x<MAPSIZE;x++)
	{
		for(y=0;y<MAPSIZE;y++)
		{
			tile = (uintptr_t)actorat[x][y];
			if (ISPOINTER(tile) && ((objtype *)tile)->flags&FL_SHOOTABLE) color = 72;  // enemy
			else if (!tile || ISPOINTER(tile))
			{
				if (spotvis[x][y]) color = 111;  // visable
				else color = 0;  // nothing
			}
			//else if (MAPSPOT(x,y,1) == PUSHABLETILE) color = 171;  // pushwall
			else if (tile == 64) color = 158; // solid obj
			else if (tile < 128) color = 154;  // walls
			else if (tile < 256) color = 146;  // doors

			VWB_Bar(x*z+offx, y*z+offy,z,z,color);
		}
	}

	VWB_Bar(players[0].mo->tilex*z+offx,players[0].mo->tiley*z+offy,z,z,15); // players[0].mo

	// resize the border to match

	VW_UpdateScreen();
	IN_Ack();

#ifdef MAPBORDER
	NewViewSize(temp);
	DrawPlayBorder();
#endif
#endif
}

//===========================================================================


/*
================
=
= DebugKeys
=
================
*/

int DebugKeys (void)
{
	bool esc;
	int level;
	char str[80];

	if (Keyboard[sc_B])             // B = border color
	{
		US_CenterWindow(22,3);
		PrintY+=6;
		US_Print(SmallFont, " Border texture: ");
		VW_UpdateScreen();
		esc = !US_LineInput (PrintX,py,str,NULL,true,8,WindowX+WindowW-PrintX,GPalette.WhiteIndex);
		if (!esc)
		{
			FTextureID texID = TexMan.CheckForTexture(str, FTexture::TEX_Any);
			if (texID.isValid())
			{
				levelInfo->BorderTexture = texID;
				DrawPlayBorder();

				return 0;
			}
		}
		return 1;
	}
	if (Keyboard[sc_C])             // C = count objects
	{
		US_CenterWindow (17,4);

		FString actorCount;
		actorCount.Format("\nTotal actors : %d", AActor::actors.Size());

		US_Print (SmallFont, actorCount);

		VW_UpdateScreen();
		IN_Ack ();
		return 1;
	}
	if (Keyboard[sc_D])             // D = Darkone's FPS counter
	{
		US_CenterWindow (22,2);
		if (fpscounter)
			US_PrintCentered ("Darkone's FPS Counter OFF");
		else
			US_PrintCentered ("Darkone's FPS Counter ON");
		VW_UpdateScreen();
		IN_Ack();
		fpscounter ^= 1;
		return 1;
	}
	if (Keyboard[sc_E])             // E = quit level
		playstate = ex_completed;

	if (Keyboard[sc_F])             // F = facing spot
	{
		FString position;
		position.Format("X: %d\nY: %d\nA: %d",
			players[0].mo->x >> 10,
			players[0].mo->y >> 10,
			players[0].mo->angle/ANGLE_1
		);
		char str[60];
		US_CenterWindow (14,6);
		US_PrintCentered(position);
		VW_UpdateScreen();
		IN_Ack();
		return 1;
	}

	if (Keyboard[sc_G])             // G = god mode
	{
		US_CenterWindow (12,2);
		if (godmode == 0)
			US_PrintCentered ("God mode ON");
		else if (godmode == 1)
			US_PrintCentered ("God (no flash)");
		else if (godmode == 2)
			US_PrintCentered ("God mode OFF");

		VW_UpdateScreen();
		IN_Ack();
		if (godmode != 2)
			godmode++;
		else
			godmode = 0;
		return 1;
	}
	if (Keyboard[sc_H])             // H = hurt self
	{
		IN_ClearKeysDown ();
		TakeDamage (16,NULL);
	}
	else if (Keyboard[sc_I])        // I = item cheat
	{
		US_CenterWindow (12,3);
		US_PrintCentered ("Free items!");
		VW_UpdateScreen();
		// Give Weapons and Max out ammo
		ClassDef::ClassIterator iter = ClassDef::GetClassIterator();
		ClassDef::ClassPair *pair;
		while(iter.NextPair(pair))
		{
			const ClassDef *cls = pair->Value;
			AInventory *inv = NULL;
			if((cls->IsDescendantOf(NATIVE_CLASS(Weapon)) && cls != NATIVE_CLASS(Weapon)) ||
				(cls->GetParent() == NATIVE_CLASS(Ammo))
			)
			{
				inv = (AInventory *) AActor::Spawn(cls, 0, 0, 0);
				inv->RemoveFromWorld();
				if(cls->GetParent() == NATIVE_CLASS(Ammo))
					inv->amount = inv->maxamount;
				else if(!cls->FindState("Ready"))
				{ // Only give valid weapons
					inv->Destroy();
					continue;
				}

				if(!inv->TryPickup(players[0].mo))
					inv->Destroy();
			}
		}
		GivePoints (100000);
		players[0].health = 100;
		DrawStatusBar();
		IN_Ack ();
		return 1;
	}
	else if (Keyboard[sc_K])        // K = give keys
	{
		US_CenterWindow(16,3);
		PrintY+=6;
		US_Print(SmallFont, "  Give Key (#): ");
		VW_UpdateScreen();
		esc = !US_LineInput (PrintX,py,str,NULL,true,3,WindowX+WindowW-PrintX,GPalette.WhiteIndex);
		if (!esc)
		{
			level = atoi (str);
			P_GiveKeys(players[0].mo, level);
		}
		return 1;
	}
	else if (Keyboard[sc_L])        // L = level ratios
	{
		int ak = 0, as = 0, at = 0;
		if(LevelRatios.numLevels)
		{
			ak = LevelRatios.killratio / LevelRatios.numLevels;
			as = LevelRatios.secretsratio / LevelRatios.numLevels;
			at = LevelRatios.treasureratio / LevelRatios.numLevels;
		}
		FString ratios;
		ratios.Format(
			"Current Level: %02d:%02d\nKills: %d%%\nSecrets: %d%%\nTreasure: %d%%\n\n"
			"Averages: %02d:%02d\nKills: %d%%\nSecrets: %d%%\nTreasure: %d%%",
			gamestate.TimeCount/4200, (gamestate.TimeCount/70)%60,
			gamestate.killcount*100/gamestate.killtotal,
			gamestate.secretcount*100/gamestate.secrettotal,
			gamestate.treasurecount*100/gamestate.treasuretotal,
			LevelRatios.time/60, LevelRatios.time%60,
			ak, as, at
 		);
		US_CenterWindow(17, 12);
		US_PrintCentered(ratios);
		VW_UpdateScreen();
		IN_Ack();

		return 1;
	}
	else if (Keyboard[sc_N])        // N = no clip
	{
		noclip^=1;
		US_CenterWindow (18,3);
		if (noclip)
			US_PrintCentered ("No clipping ON");
		else
			US_PrintCentered ("No clipping OFF");
		VW_UpdateScreen();
		IN_Ack ();
		return 1;
	}
	else if (Keyboard[sc_O])        // O = basic overhead
	{
		BasicOverhead();
		return 1;
	}
	else if(Keyboard[sc_P])         // P = Ripper's picture grabber
	{
		PictureGrabber();
		return 1;
	}
	else if (Keyboard[sc_Q])        // Q = fast quit
		Quit (NULL);
	else if (Keyboard[sc_S])        // S = slow motion
	{
		US_CenterWindow(30,3);
		PrintY+=6;
		US_Print(SmallFont, " Slow Motion steps (default 14): ");
		VW_UpdateScreen();
		esc = !US_LineInput (PrintX,py,str,NULL,true,2,WindowX+WindowW-PrintX,GPalette.WhiteIndex);
		if (!esc)
		{
			level = atoi (str);
			if (level>=0 && level<=50)
				singlestep = level;
		}
		return 1;
	}
	else if (Keyboard[sc_T])
	{
		notargetmode = !notargetmode;
		US_CenterWindow (20,3);
		if(notargetmode)
			US_PrintCentered("No target mode ON");
		else
			US_PrintCentered("No target mode OFF");
		VW_UpdateScreen();
		IN_Ack ();
		return 1;
	}
	else if (Keyboard[sc_V])        // V = extra VBLs
	{
		US_CenterWindow(30,3);
		PrintY+=6;
		US_Print(SmallFont, "  Add how many extra VBLs(0-8): ");
		VW_UpdateScreen();
		esc = !US_LineInput (PrintX,py,str,NULL,true,1,WindowX+WindowW-PrintX,GPalette.WhiteIndex);
		if (!esc)
		{
			level = atoi (str);
			if (level>=0 && level<=8)
				extravbls = level;
		}
		return 1;
	}
	else if (Keyboard[sc_W])        // W = warp to level
	{
		US_CenterWindow(26,3);
		PrintY+=6;
		US_Print(SmallFont, "  Warp to which level: ");
		VW_UpdateScreen();
		esc = !US_LineInput (PrintX,py,str,NULL,true,8,WindowX+WindowW-PrintX,GPalette.WhiteIndex);
		if (!esc && Wads.CheckNumForName(str) != -1)
		{
			strncpy(gamestate.mapname, str, 8);
			gamestate.mapname[8] = 0;
			playstate = ex_warped;
		}
		return 1;
	}
	else if (Keyboard[sc_X])        // X = item cheat
	{
		US_CenterWindow (22,3);
		PrintY += 6;
		US_Print (SmallFont, "Give: ");
		VW_UpdateScreen();
		esc = !US_LineInput (PrintX,py,str,NULL,true,22,WindowX+WindowW-PrintX,GPalette.WhiteIndex);
		if (!esc)
		{
			const ClassDef *cls = ClassDef::FindClass(str);
			if(!cls || !cls->IsDescendantOf(NATIVE_CLASS(Inventory)))
				return 1;

			AInventory *inv = (AInventory *) AActor::Spawn(cls, 0, 0, 0);
			inv->RemoveFromWorld();
			if(!inv->TryPickup(players[0].mo))
				inv->Destroy();
		}
		return 1;
	}
#ifdef USE_CLOUDSKY
	else if(Keyboard[sc_Z])
	{
		char defstr[15];

		US_CenterWindow(34,4);
		PrintY+=6;
		US_Print(SmallFont, "  Recalculate sky with seek: ");
		int seekpx = px, seekpy = py;
		US_PrintUnsigned(curSky->seed);
		US_Print(SmallFont, "\n  Use color map (0-");
		US_PrintUnsigned(numColorMaps - 1);
		US_Print(SmallFont, "): ");
		int mappx = px, mappy = py;
		US_PrintUnsigned(curSky->colorMapIndex);
		VW_UpdateScreen();

		sprintf(defstr, "%u", curSky->seed);
		esc = !US_LineInput(seekpx, seekpy, str, defstr, true, 10, 0,GPalette.WhiteIndex);
		if(esc) return 0;
		curSky->seed = (uint32_t) atoi(str);

		sprintf(defstr, "%u", curSky->colorMapIndex);
		esc = !US_LineInput(mappx, mappy, str, defstr, true, 10, 0,GPalette.WhiteIndex);
		if(esc) return 0;
		uint32_t newInd = (uint32_t) atoi(str);
		if(newInd < (uint32_t) numColorMaps)
		{
			curSky->colorMapIndex = newInd;
			InitSky();
		}
		else
		{
			US_CenterWindow (18,3);
			US_PrintCentered ("Illegal color map!");
			VW_UpdateScreen();
			IN_Ack ();
		}
	}
#endif

	return 0;
}


#if 0
/*
===================
=
= OverheadRefresh
=
===================
*/

void OverheadRefresh (void)
{
	unsigned        x,y,endx,endy,sx,sy;
	unsigned        tile;


	endx = maporgx+VIEWTILEX;
	endy = maporgy+VIEWTILEY;

	for (y=maporgy;y<endy;y++)
	{
		for (x=maporgx;x<endx;x++)
		{
			sx = (x-maporgx)*16;
			sy = (y-maporgy)*16;

			switch (viewtype)
			{
#if 0
				case mapview:
					tile = *(mapsegs[0]+farmapylookup[y]+x);
					break;

				case tilemapview:
					tile = tilemap[x][y];
					break;

				case visview:
					tile = spotvis[x][y];
					break;
#endif
				case actoratview:
					tile = (unsigned)actorat[x][y];
					break;
			}

			if (tile<MAXWALLTILES)
				LatchDrawTile(sx,sy,tile);
			else
			{
				LatchDrawChar(sx,sy,NUMBERCHARS+((tile&0xf000)>>12));
				LatchDrawChar(sx+8,sy,NUMBERCHARS+((tile&0x0f00)>>8));
				LatchDrawChar(sx,sy+8,NUMBERCHARS+((tile&0x00f0)>>4));
				LatchDrawChar(sx+8,sy+8,NUMBERCHARS+(tile&0x000f));
			}
		}
	}
}
#endif

#if 0
/*
===================
=
= ViewMap
=
===================
*/

void ViewMap (void)
{
	boolean         button0held;

	viewtype = actoratview;
	//      button0held = false;


	maporgx = players[0].mo->tilex - VIEWTILEX/2;
	if (maporgx<0)
		maporgx = 0;
	if (maporgx>MAPSIZE-VIEWTILEX)
		maporgx=MAPSIZE-VIEWTILEX;
	maporgy = players[0].mo->tiley - VIEWTILEY/2;
	if (maporgy<0)
		maporgy = 0;
	if (maporgy>MAPSIZE-VIEWTILEY)
		maporgy=MAPSIZE-VIEWTILEY;

	do
	{
		//
		// let user pan around
		//
		PollControls ();
		if (controlx < 0 && maporgx>0)
			maporgx--;
		if (controlx > 0 && maporgx<mapwidth-VIEWTILEX)
			maporgx++;
		if (controly < 0 && maporgy>0)
			maporgy--;
		if (controly > 0 && maporgy<mapheight-VIEWTILEY)
			maporgy++;

#if 0
		if (c.button0 && !button0held)
		{
			button0held = true;
			viewtype++;
			if (viewtype>visview)
				viewtype = mapview;
		}
		if (!c.button0)
			button0held = false;
#endif

		OverheadRefresh ();

	} while (!Keyboard[sc_Escape]);

	IN_ClearKeysDown ();
}
#endif
