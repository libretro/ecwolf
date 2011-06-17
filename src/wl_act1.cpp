// WL_ACT1.C

#include "wl_act.h"
#include "wl_def.h"
#include "id_ca.h"
#include "id_sd.h"
#include "thingdef.h"

/*
=============================================================================

														STATICS

=============================================================================
*/


statobj_t       statobjlist[MAXSTATS];
statobj_t       *laststatobj;


struct StatInfo
{
	short      picnum;
	wl_stat_t  type;
	uint32_t   specialFlags;    // they are ORed to the statobj_t flags
};

StatInfo statinfo[] =
{
	{SPR_STAT_0},                           // puddle          spr1v
	{SPR_STAT_1,block},                     // Green Barrel    "
	{SPR_STAT_2,block},                     // Table/chairs    "
	{SPR_STAT_3,block,FL_FULLBRIGHT},       // Floor lamp      "
	{SPR_STAT_4,none,FL_FULLBRIGHT},        // Chandelier      "
	{SPR_STAT_5,block},                     // Hanged man      "
	{SPR_STAT_6,bo_alpo},                   // Bad food        "
	{SPR_STAT_7,block},                     // Red pillar      "
	//
	// NEW PAGE
	//
	{SPR_STAT_8,block},                     // Tree            spr2v
	{SPR_STAT_9},                           // Skeleton flat   "
	{SPR_STAT_10,block},                    // Sink            " (SOD:gibs)
	{SPR_STAT_11,block},                    // Potted plant    "
	{SPR_STAT_12,block},                    // Urn             "
	{SPR_STAT_13,block},                    // Bare table      "
	{SPR_STAT_14,none,FL_FULLBRIGHT},       // Ceiling light   "
	#ifndef SPEAR
	{SPR_STAT_15},                          // Kitchen stuff   "
	#else
	{SPR_STAT_15,block},                    // Gibs!
	#endif
	//
	// NEW PAGE
	//
	{SPR_STAT_16,block},                    // suit of armor   spr3v
	{SPR_STAT_17,block},                    // Hanging cage    "
	{SPR_STAT_18,block},                    // SkeletoninCage  "
	{SPR_STAT_19},                          // Skeleton relax  "
	{SPR_STAT_20,bo_key1},                  // Key 1           "
	{SPR_STAT_21,bo_key2},                  // Key 2           "
	{SPR_STAT_22,block},                    // stuff             (SOD:gibs)
	{SPR_STAT_23},                          // stuff
	//
	// NEW PAGE
	//
	{SPR_STAT_24,bo_food},                  // Good food       spr4v
	{SPR_STAT_25,bo_firstaid},              // First aid       "
	{SPR_STAT_26,bo_clip},                  // Clip            "
	{SPR_STAT_27,bo_machinegun},            // Machine gun     "
	{SPR_STAT_28,bo_chaingun},              // Gatling gun     "
	{SPR_STAT_29,bo_cross},                 // Cross           "
	{SPR_STAT_30,bo_chalice},               // Chalice         "
	{SPR_STAT_31,bo_bible},                 // Bible           "
	//
	// NEW PAGE
	//
	{SPR_STAT_32,bo_crown},                 // crown           spr5v
	{SPR_STAT_33,bo_fullheal,FL_FULLBRIGHT},// one up          "
	{SPR_STAT_34,bo_gibs},                  // gibs            "
	{SPR_STAT_35,block},                    // barrel          "
	{SPR_STAT_36,block},                    // well            "
	{SPR_STAT_37,block},                    // Empty well      "
	{SPR_STAT_38,bo_gibs},                  // Gibs 2          "
	{SPR_STAT_39,block},                    // flag            "
	//
	// NEW PAGE
	//
	#ifndef SPEAR
	{SPR_STAT_40,block},                    // Call Apogee          spr7v
	#else
	{SPR_STAT_40},                          // Red light
	#endif
	//
	// NEW PAGE
	//
	{SPR_STAT_41},                          // junk            "
	{SPR_STAT_42},                          // junk            "
	{SPR_STAT_43},                          // junk            "
	#ifndef SPEAR
	{SPR_STAT_44},                          // pots            "
	#else
	{SPR_STAT_44,block},                    // Gibs!
	#endif
	{SPR_STAT_45,block},                    // stove           " (SOD:gibs)
	{SPR_STAT_46,block},                    // spears          " (SOD:gibs)
	{SPR_STAT_47},                          // vines           "
	//
	// NEW PAGE
	//
	#ifdef SPEAR
	{SPR_STAT_48,block},                    // marble pillar
	{SPR_STAT_49,bo_25clip},                // bonus 25 clip
	{SPR_STAT_50,block},                    // truck
	{SPR_STAT_51,bo_spear},                 // SPEAR OF DESTINY!
	#endif

	{SPR_STAT_26,bo_clip2},                 // Clip            "
#ifdef USE_DIR3DSPR
	// These are just two examples showing the new way of using dir 3d sprites.
	// You can find the allowed values in the objflag_t enum in wl_def.h.
	{SPR_STAT_47,none,FL_DIR_VERT_MID},
	{SPR_STAT_47,block,FL_DIR_HORIZ_MID},
#endif
	{-1}                                    // terminator
};

