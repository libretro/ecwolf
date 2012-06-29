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
#include "farchive.h"
#include "gamemap.h"
#include "id_ca.h"
#include "thinker.h"
#include "thingdef/thingdef.h"
#include "thingdef/thingdef_expression.h"
#include "wl_agent.h"
#include "wl_game.h"
#include "id_us.h"
#include "m_random.h"

void T_Projectile(AActor *self);

////////////////////////////////////////////////////////////////////////////////

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

FArchive &operator<< (FArchive &arc, const Frame *&frame)
{
	if(arc.IsStoring())
	{
		// Find a class which held this state.
		// This should always be able to be found.
		const ClassDef *cls;
		ClassDef::ClassIterator iter = ClassDef::GetClassIterator();
		ClassDef::ClassPair *pair;
		while(iter.NextPair(pair))
		{
			cls = pair->Value;
			if(cls->IsStateOwner(frame))
				break;
		}

		arc << cls;
		arc << const_cast<Frame *>(frame)->index;
	}
	else
	{
		const ClassDef *cls;
		unsigned int frameIndex;

		arc << cls;
		arc << frameIndex;

		frame = cls->GetState(frameIndex);
	}

	return arc;
}

////////////////////////////////////////////////////////////////////////////////

// We can't make AActor a thinker since we create non-thinking objects.
class AActorProxy : public Thinker
{
	DECLARE_CLASS(AActorProxy, Thinker)
	HAS_OBJECT_POINTERS

	public:
		AActorProxy(AActor *parent) : enabled(true), parent(parent)
		{
		}

