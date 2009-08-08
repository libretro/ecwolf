#ifndef __LUMPREMAP_H__
#define __LUMPREMAP_H__

#include <string>
#include <deque>
#include <map>

#include "resourcefiles/resourcefile.h"
#include "zstring.h"

class LumpRemaper
{
	public:
		enum Type
		{
			VGAGRAPH
		};

		LumpRemaper(const char* extension);

		void		AddFile(FResourceFile *file, Type type);
		void		DoRemap();

		static void	AddFile(const char* extension, FResourceFile *file, Type type);
		static void RemapAll();
	protected:
		bool		LoadMap();
	private:
		struct RemapFile
		{
			FResourceFile	*file;
			Type			type;
		};

		FString					mapLumpName;
		std::deque<std::string>	graphics, sprites, sounds, music, textures;
		std::deque<RemapFile>	files;

		static std::map<std::string, LumpRemaper>	remaps;
};

#endif
