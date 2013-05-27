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
#include "g_mapinfo.h"
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

#define DEFINE_SPECIAL(name,num,argc) LN_##name,
#include "lnspecials.h"
#undef DEFINE_SPECIAL
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

// Temporary function until sound sequences are supported
static void PlaySoundSequence(MapSpot spot, bool door, bool open)
{
	// Valid squences:
	// DoorNormal
	// DoorMetal (Lost Episodes)
	// PushwallNormal
	// PushwallHeavy (Lost Episodes)

	FName sequence = door ? gameinfo.DoorSoundSequence : gameinfo.PushwallSoundSequence;

	if(open)
	{
		switch(sequence)
		{
			case NAME_DoorNormal:
				PlaySoundLocMapSpot("doors/open", spot);
				break;
			case NAME_DoorMetal:
				PlaySoundLocMapSpot("doors/metal_open", spot);
				break;
			case NAME_PushwallNormal:
				PlaySoundLocMapSpot("world/pushwall", spot);
				break;
			case NAME_PushwallHeavy:
				PlaySoundLocMapSpot("world/pushwall_heavy", spot);
				break;
		}
	}
	else
	{
		switch(sequence)
		{
			case NAME_DoorNormal:
				PlaySoundLocMapSpot("doors/close", spot);
				break;
			case NAME_DoorMetal:
				PlaySoundLocMapSpot("doors/metal_close", spot);
				break;
		}
	}
}

FUNC(NOP)
{
	return 0;
}

class EVDoor : public Thinker
{
	DECLARE_CLASS(EVDoor, Thinker)

	public:
		EVDoor(MapSpot spot, unsigned int speed, unsigned int opentics, bool direction) : Thinker(ThinkerList::WORLD),
			state(Closed), spot(spot), amount(0), opentics(opentics), direction(direction)
		{
			ChangeState(Opening);
			spot->thinker = this;
			this->speed = 64*speed;
		}

		void Destroy()
		{
			if(spot->thinker == this)
				spot->thinker = NULL;
			Super::Destroy();
		}

		void Tick()
		{
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
							PlaySoundSequence(spot, true, true);
					}

					if(amount < 0xffff)
						amount += speed;
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
						amount -= speed;
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
						wait = opentics;
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
				<< speed
				<< amount
				<< wait
				<< direction;

			Super::Serialize(arc);
		}

	private:
		enum State { Opening, Opened, Closing, Closed };

		// Returns true if the actor isn't blocking the door.
		bool CheckClears(AActor *obj)
		{
			if(direction == 0)
			{
				if(spot->GetY() == obj->tiley &&
					abs((((fixed)spot->GetX()<<FRACBITS)+(FRACUNIT/2))-obj->x) < FRACUNIT/2 + obj->radius)
				{
					return false;
				}
			}
			else
			{
				if(spot->GetX() == obj->tilex &&
					abs((((fixed)spot->GetY()<<FRACBITS)+(FRACUNIT/2))-obj->y) < FRACUNIT/2 + obj->radius)
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
					wait = opentics;
					break;
				case Closing:
					if(map->CheckLink(spot->GetAdjacent(MapTile::Side(direction))->zone, players[0].mo->GetZone(), true))
						PlaySoundSequence(spot, true, false);
					break;
			}
			return true;
		}

		State state;
		MapSpot spot;
		unsigned int speed;
		int amount;
		unsigned int opentics;
		unsigned int wait;
		bool direction;
};
IMPLEMENT_INTERNAL_CLASS(EVDoor)

