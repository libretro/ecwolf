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

void AInventory::Touch(AActor *toucher)
{
	if(!TryPickup(toucher))
		return;

	PlaySoundLocActor(pickupsound, toucher);
	StartBonusFlash();
	Destroy();
}

bool AInventory::TryPickup(AActor *toucher)
{
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
	}
	return true;
}
