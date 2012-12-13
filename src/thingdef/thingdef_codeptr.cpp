/*
** thingdef_codeptr.cpp
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
** Some action pointer code may closely resemble ZDoom code since they
** should behave the same.
**
*/

#include "actor.h"
#include "id_ca.h"
#include "id_sd.h"
#include "g_mapinfo.h"
#include "g_shared/a_deathcam.h"
#include "g_shared/a_inventory.h"
#include "lnspec.h"
#include "m_random.h"
#include "thingdef/thingdef.h"
#include "wl_act.h"
#include "wl_def.h"
#include "wl_agent.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_play.h"
#include "wl_state.h"

static ActionTable *actionFunctions = NULL;
ActionInfo::ActionInfo(ActionPtr func, const FName &name) : func(func), name(name),
	minArgs(0), maxArgs(0), varArgs(false)
{
	if(actionFunctions == NULL)
		actionFunctions = new ActionTable;
#if 0
	// Debug code - Show registered action functions
	printf("Adding %s @ %d\n", name.GetChars(), actionFunctions->Size());
#endif
	actionFunctions->Push(this);
}

int FunctionTableComp(const void *f1, const void *f2)
{
	const ActionInfo * const func1 = *((const ActionInfo **)f1);
	const ActionInfo * const func2 = *((const ActionInfo **)f2);
	if(func1->name < func2->name)
		return -1;
	else if(func1->name > func2->name)
		return 1;
	return 0;
}

void InitFunctionTable(ActionTable *table)
{
	if(table == NULL)
		table = actionFunctions;

	qsort(&(*table)[0], table->Size(), sizeof((*table)[0]), FunctionTableComp);
	for(unsigned int i = 1;i < table->Size();++i)
		assert((*table)[i]->name > (*table)[i-1]->name);
}

void ReleaseFunctionTable()
{
	delete actionFunctions;
}

