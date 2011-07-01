#include "wl_def.h"
#include "id_vl.h"
#include "textures/textures.h"

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
	FTexture *tex = TexMan(chunk);
	if(!tex)
		return;

	if(tex->GetWidth() != 320 || tex->GetHeight() != 200)
		return;
	const BYTE* pic = tex->GetPixels();
	byte *vbuf = LOCK();
	for(int y = 0, scy = 0; y < 200; y++, scy += scaleFactor)
	{
		for(int x = 0, scx = 0; x < 320; x++, scx += scaleFactor)
		{
			//byte col = pic[(y * 80 + (x >> 2)) + (x & 3) * 80 * 200];
			byte col = pic[x*200+y];
			for(unsigned i = 0; i < scaleFactor; i++)
				for(unsigned j = 0; j < scaleFactor; j++)
					vbuf[(scy + i) * curPitch + scx + j] = col;
		}
	}
	UNLOCK();
}
