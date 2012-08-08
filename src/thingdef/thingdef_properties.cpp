/*
** thingdef_properties.cpp
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
#include "thingdef.h"
#include "a_inventory.h"
#include "a_playerpawn.h"
#include "scanner.h"
#include "thingdef/thingdef_type.h"
#include "thingdef/thingdef_expression.h"

#define IS_EXPR(no) params[no].isExpression
#define EXPR_PARAM(var, no) ExpressionNode *var = params[no].expr;
#define INT_PARAM(var, no) int64_t var; if(IS_EXPR(no)) { var = params[no].expr->Evaluate(defaults).GetInt(); delete params[no].expr; } else var = params[no].i
#define FLOAT_PARAM(var, no) double var = params[no].f;
#define STRING_PARAM(var, no) const char* var = params[no].s;

HANDLE_PROPERTY(ammogive1)
{
	INT_PARAM(give, 0);
	((AWeapon *)defaults)->ammogive1 = give;
}

HANDLE_PROPERTY(ammotype1)
{
	STRING_PARAM(type, 0);
	if(stricmp(type, "none") == 0 || *type == '\0')
		((AWeapon *)defaults)->ammotype1 = NULL;
	else
		((AWeapon *)defaults)->ammotype1 = ClassDef::FindClassTentative(type, NATIVE_CLASS(Ammo));
}

HANDLE_PROPERTY(ammouse1)
{
	INT_PARAM(use, 0);
	((AWeapon *)defaults)->ammouse1 = use;
}

HANDLE_PROPERTY(amount)
{
	INT_PARAM(amt, 0);
	((AInventory *)defaults)->amount = amt;
}

HANDLE_PROPERTY(attacksound)
{
	STRING_PARAM(snd, 0);
	defaults->attacksound = snd;
}

HANDLE_PROPERTY(damage)
{
	if(!IS_EXPR(0))
	{
		INT_PARAM(dmg, 0);
		if(dmg == 0)
			cls->Meta.SetMetaInt(AMETA_Damage, -1);
		else
		{
			FString defFormula;
			defFormula.Format("random(1,8)*%d", (int) dmg);
			Scanner sc(defFormula.GetChars(), defFormula.Len());
			cls->Meta.SetMetaInt(AMETA_Damage,
				AActor::damageExpressions.Push(ExpressionNode::ParseExpression(defaults->GetClass(), TypeHierarchy::staticTypes, sc, NULL)));
		}
	}
	else
	{
		EXPR_PARAM(dmg, 0);
		cls->Meta.SetMetaInt(AMETA_Damage, AActor::damageExpressions.Push(dmg));
	}
}

HANDLE_PROPERTY(deathsound)
{
	STRING_PARAM(snd, 0);
	defaults->deathsound = snd;
}

HANDLE_PROPERTY(displayname)
{
	STRING_PARAM(name, 0);

	cls->Meta.SetMetaString(APMETA_DisplayName, name);
}

HANDLE_PROPERTY(dropitem)
{
	// NOTE: When used with inheritance the old list is wiped.
	STRING_PARAM(item, 0);

	if(cls->Meta.GetMetaInt(AMETA_DropItems, -1) == -1)
		cls->Meta.SetMetaInt(AMETA_DropItems, AActor::dropItems.Push(new AActor::DropList()));

	AActor::DropItem drop;
	drop.className = item;
	drop.amount = 1;
	drop.probabilty = 255;

	if(PARAM_COUNT > 1)
	{
		INT_PARAM(amt, 1);
		drop.amount = amt;
		if(PARAM_COUNT > 2)
		{
			INT_PARAM(prb, 2);
			if(prb > 255)
				prb = 255;
			else if(prb < 0)
				prb = 0;
			drop.probabilty = prb;
		}
	}

	AActor::dropItems[cls->Meta.GetMetaInt(AMETA_DropItems)]->Push(drop);
}

HANDLE_PROPERTY(health)
{
	INT_PARAM(health, 0);

	defaults->defaultHealth[0] = defaults->health = health;

	int currentSlope = 0;
	unsigned int lastValue = health;
	for(unsigned int i = 1;i < PARAM_COUNT;i++)
	{
		INT_PARAM(skillHealth, i);

		defaults->defaultHealth[i] = skillHealth;
		currentSlope = skillHealth - lastValue;
	}

	for(unsigned int i = PARAM_COUNT;i < 9;i++)
	{
		defaults->defaultHealth[i] = defaults->defaultHealth[i-1] + currentSlope;
	}
}

HANDLE_PROPERTY(height)
{
	INT_PARAM(height, 0);

	// Dummy property so using it doesn't print an error/warning.
	// For forwards compatibility solid actors should have a height of 64
}

HANDLE_PROPERTY(icon)
{
	STRING_PARAM(icon, 0);

	((AInventory *)defaults)->icon = TexMan.CheckForTexture(icon, FTexture::TEX_MiscPatch);
}

HANDLE_PROPERTY(interhubamount)
{
	INT_PARAM(amt, 0);
	((AInventory *)defaults)->interhubamount = amt;
}

HANDLE_PROPERTY(maxamount)
{
	INT_PARAM(maxamt, 0);
	((AInventory *)defaults)->maxamount = maxamt;
}

HANDLE_PROPERTY(maxhealth)
{
	INT_PARAM(maxhealth, 0);
	((APlayerPawn *)defaults)->maxhealth = maxhealth;
}

HANDLE_PROPERTY(missilechance)
{
	INT_PARAM(chance, 0);

	if(chance > 0xFFFF)
		chance = 0xFFFF;
	else if(chance < 0)
		chance = 0;

	defaults->missilechance = chance;
}

HANDLE_PROPERTY(pickupsound)
{
	STRING_PARAM(snd, 0);
	((AInventory *)defaults)->pickupsound = snd;
}

HANDLE_PROPERTY(points)
{
	INT_PARAM(pts, 0);
	defaults->points = pts;
}

HANDLE_PROPERTY(radius)
{
	INT_PARAM(radius, 0);
	defaults->radius = radius*FRACUNIT/64;
}

HANDLE_PROPERTY(secretdeathsound)
{
	STRING_PARAM(snd, 0);
	cls->Meta.SetMetaString(AMETA_SecretDeathSound, snd);
}

HANDLE_PROPERTY(seesound)
{
	STRING_PARAM(snd, 0);
	defaults->seesound = snd;
}

HANDLE_PROPERTY(selectionorder)
{
	INT_PARAM(order, 0);
	cls->Meta.SetMetaInt(AWMETA_SelectionOrder, order);
}

HANDLE_PROPERTY(sighttime)
{
	INT_PARAM(time, 0);
	defaults->sighttime = time;

	if(PARAM_COUNT == 2)
	{
		INT_PARAM(rnd, 1);
		defaults->sightrandom = rnd <= 255 ? rnd : 0;
	}
	else
		defaults->sightrandom = 0;
}

HANDLE_PROPERTY(slotnumber)
{
	INT_PARAM(slot, 0);
	if(slot < 0 || slot > 9)
		I_Error("Valid slots range from 0 and 9.");

	cls->Meta.SetMetaInt(AWMETA_SlotNumber, slot);
}

HANDLE_PROPERTY(slotpriority)
{
	FLOAT_PARAM(priority, 0);

	cls->Meta.SetMetaFixed(AWMETA_SlotPriority, priority*FRACUNIT);
}

HANDLE_PROPERTY(speed)
{
	// Speed = units per 2 tics (1/35 of second)
	FLOAT_PARAM(speed, 0);
	defaults->speed = int32_t(speed*FRACUNIT)/128;

	if(PARAM_COUNT == 2)
	{
		FLOAT_PARAM(runspeed, 1);
		defaults->runspeed = int32_t(runspeed*FRACUNIT)/128;
	}
	else
		defaults->runspeed = defaults->speed;
}

HANDLE_PROPERTY(startitem)
{
	// NOTE: This property is not inherited.
	STRING_PARAM(item, 0);

	APlayerPawn *def = (APlayerPawn *)defaults;

	if(cls->Meta.GetMetaInt(APMETA_StartInventory, -1) == -1)
		cls->Meta.SetMetaInt(APMETA_StartInventory, APlayerPawn::startInventory.Push(new AActor::DropList()));

	AActor::DropItem drop;
	drop.className = item;
	drop.amount = 1;
	drop.probabilty = 255;

	if(PARAM_COUNT > 1)
	{
		INT_PARAM(amt, 1);
		drop.amount = amt;
	}

	APlayerPawn::startInventory[cls->Meta.GetMetaInt(APMETA_StartInventory)]->Push(drop);
}

HANDLE_PROPERTY(weaponslot)
{
	INT_PARAM(slot, 0);
	if(slot < 0 || slot > 9)
		I_Error("Valid slots range from 0 and 9.");

	STRING_PARAM(firstWeapon, 1);
	FString weaponsList = firstWeapon;
	for(unsigned int i = 2;i < PARAM_COUNT;++i)
	{
		STRING_PARAM(weapon, i);
		weaponsList << ' ' << weapon;
	}

	cls->Meta.SetMetaString(APMETA_Slot0 + slot, weaponsList);
}

HANDLE_PROPERTY(yadjust)
{
	FLOAT_PARAM(adjust, 0);

	AWeapon *def = (AWeapon *)defaults;
	def->yadjust = adjust*FRACUNIT;
}

////////////////////////////////////////////////////////////////////////////////

HANDLE_PROPERTY(MONSTER)
{
	defaults->flags |= FL_ISMONSTER|FL_SHOOTABLE|FL_CANUSEWALLS|FL_COUNTKILL|FL_SOLID;
}

HANDLE_PROPERTY(PROJECTILE)
{
	defaults->flags |= FL_MISSILE;
}

////////////////////////////////////////////////////////////////////////////////

#define DEFINE_PROP_PREFIX(name, class, prefix, params) { A##class::__StaticClass, #prefix, #name, #params, __Handler_##name }
#define DEFINE_PROP(name, class, params) DEFINE_PROP_PREFIX(name, class, class, params)

extern const PropDef properties[] =
{
	DEFINE_PROP(ammogive1, Weapon, I),
	DEFINE_PROP(ammotype1, Weapon, S),
	DEFINE_PROP(ammouse1, Weapon, I),
	DEFINE_PROP(amount, Inventory, I),
	DEFINE_PROP(attacksound, Actor, S),
	DEFINE_PROP(damage, Actor, I),
	DEFINE_PROP(deathsound, Actor, S),
	DEFINE_PROP_PREFIX(displayname, PlayerPawn, Player, S),
	DEFINE_PROP(dropitem, Actor, S_II),
	DEFINE_PROP(health, Actor, I_IIIIIIII),
	DEFINE_PROP(height, Actor, I),
	DEFINE_PROP(icon, Inventory, S),
	DEFINE_PROP(interhubamount, Inventory, I),
	DEFINE_PROP(maxamount, Inventory, I),
	DEFINE_PROP_PREFIX(maxhealth, PlayerPawn, Player, I),
	DEFINE_PROP(missilechance, Actor, I),
	DEFINE_PROP(MONSTER, Actor,),
	DEFINE_PROP(pickupsound, Inventory, S),
	DEFINE_PROP(points, Actor, I),
	DEFINE_PROP(PROJECTILE, Actor,),
	DEFINE_PROP(radius, Actor, I),
	DEFINE_PROP(secretdeathsound, Actor, S),
	DEFINE_PROP(seesound, Actor, S),
	DEFINE_PROP(selectionorder, Weapon, I),
	DEFINE_PROP(sighttime, Actor, I_I),
	DEFINE_PROP(slotnumber, Weapon, I),
	DEFINE_PROP(slotpriority, Weapon, F),
	DEFINE_PROP(speed, Actor, F_F),
	DEFINE_PROP_PREFIX(startitem, PlayerPawn, Player, S_I),
	DEFINE_PROP_PREFIX(weaponslot, PlayerPawn, Player, IS_SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS),
	DEFINE_PROP(yadjust, Weapon, F),

	{ NULL, NULL, NULL, NULL }
};
