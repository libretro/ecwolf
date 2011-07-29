/*
** a_inventory.cpp
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

#include "a_inventory.h"
#include "id_sd.h"
#include "thingdef/thingdef.h"
#include "wl_def.h"

IMPLEMENT_CLASS(Inventory, Actor)

// Either creates a copy if the item or returns itself if it is safe to place
// in the actor's inventory.
AInventory *AInventory::CreateCopy(AActor *holder)
{
	if(!GoesAway())
		return this;

	AInventory *copy = reinterpret_cast<AInventory *>(classType->CreateInstance());
	copy->RemoveFromWorld();
	copy->amount = amount;
	copy->maxamount = maxamount;
	return copy;
}

// Used for items which aren't placed into an inventory and don't respawn.
void AInventory::GoAwayAndDie()
{
	if(GoesAway())
	{
		Destroy();
	}
}

// Returns false if this is safe to place into inventory.  True if it will be
// reused in the world.
bool AInventory::GoesAway()
{
	return false;
}

// Returns true if the pickup was handled by an already existing inventory item.
bool AInventory::HandlePickup(AInventory *item, bool &good)
{
	if(item->classType == classType)
	{
		if(amount < maxamount)
		{
			amount += item->amount;
			if(amount > maxamount)
				amount = maxamount;
			good = true;
		}
		else
			good = false;
		return true;
	}
	else if(inventory)
		return inventory->HandlePickup(item, good);
	return false;
}

void AInventory::Touch(AActor *toucher)
{
	if(!TryPickup(toucher))
		return;

	PlaySoundLocActor(pickupsound, toucher);
	StartBonusFlash();
}

bool AInventory::TryPickup(AActor *toucher)
{
	bool pickupGood = false;
	if(toucher->inventory && toucher->inventory->HandlePickup(this, pickupGood))
	{
		// The actor has this item in their inventory and it has been handled.
		if(!pickupGood)
			return false;
		GoAwayAndDie();
	}
	else
	{
		AInventory *invItem = CreateCopy(toucher);
		if(invItem != this)
			GoAwayAndDie();

		toucher->AddInventory(invItem);
	}
	return true;
}

bool AInventory::Use()
{
	GivePoints(points);
	return true;
}

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(Health, Inventory)

bool AHealth::TryPickup(AActor *toucher)
{
	//if(!Super::TryPickup(toucher))
	//	return false;

	unsigned int max = maxamount;
	if(max == 0)
		max = player->health;
	printf("%d %d %d %d\n", amount, maxamount, player->health, gamestate.health);

	if(gamestate.health >= max)
		return false;
	else
	{
		gamestate.health += amount;
		if(gamestate.health > max)
			gamestate.health = max;
		DrawHealth();
		DrawFace();
		Destroy();
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(Ammo, Inventory)

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(Weapon, Inventory)