FUNC(Door_Open)
{
	static const unsigned int DOOR_TYPE_DIRECTION = 0x1;

	if(activator->player)
	{
		if(buttonheld[bt_use])
			return 0;
		buttonheld[bt_use] = true;
	}

	if(activator->player || (activator->flags & FL_REQUIREKEYS))
	{
		if(args[3] != 0)
		{
			if(!P_CheckKeys(activator, args[3], false))
				return 0;
		}
	}

	if(args[0] == 0)
	{
		if(spot->thinker)
		{
			if(spot->thinker->IsThinkerType<EVDoor>())
			{
				return static_cast<EVDoor *>((Thinker*)spot->thinker)->Reactivate(activator, !!(activator->flags & FL_ISMONSTER));
			}
			return 0;
		}

		new EVDoor(spot, args[1], args[2], args[4]&DOOR_TYPE_DIRECTION);
	}
	else
	{
		bool activated = false;
		MapSpot door = NULL;
		while((door = map->GetSpotByTag(args[0], door)))
		{
			if(door->thinker)
			{
				if(door->thinker->IsThinkerType<EVDoor>())
				{
					return static_cast<EVDoor *>((Thinker*)door->thinker)->Reactivate(activator, !!(activator->flags & FL_ISMONSTER));
				}
				continue;
			}

			activated = true;
			new EVDoor(door, args[1], args[2], args[4]&DOOR_TYPE_DIRECTION);
		}
		return activated;
	}
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

		static bool CheckSpotFree(MapSpot spot)
		{
			if(spot->tile)
			{
				// Hit a wall so we're done.
				return false;
			}

			// Check for any blocking actors
			AActor::Iterator *iter = AActor::GetIterator();
			int movex = spot->GetX();
			int movey = spot->GetY();
			while(iter)
			{
				AActor *actor = iter->Item();
				if((actor->flags&FL_ISMONSTER) || actor->player)
				{
					if(actor->tilex+dirdeltax[actor->dir] == movex &&
						actor->tiley+dirdeltay[actor->dir] == movey)
					{
						// Blocked!
						return false;
					}
				}
				iter = iter->Next();
			}
			return true;
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
				PlaySoundSequence(spot, false, true);

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

				if(!CheckSpotFree(moveTo))
				{
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
			if((position += speed) > 1024)
			{
				position -= 1024;
				spot->pushAmount = 0;
				spot->SetTile(NULL);
				spot->thinker = NULL;
				moveTo->pushReceptor = NULL;
				moveTo->thinker = this;
				spot = moveTo;
				moveTo = NULL;
			}
			else
				spot->pushAmount = position/16;

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
	static const unsigned int PUSHWALL_DIR_DIAGONAL = 0x1;
	static const unsigned int PUSHWALL_DIR_ABSOLUTE = 0x8;

	if(args[2] & PUSHWALL_DIR_DIAGONAL)
		throw CRecoverableError("Diagonal pushwalls not yet supported!");

	bool absolute = !!(args[2]&PUSHWALL_DIR_ABSOLUTE);
	MapTrigger::Side dir = absolute ? MapTrigger::Side((args[2]>>1)&0x3) : MapTrigger::Side((direction + 1 + (args[2]>>1))&0x3);

	if(args[0] == 0)
	{
		if(spot->thinker || !spot->tile || spot->GetAdjacent(MapTile::Side(dir))->tile)
		{
			return 0;
		}

		new EVPushwall(spot, args[1], dir, args[3]);
	}
	else
	{
		bool activated = false;
		MapSpot pwall = NULL;
		while((pwall = map->GetSpotByTag(args[0], pwall)))
		{
			if(pwall->thinker || !pwall->tile || pwall->GetAdjacent(MapTile::Side(dir))->tile)
			{
				continue;
			}

			activated = true;
			new EVPushwall(pwall, args[1], dir, args[3]);
		}
		return activated;
	}
	return 1;
}

FUNC(Exit_Normal)
{
	if(buttonheld[bt_use])
		return 0;
	buttonheld[bt_use] = true;

	playstate = ex_completed;
	SD_WaitSoundDone();
	return 1;
}

FUNC(Exit_Secret)
{
	if(buttonheld[bt_use])
		return 0;
	buttonheld[bt_use] = true;

	playstate = ex_secretlevel;
	SD_WaitSoundDone();
	return 1;
}

FUNC(Exit_Victory)
{
	if(buttonheld[bt_use])
		return 0;
	buttonheld[bt_use] = true;

	playstate = ex_victorious;
	SD_WaitSoundDone();
	return 1;
}

FUNC(Teleport_NewMap)
{
	if(buttonheld[bt_use])
		return 0;
	buttonheld[bt_use] = true;

	playstate = ex_newmap;
	NewMap.newmap = args[0];
	NewMap.flags = args[2];
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

			runner = AActor::Spawn(ClassDef::FindClass("BJRun"), activator->x, activator->y, 0, true);
			runner->flags |= FL_PATHING;
			runner->angle = ((direction+2)%4)*ANGLE_90;
			runner->dir = static_cast<dirtype>(runner->angle/ANGLE_45);
			runner->GetThinker()->SetPriority(ThinkerList::VICTORY);

			activator->GetThinker()->SetPriority(ThinkerList::VICTORY);
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

			// Keep the runner in line with the player (A_Chase will try to tile align)
			switch(runner->dir)
			{
				case north:
				case south:
					runner->x = activator->x;
					break;
				case east:
				case west:
					runner->y = activator->y;
					break;
				default: break;
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
