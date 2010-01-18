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

#include <sys/types.h>
#if defined _WIN32
	#include <io.h>
#elif !defined _arch_dreamcast
	#include <sys/uio.h>
	#include <unistd.h>
#endif

#pragma pack(1)
#include "wl_def.h"
#include "id_sd.h"
#include "id_vl.h"
#include "id_vh.h"
#include "w_wad.h"

/*
=============================================================================

							LOCAL CONSTANTS

=============================================================================
*/

typedef struct
{
	word RLEWtag;
	int32_t headeroffsets[100];
} mapfiletype;


/*
=============================================================================

							GLOBAL VARIABLES

=============================================================================
*/

#define BUFFERSIZE 0x1000
static int32_t bufferseg[BUFFERSIZE/4];

int     mapon;

word    *mapsegs[MAPPLANES];
static maptype* mapheaderseg[NUMMAPS];

word    RLEWtag;

int     numEpisodesMissing = 0;

/*
=============================================================================

							LOCAL VARIABLES

=============================================================================
*/

char extension[5]; // Need a string, not constant to change cache files
static const char mheadname[] = "maphead.";
static const char mfilename[] = "maptemp.";

void CA_CannotOpen(const char *string);

int    maphandle = -1;              // handle to MAPTEMP / GAMEMAPS

SDMode oldsoundmode;

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
==========================
=
= CA_LoadFile
=
= Allocate space for and load a file
=
==========================
*/

boolean CA_LoadFile (const char *filename, memptr *ptr)
{
	int32_t size;

	const int handle = open(filename, O_RDONLY | O_BINARY);
	if (handle == -1)
		return false;

	size = lseek(handle, 0, SEEK_END);
	lseek(handle, 0, SEEK_SET);
	*ptr=malloc(size);
	CHECKMALLOCRESULT(*ptr);
	if (!read (handle,*ptr,size))
	{
		close (handle);
		return false;
	}
	close (handle);
	return true;
}

/*
============================================================================

				COMPRESSION routines, see JHUFF.C for more

============================================================================
*/


/*
======================
=
= CAL_CarmackExpand
=
= Length is the length of the EXPANDED data
=
======================
*/

#define NEARTAG 0xa7
#define FARTAG  0xa8

void CAL_CarmackExpand (byte *source, word *dest, int length)
{
	word ch,chhigh,count,offset;
	byte *inptr;
	word *copyptr, *outptr;

	length/=2;

	inptr = (byte *) source;
	outptr = dest;

	while (length>0)
	{
		ch = READWORD(inptr);
		chhigh = ch>>8;
		if (chhigh == NEARTAG)
		{
			count = ch&0xff;
			if (!count)
			{                               // have to insert a word containing the tag byte
				ch |= *inptr++;
				*outptr++ = ch;
				length--;
			}
			else
			{
				offset = *inptr++;
				copyptr = outptr - offset;
				length -= count;
				if(length<0) return;
				while (count--)
					*outptr++ = *copyptr++;
			}
		}
		else if (chhigh == FARTAG)
		{
			count = ch&0xff;
			if (!count)
			{                               // have to insert a word containing the tag byte
				ch |= *inptr++;
				*outptr++ = ch;
				length --;
			}
			else
			{
				offset = READWORD(inptr);
				copyptr = dest + offset;
				length -= count;
				if(length<0) return;
				while (count--)
					*outptr++ = *copyptr++;
			}
		}
		else
		{
			*outptr++ = ch;
			length --;
		}
	}
}

/*
======================
=
= CA_RLEWcompress
=
======================
*/

int32_t CA_RLEWCompress (word *source, int32_t length, word *dest, word rlewtag)
{
	word value,count;
	unsigned i;
	word *start,*end;

	start = dest;

	end = source + (length+1)/2;

	//
	// compress it
	//
	do
	{
		count = 1;
		value = *source++;
		while (*source == value && source<end)
		{
			count++;
			source++;
		}
		if (count>3 || value == rlewtag)
		{
			//
			// send a tag / count / value string
			//
			*dest++ = rlewtag;
			*dest++ = count;
			*dest++ = value;
		}
		else
		{
			//
			// send word without compressing
			//
			for (i=1;i<=count;i++)
				*dest++ = value;
		}

	} while (source<end);

	return (int32_t)(2*(dest-start));
}


/*
======================
=
= CA_RLEWexpand
= length is EXPANDED length
=
======================
*/

void CA_RLEWexpand (word *source, word *dest, int32_t length, word rlewtag)
{
	word value,count,i;
	word *end=dest+length/2;

//
// expand it
//
	do
	{
		value = *source++;
		if (value != rlewtag)
			//
			// uncompressed
			//
			*dest++=value;
		else
		{
			//
			// compressed string
			//
			count = *source++;
			value = *source++;
			for (i=1;i<=count;i++)
				*dest++ = value;
		}
	} while (dest<end);
}



