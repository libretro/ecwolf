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
#include "g_mapinfo.h"
#include "id_ca.h"
#include "id_sd.h"
#include "thinker.h"
#include "thingdef/thingdef.h"
#include "thingdef/thingdef_expression.h"
#include "wl_agent.h"
#include "wl_game.h"
#include "wl_loadsave.h"
#include "id_us.h"
#include "m_random.h"

void T_ExplodeProjectile(AActor *self, AActor *target);
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

static FRandom pr_statetics("StateTics");
int Frame::GetTics() const
{
	if(randDuration)
		return duration + pr_statetics.GenRand32() % (randDuration + 1);
	return duration;
}

void Frame::ActionCall::operator() (AActor *self, AActor *stateOwner, const Frame * const caller) const
{
	if(pointer)
	{
		args->Evaluate(self);
		pointer(self, stateOwner, caller, *args);
	}
}

FArchive &operator<< (FArchive &arc, const Frame *&frame)
{
	if(arc.IsStoring())
	{
		// Find a class which held this state.
		// This should always be able to be found.
		const ClassDef *cls = NULL;
		if(frame)
		{
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
			arc << cls;
		}
	}
	else
	{
		const ClassDef *cls;
		unsigned int frameIndex;

		arc << cls;
		if(cls)
		{
			arc << frameIndex;

			frame = cls->GetState(frameIndex);
		}
		else
			frame = NULL;
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

		void Destroy()
		{
			Super::Destroy();

			if(enabled && parent)
			{
				parent->Destroy();
				parent = NULL;
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

		void PostBeginPlay()
		{
			if(enabled)
				parent->PostBeginPlay();
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
PointerIndexTable<ExpressionNode> AActor::damageExpressions;
PointerIndexTable<AActor::DropList> AActor::dropItems;
IMPLEMENT_POINTY_CLASS(Actor)
	DECLARE_POINTER(inventory)
	DECLARE_POINTER(thinker)
END_POINTERS

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

				// Prevent the GC from cleaning this item
				GC::WriteBarrier(item);
				break;
			}
		}
		while((next = next->inventory));
	}
}

void AActor::Destroy()
{
	Super::Destroy();
	RemoveFromWorld();

	// Inventory items don't have a registered thinker so we must free them now
	if(inventory)
	{
		assert(inventory->thinker == NULL);

		inventory->Destroy();
		inventory = NULL;
	}
}

static FRandom pr_dropitem("DropItem");
void AActor::Die()
{
	GivePoints(points);
	if(flags & FL_COUNTKILL)
		gamestate.killcount++;
	flags &= ~FL_SHOOTABLE;

	if(flags & FL_MISSILE)
	{
		T_ExplodeProjectile(this, NULL);
		return;
	}

	DropList *dropitems = GetDropList();
	if(dropitems)
	{
		DropList::Node *item = dropitems->Head();
		DropItem *bestDrop = NULL; // For FL_DROPBASEDONTARGET
		do
		{
			DropItem *drop = &item->Item();
			if(pr_dropitem() <= drop->probability)
			{
				const ClassDef *cls = ClassDef::FindClass(drop->className);
				if(cls)
				{
					if(flags & FL_DROPBASEDONTARGET)
					{
						AActor *target = players[0].mo;
						AInventory *inv = target->FindInventory(cls->GetReplacement());
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

					AActor * const actor = AActor::Spawn(cls, (x&TILEMASK)+TILEGLOBAL/2, (y&TILEMASK)+TILEGLOBAL/2, 0, SPAWN_AllowReplacement);
					actor->angle = angle;
					actor->dir = dir;

					if(cls->IsDescendantOf(NATIVE_CLASS(Inventory)))
					{
						AInventory * const inv = static_cast<AInventory *>(actor);

						// Ammo is multiplied by 0.5
						if(drop->amount > 0)
						{
							// TODO: When a dropammofactor is specified it should
							// apply here too.
							inv->amount = drop->amount;
						}
						else if(cls->IsDescendantOf(NATIVE_CLASS(Ammo)) && inv->amount > 1)
							inv->amount /= 2;

						// TODO: In ZDoom dropped weapons have their ammo multiplied as well
						// but in Wolf3D weapons always have 6 bullets.
					}
				}
			}
		}
		while((item = item->Next()));
	}

	bool isExtremelyDead = health < -GetClass()->Meta.GetMetaInt(AMETA_GibHealth, (GetDefault()->health*gameinfo.GibFactor)>>FRACBITS);
	const Frame *deathstate = NULL;
	if(isExtremelyDead)
		deathstate = FindState(NAME_XDeath);
	if(!deathstate)
		deathstate = FindState(NAME_Death);
	if(deathstate)
		SetState(deathstate);
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

int AActor::GetDamage()
{
	int expression = GetClass()->Meta.GetMetaInt(AMETA_Damage, -1);
	if(expression >= 0)
		return static_cast<int>(damageExpressions[expression]->Evaluate(this).GetInt());
	return 0;
}

AActor::DropList *AActor::GetDropList() const
{
	int dropitemsIndex = GetClass()->Meta.GetMetaInt(AMETA_DropItems, -1);
	if(dropitemsIndex == -1)
		return NULL;
	return dropItems[dropitemsIndex];
}

const AActor *AActor::GetDefault() const
{
	return GetClass()->GetDefault();
}

Thinker *AActor::GetThinker()
{
	return (Thinker*)thinker;
}

void AActor::Init()
{
	ObjectFlags |= OF_JustSpawned;

	distance = 0;
	dir = nodir;
	soundZone = NULL;
	inventory = NULL;

	actorRef = actors.Push(this);
	if(!loadedgame)
	{
		thinker = new AActorProxy(this);
		thinker->ObjectFlags |= OF_JustSpawned;
	}

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
	bool hasActorRef = actorRef != NULL;

	if(arc.IsStoring())
		arc.WriteSprite(sprite);
	else
		sprite = arc.ReadSprite();

	BYTE dir = this->dir;
	arc << dir;
	this->dir = static_cast<dirtype>(dir);

	arc << flags
		<< distance
		<< x
		<< y
		<< velx
		<< vely
		<< angle
		<< pitch
		<< health
		<< speed
		<< runspeed
		<< points
		<< radius
		<< ticcount
		<< state
		<< viewx
		<< viewheight
		<< transx
		<< transy
		<< sighttime
		<< sightrandom
		<< minmissilechance
		<< painchance
		<< missilefrequency
		<< movecount
		<< meleerange
		<< activesound
		<< attacksound
		<< deathsound
		<< seesound
		<< painsound
		<< temp1
		<< hidden
		<< player
		<< inventory
		<< soundZone
		<< thinker
		<< hasActorRef;

	if(GameSave::SaveVersion > 1374914454)
		arc << projectilepassheight;

	if(!arc.IsStoring())
	{
		if(!hasActorRef)
		{
			actors.Remove(actorRef);
			actorRef = NULL;
		}
	}

	Super::Serialize(arc);
}

void AActor::SetState(const Frame *state, bool norun)
{
	if(state == NULL)
	{
		Destroy();
		return;
	}

	this->state = state;
	sprite = state->spriteInf;
	ticcount = state->GetTics();
	if(!norun)
	{
		state->action(this, this, state);

		while(ticcount == 0)
		{
			this->state = this->state->next;
			if(!this->state)
			{
				Destroy();
				break;
			}
			else
			{
				sprite = this->state->spriteInf;
				ticcount = this->state->GetTics();
				this->state->action(this, this, this->state);
			}
		}
	}
}

void AActor::Tick()
{
	// If we just spawned we're not ready to be ticked yet
	// Otherwise we might tick on the same tick we're spawned which would cause
	// an actor with a duration of 1 tic to never display
	if(ObjectFlags & OF_JustSpawned)
	{
		ObjectFlags &= ~OF_JustSpawned;
		return;
	}

	if(state == NULL)
	{
		Destroy();
		return;
	}

	if(ticcount > 0)
		--ticcount;

	if(ticcount == 0)
	{
		SetState(state->next);
		if(ObjectFlags & OF_EuthanizeMe)
			return;
	}

	state->thinker(this, this, state);

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
		((AActorProxy*)(Thinker*)thinker)->Disable();
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
	while(*next && (next = &(*next)->inventory));

	item->DetachFromOwner();
}

/* When we spawn an actor we add them to this list. After the tic has finished
 * processing we process this list to handle any start up actions.
 *
 * This is done so that we don't duplicate tics and actors appear on screen
 * when they should. We can't do this in Spawn() since we want certain
 * properties of the actor (velocity) to be setup before calling actions.
 */
static LinkedList<AActor *> SpawnedActors;
void AActor::FinishSpawningActors()
{
	LinkedList<AActor *>::Node *node = SpawnedActors.Head();
	while((node = SpawnedActors.Head()))
	{
		// Kind of a weird way to iterate, but remember that new actors can be
		// pushed to this list. Currently our LinkedList class only tracks the
		// head.
		AActor *actor = node->Item();
		SpawnedActors.Remove(node);

		// Run the first action pointer and all zero tic states!
		actor->SetState(actor->state);
		actor->ObjectFlags &= ~OF_JustSpawned;
	}
}

FRandom pr_spawnmobj("SpawnActor");
AActor *AActor::Spawn(const ClassDef *type, fixed x, fixed y, fixed z, int flags)
{
	if(type == NULL)
	{
		printf("Tried to spawn classless actor.\n");
		return NULL;
	}

	if(flags & SPAWN_AllowReplacement)
		type = type->GetReplacement();

	AActor *actor = type->CreateInstance();
	actor->x = x;
	actor->y = y;
	actor->velx = 0;
	actor->vely = 0;
	actor->health = type->Meta.GetMetaInt(AMETA_DefaultHealth1 + gamestate.difficulty, actor->health);

	MapSpot spot = map->GetSpot(actor->tilex, actor->tiley, 0);
	actor->EnterZone(spot->zone);

	// Execute begin play hook and then check if the actor is still alive.
	actor->BeginPlay();
	if(actor->ObjectFlags & OF_EuthanizeMe)
		return NULL;

	if(actor->flags & FL_COUNTKILL)
		++gamestate.killtotal;
	if(actor->flags & FL_COUNTITEM)
		++gamestate.treasuretotal;
	if(actor->flags & FL_COUNTSECRET)
		++gamestate.secrettotal;

	if(levelInfo && levelInfo->SecretDeathSounds)
	{
		const char* snd = type->Meta.GetMetaString(AMETA_SecretDeathSound);
		if(snd)
			actor->deathsound = snd;
	}

	if(actor->flags & FL_MISSILE)
	{
		PlaySoundLocActor(actor->seesound, actor);
		if((actor->flags & FL_RANDOMIZE) && actor->ticcount > 0)
		{
			actor->ticcount -= pr_spawnmobj() & 7;
			if(actor->ticcount < 1)
				actor->ticcount = 1;
		}
	}
	else
	{
		if((actor->flags & FL_RANDOMIZE) && actor->ticcount > 0)
			actor->ticcount = pr_spawnmobj() % actor->ticcount;
	}

	// Change between patrolling and normal spawn and also execute any zero
	// tic functions.
	if(flags & SPAWN_Patrol)
	{
		actor->flags |= FL_PATHING;

		// Pathing monsters should take at least a one tile step.
		// Otherwise the paths will break early.
		actor->distance = TILEGLOBAL;
		if(actor->PathState)
		{
			actor->SetState(actor->PathState, true);
			if(actor->flags & FL_RANDOMIZE)
				actor->ticcount = pr_spawnmobj() % actor->ticcount;
		}
	}

	SpawnedActors.Push(actor);

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
	gamestate.victoryflag = false;

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
					players[0].camera = player;
					playertmp->Destroy();

					// We must move the linked list iterator here since we'll
					// transfer to the new linked list at the SetPriority call
					node = node->Next();
					player->GetThinker()->SetPriority(ThinkerList::PLAYER);
					continue;
				}
			}
		}

		node = node->Next();
	}
	while(node);
}
