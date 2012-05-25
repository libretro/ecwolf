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
#include "templates.h"
#include "thingdef/thingdef.h"
#include "wl_def.h"
#include "wl_agent.h"
#include "wl_game.h"
#include "wl_play.h"

IMPLEMENT_CLASS(Inventory, Actor)

void AInventory::AttachToOwner(AActor *owner)
{
	this->owner = owner;
}

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

void AInventory::DetachFromOwner()
{
	owner = NULL;
}

// Used for items which aren't placed into an inventory and don't respawn.
void AInventory::GoAwayAndDie()
{
	if(!GoesAway())
	{
		Destroy();
	}
}

// Returns false if this is safe to place into inventory.  True if it hides
// itself to be reused later.
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

void AInventory::InitClean()
{
	Super::InitClean();
	itemFlags = 0;
	player = NULL;
}

void AInventory::Touch(AActor *toucher)
{
	if(!(toucher->flags & FL_PICKUP))
		return;

	if(!TryPickup(toucher))
		return;

	if(flags & FL_COUNTITEM)
		++gamestate.treasurecount;
	if(flags & FL_COUNTSECRET)
		++gamestate.secretcount;

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
	else if(maxamount == 0)
	{
		// We can add maxamount = 0 items if we can use them right away.
		if(!(itemFlags & IF_AUTOACTIVATE))
			return false;

		toucher->AddInventory(this);
		bool good = Use();
		toucher->RemoveInventory(this);

		if(good)
			GoAwayAndDie();
		else
			return false;
	}
	else
	{
		AInventory *invItem = CreateCopy(toucher);
		if(invItem != this)
			GoAwayAndDie();

		toucher->AddInventory(invItem);
		invItem->RemoveFromWorld();

		if((itemFlags & IF_AUTOACTIVATE) && invItem->Use())
			--invItem->amount;
	}
	return true;
}

bool AInventory::Use()
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(Health, Inventory)

bool AHealth::TryPickup(AActor *toucher)
{
	//if(!Super::TryPickup(toucher))
	//	return false;

	unsigned int max = maxamount;
	if(max == 0)
		max = toucher->health;

	if(toucher->player->health >= max)
		return false;
	else
	{
		toucher->player->health += amount;
		if(toucher->player->health > max)
			toucher->player->health = max;
		DrawStatusBar();
		Destroy();
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(Ammo, Inventory)

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(Weapon, Inventory)

void AWeapon::AttachToOwner(AActor *owner)
{
	Super::AttachToOwner(owner);

	ammo1 = static_cast<AAmmo *>(owner->FindInventory(ammotype1));
	if(!ammo1)
	{
		ammo1 = static_cast<AAmmo *>(Spawn(ammotype1, 0, 0, 0));
		ammo1->amount = MIN(ammogive1, ammo1->maxamount);
		owner->AddInventory(ammo1);
		ammo1->RemoveFromWorld();
	}
	else if(ammo1->amount < ammo1->maxamount)
	{
		ammo1->amount += ammogive1;
		if(ammo1->amount > ammo1->maxamount)
			ammo1->amount = ammo1->maxamount;
	}
}

bool AWeapon::CheckAmmo(AWeapon::FireMode fireMode, bool autoSwitch, bool requireAmmo)
{
	const unsigned int amount1 = ammo1 != NULL ? ammo1->amount : 0;

	if(amount1 >= ammouse1)
		return true;

	if(autoSwitch)
	{
		static_cast<APlayerPawn *>(owner)->PickNewWeapon();
	}

	return false;
}

bool AWeapon::DepleteAmmo()
{
	if(!CheckAmmo(mode, false))
		return false;

	AAmmo * const ammo = ammo1;
	const unsigned int ammouse = ammouse1;

	if(ammo == NULL)
		return true;

	if(ammo->amount < ammouse1)
		ammo->amount = 0;
	else
		ammo->amount -= ammouse;

	return true;
}

const Frame *AWeapon::GetAtkState(bool hold) const
{
	const Frame *ret = NULL;
	if(hold)
		ret = FindState(NAME_Hold);
	if(ret == NULL)
		ret = FindState(NAME_Fire);
	return ret;
}

const Frame *AWeapon::GetUpState() const
{
	return FindState(NAME_Select);
}

const Frame *AWeapon::GetDownState() const
{
	return FindState(NAME_Deselect);
}

const Frame *AWeapon::GetReadyState() const
{
	return FindState(NAME_Ready);
}

bool AWeapon::HandlePickup(AInventory *item, bool &good)
{
	if(item->GetClass() == GetClass())
	{
		good = static_cast<AWeapon *>(item)->UseForAmmo(this);
		return true;
	}
	else if(inventory)
		return inventory->HandlePickup(item, good);
	return false;
}

bool AWeapon::UseForAmmo(AWeapon *owned)
{
	AAmmo *ammo1 = owned->ammo1;
	if(!ammo1 || ammogive1 <= 0)
		return false;

	if(ammo1->amount < ammo1->maxamount)
	{
		ammo1->amount += ammogive1;
		if(ammo1->amount > ammo1->maxamount)
			ammo1->amount = ammo1->maxamount;
		return true;
	}
	return false;
}

ACTION_FUNCTION(A_ReFire)
{
	player_t *player = self->player;
	if(!player)
		return;

	if(!player->ReadyWeapon->CheckAmmo(AWeapon::PrimaryFire, true))
		return;

	if(buttonstate[bt_attack] && player->PendingWeapon == WP_NOCHANGE)
		player->SetPSprite(player->ReadyWeapon->GetAtkState(true));
}

ACTION_FUNCTION(A_WeaponReady)
{
	self->player->flags |= player_t::PF_WEAPONREADY;
}

////////////////////////////////////////////////////////////////////////////////

class AScoreItem : public AInventory
{
	DECLARE_NATIVE_CLASS(ScoreItem, Inventory)

	protected:
		bool TryPickup(AActor *toucher)
		{
			GivePoints(amount);
			GoAwayAndDie();
			return true;
		}
};
IMPLEMENT_CLASS(ScoreItem, Inventory)