/*
=============================================================================

										CACHE MANAGER ROUTINES

=============================================================================
*/

//==========================================================================


/*
======================
=
= CAL_SetupMapFile
=
======================
*/

void CAL_SetupMapFile (void)
{
	int     i;
	int handle;
	int32_t length,pos;
	char fname[13];

//
// load maphead.ext (offsets and tileinfo for map file)
//
	strcpy(fname,mheadname);
	strcat(fname,extension);

	handle = open(fname, O_RDONLY | O_BINARY);
	if (handle == -1)
		CA_CannotOpen(fname);

	length = NUMMAPS*4+2; // used to be "filelength(handle);"
	mapfiletype *tinf=(mapfiletype *) malloc(sizeof(mapfiletype));
	CHECKMALLOCRESULT(tinf);
	read(handle, tinf, length);
	close(handle);

	RLEWtag=tinf->RLEWtag;

//
// open the data file
//
#ifdef CARMACIZED
	strcpy(fname, "gamemaps.");
	strcat(fname, extension);

	maphandle = open(fname, O_RDONLY | O_BINARY);
	if (maphandle == -1)
		CA_CannotOpen(fname);
#else
	strcpy(fname,mfilename);
	strcat(fname,extension);

	maphandle = open(fname, O_RDONLY | O_BINARY);
	if (maphandle == -1)
		CA_CannotOpen(fname);
#endif

//
// load all map header
//
	for (i=0;i<NUMMAPS;i++)
	{
		pos = tinf->headeroffsets[i];
		if (pos<0)                          // $FFFFFFFF start is a sparse map
			continue;

		mapheaderseg[i]=(maptype *) malloc(sizeof(maptype));
		CHECKMALLOCRESULT(mapheaderseg[i]);
		lseek(maphandle,pos,SEEK_SET);
		read (maphandle,(memptr)mapheaderseg[i],sizeof(maptype));
	}

	free(tinf);

//
// allocate space for 3 64*64 planes
//
	for (i=0;i<MAPPLANES;i++)
	{
		mapsegs[i]=(word *) malloc(maparea*2);
		CHECKMALLOCRESULT(mapsegs[i]);
	}
}

//==========================================================================


/*
======================
=
= CA_Startup
=
= Open all files and load in headers
=
======================
*/

void CA_Startup (void)
{
#ifdef PROFILE
	unlink ("PROFILE.TXT");
	profilehandle = open("PROFILE.TXT", O_CREAT | O_WRONLY | O_TEXT);
#endif

	CAL_SetupMapFile ();

	mapon = -1;
}

//==========================================================================


/*
======================
=
= CA_Shutdown
=
= Closes all files
=
======================
*/

void CA_Shutdown (void)
{
	int i,start;

	if(maphandle != -1)
		close(maphandle);
}

//==========================================================================

/*
======================
=
= CA_CacheMap
=
= WOLF: This is specialized for a 64*64 map size
=
======================
*/

void CA_CacheMap (int mapnum)
{
	int32_t   pos,compressed;
	int       plane;
	word     *dest;
	memptr    bigbufferseg;
	unsigned  size;
	word     *source;
#ifdef CARMACIZED
	word     *buffer2seg;
	int32_t   expanded;
#endif

	mapon = mapnum;

//
// load the planes into the allready allocated buffers
//
	size = maparea*2;

	for (plane = 0; plane<MAPPLANES; plane++)
	{
		pos = mapheaderseg[mapnum]->planestart[plane];
		compressed = mapheaderseg[mapnum]->planelength[plane];

		dest = mapsegs[plane];

		lseek(maphandle,pos,SEEK_SET);
		if (compressed<=BUFFERSIZE)
			source = (word *) bufferseg;
		else
		{
			bigbufferseg=malloc(compressed);
			CHECKMALLOCRESULT(bigbufferseg);
			source = (word *) bigbufferseg;
		}

		read(maphandle,source,compressed);
#ifdef CARMACIZED
		//
		// unhuffman, then unRLEW
		// The huffman'd chunk has a two byte expanded length first
		// The resulting RLEW chunk also does, even though it's not really
		// needed
		//
		expanded = *source;
		source++;
		buffer2seg = (word *) malloc(expanded);
		CHECKMALLOCRESULT(buffer2seg);
		CAL_CarmackExpand((byte *) source, buffer2seg,expanded);
		CA_RLEWexpand(buffer2seg+1,dest,size,RLEWtag);
		free(buffer2seg);

#else
		//
		// unRLEW, skipping expanded length
		//
		CA_RLEWexpand (source+1,dest,size,RLEWtag);
#endif

		if (compressed>BUFFERSIZE)
			free(bigbufferseg);
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
