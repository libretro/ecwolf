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

#include "id_ca.h"
#include "id_sd.h"
#include "lnspec.h"
#include "actor.h"
#include "wl_agent.h"
#include "wl_game.h"
#include "wl_play.h"
#include "g_shared/a_keys.h"
#include "thingdef/thingdef.h"
using namespace Specials;

#define DEFINE_SPECIAL(name,num,argc) \
	static int LN_##name(MapSpot spot, int args[], MapTrigger::Side direction, AActor *activator);
#define FUNC(name) \
	static int LN_##name(MapSpot spot, int args[], MapTrigger::Side direction, AActor *activator)

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
	DECLARE_CLASS(EVDoor, Thinker)

	public:
		EVDoor(MapSpot spot, MapTrigger::Side direction) : Thinker(ThinkerList::WORLD),
			state(Closed), spot(spot), amount(0), direction(direction)
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
			Super::Destroy();
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

						if(map->CheckLink(zone1, players[0].mo->GetZone(), true))
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
						bool hold = false;
						for(AActor::Iterator *iter = AActor::GetIterator();iter;iter = iter->Next())
						{
							if(direction == 0)
							{
								if(spot->GetY() == iter->Item()->tiley &&
									abs(((spot->GetX()<<FRACBITS)+(FRACUNIT/2))-iter->Item()->x) < FRACUNIT/2 + iter->Item()->radius)
								{
									hold = true;
									break;
								}
							}
							else
							{
								if(spot->GetX() == iter->Item()->tilex &&
									abs(((spot->GetY()<<FRACBITS)+(FRACUNIT/2))-iter->Item()->y) < FRACUNIT/2 + iter->Item()->radius)
								{
									hold = true;
									break;
								}
							}
						}
						if(hold)
							break;
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

		bool Reactivate(bool ismonster)
		{
			switch(state)
			{
				case Opened:
					if(ismonster) // Monsters can reset the door
					{
						wait = OPENTICS;
						return false;
					}
				default:
					if(!ismonster)
						return ChangeState(Closing);
					break;
				case Closing:
					return ChangeState(Opening);
			}
			return false;
		}

	private:
		static const unsigned int OPENTICS = 300;
		enum State { Opening, Opened, Closing, Closed };

		bool ChangeState(State st)
		{
			if(st == state)
				return false;
			state = st;

			switch(state)
			{
				default:
					break;
				case Opened:
					wait = OPENTICS;
					break;
				case Closing:
					if(map->CheckLink(spot->GetAdjacent(MapTile::Side(direction))->zone, players[0].mo->GetZone(), true))
						PlaySoundLocMapSpot("doors/close", spot);
					break;
			}
			return true;
		}

		State state;
		MapSpot spot;
		int amount;
		unsigned int wait;
		unsigned short direction;
};
IMPLEMENT_INTERNAL_CLASS(EVDoor)

FUNC(Door_Open)
{
	if(buttonheld[bt_use])
		return 0;
	buttonheld[bt_use] = true;

	if(args[1] != 0)
	{
		if(!P_CheckKeys(activator, args[1], false))
			return 0;
	}

	if(spot->thinker)
	{
		if(spot->thinker->IsThinkerType<EVDoor>())
		{
			return static_cast<EVDoor *>((Thinker*)spot->thinker)->Reactivate(activator->flags & FL_ISMONSTER);
		}
		return 0;
	}

	new EVDoor(spot, direction);
	return 1;
}

class EVPushwall : public Thinker
{
	DECLARE_CLASS(EVPushwall, Thinker)

	public:
		EVPushwall(MapSpot spot, MapTrigger::Side direction) : Thinker(ThinkerList::WORLD),
			spot(spot), moveTo(NULL), direction(direction), position(0)
		{
			spot->thinker = this;
			spot->pushDirection = MapTile::Side(direction);
		}

		void Destroy()
		{
			if(spot->thinker == this)
				spot->thinker = NULL;
			Super::Destroy();
		}

		void Tick()
		{
			if(position == 0)
				SD_PlaySound ("world/pushwall");

			// TODO: Block pushwall movement with things and the player

			// Setup the next tile to get ready to accept this tile.
			if(moveTo == NULL)
			{
				moveTo = spot->GetAdjacent(MapTile::Side(direction));
				moveTo->tile = spot->tile;
				moveTo->pushReceptor = spot;
				moveTo->pushDirection = spot->pushDirection;

				// Try to get a sound zone.
				if(spot->zone == NULL)
					spot->zone = moveTo->zone;
			}

			// Move the tile a bit.
			if((++position)%128 == 0)
			{
				spot->pushAmount = 0;
				spot->tile = NULL;
				spot->thinker = NULL;
				moveTo->pushReceptor = NULL;
				moveTo->thinker = this;
				spot = moveTo;
				moveTo = NULL;
			}
			else
				spot->pushAmount = (position/2)%64;

			if(position == 256)
				Destroy();
		}

	private:

		MapSpot spot, moveTo;
		unsigned short	direction;
		unsigned int	position;
};
IMPLEMENT_INTERNAL_CLASS(EVPushwall)

FUNC(Pushwall_Move)
{
	if(spot->thinker || !spot->tile || spot->GetAdjacent(MapTile::Side(direction))->tile)
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
