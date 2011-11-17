/*
** a_playerpawn.cpp
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
#include "a_playerpawn.h"
#include "thingdef/thingdef.h"
#include "wl_agent.h"

IMPLEMENT_CLASS(PlayerPawn, Actor)

void GunAttack(AActor *ob);

void APlayerPawn::GiveStartingInventory()
{
	if(!startInventory)
		return;

	DropList::Node *item = startInventory->Head();
	do
	{
		DropItem &inv = item->Item();
		const ClassDef *cls = ClassDef::FindClass(inv.className);
		if(!cls || !cls->IsDescendantOf(NATIVE_CLASS(Inventory)))
			continue;

		AInventory *invItem = (AInventory *)AActor::Spawn(cls, 0, 0, 0);
		invItem->RemoveFromWorld();
		invItem->amount = inv.amount;
		if(!invItem->TryPickup(this))
			invItem->Destroy();

		if(cls->IsDescendantOf(NATIVE_CLASS(Weapon)))
			player->ReadyWeapon = (AWeapon *)invItem;
	}
	while((item = item->Next()));

	// Bring up weapon
	if(player->ReadyWeapon)
		player->ReadyWeapon->SetState(player->ReadyWeapon->FindState("Ready"));

#if 1
	AInventory *inv = inventory;
	while(inv)
	{
		Printf("%s %d/%d\n", inv->GetClass()->GetName().GetChars(), inv->amount, inv->maxamount);
		inv = inv->inventory;
	}
#endif
}

void APlayerPawn::InitClean()
{
	startInventory = NULL;
	Super::InitClean();
}

void APlayerPawn::Tick()
{
	Super::Tick();

	// Watching BJ
	if(gamestate.victoryflag)
	{
		VictorySpin();
		return;
	}

	UpdateFace();
	CheckWeaponChange();

	if(buttonstate[bt_use])
		Cmd_Use();

	if(buttonstate[bt_attack] && !buttonheld[bt_attack])
	{
		// Fire weapon
		GunAttack(this);
	}

	ControlMovement(this);
	if(gamestate.victoryflag)
		return;

	if(player->ReadyWeapon)
	{
		player->ReadyWeapon->Tick();
	}
}
