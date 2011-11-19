/*
** actor.cpp
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

#include "actor.h"
#include "a_inventory.h"
#include "gamemap.h"
#include "id_ca.h"
#include "thinker.h"
#include "thingdef/thingdef.h"
#include "wl_agent.h"
#include "id_us.h"

Frame::~Frame()
{
	if(freeActionArgs)
	{
		// We need to delete CallArguments objects. Since we don't default to
		// NULL on these we need to check if a pointer has been set to know if
		// we created an object.
		if(action.pointer)
			delete action.args;
		if(thinker.pointer)
			delete thinker.args;
	}
}

void Frame::ActionCall::operator() (AActor *self) const
{
	if(pointer)
	{
		args->Evaluate(self);
		pointer(self, *args);
	}
}

////////////////////////////////////////////////////////////////////////////////

// We can't make AActor a thinker since we create non-thinking objects.
class AActorProxy : public Thinker
{
	DECLARE_THINKER(AActorProxy)
	public:
		AActorProxy(AActor *parent) : enabled(true), parent(parent)
		{
		}

		~AActorProxy()
		{
			if(enabled)
			{
				parent->~AActor();
				free(parent);
			}
		}

		void Disable()
		{
			enabled = false;
		}

		void Tick()
		{
			if(enabled)
				parent->Tick();
		}
	private:
		bool			enabled;
		AActor * const	parent;
};
IMPLEMENT_THINKER(AActorProxy)

LinkedList<AActor *> AActor::actors;
const ClassDef *AActor::__StaticClass = ClassDef::DeclareNativeClass<AActor>("Actor", NULL);

AActor::AActor(const ClassDef *type) : classType(type), distance(0),
	dir(nodir), soundZone(NULL), inventory(NULL), dropdefined(false)
{
	// This will be called for each actor AFTER copying the defaults.
	// Use InitClean for any one time construction.
}

AActor::~AActor()
{
	RemoveFromWorld();

	if(dropdefined)
		delete dropitems;
}

void AActor::AddInventory(AInventory *item)
{
	if(inventory == NULL)
		inventory = item;
	else
	{
		AInventory *next = inventory;
		do
		{
			if(next->inventory == NULL)
			{
				next->inventory = item;
				break;
			}
		}
		while((next = next->inventory));
	}
}

void AActor::Destroy()
{
	if(thinker != NULL)
	{
		thinker->Destroy();
		thinker = NULL;
	}
}

void AActor::Die()
{
	GivePoints(points);
	if(flags & FL_COUNTKILL)
		gamestate.killcount++;
	flags &= ~FL_SHOOTABLE;

	if(dropitems)
	{
		DropList::Node *item = dropitems->Head();
		do
		{
			DropItem &drop = item->Item();
			if(US_RndT() < drop.probabilty)
			{
				const ClassDef *cls = ClassDef::FindClass(drop.className);
				if(cls)
				{
					// We can't use tilex/tiley since it's used primiarily by
					// the AI, so it can be off by one.
					static const fixed TILEMASK = ~(TILEGLOBAL-1);

					AActor *actor = AActor::Spawn(cls, (x&TILEMASK)+TILEGLOBAL/2, (y&TILEMASK)+TILEGLOBAL/2, 0);
					actor->angle = angle;
					actor->dir = dir;
				}
			}
		}
		while((item = item->Next()));
	}

	if(DeathState)
		SetState(DeathState);
	else
		Destroy();
}

void AActor::EnterZone(const MapZone *zone)
{
	if(zone)
		soundZone = zone;
}

AInventory *AActor::FindInventory(const ClassDef *cls) const
{
	if(inventory == NULL)
		return NULL;

	AInventory *check = inventory;
	do
	{
		if(check->classType == cls)
			return check;
	}
	while((check = check->inventory));
	return NULL;
}

const Frame *AActor::FindState(const FName &name) const
{
	return classType->FindState(name);
}

void AActor::InitClean()
{
	flags = 0;
	SpawnState = NULL;

	Init(true);
}

void AActor::Init(bool nothink)
{
	if(!nothink)
	{
		actorRef = actors.Push(this);
		thinker = new AActorProxy(this);

		if(SpawnState)
			SetState(SpawnState, true);
		else
		{
			state = NULL;
			Destroy();
		}
	}
	else
	{
		actorRef = NULL;
		thinker = NULL;
		state = NULL;
	}
}

void AActor::SetState(const Frame *state, bool notic)
{
	if(state == NULL)
		return;

	this->state = state;
	sprite = state->spriteInf;
	ticcount = state->duration;
	state->action(this);
}

void AActor::Tick()
{
	if(state == NULL)
	{
		Destroy();
		return;
	}

	if(ticcount > 0)
		--ticcount;

	while(ticcount == 0)
	{
		if(!state->next)
		{
			Destroy();
			return;
		}
		SetState(state->next);
	}

	state->thinker(this);
}

// Remove an actor from the game world without destroying it.  This will allow
// us to transfer items into inventory for example.
void AActor::RemoveFromWorld()
{
	if(actorRef)
	{
		actors.Remove(actorRef);
		actorRef = NULL;
	}
	if(thinker)
	{
		thinker->Disable();
		thinker->Destroy();
		thinker = NULL;
	}
}

void AActor::RemoveInventory(AInventory *item)
{
	if(inventory == NULL)
		return;

	AInventory **next = &inventory;
	do
	{
		if(*next == item)
		{
			*next = (*next)->inventory;
			break;
		}
	}
	while((next = &(*next)->inventory));
}

AActor *AActor::Spawn(const ClassDef *type, fixed x, fixed y, fixed z)
{
	if(type == NULL)
	{
		printf("Tried to spawn classless actor.\n");
		return NULL;
	}

	AActor *actor = type->CreateInstance();
	actor->x = x;
	actor->y = y;
	actor->tilex = x>>FRACBITS;
	actor->tiley = y>>FRACBITS;

	MapSpot spot = map->GetSpot(actor->tilex, actor->tiley, 0);
	actor->EnterZone(spot->zone);

	if(actor->flags & FL_COUNTKILL)
		++gamestate.killtotal;
	return actor;
}

AActor *AActor::__InPlaceConstructor(const ClassDef *classDef, void *mem)
{
	return new (mem) AActor(classDef);
}

DEFINE_SYMBOL(Actor, angle)
DEFINE_SYMBOL(Actor, health)