ActionInfo *LookupFunction(const FName &func, const ActionTable *table)
{
	if(table == NULL)
		table = actionFunctions;

	ActionInfo *inf;
	unsigned int max = table->Size()-1;
	unsigned int min = 0;
	unsigned int mid = max/2;
	do
	{
		inf = (*table)[mid];
		if(inf->name == func)
			return inf;

		if(inf->name > func)
			max = mid-1;
		else if(inf->name < func)
			min = mid+1;
		mid = (max+min)/2;
	}
	while(max >= min && max < table->Size());
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

ACTION_FUNCTION(A_CallSpecial)
{
	ACTION_PARAM_INT(special, 0);
	ACTION_PARAM_INT(arg1, 1);
	ACTION_PARAM_INT(arg2, 2);
	ACTION_PARAM_INT(arg3, 3);
	ACTION_PARAM_INT(arg4, 4);
	ACTION_PARAM_INT(arg5, 5);

	int specialArgs[5] = {arg1, arg2, arg3, arg4, arg5};

	Specials::LineSpecialFunction function = Specials::LookupFunction(static_cast<Specials::LineSpecials>(special));
	function(map->GetSpot(self->tilex, self->tiley, 0), specialArgs, MapTrigger::East, self);
}

////////////////////////////////////////////////////////////////////////////////

extern FRandom pr_chase;
ACTION_FUNCTION(A_ActiveSound)
{
	ACTION_PARAM_INT(chance, 0);

	// If chance == 3 this has the same chance as A_Chase. Useful for giving
	// wolfenstein style monsters activesounds without making it 8x as likely
	if(chance >= 256 || pr_chase() < chance)
		PlaySoundLocActor(self->activesound, self);
}

ACTION_FUNCTION(A_BossDeath)
{
	// TODO: Check if all enemies of same type are dead and then call a defined function.
	bool deathcamRun = false;
	bool alldead = true;
	AActor::Iterator *iter = AActor::GetIterator();
	while(iter)
	{
		AActor * const other = iter->Item();

		if(other != self)
		{
			if(other->GetClass() == NATIVE_CLASS(DeathCam))
				deathcamRun = true;
			else if(other->GetClass() == self->GetClass())
			{
				if(other->health > 0)
				{
					alldead = false;
					break;
				}
			}
		}
		iter = iter->Next();
	}

	if(!alldead)
		return;

	if(levelInfo->DeathCam && !deathcamRun)
	{
		ADeathCam *dc = (ADeathCam*)AActor::Spawn(NATIVE_CLASS(DeathCam), 0, 0, 0, true);
		dc->SetupDeathCam(self, players[0].mo);
	}
	else
	{
		for(unsigned int i = 0;i < levelInfo->SpecialActions.Size();++i)
		{
			const LevelInfo::SpecialAction &action = levelInfo->SpecialActions[i];
			if(action.Class == self->GetClass())
			{
				const Specials::LineSpecials special = static_cast<Specials::LineSpecials>(action.Special);
				Specials::LineSpecialFunction function = Specials::LookupFunction(special);
				function(map->GetSpot(self->tilex, self->tiley, 0), action.Args, MapTrigger::East, self);
			}
		}
	}

	if(deathcamRun && playstate == ex_stillplaying)
	{
		// Return the camera to the player if we're still going
		players[0].camera = players[0].mo;
	}
}

ACTION_FUNCTION(A_FaceTarget)
{
	ACTION_PARAM_DOUBLE(max_turn, 0);
	ACTION_PARAM_DOUBLE(max_pitch, 1);

	A_Face(self, players[0].mo, angle_t(max_turn*ANGLE_45/45));
}

ACTION_FUNCTION(A_Fall)
{
	self->flags &= ~FL_SOLID;
}

ACTION_FUNCTION(A_GiveExtraMan)
{
	ACTION_PARAM_INT(amount, 0);

	GiveExtraMan(amount);
}

ACTION_FUNCTION(A_GiveInventory)
{
	ACTION_PARAM_STRING(className, 0);
	ACTION_PARAM_INT(amount, 1);

	const ClassDef *cls = ClassDef::FindClass(className);

	if(amount == 0)
		amount = 1;

	if(cls && cls->IsDescendantOf(NATIVE_CLASS(Inventory)))
	{
		AInventory *inv = (AInventory *)AActor::Spawn(cls, 0, 0, 0, true);
		if(inv->IsKindOf(NATIVE_CLASS(Health)))
			inv->amount *= amount;
		else
			inv->amount = amount;

		inv->RemoveFromWorld();
		if(!inv->TryPickup(self))
			inv->Destroy();
	}
}

#define STATE_JUMP(frame) DoStateJump(frame, self, caller, args)
static void DoStateJump(const Frame *frame, AActor *self, const Frame * const caller, const CallArguments &args)
{
	if(!frame)
		return;

	if(self->player)
	{
		if(self->player->psprite.frame == caller)
		{
			self->player->SetPSprite(frame);
			return;
		}
	}

	self->SetState(frame);
}

static FRandom pr_cajump("CustomJump");
ACTION_FUNCTION(A_Jump)
{
	ACTION_PARAM_INT(chance, 0);

	if(chance >= 256 || pr_cajump() < chance)
	{
		ACTION_PARAM_STATE(frame, (ACTION_PARAM_COUNT == 2 ? 1 : (1 + pr_cajump() % (ACTION_PARAM_COUNT - 1))), NULL);

		STATE_JUMP(frame);
	}
}

ACTION_FUNCTION(A_JumpIf)
{
	ACTION_PARAM_BOOL(expr, 0);
	ACTION_PARAM_STATE(frame, 1, NULL);

	if(expr)
		STATE_JUMP(frame);
}

ACTION_FUNCTION(A_JumpIfCloser)
{
	ACTION_PARAM_DOUBLE(distance, 0);
	ACTION_PARAM_STATE(frame, 1, NULL);

	AActor *check;
	if(self->player)
		check = self->player->FindTarget();
	else
		check = players[0].mo;

	// << 6 - Adjusts to Doom scale
	if(check && P_AproxDistance((self->x-check->x)<<6, (self->y-check->y)<<6) < (fixed)(distance*FRACUNIT))
	{
		STATE_JUMP(frame);
	}
}

ACTION_FUNCTION(A_JumpIfInventory)
{
	ACTION_PARAM_STRING(className, 0);
	ACTION_PARAM_INT(amount, 1);
	ACTION_PARAM_STATE(frame, 2, NULL);

	const ClassDef *cls = ClassDef::FindClass(className);
	AInventory *inv = self->FindInventory(cls);

	if(!inv)
		return;

	// Amount of 0 means check if the amount is the maxamount.
	// Otherwise check if we have at least that amount.
	if((amount == 0 && inv->amount == inv->maxamount) ||
		inv->amount >= static_cast<unsigned int>(amount))
	{
		STATE_JUMP(frame);
	}
}

static FRandom pr_meleeattack("MeleeAccuracy");
ACTION_FUNCTION(A_MeleeAttack)
{
	ACTION_PARAM_INT(damage, 0);
	ACTION_PARAM_INT(accuracy, 1);

	A_Face(self, players[0].mo);
	if(CheckMeleeRange(self, players[0].mo, self->speed))
	{
		if(pr_meleeattack() < accuracy*255)
			TakeDamage(damage, self);
	}
}

static FRandom pr_monsterrefire("MonsterRefire");
ACTION_FUNCTION(A_MonsterRefire)
{
	ACTION_PARAM_INT(probability, 0);
	ACTION_PARAM_STATE(jump, 1, NULL);

	AActor *target = players[0].mo;
	A_Face(self, target);

	if(pr_monsterrefire() < probability)
		return;

	if(jump && (
		!(self->flags & FL_ATTACKMODE) ||
		!target ||
		target->health <= 0 ||
		!CheckLine(self)
	))
	{
		STATE_JUMP(jump);
	}
}

ACTION_FUNCTION(A_Pain)
{
	PlaySoundLocActor(self->painsound, self);
}

ACTION_FUNCTION(A_PlaySound)
{
	ACTION_PARAM_STRING(sound, 0);

	PlaySoundLocActor(sound, self);
}

ACTION_FUNCTION(A_SpawnItem)
{
	ACTION_PARAM_STRING(className, 0);
	ACTION_PARAM_DOUBLE(distance, 1);
	ACTION_PARAM_DOUBLE(zheight, 2);

	const ClassDef *cls = ClassDef::FindClass(className);
	if(cls == NULL)
		return;

	AActor *newobj = AActor::Spawn(cls,
		self->x + fixed(distance*finecosine[self->angle>>ANGLETOFINESHIFT])/64,
		self->y - fixed(distance*finesine[self->angle>>ANGLETOFINESHIFT])/64,
		0, true);
}

static FRandom pr_spawnitemex("SpawnItemEx");
ACTION_FUNCTION(A_SpawnItemEx)
{
	enum
	{
		SXF_TRANSFERPOINTERS = 0x1
	};

	ACTION_PARAM_STRING(className, 0);
	ACTION_PARAM_DOUBLE(xoffset, 1);
	ACTION_PARAM_DOUBLE(yoffset, 2);
	ACTION_PARAM_DOUBLE(zoffset, 3);
	ACTION_PARAM_DOUBLE(xvel, 4);
	ACTION_PARAM_DOUBLE(yvel, 5);
	ACTION_PARAM_DOUBLE(zvel, 6);
	ACTION_PARAM_DOUBLE(angle, 7);
	ACTION_PARAM_INT(flags, 8);
	ACTION_PARAM_INT(chance, 9);

	if(chance > 0 && pr_spawnitemex() < chance)
		return;

	const ClassDef *cls = ClassDef::FindClass(className);
	if(cls == NULL)
		return;

	angle_t ang = self->angle>>ANGLETOFINESHIFT;

	fixed x = self->x + fixed(xoffset*finecosine[ang])/64 + fixed(yoffset*finesine[ang])/64;
	fixed y = self->y - fixed(xoffset*finesine[ang])/64 + fixed(yoffset*finecosine[ang])/64;
	angle = angle_t((angle*ANGLE_45)/45) + self->angle;

	AActor *newobj = AActor::Spawn(cls, x, y, 0, true);

	if(flags & SXF_TRANSFERPOINTERS)
	{
		newobj->flags |= self->flags&(FL_ATTACKMODE|FL_FIRSTATTACK);
		newobj->flags &= ~(~self->flags&(FL_PATHING));
		if(newobj->flags & FL_ATTACKMODE)
			newobj->speed = newobj->runspeed;
	}

	newobj->angle = angle;

	//We divide by 128 here since Wolf is 70hz instead of 35.
	newobj->velx = (fixed(xvel*finecosine[ang]) + fixed(yvel*finesine[ang]))/128;
	newobj->vely = (-fixed(xvel*finesine[ang]) + fixed(yvel*finecosine[ang]))/128;
}

ACTION_FUNCTION(A_Stop)
{
	self->velx = 0;
	self->vely = 0;
	self->dir = nodir;
}

ACTION_FUNCTION(A_TakeInventory)
{
	ACTION_PARAM_STRING(className, 0);
	ACTION_PARAM_INT(amount, 1);

	const ClassDef *cls = ClassDef::FindClass(className);
	AInventory *inv = self->FindInventory(cls);
	if(inv)
	{
		// Taking an amount of 0 mean take all
		if(amount == 0 || amount >= static_cast<int>(inv->amount))
			inv->Destroy();
		else
			inv->amount -= amount;
	}
}
