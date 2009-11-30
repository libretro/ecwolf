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
		~Texture();

		unsigned int	GetWidth() const { return width; }
		unsigned int	GetHeight() const { return height; }
		const byte*		GetPost(unsigned int which) const;

		Texture			&operator= (const Texture &other);

	protected:
		void			AddPatch(const char* lump, unsigned int xOffset, unsigned int yOffset);

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

		void			Init();

		const Texture	*operator[] (const std::string &texture) const;

	protected:
		void	ParseTexturesLump(int lumpNum);

	private:
		std::map<std::string, Texture>	textures;
};

extern TextureManager TexMan;

#endif /* __TILES_H__ */
