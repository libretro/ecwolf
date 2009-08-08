#include "lumpremap.h"
#include "w_wad.h"
#include "zstring.h"
#include "scanner.hpp"
using namespace std;

map<string, LumpRemaper> LumpRemaper::remaps;

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
		for(unsigned int i = 0;i < file.file->LumpCount();i++)
		{
			FResourceLump *lump = file.file->GetLump(i);
			if(i < graphics.size())
			{
				lump->LumpNameSetup(graphics[i].c_str());
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

		std::deque<std::string> &map = graphics;
		if(sc.str.compare("graphics"))
			map = graphics;
		else if(sc.str.compare("sprites"))
			map = sprites;
		else if(sc.str.compare("sounds"))
			map = sprites;
		else if(sc.str.compare("music"))
			map = music;
		else if(sc.str.compare("textures"))
			map = textures;
		else
			sc.ScriptError("Unknown map section '%s'.\n", sc.str.c_str());

		if(!sc.CheckToken('{'))
			sc.ScriptError("Expected '{'.");
		if(!sc.CheckToken('}'))
		{
			while(true)
			{
				if(!sc.CheckToken(TK_StringConst))
					sc.ScriptError("Expected string constant.\n");
				map.push_back(sc.str);
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

