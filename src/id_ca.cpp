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

#include "actor.h"
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

// Deleter function for our map object. Ensures that any thinkers that may be
// referencing the map are cleaned up first. Note that this can't be done
// during the GameMap object's destructor since there can be more than one
// GameMap object.
void CA_UnloadMap(GameMap *map)
{
	// Flush pending spawn operations. Generally this does nothing, but we want
	// to ensure that defferred operations are cleared.
	AActor::FinishSpawningActors();

	thinkerList.DestroyAll();
	delete map;

	// Don't dangle a reference to the map we just unloaded.
	if(::map == map)
		::map = NULL;
}

// Owning handle for the currently cached map. File-scope (rather than a
// function-static inside CA_CacheMap) so it can be released explicitly during
// shutdown via CA_DisposeMap. Releasing it reaps the map's thinkers, which
// dereference their actors' ClassDef::FlatPointers; if that were left to a
// static-duration destructor at process exit it would run after
// ClassDef::UnloadActors() has freed those ClassDefs - a use-after-free.
static TUniquePtr<GameMap, TFuncDeleter<GameMap, CA_UnloadMap> > mapHandle;

// Release the cached map (and its thinkers) now. Called from retro_deinit
// before ClassDef::UnloadActors() so the reap happens while the actor classes
// are still alive.
void CA_DisposeMap()
{
	mapHandle.Reset();
}

/*
======================
=
= CA_CacheMap
=
======================
*/

void CA_CacheMap (const FString &mapname, bool loading)
{
	mapHandle.Reset();

	strncpy(gamestate.mapname, mapname, 8);
	levelInfo = &LevelInfo::Find(mapname);
	::map = mapHandle = new GameMap(mapname);
	if(!map->IsValid())
	{
		// Construction failed (missing/invalid map data). The error has
		// already been logged; leave the map loaded-but-empty rather than
		// dereferencing invalid data.
		return;
	}
	if(!map->LoadMap(loading))
		return;

	CalcVisibility(gLevelVisibility);
}

