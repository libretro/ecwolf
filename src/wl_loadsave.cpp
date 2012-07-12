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
#include "c_cvars.h"
#include "farchive.h"
#include "file.h"
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
#include "w_wad.h"
#include "wl_agent.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_loadsave.h"
#include "wl_main.h"
#include "wl_menu.h"
#include "wl_play.h"
#include "textures/textures.h"

void R_RenderView();
extern byte* vbuf;
extern unsigned vbufPitch;

namespace GameSave {

struct SaveFile
{
	public:
		static TArray<SaveFile>	files;

		FString	name; // Displayed on the menu.
		FString	filename;
};
TArray<SaveFile> SaveFile::files;

static bool quickSaveLoad = false;

#define MAX_SAVENAME 31
#define LSM_Y   55
#define LSM_W   175
#define LSM_X   (320-LSM_W-10)
#define LSM_H   10*13+10
#define LSA_X	96
#define LSA_Y	80
#define LSA_W	130
#define LSA_H	42

class SaveSlotMenuItem : public TextInputMenuItem
{
public:
	SaveSlotMenuItem(const FString &text, unsigned int max, MENU_LISTENER_PROTOTYPE(preeditListener)=NULL, MENU_LISTENER_PROTOTYPE(posteditListener)=NULL, bool clearFirst=false)
		: TextInputMenuItem(text, max, preeditListener, posteditListener, clearFirst)
	{
	}

