#ifndef __LUMPREMAP_H__
#define __LUMPREMAP_H__

#include "resourcefiles/resourcefile.h"
#include "tarray.h"
#include "zstring.h"

class LumpRemaper
{
	public:
		enum Type
		{
			AUDIOT,
			VGAGRAPH,
			VSWAP
		};

		LumpRemaper(const char* extension);

		void		AddFile(FResourceFile *file, Type type);
		void		DoRemap();

		static void			AddFile(const char* extension, FResourceFile *file, Type type);
		// For Read This screens which reference VGAGraph entries by number.
		static const char*	ConvertMusicIndexToLump(int num) { return musicReverseMap[num]; }
		static const char*	ConvertVGAIndexToLump(int num) { return vgaReverseMap[num]; }
		static void			RemapAll();
	protected:
		bool		LoadMap();
	private:
		struct RemapFile
		{
			FResourceFile	*file;
			Type			type;
		};

		FString				mapLumpName;
		TArray<FName>		graphics, sprites, sounds, digitalsounds, music, textures;
		TArray<RemapFile>	files;

		static TMap<FName, LumpRemaper>	remaps;
		static TMap<int, FName>			musicReverseMap;
		static TMap<int, FName>			vgaReverseMap;
};

#endif
