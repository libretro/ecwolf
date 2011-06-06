#ifndef __ID_CA__
#define __ID_CA__

//===========================================================================

#define MAPPLANES       3

//===========================================================================

extern  int   mapon;

extern  class GameMap *map;
extern  word *mapsegs[MAPPLANES];

extern  char  extension[5];

//===========================================================================

boolean CA_WriteFile (const char *filename, void *ptr, int32_t length);

void CA_CacheMap (int mapnum);

void CA_CacheScreen (const char* chunk);

void CA_CannotOpen(const char *name);

#endif
