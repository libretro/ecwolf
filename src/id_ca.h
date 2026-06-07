#ifndef __ID_CA__
#define __ID_CA__

//===========================================================================

extern  class GameMap *map;
extern	class LevelInfo *levelInfo;

//===========================================================================

void CA_CacheMap (const char *mapname, bool loading);

// Release the currently cached map and its thinkers. Used at shutdown to
// control reap ordering relative to ClassDef::UnloadActors().
void CA_DisposeMap();

void CA_CacheScreen (class FTexture *tex, bool noaspect=false);

#endif
