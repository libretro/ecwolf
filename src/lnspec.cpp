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

#include "doomerrors.h"
#include "id_ca.h"
#include "id_sd.h"
#include "lnspec.h"
#include "actor.h"
#include "wl_act.h"
#include "wl_agent.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_play.h"
#include "g_shared/a_keys.h"
#include "thingdef/thingdef.h"
using namespace Specials;

#define DEFINE_SPECIAL(name,num,argc) \
	static int LN_##name(MapSpot spot, const int args[], MapTrigger::Side direction, AActor *activator);
#define FUNC(name) \
	static int LN_##name(MapSpot spot, const int args[], MapTrigger::Side direction, AActor *activator)

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
	LN_NOP,
	LN_Exit_VictorySpin
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

LineSpecials Specials::LookupFunctionNum(const char* const function)
{
	const LineSpecialMeta *func = lnspecMeta;
	do
	{
		if(stricmp(func->name, function) == 0)
			return static_cast<LineSpecials> (func->num);
	}
	while((++func)->name != NULL);
	return NUM_POSSIBLE_SPECIALS;
}
LineSpecialFunction Specials::LookupFunction(LineSpecials function)
{
	if(function >= NUM_POSSIBLE_SPECIALS)
		return LN_NOP;

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
							if(!CheckClears(iter->Item()))
							{
								hold = true;
								break;
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

		bool Reactivate(AActor *activator, bool ismonster)
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
					if(!ismonster && CheckClears(activator))
						return ChangeState(Closing);
					break;
				case Closing:
					return ChangeState(Opening);
			}
			return false;
		}

		void Serialize(FArchive &arc)
		{
			BYTE state = this->state;
			arc << state;
			this->state = static_cast<State>(state);

			arc << spot
				<< amount
				<< wait
				<< direction;

			Super::Serialize(arc);
		}

	private:
		static const unsigned int OPENTICS = 300;
		enum State { Opening, Opened, Closing, Closed };

		// Returns true if the actor isn't blocking the door.
		bool CheckClears(AActor *obj)
		{
			if(direction == 0)
			{
				if(spot->GetY() == obj->tiley &&
					abs(((spot->GetX()<<FRACBITS)+(FRACUNIT/2))-obj->x) < FRACUNIT/2 + obj->radius)
				{
					return false;
				}
			}
			else
			{
				if(spot->GetX() == obj->tilex &&
					abs(((spot->GetY()<<FRACBITS)+(FRACUNIT/2))-obj->y) < FRACUNIT/2 + obj->radius)
				{
					return false;
				}
			}
			return true;
		}

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
			return static_cast<EVDoor *>((Thinker*)spot->thinker)->Reactivate(activator, activator->flags & FL_ISMONSTER);
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
		EVPushwall(MapSpot spot, unsigned int speed, MapTrigger::Side direction, unsigned int distance) :
			Thinker(ThinkerList::WORLD), spot(spot), moveTo(NULL), direction(direction), position(0),
			speed(speed), distance(distance)
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

				if(moveTo == NULL)
				{
					Destroy();
					return;
					// Maybe in the future this can be a flag or something
					#if 0
					FString error;
					error.Format("\"I'm free!\" -Pushwall @ (%d, %d)", spot->GetX(), spot->GetY());
					throw CRecoverableError(error);
					#endif
				}

				if(moveTo->tile)
				{
					// Hit a wall so we're done.
					Destroy();
					return;
				}

				moveTo->SetTile(spot->tile);
				moveTo->pushReceptor = spot;
				moveTo->pushDirection = spot->pushDirection;

				// Try to get a sound zone.
				if(spot->zone == NULL)
					spot->zone = moveTo->zone;
			}

			// Move the tile a bit.
			if((position += speed) > 256)
			{
				position -= 256;
				spot->pushAmount = 0;
				spot->SetTile(NULL);
				spot->thinker = NULL;
				moveTo->pushReceptor = NULL;
				moveTo->thinker = this;
				spot = moveTo;
				moveTo = NULL;
			}
			else
				spot->pushAmount = position/4;

			if(!moveTo)
			{
				if(--distance == 0)
					Destroy();
			}
		}

		void Serialize(FArchive &arc)
		{
			arc << spot
				<< moveTo
				<< direction
				<< position
				<< speed
				<< distance;

			Super::Serialize(arc);
		}

	private:

		MapSpot spot, moveTo;
		unsigned short	direction;
		unsigned int	position;
		unsigned int	speed;
		unsigned int	distance;
};
IMPLEMENT_INTERNAL_CLASS(EVPushwall)