	void draw()
	{
		TextInputMenuItem::draw();

		// We use this instead of isSelected() so that the info is drawn during animations.
		if(menu->getIndex(menu->getCurrentPosition()) == this)
		{
			static const EColorRange textColor = gameinfo.FontColors[GameInfo::MENU_HIGHLIGHTSELECTION];
			static const unsigned int SAVEPICX = 10;
			static const unsigned int SAVEPICY = LSM_Y - 1;
			static const unsigned int SAVEPICW = 108;
			static const unsigned int SAVEPICH = 67;
			static const unsigned int INFOY = SAVEPICY + SAVEPICH + 5;
			static const unsigned int INFOH = 200 - INFOY - 11;

			DrawWindow(SAVEPICX - 1, SAVEPICY - 1, SAVEPICW + 2, SAVEPICH + 2, BKGDCOLOR);
			DrawWindow(SAVEPICX - 1, INFOY, SAVEPICW + 2, INFOH, BKGDCOLOR);

			int curPos = menu->getCurrentPosition();
			if((unsigned)menu->getNumItems() > SaveFile::files.Size())
				--curPos;

			if(curPos >= 0)
			{
				SaveFile &saveFile = SaveFile::files[curPos];
				PNGHandle *png;
				FILE *file = fopen(saveFile.filename, "rb");
				if(file && (png = M_VerifyPNG(file)))
				{
					FTexture *savePicture = PNGTexture_CreateFromFile(png, saveFile.filename);
					VWB_DrawGraphic(savePicture, SAVEPICX, SAVEPICY, SAVEPICW, SAVEPICH, MENU_CENTER);

					char* creationTime = M_GetPNGText(png, "Creation Time");
					char* comment = M_GetPNGText(png, "Comment");
					px = SAVEPICX;
					py = INFOY;
					if(creationTime)
					{
						VWB_DrawPropStringWrap(SAVEPICW, INFOH, SmallFont, creationTime, textColor);
						px = SAVEPICX;
						py += 5; // Space the strings a little.
						delete[] creationTime;
					}
					if(comment)
					{
						VWB_DrawPropStringWrap(SAVEPICW, INFOH, SmallFont, comment, textColor);
						delete[] comment;
					}

					delete png;
				}
				if(file)
					fclose(file);
			}
			else
			{
				word width, height;
				VW_MeasurePropString(SmallFont, language["MNU_NOPICTURE"], width, height);

				px = SAVEPICX + (SAVEPICW - width)/2;
				py = SAVEPICY + (SAVEPICH - height)/2;
				VWB_DrawPropString(SmallFont, language["MNU_NOPICTURE"], textColor);
			}
		}
	}
};

MENU_LISTENER(BeginEditSave);
MENU_LISTENER(LoadSaveGame);
MENU_LISTENER(PerformSaveGame);

static Menu loadGame(LSM_X, LSM_Y, LSM_W, 24);
static Menu saveGame(LSM_X, LSM_Y, LSM_W, 24);
static MenuItem *loadItem, *saveItem;

Menu &GetLoadMenu() { return loadGame; }
Menu &GetSaveMenu() { return saveGame; }
MenuItem *GetLoadMenuItem() { return loadItem; }
MenuItem *GetSaveMenuItem() { return saveItem; }

//
// DRAW LOAD/SAVE IN PROGRESS
//
static void DrawLSAction (int which)
{
	DrawWindow (LSA_X, LSA_Y, LSA_W, LSA_H, TEXTCOLOR);
	DrawOutline (LSA_X, LSA_Y, LSA_W, LSA_H, 0, HIGHLIGHT);
	VWB_DrawGraphic (TexMan("M_LDING1"), LSA_X + 8, LSA_Y + 5, MENU_CENTER);

	PrintX = LSA_X + 46;
	PrintY = LSA_Y + 13;

	if (!which)
		US_Print (BigFont, language["STR_LOADING"]);
	else
		US_Print (BigFont, language["STR_SAVING"]);

	VW_UpdateScreen ();
}

////////////////////////////////////////////////////////////////////
//
// SEE WHICH SAVE GAME FILES ARE AVAILABLE & READ STRING IN
//
////////////////////////////////////////////////////////////////////
void SetupSaveGames()
{
	char title[MAX_SAVENAME+1];

	File saveDirectory("./");
	const TArray<FString> &files = saveDirectory.getFileList();

	for(unsigned int i = 0;i < files.Size();i++)
	{
		const FString &filename = files[i];
		if(filename.Len() < 5 ||
			filename.Mid(filename.Len()-4, 4).Compare(".ecs") != 0)
			continue; // Too short or incorrect name

		PNGHandle *png;
		FILE *file = fopen(filename, "rb");
		if(file)
		{
			if((png = M_VerifyPNG(file)))
			{
				SaveFile sFile;
				sFile.filename = filename;
				if(M_GetPNGText(png, "Title", title, MAX_SAVENAME))
				{
					sFile.name = title;
					SaveFile::files.Push(sFile);
				}

				delete png;
			}

			fclose(file);
		}
	}

	loadGame.clear();
	saveGame.clear();

	MenuItem *newSave = new SaveSlotMenuItem("    - NEW SAVE -", 31, NULL, PerformSaveGame, true);
	newSave->setHighlighted(true);
	saveGame.addItem(newSave);

	for(unsigned int i = 0;i < SaveFile::files.Size();i++)
	{
		loadGame.addItem(new SaveSlotMenuItem(SaveFile::files[i].name, 31, LoadSaveGame));
		saveGame.addItem(new SaveSlotMenuItem(SaveFile::files[i].name, 31, BeginEditSave, PerformSaveGame));
	}
}

MENU_LISTENER(BeginEditSave)
{
	bool ret = Confirm(language["GAMESVD"]);
	saveGame.draw();
	return ret;
}

MENU_LISTENER(PerformSaveGame)
{
	SaveFile file;

	// Copy the name
	file.name = static_cast<SaveSlotMenuItem *> (saveGame[which])->getValue();
	if(which == 0) // New
	{
		// Locate a available filename.  I don't want to assume savegamX.yza so this
		// might not be the fastest way to do things.
		bool nextSaveNumber = false;
		for(unsigned int i = 0;i < 10000;i++)
		{
			file.filename.Format("savegam%u.ecs", i);
			for(unsigned int j = 0;j < SaveFile::files.Size();j++)
			{
				if(stricmp(file.filename, SaveFile::files[j].filename) == 0)
				{
					nextSaveNumber = true;
					continue;
				}
			}
			if(nextSaveNumber)
			{
				nextSaveNumber = false;
				continue;
			}
			break;
		}

		SaveFile::files.Push(file);

		loadGame.addItem(new SaveSlotMenuItem(file.name, 31, LoadSaveGame));
		saveGame.addItem(new SaveSlotMenuItem(file.name, 31, BeginEditSave, PerformSaveGame));

		saveGame.setCurrentPosition(saveGame.getNumItems()-1);
		loadGame.setCurrentPosition(saveGame.getNumItems()-1);

		mainMenu[2]->setEnabled(true);
	}
	else
	{
		file.filename = SaveFile::files[which-1].filename;
		SaveFile::files[which-1] = file;
		loadGame.setCurrentPosition(which-1);
	}

	Save(file.filename, file.name);

	if(!quickSaveLoad)
		Menu::closeMenus(true);

	return true;
}

MENU_LISTENER(LoadSaveGame)
{
	loadedgame = true;
	Load(SaveFile::files[which].filename);
	
	ShootSnd();
	if(!quickSaveLoad)
		Menu::closeMenus(true);

	saveGame.setCurrentPosition(which+1);
	return false;
}

void InitMenus()
{
	SetupSaveGames();

	loadGame.setHeadPicture("M_LOADGM");
	saveGame.setHeadPicture("M_SAVEGM");

	loadItem = new MenuSwitcherMenuItem(language["STR_LG"], GameSave::GetLoadMenu());
	loadItem->setEnabled(SaveFile::files.Size() > 0);
	saveItem = new MenuSwitcherMenuItem(language["STR_SG"], GameSave::GetSaveMenu());
	saveItem->setEnabled(false);
}

void QuickSave()
{
	if(saveGame.getCurrentPosition() != 0)
	{
		quickSaveLoad = true;
		PerformSaveGame(saveGame.getCurrentPosition());
		quickSaveLoad = false;

		return;
	}

	ShowMenu(saveGame);
}

void QuickLoad()
{
	if(saveGame.getCurrentPosition() != 0)
	{
		quickSaveLoad = true;
		char string[100];
		sprintf(string, "%s%s\"?", language["STR_LGC"], SaveFile::files[saveGame.getCurrentPosition()-1].name.GetChars());
		if(Confirm(string))
			LoadSaveGame(saveGame.getCurrentPosition()-1);
		quickSaveLoad = false;

		return;
	}

	ShowMenu(loadGame);
}

static void Serialize(FArchive &arc)
{
	arc << gamestate.difficulty
		<< gamestate.secretcount
		<< gamestate.treasurecount
		<< gamestate.killcount
		<< gamestate.secrettotal
		<< gamestate.treasuretotal
		<< gamestate.killtotal
		<< gamestate.TimeCount
		<< gamestate.victoryflag;

	thinkerList->Serialize(arc);

	arc << map;

	players[0].Serialize(arc);
}

#define SNAP_ID MAKE_ID('s','n','A','p')

bool Load(const FString &filename)
{
	FILE *fileh = fopen(filename, "rb");
	PNGHandle *png = M_VerifyPNG(fileh);
	if(png == NULL)
	{
		fclose(fileh);
		return false;
	}

	if(!quickSaveLoad)
		DrawLSAction(0);

	char level[9];
	M_GetPNGText(png, "Current Map", level, 8);
	CA_CacheMap(level);

	{
		unsigned int chunkLength = M_FindPNGChunk(png, SNAP_ID);
		FPNGChunkArchive arc(fileh, SNAP_ID, chunkLength);
		FCompressedMemFile snapshot;
		snapshot.Serialize(arc);
		snapshot.Reopen();
		FArchive snarc(snapshot);
		Serialize(snarc);
	}

	FRandom::StaticReadRNGState(png);

	delete png;
	fclose(fileh);
	return true;
}

void SaveScreenshot(FILE *file)
{
	static const int SAVEPICWIDTH = 216;
	static const int SAVEPICHEIGHT = 162;

	vbuf = new byte[SAVEPICHEIGHT*SAVEPICWIDTH];
	vbufPitch = SAVEPICWIDTH;

	int oldviewsize = viewsize;
	Aspect oldaspect = vid_aspect;

	vid_aspect = ASPECT_16_10;
	NewViewSize(21, SAVEPICWIDTH, SAVEPICHEIGHT);
	CalcProjection(players[0].mo->radius);
	R_RenderView();

	M_CreatePNG(file, vbuf, GPalette.BaseColors, SS_PAL, SAVEPICWIDTH, SAVEPICHEIGHT, vbufPitch);

	delete[] vbuf;
	vbuf = NULL;

	vid_aspect = oldaspect;
	NewViewSize(oldviewsize); // Restore
}

bool Save(const FString &filename, const FString &title)
{
	FILE *fileh = fopen(filename, "wb");

	if(!quickSaveLoad)
		DrawLSAction(1);
	else
		Message (language["STR_SAVING"]);

	// If we get hubs this will need to be moved so that we can have multiple of them
	FCompressedMemFile snapshot;
	snapshot.Open();
	{
		FArchive arc(snapshot);
		Serialize(arc);
	}

	SaveScreenshot(fileh);
	M_AppendPNGText(fileh, "Software", "ECWolf");
	M_AppendPNGText(fileh, "Engine", GAMESIG);
	M_AppendPNGText(fileh, "ECWolf Save Version", GAMESIG);
	M_AppendPNGText(fileh, "Title", title);
	M_AppendPNGText(fileh, "Current Map", levelInfo->MapName);

	FString comment;
	// Creation Time
	{
		time_t currentTime;
		struct tm *timeInfo;

		time(&currentTime);
		timeInfo = localtime(&currentTime);
		M_AppendPNGText(fileh, "Creation Time", asctime(timeInfo));
	}

	unsigned int gametime = gamestate.TimeCount/70;
	FString mapname = gamestate.mapname;
	mapname.ToUpper();
	comment.Format("%s - %s\nTime: %02d:%02d:%02d", mapname.GetChars(), levelInfo->GetName(map).GetChars(),
		gametime/3600, (gametime%3600)/60, gametime%60);
	M_AppendPNGText(fileh, "Comment", comment);

	M_AppendPNGText(fileh, "Game WAD", Wads.GetWadName(FWadCollection::IWAD_FILENUM));
	// TODO: Also write WAD that the map resides in

	FRandom::StaticWriteRNGState(fileh);

	{
		FPNGChunkArchive snapshotArc(fileh, SNAP_ID);
		snapshot.Serialize(snapshotArc);
	}

	M_FinishPNG(fileh);
	fclose(fileh);
	return true;
}

/* end namespace */ }
