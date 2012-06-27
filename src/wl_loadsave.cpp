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

#include "file.h"
#include "id_us.h"
#include "id_vh.h"
#include "language.h"
#include "m_classes.h"
#include "m_png.h"
#include "wl_main.h"
#include "wl_menu.h"
#include "textures/textures.h"

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

#define LSM_X   85
#define LSM_Y   55
#define LSM_W   175
#define LSM_H   10*13+10
#define LSA_X	96
#define LSA_Y	80
#define LSA_W	130
#define LSA_H	42

MENU_LISTENER(BeginEditSave);
MENU_LISTENER(EnterLoadMenu);
MENU_LISTENER(EnterSaveMenu);
MENU_LISTENER(LoadSaveGame);
MENU_LISTENER(PerformSaveGame);

static Menu loadGame(LSM_X, LSM_Y, LSM_W, 24, EnterLoadMenu);
static Menu saveGame(LSM_X, LSM_Y, LSM_W, 24, EnterSaveMenu);
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
	char title[65];

	File saveDirectory("./");
	const TArray<FString> &files = saveDirectory.getFileList();
	for(unsigned int i = 0;i < files.Size();i++)
	{
		const FString &filename = files[i];
		if(filename.Len() <= 11 ||
			filename.Len() >= 15 ||
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
				if(!M_GetPNGText(png, "Title", title, 64))
				{
					sFile.name = title;
					SaveFile::files.Push(sFile);
				}

				delete png;
			}

			fclose(file);
		}
	}
}

MENU_LISTENER(EnterLoadMenu)
{
	if(saveGame.getNumItems() == 0)
		EnterSaveMenu(which); // This is needed so that there are no crashes on quick save/load

	loadGame.clear();

	for(unsigned int i = 0;i < SaveFile::files.Size();i++)
		loadGame.addItem(new TextInputMenuItem(SaveFile::files[i].name, 31, LoadSaveGame));

	return true;
}

MENU_LISTENER(EnterSaveMenu)
{
	// Create the menu now
	saveGame.clear();

	MenuItem *newSave = new TextInputMenuItem("    - NEW SAVE -", 31, NULL, PerformSaveGame, true);
	newSave->setHighlighted(true);
	saveGame.addItem(newSave);

	for(unsigned int i = 0;i < SaveFile::files.Size();i++)
		saveGame.addItem(new TextInputMenuItem(SaveFile::files[i].name, 31, BeginEditSave, PerformSaveGame));

	return true;
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
	file.name = static_cast<TextInputMenuItem *> (saveGame[which])->getValue();
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

		saveGame[0]->setHighlighted(false);
		saveGame.setCurrentPosition(1);
		loadGame.setCurrentPosition(0);

		mainMenu[2]->setEnabled(true);
	}
	else
	{
		file.filename = SaveFile::files[which-1].filename;
		SaveFile::files[which-1] = file;
		loadGame.setCurrentPosition(which-1);
	}

	FILE *fileh = fopen(file.filename, "wb");
	fwrite(file.name, 32, 1, fileh);
	fseek(fileh, 32, SEEK_SET);
	if(!quickSaveLoad)
	{
		DrawLSAction(1);
		//SaveTheGame(fileh, LSA_X + 8, LSA_Y + 5);
	}
	else
	{
		Message (language["STR_SAVING"]);
		//SaveTheGame(fileh, 0, 0);
	}
	fclose(fileh);
	if(!quickSaveLoad)
		Menu::closeMenus(true);

	return true;
}

MENU_LISTENER(LoadSaveGame)
{
	FILE *file = fopen(SaveFile::files[which].filename, "rb");
	fseek(file, 32, SEEK_SET);
	if(!quickSaveLoad)
		DrawLSAction(0);
	loadedgame = true;
	/*if(!quickSaveLoad)
		LoadTheGame(file, LSA_X + 8, LSA_Y + 5);
	else
		LoadTheGame(file, 0, 0);*/
	fclose(file);
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

/* end namespace */ }