		~AActorProxy()
		{
			if(enabled && parent)
			{
				parent->Destroy();
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

		void Serialize(FArchive &arc)
		{
			arc << enabled << parent;
			Super::Serialize(arc);
		}

		bool			enabled;
		TObjPtr<AActor>	parent;
};
IMPLEMENT_INTERNAL_POINTY_CLASS(AActorProxy)
	DECLARE_POINTER(parent)
END_POINTERS

LinkedList<AActor *> AActor::actors;
IMPLEMENT_POINTY_CLASS(Actor)
	DECLARE_POINTER(inventory)
	DECLARE_POINTER(thinker)
END_POINTERS

AActor::~AActor()
{
	if(this == GetDefault())
	{
		if(dropitems)
			delete dropitems;
		if(damage)
			delete damage;
	}
}

void AActor::AddInventory(AInventory *item)
{
	item->AttachToOwner(this);

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

	RemoveFromWorld();

	// Inventory items don't have a registered thinker so we must free them now
	if(inventory)
	{
		assert(inventory->thinker == NULL);

		inventory->Destroy();
	}

	Super::Destroy();
}

static FRandom pr_dropitem("DropItem");
void AActor::Die()
{
	GivePoints(points);
	if(flags & FL_COUNTKILL)
		gamestate.killcount++;
	flags &= ~FL_SHOOTABLE;

	if(flags & FL_MISSILE)
		flags &= ~FL_MISSILE;

	if(dropitems)
	{
		DropList::Node *item = dropitems->Head();
		DropItem *bestDrop = NULL; // For FL_DROPBASEDONTARGET
		do
		{
			DropItem *drop = &item->Item();
			if(pr_dropitem() < drop->probabilty)
			{
				const ClassDef *cls = ClassDef::FindClass(drop->className);
				if(cls)
				{
					if(flags & FL_DROPBASEDONTARGET)
					{
						AActor *target = players[0].mo;
						AInventory *inv = target->FindInventory(cls);
						if(!inv || !bestDrop)
							bestDrop = drop;

						if(item->Next() != NULL)
							continue;
						else
						{
							cls = ClassDef::FindClass(bestDrop->className);
							drop = bestDrop;
						}
					}

					// We can't use tilex/tiley since it's used primiarily by
					// the AI, so it can be off by one.
					static const fixed TILEMASK = ~(TILEGLOBAL-1);

					AActor *actor = AActor::Spawn(cls, (x&TILEMASK)+TILEGLOBAL/2, (y&TILEMASK)+TILEGLOBAL/2, 0);
					actor->angle = angle;
					actor->dir = dir;
					if(cls->IsDescendantOf(NATIVE_CLASS(Ammo)))
						static_cast<AAmmo *>(actor)->amount = drop->amount;
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

AInventory *AActor::FindInventory(const ClassDef *cls)
{
	if(inventory == NULL)
		return NULL;

	AInventory *check = inventory;
	do
	{
		if(check->IsA(cls))
			return check;
	}
	while((check = check->inventory));
	return NULL;
}

const Frame *AActor::FindState(const FName &name) const
{
	return GetClass()->FindState(name);
}

const AActor *AActor::GetDefault() const
{
	return GetClass()->GetDefault();
}

Thinker *AActor::GetThinker()
{
	return (AActorProxy*)thinker;
}

void AActor::Init()
{
	distance = 0;
	dir = nodir;
	soundZone = NULL;
	inventory = NULL;

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

void AActor::Serialize(FArchive &arc)
{
	if(arc.IsStoring())
		arc.WriteSprite(sprite);
	else
		sprite = arc.ReadSprite();

	arc << flags
		<< distance
		<< x
		<< y
		<< velx
		<< vely
		<< angle
		<< health
		<< speed
		<< runspeed
		<< points
		<< radius
		<< ticcount
		<< state
		<< sprite
		<< viewx
		<< viewheight
		<< transx
		<< transy
		<< sighttime
		<< sightrandom
		<< missilechance
		<< attacksound
		<< deathsound
		<< seesound
		<< player
		<< inventory
		<< thinker;

	Super::Serialize(arc);
}

void AActor::SetState(const Frame *state, bool notic)
{
	if(state == NULL)
		return;

	this->state = state;
	sprite = state->spriteInf;
	ticcount = state->duration;
	if(!notic)
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

	if(flags & FL_MISSILE)
		T_Projectile(this);
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

	item->DetachFromOwner();

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
	actor->velx = 0;
	actor->vely = 0;

	MapSpot spot = map->GetSpot(actor->tilex, actor->tiley, 0);
	actor->EnterZone(spot->zone);

	if(actor->flags & FL_COUNTKILL)
		++gamestate.killtotal;
	if(actor->flags & FL_COUNTITEM)
		++gamestate.treasuretotal;
	if(actor->flags & FL_COUNTSECRET)
		++gamestate.secrettotal;
	return actor;
}

DEFINE_SYMBOL(Actor, angle)
DEFINE_SYMBOL(Actor, health)

//==============================================================================

/*
===================
=
= Player travel functions
=
===================
*/

void StartTravel ()
{
	// Set thinker priorities to TRAVEL so that they don't get wiped on level
	// load.  We'll transfer them to a new actor.

	AActor *player = players[0].mo;

	player->GetThinker()->SetPriority(ThinkerList::TRAVEL);
}

void FinishTravel ()
{
	ThinkerList::Iterator node = thinkerList->GetHead(ThinkerList::TRAVEL);
	if(!node)
		return;

	do
	{
		if(node->Item()->IsThinkerType<AActorProxy>())
		{
			AActorProxy *proxy = static_cast<AActorProxy *>((Thinker*)node->Item());
			if(proxy->parent->IsKindOf(NATIVE_CLASS(PlayerPawn)))
			{
				APlayerPawn *player = static_cast<APlayerPawn *>((AActor*)proxy->parent);
				if(player->player == &players[0])
				{
					AActor *playertmp = players[0].mo;
					player->x = playertmp->x;
					player->y = playertmp->y;
					player->angle = playertmp->angle;
					player->EnterZone(playertmp->GetZone());

					players[0].mo = player;
					playertmp->Destroy();

					// We must move the linked list iterator here since we'll
					// transfer to the new linked list at the SetPriority call
					node = node->Next();
					player->GetThinker()->SetPriority(ThinkerList::NORMAL);
					continue;
				}
			}
		}

		node = node->Next();
	}
	while(node);
}
