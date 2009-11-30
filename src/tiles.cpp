#include <cmath>

#include "tiles.h"
#include "scanner.hpp"
#include "w_wad.h"

using namespace std;

Texture::Texture(const char* name, unsigned int width, unsigned int height) : name(name), width(width), height(height), pixels(NULL)
{
	pixels = new byte[width*height];
	memset(pixels, 0, width*height); // Initialize the pixels
}

Texture::~Texture()
{
//	if(pixels != NULL)
//		delete[] pixels;
}

void Texture::AddPatch(const char* lump, unsigned int xOffset, unsigned int yOffset)
{
	int lumpNum = Wads.CheckNumForName(lump, ns_patches);
	if(lumpNum == -1)
	{
		printf("WARNING: Patch '%s' missing for texture '%s'.\n", lump, name.c_str());
		return;
	}

	// Lump found, draw it to the texture.
	FWadLump lumpReader = Wads.OpenLumpNum(lumpNum);
	int lumpSize = Wads.LumpLength(lumpNum);
	int patchSize = static_cast<int> (floor(sqrt(static_cast<float> (lumpSize))));
	for(int x = 0;x < patchSize && x+xOffset < width;x++)
	{
		byte* column = new byte[patchSize];
		lumpReader.Read(column, patchSize);
		for(int y = 0;y < patchSize && y+yOffset < height;y++)
		{
			pixels[(x+xOffset)*height+(y+yOffset)] = column[y];
		}
		delete[] column;
	}
}

const byte* Texture::GetPost(unsigned int which) const
{
	return pixels+(which % width)*height;
}

Texture &Texture::operator= (const Texture &other)
{
	name = other.name;
	width = other.width;
	height = other.height;

	if(pixels != NULL)
	{
		delete[] pixels;
		pixels = NULL;
	}
	pixels = new byte[width*height];
	memcpy(pixels, other.pixels, width*height);
	return *this;
}

////////////////////////////////////////////////////////////////////////////////

TextureManager TexMan;

TextureManager::TextureManager()
{
}

void TextureManager::Init()
{
	printf("TextureManager: Loading texture defintions.\n");

	int lastLump = 0;
	int lump = 0;
	while((lump = Wads.FindLump("TEXTURES", &lastLump)) != -1)
	{
		ParseTexturesLump(lump);
	}
}

void TextureManager::ParseTexturesLump(int lumpNum)
{
	FWadLump lump = Wads.OpenLumpNum(lumpNum);
	char* data = new char[Wads.LumpLength(lumpNum)];
	lump.Read(data, Wads.LumpLength(lumpNum));
	Scanner sc(data, Wads.LumpLength(lumpNum));

	while(sc.TokensLeft())
	{
		sc.MustGetToken(TK_Identifier);
		if(sc.str.compare("texture") == 0)
		{
			string name;
			unsigned int width;
			unsigned int height;

			sc.MustGetToken(TK_StringConst);
			name = sc.str;
			sc.MustGetToken(',');
			sc.MustGetToken(TK_IntConst);
			width = sc.number;
			sc.MustGetToken(',');
			sc.MustGetToken(TK_IntConst);
			height = sc.number;

			Texture tex(name.c_str(), width, height);
			// See if we're defining more than a NULL texture
			if(sc.CheckToken('{'))
			{
				while(!sc.CheckToken('}'))
				{
					sc.MustGetToken(TK_Identifier);
					if(sc.str.compare("patch") == 0)
					{
						string patchName;
						unsigned int xOffset;
						unsigned int yOffset;

						sc.MustGetToken(TK_StringConst);
						patchName = sc.str;
						sc.MustGetToken(',');
						sc.MustGetToken(TK_IntConst);
						xOffset = sc.number;
						sc.MustGetToken(',');
						sc.MustGetToken(TK_IntConst);
						yOffset = sc.number;
						tex.AddPatch(patchName.c_str(), xOffset, yOffset);
					}
				}
			}
			textures[name] = tex;
		}
	}
}

const Texture *TextureManager::operator[] (const string &texture) const
{
	map<string, Texture>::const_iterator it = textures.find(texture);
	if(it != textures.end())
		return &it->second;
	return NULL;
}
