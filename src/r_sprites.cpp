/*
** r_sprites.cpp
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include "textures/textures.h"
#include "r_sprites.h"
#include "linkedlist.h"
#include "tarray.h"

static const uint8_t MAX_SPRITE_FRAMES = 29; // A-Z, [, \, ]

TArray<Sprite> spriteFrames;
TArray<SpriteInfo> loadedSprites;

void R_InstallSprite(Sprite &frame, FTexture *tex, int dir, bool mirror)
{
	if(dir < -1 || dir >= 8)
	{
		printf("Invalid frame data for '%s' %d.\n", tex->Name, dir);
		return;
	}

	if(dir == -1)
	{
		frame.rotations = 0;
		dir = 0;
	}
	else
		frame.rotations = 8;

	frame.texture[dir] = tex->GetID();
	frame.mirror |= 1<<dir;
}

int SpriteCompare(const void *s1, const void *s2)
{
	uint32_t n1 = static_cast<const SpriteInfo *>(s1)->iname;
	uint32_t n2 = static_cast<const SpriteInfo *>(s2)->iname;
	if(n1 < n2)
		return -1;
	else if(n1 > n2)
		return 1;
	return 0;
}

void R_InitSprites()
{
	// First sort the loaded sprites list
	qsort(&loadedSprites[0], loadedSprites.Size(), sizeof(loadedSprites[0]), SpriteCompare);

	typedef LinkedList<FTexture*> SpritesList;
	typedef TMap<uint32_t, SpritesList> SpritesMap;

	SpritesMap spritesMap;

	// Collect potential sprite list (linked list of sprites by name)
	for(unsigned int i = 0;i < TexMan.NumTextures();++i)
	{
		FTexture *tex = TexMan.ByIndex(i);
		if(tex->UseType == FTexture::TEX_Sprite && strlen(tex->Name) >= 6)
		{
			SpritesList &list = spritesMap[tex->dwName];
			list.Push(tex);
		}
	}

	// Now process the sprites if we need to load them
	for(unsigned int i = 0;i < loadedSprites.Size();++i)
	{
		SpritesList &list = spritesMap[loadedSprites[i].iname];
		if(list.Size() == 0)
		{
			printf("Sprite '%s' has no frames.\n", loadedSprites[i].name);
			continue;
		}
		loadedSprites[i].frames = spriteFrames.Size();

		Sprite frames[MAX_SPRITE_FRAMES];
		uint8_t maxframes = 0;
		for(unsigned int j = 0;j < MAX_SPRITE_FRAMES;++j)
		{
			frames[j].rotations = Sprite::NO_FRAMES;
			frames[j].mirror = 0;
		}

		for(SpritesList::Node *iter = list.Head();iter;iter = iter->Next())
		{
			FTexture *tex = iter->Item();
			char frame = tex->Name[4] - 'A';
			if(frame < MAX_SPRITE_FRAMES)
			{
				if(frame > maxframes)
					maxframes = frame;
				R_InstallSprite(frames[frame], tex, tex->Name[5] - '1', false);

				if(strlen(tex->Name) == 8)
				{
					frame = tex->Name[6] - 'A';
					if(frame < MAX_SPRITE_FRAMES)
					{
						if(frame > maxframes)
							maxframes = frame;
						R_InstallSprite(frames[frame], tex, tex->Name[7] - '1', true);
					}
				}
			}
		}

		++maxframes;
		for(unsigned int j = 0;j < maxframes;++j)
		{
			// Check rotations
			if(frames[j].rotations == 8)
			{
				for(unsigned int r = 0;r < 8;++r)
				{
					if(!frames[j].texture[r].isValid())
					{
						printf("Sprite %s is missing rotations for frame %c.\n", loadedSprites[i].name, j);
						break;
					}
				}
			}

			spriteFrames.Push(frames[j]);
		}
	}
}

void R_LoadSprite(const FString &name)
{
	if(name.Len() != 4)
	{
		printf("Sprite name invalid.\n");
		return;
	}

	static uint32_t lastSprite = 0;
	SpriteInfo sprInf;

	strcpy(sprInf.name, name.GetChars());
	if(loadedSprites.Size() > 0)
	{
		if(sprInf.iname == lastSprite)
			return;

		for(unsigned int i = 0;i < loadedSprites.Size();++i)
		{
			if(loadedSprites[i].iname == sprInf.iname)
			{
				sprInf = loadedSprites[i];
				lastSprite = sprInf.iname;
				return;
			}
		}
	}
	lastSprite = sprInf.iname;

	loadedSprites.Push(sprInf);
}
