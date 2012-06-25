// WL_DEBUG.C

#ifdef _WIN32
	#include <io.h>
#else
	#include <unistd.h>
#endif

#include "wl_def.h"
#include "wl_menu.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#include "actor.h"
#include "wl_agent.h"
#include "wl_debug.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_play.h"
#include "w_wad.h"

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
==================
=
= CountObjects
=
==================
*/

void CountObjects (void)
{
	CenterWindow (17,7);

	US_Print (BigFont, "\nTotal actors  :");
	US_PrintUnsigned (AActor::actors.Size());

	VW_UpdateScreen();
	IN_Ack ();
}


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

	CenterWindow (18,2);
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
		CenterWindow(20,3);
		PrintY+=6;
		US_Print(SmallFont, " Border color (0-56): ");
		VW_UpdateScreen();
		esc = !US_LineInput (px,py,str,NULL,true,2,0);
		if (!esc)
		{
			level = atoi (str);
			if (level>=0 && level<=99)
			{
				if (level<30) level += 31;
				else
				{
					if (level > 56) level=31;
					else level -= 26;
				}

				bordercol=level*4+3;

				if (bordercol == VIEWCOLOR)
					DrawStatusBorder(bordercol);

				DrawPlayBorder();

				return 0;
			}
		}
		return 1;
	}
	if (Keyboard[sc_C])             // C = count objects
	{
		CountObjects();
		return 1;
	}
	if (Keyboard[sc_D])             // D = Darkone's FPS counter
	{
		CenterWindow (22,2);
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
		char str[60];
		CenterWindow (14,6);
		US_Print (SmallFont, "x:");     US_PrintUnsigned (players[0].mo->x);
		US_Print (SmallFont, " (");     US_PrintUnsigned (players[0].mo->x%65536);
		US_Print (SmallFont, ")\ny:");  US_PrintUnsigned (players[0].mo->y);
		US_Print (SmallFont, " (");     US_PrintUnsigned (players[0].mo->y%65536);
		US_Print (SmallFont, ")\nA:");  US_PrintUnsigned (players[0].mo->angle);
		US_Print (SmallFont, " X:");    US_PrintUnsigned (players[0].mo->tilex);
		US_Print (SmallFont, " Y:");    US_PrintUnsigned (players[0].mo->tiley);
		//US_Print ("\n1:");   US_PrintUnsigned (tilemap[players[0].mo->tilex][players[0].mo->tiley]);
		//sprintf(str," 2:%.8X",(unsigned)(uintptr_t)actorat[players[0].mo->tilex][players[0].mo->tiley]); US_Print(str);
		//US_Print (" 2:");    US_PrintUnsigned (MAPSPOT(players[0].mo->tilex,players[0].mo->tiley,1));
		//US_Print (" 3:");
		//if ((unsigned)(uintptr_t)actorat[players[0].mo->tilex][players[0].mo->tiley] < 256)
		//	US_PrintUnsigned (spotvis[players[0].mo->tilex][players[0].mo->tiley]);
		//else
		//	US_PrintUnsigned (actorat[players[0].mo->tilex][players[0].mo->tiley]->flags);
		VW_UpdateScreen();
		IN_Ack();
		return 1;
	}

	if (Keyboard[sc_G])             // G = god mode
	{
		CenterWindow (12,2);
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
		CenterWindow (12,3);
		US_PrintCentered ("Free items!");
		VW_UpdateScreen();
		GivePoints (100000);
		players[0].health = 100;
		DrawStatusBar();
		IN_Ack ();
		return 1;
	}
	else if (Keyboard[sc_K])        // K = give keys
	{
		CenterWindow(16,3);
		PrintY+=6;
		US_Print(SmallFont, "  Give Key (1-4): ");
		VW_UpdateScreen();
		esc = !US_LineInput (px,py,str,NULL,true,1,0);
		if (!esc)
		{
			level = atoi (str);
			if (level>0 && level<5)
			{}//GiveKey(level-1);
		}
		return 1;
	}
	else if (Keyboard[sc_L])        // L = level ratios
	{
/*		byte x,start,end=LRpack;

		if (end == 8)   // wolf3d
		{
			CenterWindow(17,10);
			start = 0;
		}
		else            // sod
		{
			CenterWindow(17,12);
			start = 0; end = 10;
		}
again:
		for(x=start;x<end;x++)
		{
			US_PrintUnsigned(x+1);
			US_Print(" ");
			US_PrintUnsigned(LevelRatios[x].time/60);
			US_Print(":");
			if (LevelRatios[x].time%60 < 10)
				US_Print("0");
			US_PrintUnsigned(LevelRatios[x].time%60);
			US_Print(" ");
			US_PrintUnsigned(LevelRatios[x].kill);
			US_Print("% ");
			US_PrintUnsigned(LevelRatios[x].secret);
			US_Print("% ");
			US_PrintUnsigned(LevelRatios[x].treasure);
			US_Print("%\n");
		}
		VW_UpdateScreen();
		IN_Ack();
		if (end == 10 && gamestate.mapon > 9)
		{
			start = 10; end = 20;
			CenterWindow(17,12);
			goto again;
		}*/

		return 1;
	}
	else if (Keyboard[sc_N])        // N = no clip
	{
		noclip^=1;
		CenterWindow (18,3);
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
		CenterWindow(30,3);
		PrintY+=6;
		US_Print(SmallFont, " Slow Motion steps (default 14): ");
		VW_UpdateScreen();
		esc = !US_LineInput (px,py,str,NULL,true,2,0);
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
		CenterWindow (20,3);
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
		CenterWindow(30,3);
		PrintY+=6;
		US_Print(SmallFont, "  Add how many extra VBLs(0-8): ");
		VW_UpdateScreen();
		esc = !US_LineInput (px,py,str,NULL,true,1,0);
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
		CenterWindow(26,3);
		PrintY+=6;
		US_Print(SmallFont, "  Warp to which level: ");
		VW_UpdateScreen();
		esc = !US_LineInput (px,py,str,NULL,true,8,0);
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
		CenterWindow (12,3);
		US_PrintCentered ("Extra stuff!");
		VW_UpdateScreen();
		// DEBUG: put stuff here
		IN_Ack ();
		return 1;
	}
#ifdef USE_CLOUDSKY
	else if(Keyboard[sc_Z])
	{
		char defstr[15];

		CenterWindow(34,4);
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
		esc = !US_LineInput(seekpx, seekpy, str, defstr, true, 10, 0);
		if(esc) return 0;
		curSky->seed = (uint32_t) atoi(str);

		sprintf(defstr, "%u", curSky->colorMapIndex);
		esc = !US_LineInput(mappx, mappy, str, defstr, true, 10, 0);
		if(esc) return 0;
		uint32_t newInd = (uint32_t) atoi(str);
		if(newInd < (uint32_t) numColorMaps)
		{
			curSky->colorMapIndex = newInd;
			InitSky();
		}
		else
		{
			CenterWindow (18,3);
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
