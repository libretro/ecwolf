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

		static void			AddFile(const char* extension, FResourceFile *file, Type type);
		// For Read This screens which reference VGAGraph entries by number.
		static const char*	ConvertIndexToLump(int num) { return reverseMap[num].c_str(); }
		static void			RemapAll();
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
		static std::map<int, std::string>			reverseMap;
};

#endif
