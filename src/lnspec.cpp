/*
** lnspec.cpp
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
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

#include "id_sd.h"
#include "lnspec.h"
#include "thingdef.h"
using namespace Specials;

#define DEFINE_SPECIAL(name,num,args) \
	static int LN_##name(MapSpot spot, MapTrigger::Side direction, AActor *activator);
#define FUNC(name) \
	static int LN_##name(MapSpot spot, MapTrigger::Side direction, AActor *activator)

DEFINE_SPECIAL(NOP, 0, 0)
#include "lnspecials.h"
#undef DEFINE_SPECIAL

LineSpecialFunction lnspecFunctions[NUM_POSSIBLE_SPECIALS] =
{
	LN_NOP,
	LN_Door_Open,
	LN_Pushwall_Move,
	LN_Exit_Normal,
	LN_Exit_Secret,
	LN_NOP
};

#define DEFINE_SPECIAL(name,num,args) { #name, num, args},
const struct LineSpecialMeta
{
	const char* const		name;
	const unsigned int		num;
	const unsigned short	args;
} lnspecMeta[] = {
#include "lnspecials.h"
	{ NULL, 0, 0 }
};

LineSpecialFunction Specials::LookupFunction(const char* const function)
{
	const LineSpecialMeta *func = lnspecMeta;
	do
	{
		if(stricmp(func->name, function) == 0)
			return lnspecFunctions[func->num];
	}
	while((++func)->name != NULL);
	return LN_NOP;
}
LineSpecialFunction Specials::LookupFunction(LineSpecials function)
{
	return lnspecFunctions[function];
}

////////////////////////////////////////////////////////////////////////////////

#include "thinker.h"

FUNC(NOP)
{
	return 0;
}

class EVDoor : public Thinker
{
	DECLARE_THINKER(EVDoor)

	public:
		EVDoor(MapSpot spot, MapTrigger::Side direction) : Thinker(), state(Closed), spot(spot), amount(0), direction(direction)
		{
			ChangeState(Opening);
			spot->thinker = this;
			if(direction > 1)
				this->direction = direction%2;
		}

		void Destroy()
		{
			if(spot->thinker == this)
				spot->thinker = NULL;
			Thinker::Destroy();
		}

		void Tick()
		{
			static const unsigned int MOVE_AMOUNT = 1<<10;
			switch(state)
			{
				default:
					break;
				case Opening:
					// Connect sound
					if(amount == 0)
					{
						const MapZone *zone1 = spot->GetAdjacent(MapTile::Side(direction))->zone;
						const MapZone *zone2 = spot->GetAdjacent(MapTile::Side(direction), true)->zone;
						map->LinkZones(zone1, zone2, true);

						if(map->CheckLink(zone1, player->GetZone(), true))
							PlaySoundLocMapSpot("doors/open", spot);
					}

					if(amount < 0xffff)
						amount += MOVE_AMOUNT;
					if(amount >= 0xffff)
					{
						amount = 0xffff;
						ChangeState(Opened);
					}
					spot->slideAmount[direction] = spot->slideAmount[direction+2] = amount;
					break;
				case Opened:
					if(wait == 0)
					{
						if((spot->GetX() == player->tilex && spot->GetY() == player->tiley) ||
							actorat[spot->GetX()][spot->GetY()] != NULL)
							break;

						if(direction == 0)
						{
							if(spot->GetY() == player->tiley &&
								((player->x+MINDIST)>>TILESHIFT == spot->GetX() ||
								(player->x-MINDIST)>>TILESHIFT == spot->GetX()))
								break;
							if((actorat[spot->GetX()-1][spot->GetY()] != NULL &&
								(actorat[spot->GetX()-1][spot->GetY()]->x+MINDIST)>>TILESHIFT == spot->GetX()) ||
								(actorat[spot->GetX()+1][spot->GetY()] != NULL &&
								(actorat[spot->GetX()+1][spot->GetY()]->x-MINDIST)>>TILESHIFT == spot->GetX()))
								break;
						}
						else
						{
							if(spot->GetX() == player->tilex &&
								((player->y+MINDIST)>>TILESHIFT == spot->GetY() ||
								(player->y-MINDIST)>>TILESHIFT == spot->GetY()))
								break;
							if((actorat[spot->GetX()][spot->GetY()-1] != NULL &&
								(actorat[spot->GetX()][spot->GetY()-1]->y+MINDIST)>>TILESHIFT == spot->GetY()) ||
								(actorat[spot->GetX()][spot->GetY()+1] != NULL &&
								(actorat[spot->GetX()][spot->GetY()+1]->y-MINDIST)>>TILESHIFT == spot->GetY()))
								break;
						}

						ChangeState(Closing);
					}
					else
						--wait;
					break;
				case Closing:
					if(amount > 0)
						amount -= MOVE_AMOUNT;
					if(amount <= 0)
					{
						amount = 0;
						Destroy();

						// Disconnect Sound
						const MapZone *zone1 = spot->GetAdjacent(MapTile::Side(direction))->zone;
						const MapZone *zone2 = spot->GetAdjacent(MapTile::Side(direction), true)->zone;
						map->LinkZones(zone1, zone2, false);
					}
					spot->slideAmount[direction] = spot->slideAmount[direction+2] = amount;
					break;
			}
		}

		void Reactivate()
		{
			switch(state)
			{
				default:
					ChangeState(Closing);
					break;
				case Closing:
					ChangeState(Opening);
					break;
			}
		}

	private:
		enum State { Opening, Opened, Closing, Closed };

		void ChangeState(State st)
		{
			static const unsigned int OPENTICS = 300;

			if(st == state)
				return;
			state = st;

			switch(state)
			{
				default:
					break;
				case Opened:
					wait = OPENTICS;
					spot->solid = false;
					break;
				case Closing:
					spot->solid = true;
					if(map->CheckLink(spot->GetAdjacent(MapTile::Side(direction))->zone, player->GetZone(), true))
						PlaySoundLocMapSpot("doors/close", spot);
					break;
			}
		}

		State state;
		MapSpot spot;
		int amount;
		unsigned int wait;
		unsigned short direction;
};
IMPLEMENT_THINKER(EVDoor)

FUNC(Door_Open)
{
	if(buttonheld[bt_use])
		return 0;

	if(spot->thinker)
	{
		if(spot->thinker->IsThinkerType<EVDoor>())
		{
			static_cast<EVDoor *>(spot->thinker)->Reactivate();
			return 1;
		}
		return 0;
	}

	new EVDoor(spot, direction);
	return 1;
}

class EVPushwall : public Thinker
{
	DECLARE_THINKER(EVPushwall)

	public:
		EVPushwall(MapSpot spot, MapTrigger::Side direction) : Thinker(), spot(spot), direction(direction)
		{
			spot->thinker = this;
		}

		void Destroy()
		{
			if(spot->thinker == this)
				spot->thinker = NULL;
			Thinker::Destroy();
		}

		void Tick()
		{
			static const unsigned int MOVE_AMOUNT = 1<<10;
			Destroy();
		}

	private:

		MapSpot spot;
		unsigned short direction;
};
IMPLEMENT_THINKER(EVPushwall)

FUNC(Pushwall_Move)
{
	if(spot->thinker)
	{
		return 0;
	}

	new EVPushwall(spot, direction);
	return 1;
}

FUNC(Exit_Normal)
{
	if(buttonheld[bt_use])
		return 0;
	buttonheld[bt_use] = true;

	playstate = ex_completed;
	SD_PlaySound ("world/level_done");
	SD_WaitSoundDone();
	return 1;
}

FUNC(Exit_Secret)
{
	if(buttonheld[bt_use])
		return 0;
	buttonheld[bt_use] = true;

	playstate = ex_secretlevel;
	SD_PlaySound ("world/level_done");
	SD_WaitSoundDone();
	return 1;
}
