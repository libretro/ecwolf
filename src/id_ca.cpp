// ID_CA.C

// this has been customized for WOLF

/*
=============================================================================

Id Software Caching Manager
---------------------------

Must be started BEFORE the memory manager, because it needs to get the headers
loaded into the data segment

=============================================================================
*/

#include "g_mapinfo.h"
#include "gamemap.h"
#include "tmemory.h"
#include "wl_def.h"
#include "wl_game.h"
#include "w_wad.h"
#include "wl_main.h"
#include "wl_shade.h"

/*
=============================================================================

							GLOBAL VARIABLES

=============================================================================
*/

LevelInfo *levelInfo = NULL;
GameMap *map = NULL;


/*
=============================================================================

										CACHE MANAGER ROUTINES

=============================================================================
*/

/*
======================
=
= CA_CacheMap
=
======================
*/

void CA_CacheMap (const FString &mapname, bool loading)
{
	static TUniquePtr<GameMap> map;
	map.Reset();

	Printf("\n");

	strncpy(gamestate.mapname, mapname, 8);
	levelInfo = &LevelInfo::Find(mapname);
	::map = map = new GameMap(mapname);
	map->LoadMap(loading);

	Printf("\n%s - %s\n\n", mapname.GetChars(), levelInfo->GetName(map).GetChars());

	CalcVisibility(gLevelVisibility);
}

