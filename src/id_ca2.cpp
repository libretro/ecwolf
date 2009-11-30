#include "wl_def.h"
#include "id_sd.h"
#include "id_vl.h"
#include "id_vh.h"
#include "w_wad.h"

//==========================================================================

/*
======================
=
= CA_CacheScreen
=
= Decompresses a chunk from disk straight onto the screen
=
======================
*/

void CA_CacheScreen(const char* chunk)
{
    const int lumpNum = Wads.GetNumForName(chunk, ns_graphics);
	if(lumpNum == -1)
	{
		printf("\n");
		exit(0);
	}
	int lumpSize = Wads.LumpLength(lumpNum);
	FWadLump lump = Wads.OpenLumpNum(lumpNum);

	if(lumpSize > 64000)
		lump.Seek(4, SEEK_SET); // Probably a standard image.
	else if(lumpSize < 64000)
		return; // Not big enough
	byte* pic = new byte[64000];
	lump.Read(pic, 64000);
	byte *vbuf = LOCK();
	for(int y = 0, scy = 0; y < 200; y++, scy += scaleFactor)
	{
		for(int x = 0, scx = 0; x < 320; x++, scx += scaleFactor)
		{
			byte col = pic[(y * 80 + (x >> 2)) + (x & 3) * 80 * 200];
			for(unsigned i = 0; i < scaleFactor; i++)
				for(unsigned j = 0; j < scaleFactor; j++)
					vbuf[(scy + i) * curPitch + scx + j] = col;
		}
	}
	UNLOCK();
	delete[] pic;
}