/*
===============
=
= InitStaticList
=
===============
*/

void InitStaticList (void)
{
	laststatobj = &statobjlist[0];
}



/*
===============
=
= SpawnStatic
=
===============
*/

void SpawnStatic (int tilex, int tiley, int type)
{
	laststatobj->shapenum = statinfo[type].picnum;
	laststatobj->tilex = tilex;
	laststatobj->tiley = tiley;
	laststatobj->visspot = &spotvis[tilex][tiley];

	switch (statinfo[type].type)
	{
		case block:
			actorat[tilex][tiley] = (objtype *) 64;          // consider it a blocking tile
		case none:
			laststatobj->flags = 0;
			break;

		case    bo_cross:
		case    bo_chalice:
		case    bo_bible:
		case    bo_crown:
		case    bo_fullheal:
			if (!loadedgame)
				gamestate.treasuretotal++;

		case    bo_firstaid:
		case    bo_key1:
		case    bo_key2:
		case    bo_key3:
		case    bo_key4:
		case    bo_clip:
		case    bo_25clip:
		case    bo_machinegun:
		case    bo_chaingun:
		case    bo_food:
		case    bo_alpo:
		case    bo_gibs:
		case    bo_spear:
			laststatobj->flags = FL_BONUS;
			laststatobj->itemnumber = statinfo[type].type;
			break;
	}

	laststatobj->flags |= statinfo[type].specialFlags;

	laststatobj++;

	if (laststatobj == &statobjlist[MAXSTATS])
		Quit ("Too many static objects!\n");
}


/*
===============
=
= PlaceItemType
=
= Called during game play to drop actors' items.  It finds the proper
= item number based on the item type (bo_???).  If there are no free item
= spots, nothing is done.
=
===============
*/

void PlaceItemType (int itemtype, int tilex, int tiley)
{
	int type;
	statobj_t *spot;

	//
	// find the item number
	//
	for (type=0; ; type++)
	{
		if (statinfo[type].picnum == -1)                    // end of list
			Quit ("PlaceItemType: couldn't find type!");
		if (statinfo[type].type == itemtype)
			break;
	}

	//
	// find a spot in statobjlist to put it in
	//
	for (spot=&statobjlist[0]; ; spot++)
	{
		if (spot==laststatobj)
		{
			if (spot == &statobjlist[MAXSTATS])
				return;                                     // no free spots
			laststatobj++;                                  // space at end
			break;
		}

		if (spot->shapenum == -1)                           // -1 is a free spot
			break;
	}
	//
	// place it
	//
	spot->shapenum = statinfo[type].picnum;
	spot->tilex = tilex;
	spot->tiley = tiley;
	spot->visspot = &spotvis[tilex][tiley];
	spot->flags = FL_BONUS | statinfo[type].specialFlags;
	spot->itemnumber = statinfo[type].type;
}



/*
=============================================================================

								DOORS

doorobjlist[] holds most of the information for the doors

doorposition[] holds the amount the door is open, ranging from 0 to 0xffff
		this is directly accessed by AsmRefresh during rendering

The number of doors is limited to 64 because a spot in tilemap holds the
		door number in the low 6 bits, with the high bit meaning a door center
		and bit 6 meaning a door side tile

Open doors conect two areas, so sounds will travel between them and sight
		will be checked when the player is in a connected area.

Areaconnect is incremented/decremented by each door. If >0 they connect

Every time a door opens or closes the areabyplayer matrix gets recalculated.
		An area is true if it connects with the player's current spor.

=============================================================================
*/

#define DOORWIDTH       0x7800
#define OPENTICS        300

byte            areaconnect[NUMAREAS][NUMAREAS];

boolean         areabyplayer[NUMAREAS];


/*
==============
=
= ConnectAreas
=
= Scans outward from playerarea, marking all connected areas
=
==============
*/

void RecursiveConnect (int areanumber)
{
	int i;

	for (i=0;i<NUMAREAS;i++)
	{
		if (areaconnect[areanumber][i] && !areabyplayer[i])
		{
			areabyplayer[i] = true;
			RecursiveConnect (i);
		}
	}
}


void ConnectAreas (void)
{
	memset (areabyplayer,0,sizeof(areabyplayer));
	areabyplayer[player->areanumber] = true;
	RecursiveConnect (player->areanumber);
}


void InitAreas (void)
{
	memset (areabyplayer,0,sizeof(areabyplayer));
	if (player->areanumber < NUMAREAS)
		areabyplayer[player->areanumber] = true;
}



