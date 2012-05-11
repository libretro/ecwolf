#ifndef __ID_CA__
#define __ID_CA__

//===========================================================================

extern  int   mapon;

extern  class GameMap *map;
extern	class LevelInfo *levelInfo;

//===========================================================================

void CA_CacheMap (int mapnum);

void CA_CacheScreen (const char* chunk);

#endif
