#include "lumpremap.h"
#include "w_wad.h"
#include "zstring.h"
#include "scanner.hpp"
using namespace std;

map<string, LumpRemaper> LumpRemaper::remaps;
map<int, string> LumpRemaper::musicReverseMap;
map<int, string> LumpRemaper::vgaReverseMap;

LumpRemaper::LumpRemaper(const char* extension) : mapLumpName(extension)
{
	mapLumpName.ToUpper();
	mapLumpName += "MAP";
}

void LumpRemaper::AddFile(const char* extension, FResourceFile *file, Type type)
{
	map<string, LumpRemaper>::iterator iter = remaps.find(extension);
	if(iter == remaps.end())
	{
		LumpRemaper remaper(extension);
		remaper.AddFile(file, type);
		remaps.insert(pair<string, LumpRemaper>(extension, remaper));
		return;
	}
	(*iter).second.AddFile(file, type);
}

void LumpRemaper::AddFile(FResourceFile *file, Type type)
{
	RemapFile rFile;
	rFile.file = file;
	rFile.type = type;
	files.push_back(rFile);
}

void LumpRemaper::DoRemap()
{
	if(!LoadMap())
		return;

	for(unsigned int i = 0;i < files.size();i++)
	{
		RemapFile &file = files[i];
		int temp = 0; // Use to count something
		int temp2 = 0;
		for(unsigned int i = 0;i < file.file->LumpCount();i++)
		{
			FResourceLump *lump = file.file->GetLump(i);
			switch(file.type)
			{
				case AUDIOT:
					if(lump->Namespace == ns_sounds)
					{
						if(i < sounds.size())
							lump->LumpNameSetup(sounds[i].c_str());
						temp++;
					}
					else if(lump->Namespace == ns_music && i-temp < music.size())
						lump->LumpNameSetup(music[i-temp].c_str());
					break;
				case VGAGRAPH:
					if(i < graphics.size())
						lump->LumpNameSetup(graphics[i].c_str());
					break;
				case VSWAP:
					if(lump->Namespace == ns_newtextures)
					{
						if(i < textures.size())
							lump->LumpNameSetup(textures[i].c_str());
						temp++;
						temp2++;
					}
					else if(lump->Namespace == ns_sprites)
					{
						if(i-temp < sprites.size())
							lump->LumpNameSetup(sprites[i-temp].c_str());
						temp2++;
					}
					else if(lump->Namespace == ns_sounds && i-temp2 < digitalsounds.size())
					{
						lump->LumpNameSetup(digitalsounds[i-temp2].c_str());
					}
					break;
				default:
					break;
			}
		}
	}
	Wads.InitHashChains();
}

bool LumpRemaper::LoadMap()
{
	int lump = Wads.GetNumForName(mapLumpName);
	if(lump == -1)
	{
		printf("\n");
		return false;
	}
	FWadLump mapLump = Wads.OpenLumpNum(lump);

	char* mapData = new char[Wads.LumpLength(lump)];
	mapLump.Read(mapData, Wads.LumpLength(lump));
	Scanner sc(mapData, Wads.LumpLength(lump));

	while(sc.TokensLeft() > 0)
	{
		if(!sc.CheckToken(TK_Identifier))
			sc.ScriptError("Expected identifier in map.\n");

		map<int, string> *reverse = NULL;
		std::deque<std::string> *map = NULL;
		if(sc.str.compare("graphics") == 0)
		{
			reverse = &vgaReverseMap;
			map = &graphics;
		}
		else if(sc.str.compare("sprites") == 0)
			map = &sprites;
		else if(sc.str.compare("sounds") == 0)
			map = &sounds;
		else if(sc.str.compare("digitalsounds") == 0)
			map = &digitalsounds;
		else if(sc.str.compare("music") == 0)
		{
			reverse = &musicReverseMap;
			map = &music;
		}
		else if(sc.str.compare("textures") == 0)
			map = &textures;
		else
			sc.ScriptError("Unknown map section '%s'.\n", sc.str.c_str());

		if(!sc.CheckToken('{'))
			sc.ScriptError("Expected '{'.");
		if(!sc.CheckToken('}'))
		{
			int i = 0;
			while(true)
			{
				if(!sc.CheckToken(TK_StringConst))
					sc.ScriptError("Expected string constant.\n");
				if(reverse != NULL)
					(*reverse)[i++] = sc.str;
				map->push_back(sc.str);
				if(sc.CheckToken('}'))
					break;
				if(!sc.CheckToken(','))
					sc.ScriptError("Expected ','.\n");
			}
		}
	}
	return true;
}

void LumpRemaper::RemapAll()
{
	for(map<string, LumpRemaper>::iterator iter = remaps.begin();iter != remaps.end();iter++)
	{
		(*iter).second.DoRemap();
	}
}

