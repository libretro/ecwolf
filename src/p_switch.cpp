/*
** p_switch.cpp
** Switch and button maintenance and animation
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
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
*/

#include "templates.h"
#include "wl_def.h"
#include "m_random.h"
#include "w_wad.h"
#include "tarray.h"
#include "farchive.h"
#include "gamemap.h"
#include "thinker.h"
#include "thingdef/thingdef.h"
#include "lnspec.h"
#include "id_sd.h"
#include "wl_game.h"


#define BUTTONTIME 70

static FRandom pr_switchanim ("AnimSwitch");

class DActiveButton : public Thinker
{
	DECLARE_CLASS (DActiveButton, Thinker)
public:
	DActiveButton ();
	DActiveButton (MapSpot, MapTile::Side, FSwitchDef *, fixed_t x, fixed_t y, bool flippable);

	void Serialize (FArchive &arc);
	void Tick ();

	MapSpot			m_Spot;
	MapTile::Side	m_Side;
	bool			bFlippable;
	bool			bReturning;
	FSwitchDef		*m_SwitchDef;
	int32_t			m_Frame;
	uint32_t			m_Timer;
	fixed_t			m_X, m_Y;	// Location of timer sound

protected:
	bool AdvanceFrame ();
};


//==========================================================================
//
// Start a button counting down till it turns off.
// [RH] Rewritten to remove MAXBUTTONS limit.
//
//==========================================================================

static bool P_StartButton (MapSpot spot, MapTile::Side side, FSwitchDef *Switch, fixed_t x, fixed_t y, bool useagain)
{
	ThinkerList::Iterator iter = thinkerList.GetHead(ThinkerList::WORLD);
	while(iter)
	{
		if(iter->IsThinkerType<DActiveButton>())
		{
			DActiveButton *button = (DActiveButton*) ((Thinker*)iter);
			if(button->m_Side == side && button->m_Spot == spot)
			{
				button->m_Timer=1;	// force advancing to the next frame
				return false;
			}
		}
		++iter;
	}

	new DActiveButton (spot, side, Switch, x, y, useagain);
	return true;
}

//==========================================================================
//
// Function that changes wall texture.
// Tell it if switch is ok to use again (1=yes, it's a button).
//
//==========================================================================

// Temp function
static inline ENamedName S_FindSound(FName sound) { return (ENamedName)(int)sound; }
bool P_ChangeSwitchTexture (MapSpot spot, MapTile::Side side, int useAgain, uint8_t special, bool *quest)
{
//	int texture;
	ENamedName sound;
	FSwitchDef *Switch;

	Switch = TexMan.FindSwitch(spot->texture[side]);
	if(!Switch)
		return false;

	// EXIT SWITCH?
	if (Switch->Sound != 0)
		sound = (ENamedName)Switch->Sound;
	else
	{
		sound = S_FindSound (
			special == Specials::Exit_Normal ||
			special == Specials::Exit_Secret ||
			special == Specials::Teleport_NewMap// ||
			//special == Specials::Teleport_EndGame
		   ? "switches/exitbutn" : "switches/normbutn");
	}

	// [RH] The original code played the sound at buttonlist->soundorg,
	//		which wasn't necessarily anywhere near the switch if it was
	//		facing a big sector (and which wasn't necessarily for the
	//		button just activated, either).
	fixed_t pt[2];
	bool playsound;

	pt[0] = spot->GetX();
	pt[1] = spot->GetY();
	spot->texture[side] = Switch->frames[0].Texture;
	if (useAgain || Switch->NumFrames > 1)
	{
		playsound = P_StartButton (spot, side, Switch, pt[0], pt[1], !!useAgain);
	}
	else 
	{
		playsound = true;
	}
	if (playsound)
	{
		//FName test = "switches/normbutn";
		//Printf("Here %d %s %d %s\n", (int)test, test.GetChars(), (int)sound, sound.GetChars());
		SD_PlaySound(FName(sound).GetChars());
		//PlaySoundLocMapSpot(FName(sound), spot);
	}
	if (quest != NULL)
	{
		*quest = Switch->QuestPanel;
	}
	return true;
}

//==========================================================================
//
// Button thinker
//
//==========================================================================

IMPLEMENT_INTERNAL_CLASS (DActiveButton)

DActiveButton::DActiveButton () : Thinker(ThinkerList::WORLD)
{
	m_Spot = NULL;
	m_Side = MapTile::East;
	m_SwitchDef = 0;
	m_Timer = 0;
	m_X = 0;
	m_Y = 0;
	bFlippable = false;
	bReturning = false;
	m_Frame = 0;
}

DActiveButton::DActiveButton (MapSpot spot, MapTile::Side side, FSwitchDef *Switch,
							  fixed_t x, fixed_t y, bool useagain) : Thinker(ThinkerList::WORLD)
{
	m_Spot = spot;
	m_Side = side;
	m_X = x;
	m_Y = y;
	bFlippable = useagain;
	bReturning = false;

	m_SwitchDef = Switch;
	m_Frame = -1;
	AdvanceFrame ();
}

//==========================================================================
//
//
//
//==========================================================================

void DActiveButton::Serialize (FArchive &arc)
{
	Super::Serialize (arc);

	uint8_t side = m_Side;
	arc << side;
	m_Side = static_cast<MapTile::Side> (side);

	arc << m_Spot << m_SwitchDef << m_Frame << m_Timer << bFlippable << m_X << m_Y << bReturning;
}

//==========================================================================
//
//
//
//==========================================================================

void DActiveButton::Tick ()
{
	if (m_SwitchDef == NULL)
	{
		// We lost our definition due to a bad savegame.
		Destroy();
		return;
	}

	FSwitchDef *def = bReturning? m_SwitchDef->PairDef : m_SwitchDef;
	if (--m_Timer == 0)
	{
		if (m_Frame == def->NumFrames - 1)
		{
			bReturning = true;
			def = m_SwitchDef->PairDef;
			if (def != NULL)
			{
				m_Frame = -1;
				/*S_Sound (m_X, m_Y, 0, CHAN_VOICE|CHAN_LISTENERZ,
					def->Sound != 0 ? FSoundID(def->Sound) : FSoundID("switches/normbutn"),
					1, ATTN_STATIC);*/
				bFlippable = false;
			}
			else
			{
				Destroy ();
				return;
			}
		}
		bool killme = AdvanceFrame ();

		m_Spot->texture[m_Side] = def->frames[m_Frame].Texture;

		if (killme)
		{
			Destroy ();
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

bool DActiveButton::AdvanceFrame ()
{
	bool ret = false;
	FSwitchDef *def = bReturning? m_SwitchDef->PairDef : m_SwitchDef;

	if (++m_Frame == def->NumFrames - 1)
	{
		if (bFlippable == true)
		{
			m_Timer = BUTTONTIME;
		}
		else
		{
			ret = true;
		}
	}
	else
	{
		m_Timer = def->frames[m_Frame].TimeMin;
		if (def->frames[m_Frame].TimeRnd != 0)
		{
			m_Timer += pr_switchanim(def->frames[m_Frame].TimeRnd);
		}
	}
	return ret;
}
