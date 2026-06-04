/*
** wl_loadsave.cpp
**
**---------------------------------------------------------------------------
** Copyright 2012 Braden Obrzut
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

#include <ctime>
#include "config.h"
#include "c_cvars.h"
#include "farchive.h"
#include "filesys.h"
#include "g_mapinfo.h"
#include "gamemap.h"
#include "id_ca.h"
#include "id_us.h"
#include "id_vh.h"
#include "language.h"
#include "m_classes.h"
#include "m_png.h"
#include "m_random.h"
#include "thinker.h"
#include "version.h"
#include "w_wad.h"
#include "wl_agent.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_inter.h"
#include "wl_iwad.h"
#include "wl_loadsave.h"
#include "wl_main.h"
#include "wl_menu.h"
#include "wl_play.h"
#include "textures/textures.h"
#include "libretro.h"
#include "streams/file_stream.h"

// Frontend save directory and the in-progress-game flag.
extern const char *Libretro_GetSaveDir(void);
extern bool ingame;

void R_RenderView();
extern uint8_t* vbuf;
extern unsigned vbufPitch;

namespace GameSave {

unsigned long long SaveVersion = GetSaveVersion();
uint32_t SaveProdVersion = SAVEPRODVER;
bool param_foreginsave = false;

#define MAX_SAVENAME 31
#define LSM_Y   55
#define LSM_W   175
#define LSM_X   (320-LSM_W-10)

// Native (non-savestate) ECWolf save files. The format is intentionally simple
// and contains no PNG/thumbnail: a short text header (one field per line)
// followed by the raw game-state snapshot produced by FCompressedMemFile. All
// I/O goes through the libretro VFS (filestream_*), never raw stdio.
static const char* const ECS_MAGIC = "ECWOLFSAVE2\n";
static const char* const ECS_EXT   = ".ecs";

struct SaveFile
{
	public:
		static TArray<SaveFile> files;

		bool    hide;
		bool    hasFiles;
		bool    oldVersion;
		FString name;     // Displayed on the menu.
		FString filename;
};
TArray<SaveFile> SaveFile::files;

static Menu loadGame(LSM_X, LSM_Y, LSM_W, 24);
static Menu saveGame(LSM_X, LSM_Y, LSM_W, 24);
static MenuItem *loadItem = NULL;
static MenuItem *saveItem = NULL;

Menu &GetLoadMenu() { return loadGame; }
Menu &GetSaveMenu() { return saveGame; }
MenuItem *GetLoadMenuItem() { return loadItem; }
MenuItem *GetSaveMenuItem() { return saveItem; }

static FString GetSaveDir()
{
	const char *dir = Libretro_GetSaveDir();
	return (dir && *dir) ? FString(dir) : FString(".");
}
static FString GetFullSaveFileName(const FString &filename)
{
	return GetSaveDir() + PATH_SEPARATOR + filename;
}

// ---- VFS helpers -----------------------------------------------------------

// Reads a whole file into a malloc'd buffer via the VFS. Returns NULL on
// failure; on success *outLen receives the length and the caller frees().
static uint8_t *VFS_ReadFile(const FString &path, long *outLen)
{
	RFILE *f = filestream_open(path.GetChars(),
		RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
	if(!f)
		return NULL;

	filestream_seek(f, 0, RETRO_VFS_SEEK_POSITION_END);
	int64_t len = filestream_tell(f);
	filestream_seek(f, 0, RETRO_VFS_SEEK_POSITION_START);
	if(len <= 0)
	{
		filestream_close(f);
		return NULL;
	}

	uint8_t *buf = (uint8_t*)malloc((size_t)len);
	if(!buf)
	{
		filestream_close(f);
		return NULL;
	}
	int64_t got = filestream_read(f, buf, len);
	filestream_close(f);
	if(got != len)
	{
		free(buf);
		return NULL;
	}
	*outLen = (long)len;
	return buf;
}

static bool VFS_WriteFile(const FString &path, const void *data, size_t len)
{
	return filestream_write_file(path.GetChars(), data, (int64_t)len);
}

// Portable decimal string -> unsigned long long. strtoull() is C99 and is not
// available on older MSVC (pre-2013) which the libretro DOS/Win targets still
// build with, so parse the digits by hand. Leading whitespace is skipped;
// parsing stops at the first non-digit. Overflow is not expected for the save
// version stamp, which is a fixed-width date integer.
static unsigned long long ECS_ParseU64(const char *s)
{
	unsigned long long v = 0;
	if(s == NULL)
		return 0;
	while(*s == ' ' || *s == '\t')
		++s;
	while(*s >= '0' && *s <= '9')
	{
		v = v * 10ull + (unsigned long long)(*s - '0');
		++s;
	}
	return v;
}

// Splits a save image into its header lines and the snapshot body. Returns the
// offset of the body (just past the header) or -1 if the magic doesn't match.
// Each recognised "Key: value" line is stored into the matching out-param.
static long ParseSaveHeader(const uint8_t *buf, long len,
	FString &title, FString &mapname, FString &gameWad, FString &mapWad,
	unsigned long long &saveVer, uint32_t &prodVer)
{
	size_t magicLen = strlen(ECS_MAGIC);
	if((size_t)len < magicLen || memcmp(buf, ECS_MAGIC, magicLen) != 0)
		return -1;

	long pos = (long)magicLen;
	saveVer = 0;
	prodVer = 0;
	// Header ends at the first blank line ("\n").
	while(pos < len)
	{
		// Find end of this line.
		long start = pos;
		while(pos < len && buf[pos] != '\n')
			++pos;
		long lineLen = pos - start;
		if(pos < len)
			++pos; // skip '\n'

		if(lineLen == 0)
			return pos; // blank line: body follows

		FString line((const char*)(buf + start), (size_t)lineLen);
		long colon = line.IndexOf(':');
		if(colon < 0)
			continue;
		FString key = line.Left(colon);
		FString val = line.Mid(colon + 1);
		while(val.Len() && val[0] == ' ')
			val = val.Mid(1);

		if(key.Compare("Title") == 0)            title = val;
		else if(key.Compare("Map") == 0)         mapname = val;
		else if(key.Compare("GameWad") == 0)     gameWad = val;
		else if(key.Compare("MapWad") == 0)      mapWad = val;
		else if(key.Compare("SaveVer") == 0)     saveVer = ECS_ParseU64(val.GetChars());
		else if(key.Compare("ProdVer") == 0)     prodVer = (uint32_t)strtoul(val.GetChars(), NULL, 10);
	}
	return -1; // no blank line: malformed
}

// ---- Load / Save -----------------------------------------------------------

bool Load(const FString &filename)
{
	long len = 0;
	uint8_t *buf = VFS_ReadFile(GetFullSaveFileName(filename), &len);
	if(!buf)
	{
		Message(language["STR_FAILREAD"]);
		return false;
	}

	FString title, mapname, gameWad, mapWad;
	unsigned long long saveVer = 0;
	uint32_t prodVer = 0;
	long body = ParseSaveHeader(buf, len, title, mapname, gameWad, mapWad, saveVer, prodVer);
	if(body < 0)
	{
		free(buf);
		return false;
	}

	SaveVersion = saveVer;
	SaveProdVersion = prodVer;

	strncpy(gamestate.mapname, mapname, 8);
	gamestate.mapname[8] = 0;

	// Mirror retro_unserialize's preparation: a save can be loaded from the
	// title (no game running), so clear any existing thinkers and mark the
	// level as a load before SetupGameLevel. With loadedgame set, SetupGameLevel
	// caches the map for restore instead of spawning a fresh level.
	thinkerList.DestroyAll(ThinkerList::TRAVEL);
	loadedgame = true;
	SetupGameLevel();

	{
		FCompressedMemFile snapshot;
		snapshot.Open((void*)(buf + body));
		FArchive arc(snapshot);
		Serialize(arc);
		uint32_t rngcount = 0;
		arc << rngcount;
		FRandom::StaticReadRNGState(arc, rngcount);

		if(arc.Failed())
		{
			// Corrupt save data was detected during deserialization.
			free(buf);
			libretro_log("Save file is corrupt; load aborted.\n");
			return false;
		}
	}

	free(buf);

	// Loading at just the wrong moment can leave the wrong play state.
	playstate = ex_stillplaying;
	return true;
}

bool Save(const FString &filename, const FString &title)
{
	SaveVersion = GetSaveVersion();
	SaveProdVersion = SAVEPRODVER;

	// Build the snapshot body in memory.
	FCompressedMemFile snapshot;
	snapshot.Open();
	{
		FArchive arc(snapshot);
		Serialize(arc);
		uint32_t rngcount = FRandom::GetRNGCount();
		arc << rngcount;
		FRandom::StaticWriteRNGState(arc);
	}
	unsigned int snapSize = snapshot.GetSerializedSize();

	// Header.
	FString gameWadString;
	for(unsigned int i = 0;i < IWad::GetNumIWads();++i)
	{
		if(i)
			gameWadString += ';';
		gameWadString += Wads.GetWadName(FWadCollection::IWAD_FILENUM + i);
	}
	FString mapWad = Wads.GetWadName(Wads.GetLumpFile(map->GetMarketLumpNum()));

	FString header;
	header += ECS_MAGIC;
	header.AppendFormat("SaveVer: %llu\n", (unsigned long long)SaveVersion);
	header.AppendFormat("ProdVer: %u\n", (unsigned)SaveProdVersion);
	header.AppendFormat("Title: %s\n", title.GetChars());
	header.AppendFormat("Map: %s\n", gamestate.mapname);
	header.AppendFormat("GameWad: %s\n", gameWadString.GetChars());
	header.AppendFormat("MapWad: %s\n", mapWad.GetChars());
	header += "\n"; // blank line terminates the header

	size_t headerLen = header.Len();
	uint8_t *image = (uint8_t*)malloc(headerLen + snapSize);
	if(!image)
	{
		Message(language["STR_FAILWRITE"]);
		return false;
	}
	memcpy(image, header.GetChars(), headerLen);
	snapshot.SerializeToBuffer(image + headerLen);

	bool ok = VFS_WriteFile(GetFullSaveFileName(filename), image, headerLen + snapSize);
	free(image);

	if(!ok)
	{
		Message(language["STR_FAILWRITE"]);
		return false;
	}
	return true;
}

// ---- Save slot enumeration -------------------------------------------------

bool SetupSaveGames()
{
	bool canLoad = false;

	SaveFile::files.Clear();

	File saveDir(GetSaveDir());
	const TArray<FString> &files = saveDir.getFileList();

	for(unsigned int i = 0;i < files.Size();i++)
	{
		const FString &fn = files[i];
		if(fn.Len() < 5 || fn.Mid(fn.Len()-4, 4).Compare(ECS_EXT) != 0)
			continue;

		long len = 0;
		uint8_t *buf = VFS_ReadFile(GetFullSaveFileName(fn), &len);
		if(!buf)
			continue;

		FString title, mapname, gameWad, mapWad;
		unsigned long long saveVer = 0;
		uint32_t prodVer = 0;
		long body = ParseSaveHeader(buf, len, title, mapname, gameWad, mapWad, saveVer, prodVer);
		free(buf);
		if(body < 0)
			continue;

		SaveFile sFile;
		sFile.filename = fn;
		sFile.name = title.Len() ? title : fn;
		sFile.hasFiles = true;
		sFile.hide = false;

		// Version check.
		if((saveVer == SAVEVERUNDEFINED && prodVer < MINSAVEPRODVER) || saveVer < MINSAVEVER)
			sFile.oldVersion = true;
		else
			sFile.oldVersion = false;

		// WAD checks: the map WAD and every game WAD must be loaded.
		if(mapWad.Len() && Wads.CheckIfWadLoaded(mapWad) < 0 && !param_foreginsave)
			sFile.hasFiles = false;
		if(gameWad.Len())
		{
			int lastIndex = 0, nextIndex = 0;
			int expectedIwads = IWad::GetNumIWads();
			do
			{
				nextIndex = gameWad.IndexOf(';', lastIndex);
				FString one = gameWad.Mid(lastIndex, nextIndex < 0 ? gameWad.Len()-lastIndex : nextIndex-lastIndex);
				if(Wads.CheckIfWadLoaded(one) < 0 && !param_foreginsave)
				{
					sFile.hide = true;
					break;
				}
				else
					--expectedIwads;
				lastIndex = nextIndex + 1;
			}
			while(nextIndex != -1);

			if(expectedIwads != 0 && !param_foreginsave)
				sFile.hide = true;
			else if(!sFile.hide)
				canLoad = true;
		}
		else
			canLoad = true;

		SaveFile::files.Push(sFile);
	}

	loadGame.clear();
	saveGame.clear();

	// The save menu always offers a "new save" slot first, then one entry per
	// existing save (selecting it overwrites that save).
	saveGame.addItem(new MenuItem("New Save"));

	for(unsigned int i = 0;i < SaveFile::files.Size();i++)
	{
		MenuItem *litem = new MenuItem(SaveFile::files[i].name);
		litem->setEnabled(!SaveFile::files[i].hide &&
			!SaveFile::files[i].oldVersion && SaveFile::files[i].hasFiles);
		loadGame.addItem(litem);

		// Overwriting hidden/foreign saves isn't offered.
		MenuItem *sitem = new MenuItem(SaveFile::files[i].name);
		sitem->setEnabled(!SaveFile::files[i].hide);
		saveGame.addItem(sitem);
	}

	return canLoad;
}

// Loads the save in load-menu slot "which"; returns true if a game was loaded.
bool LoadFromSlot(int which)
{
	if(which < 0 || (unsigned)which >= SaveFile::files.Size())
		return false;
	if(SaveFile::files[which].oldVersion || !SaveFile::files[which].hasFiles ||
		SaveFile::files[which].hide)
		return false;
	return Load(SaveFile::files[which].filename);
}

// Writes a new save with an auto-generated name (map + timestamp). Interactive
// naming is a later step.
// Generates the auto title "MAPNAME hh:mm:ss" for a new save.
static FString MakeAutoTitle()
{
	FString title;
	unsigned int gametime = gamestate.TimeCount/70;
	FString mapname = gamestate.mapname;
	mapname.ToUpper();
	title.Format("%s %02d:%02d:%02d", mapname.GetChars(),
		gametime/3600, (gametime%3600)/60, gametime%60);
	return title;
}

// Writes a save for the save-menu slot "which": slot 0 is a brand new save (a
// free savegamN.ecs, auto-named); slots 1..N overwrite existing save N-1,
// keeping its filename and name. Returns true on success. Interactive naming is
// a later step.
bool SaveToSlot(int which)
{
	if(!ingame)
		return false;

	FString filename;
	FString title;

	if(which <= 0)
	{
		// New save: find a free savegamN.ecs.
		for(unsigned int i = 0;i < 10000;i++)
		{
			filename.Format("savegam%u%s", i, ECS_EXT);
			long probeLen = 0;
			uint8_t *probe = VFS_ReadFile(GetFullSaveFileName(filename), &probeLen);
			if(probe)
			{
				free(probe);
				continue; // exists, try next
			}
			break;
		}
		title = MakeAutoTitle();
	}
	else
	{
		// Overwrite an existing save (menu index "which" maps to files[which-1]
		// because slot 0 is the "new save" entry).
		unsigned int idx = (unsigned)(which - 1);
		if(idx >= SaveFile::files.Size())
			return false;
		filename = SaveFile::files[idx].filename;
		title = SaveFile::files[idx].name;
	}

	return Save(filename, title);
}

void InitMenus()
{
	loadGame.setHeadPicture("M_LOADGM");
	saveGame.setHeadPicture("M_SAVEGM");

	bool canLoad = SetupSaveGames();

	// Always create these fresh: CreateMenus clears mainMenu (which owns and
	// deletes them) before each rebuild, so reusing the old pointers would
	// dereference freed memory. InitMenus runs from inside CreateMenus, after
	// that clear, so the previous items are already gone here.
	loadItem = new MenuItem(language["STR_LG"]);
	saveItem = new MenuItem(language["STR_SG"]);

	loadItem->setEnabled(canLoad);
	// Saving is enabled only while a game is running.
	saveItem->setEnabled(ingame);
}

void QuickLoadOrSave(bool load)
{
	// Quick save/load uses the slot system; not wired to a hotkey yet.
	(void)load;
}



void Serialize(FArchive &arc)
{
	short difficulty;
	if(arc.IsStoring())
	{
		difficulty = SkillInfo::GetSkillIndex(*gamestate.difficulty);
		arc << difficulty;
	}
	else
	{
		arc << difficulty;
		gamestate.difficulty = &SkillInfo::GetSkill(difficulty);
	}

	arc << gamestate.playerClass
		<< gamestate.secretcount
		<< gamestate.treasurecount
		<< gamestate.killcount
		<< gamestate.secrettotal
		<< gamestate.treasuretotal
		<< gamestate.killtotal
		<< gamestate.TimeCount
		<< gamestate.victoryflag;
	if(SaveVersion >= 1393719642)
		arc << gamestate.fullmap;
	arc << LevelRatios.killratio
		<< LevelRatios.secretsratio
		<< LevelRatios.treasureratio
		<< LevelRatios.numLevels
		<< LevelRatios.time;
	if(SaveVersion > 1395865826)
		arc << LevelRatios.par;

	thinkerList.Serialize(arc);

	arc << map;

	players[0].Serialize(arc);
}

/* end namespace */ }
