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

void R_RenderView();
extern uint8_t* vbuf;
extern unsigned vbufPitch;

namespace GameSave {

unsigned long long SaveVersion = GetSaveVersion();
uint32_t SaveProdVersion = SAVEPRODVER;


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
