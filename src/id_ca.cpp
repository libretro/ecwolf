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
#include "wl_def.h"
#include "id_sd.h"
#include "id_vl.h"
#include "id_vh.h"
#include "w_wad.h"

/*
=============================================================================

							GLOBAL VARIABLES

=============================================================================
*/

int     mapon = -1;

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

void CA_CacheMap (int mapnum)
{
	mapon = mapnum;

	char mapname[9];
	sprintf(mapname, "MAP%02d", mapnum+1);
	delete map;

	levelInfo = &LevelInfo::Find(mapname);
	map = new GameMap(mapname);
}

