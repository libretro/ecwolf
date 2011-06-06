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

GameMap *map = NULL;
word    *mapsegs[MAPPLANES] = {NULL, NULL, NULL};

int     numEpisodesMissing = 0;

/*
=============================================================================

							LOCAL VARIABLES

=============================================================================
*/

char extension[5]; // Need a string, not constant to change cache files

/*
=============================================================================

							LOW LEVEL ROUTINES

=============================================================================
*/

/*
==========================
=
= CA_WriteFile
=
= Writes a file from a memory buffer
=
==========================
*/

boolean CA_WriteFile (const char *filename, void *ptr, int32_t length)
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
	map = new GameMap(mapname);
	int lmp = Wads.GetNumForName(mapname);
	FWadLump lump = Wads.OpenLumpNum(lmp+1);
	lump.Seek(0, SEEK_SET);

	WORD dimensions[2];
	lump.Read(dimensions, 4);
	LittleShort(dimensions[0]);
	LittleShort(dimensions[1]);
	DWORD size = dimensions[0]*dimensions[1]*2;
	lump.Seek(20, SEEK_SET);

	for (int plane = 0; plane<MAPPLANES; plane++)
	{
		if(mapsegs[plane] != NULL)
			delete[] mapsegs[plane];
		mapsegs[plane] = new word[size/2];
		lump.Read(mapsegs[plane], size);
	}
}

//===========================================================================

void CA_CannotOpen(const char *string)
{
	char str[30];

	strcpy(str,"Can't open ");
	strcat(str,string);
	strcat(str,"!\n");
	Quit (str);
}
