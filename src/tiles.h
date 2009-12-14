#ifndef __TILES_H__
#define __TILES_H__

#include <map>
#include <string>
#include "wl_def.h"
#include "zstring.h"

class TextureManager;

class Texture
{
	public:
		Texture(const char* name="--NULL--", unsigned int width=1, unsigned int height=1);
		Texture(const Texture &other);
		~Texture();

		unsigned int	GetWidth() const { return width; }
		unsigned int	GetHeight() const { return height; }
		const byte*		GetPost(unsigned int which) const;

		Texture			&operator= (const Texture &other);

	protected:
		void			AddPatch(const char* lump, unsigned int xOffset, unsigned int yOffset);
		Texture			&Copy(const Texture &other);

		friend class TextureManager;

	private:
		std::string		name;
		unsigned int	width;
		unsigned int	height;
		byte*			pixels;
};

class TextureManager
{
	public:
		TextureManager();

		const Texture	*GetDoor(unsigned int tile, bool track=false) const;
		void			Init();

		const Texture	*operator[] (const std::string &texture) const;
		const Texture	*operator() (unsigned int tile) const;

	protected:
		void	ParseTexturesLump(int lumpNum);

	private:
		Texture							nullTexture;
		std::map<std::string, Texture>	textures;
		std::string						mapTiles[64];
		std::string						doorTiles[64][2]; // Door and track
};

extern TextureManager TexMan;

#endif /* __TILES_H__ */