FUNC(Pushwall_Move)
{
	if(spot->thinker || !spot->tile || spot->GetAdjacent(MapTile::Side(direction))->tile)
	{
		return 0;
	}

	new EVPushwall(spot, args[0], MapTrigger::Side((direction + 3 + args[1])%4), args[2]);
	return 1;
}

FUNC(Exit_Normal)
{
	if(buttonheld[bt_use])
		return 0;
	buttonheld[bt_use] = true;

	playstate = ex_completed;
	SD_PlaySound ("switches/exitbutn");
	SD_WaitSoundDone();
	return 1;
}

FUNC(Exit_Secret)
{
	if(buttonheld[bt_use])
		return 0;
	buttonheld[bt_use] = true;

	playstate = ex_secretlevel;
	SD_PlaySound ("switches/exitbutn");
	SD_WaitSoundDone();
	return 1;
}

class EVVictorySpin : public Thinker
{
	DECLARE_CLASS(EVVictorySpin, Thinker)
	HAS_OBJECT_POINTERS
	public:
		// Note that we trigger slightly a half unit before wolf3d did
		EVVictorySpin(AActor *activator, MapTrigger::Side direction) : Thinker(ThinkerList::VICTORY),
			doturn(true), dist(6*FRACUNIT + FRACUNIT/2), activator(activator)
		{
			gamestate.victoryflag = true;
			players[0].SetPSprite(NULL);

			runner = AActor::Spawn(ClassDef::FindClass("BJRun"), activator->x, activator->y, 0);
			runner->flags |= FL_PATHING;
			runner->angle = ((direction+2)%4)*ANGLE_90;
			runner->dir = static_cast<dirtype>(runner->angle/ANGLE_45);
			runner->GetThinker()->SetPriority(ThinkerList::VICTORY);
		}

		void Serialize(FArchive &arc)
		{
			arc << doturn
				<< dist
				<< activator
				<< runner;

			Super::Serialize(arc);
		}

		void Tick()
		{
			if(doturn)
			{
				angle_t oldangle = activator->angle;
				A_Face(activator, runner, 3*ANGLE_1);
				if(activator->angle == oldangle)
					doturn = false;
			}

			if(dist > 0)
			{
				static const unsigned int speed = 4096;

				if(dist <= speed)
				{
					activator->x += FixedMul(dist, finecosine[runner->angle>>ANGLETOFINESHIFT]);
					activator->y -= FixedMul(dist, finesine[runner->angle>>ANGLETOFINESHIFT]);
					dist = 0;
				}
				else
				{
					activator->x += FixedMul(speed, finecosine[runner->angle>>ANGLETOFINESHIFT]);
					activator->y -= FixedMul(speed, finesine[runner->angle>>ANGLETOFINESHIFT]);
					dist -= speed;
				}
			}
			else
			{
				// This is the gross part, we must do a "collision" check.
				// If it passes we're done here, but we must first make the
				// runner a projectile and then make sure it doesn't try to
				// explode on us.
				fixed radius = runner->radius + activator->radius + runner->speed;
				if(abs(activator->x - runner->x) <= radius &&
					abs(activator->y - runner->y) <= radius)
				{
					runner->Die();
					runner->velx = FixedMul(runner->runspeed, finecosine[runner->angle>>ANGLETOFINESHIFT]);
					runner->vely = -FixedMul(runner->runspeed, finesine[runner->angle>>ANGLETOFINESHIFT]);
					runner->flags |= FL_MISSILE;
					runner->radius = 1;
					Destroy();
				}
			}
		}

		bool			doturn;
		unsigned int	dist;
		TObjPtr<AActor>	activator;
		TObjPtr<AActor>	runner;
};
IMPLEMENT_INTERNAL_POINTY_CLASS(EVVictorySpin)
	DECLARE_POINTER(activator)
	DECLARE_POINTER(runner)
END_POINTERS
FUNC(Exit_VictorySpin)
{
	if(buttonheld[bt_use])
		return 0;
	buttonheld[bt_use] = true;

	new EVVictorySpin(activator, direction);
	return 1;
}
