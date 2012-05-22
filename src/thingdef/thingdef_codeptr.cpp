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
**
*/

#include "actor.h"
#include "id_sd.h"
#include "m_random.h"
#include "thingdef/thingdef.h"
#include "wl_def.h"
#include "wl_agent.h"
#include "wl_game.h"
#include "wl_play.h"
#include "wl_state.h"

static ActionTable *actionFunctions = NULL;
ActionInfo::ActionInfo(ActionPtr func, const FName &name) : func(func), name(name),
	minArgs(0), maxArgs(0), varArgs(false)
{
	if(actionFunctions == NULL)
		actionFunctions = new ActionTable;
	printf("Adding %s @ %d\n", name.GetChars(), actionFunctions->Size());
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
	for(int i = 1;i < table->Size();++i)
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

bool CheckMeleeRange(AActor *actor1, AActor *actor2);

ACTION_FUNCTION(A_BossDeath)
{
	// TODO: Check if all enemies of same type are dead and then call a defined function.
	playstate = ex_victorious;
}

ACTION_FUNCTION(A_Fall)
{
	self->flags &= ~FL_SOLID;
}

static FRandom pr_cajump("CustomJump");
ACTION_FUNCTION(A_Jump)
{
	ACTION_PARAM_INT(chance, 0);

	if(chance >= 256 || pr_cajump() < chance)
	{
		ACTION_PARAM_STATE(frame, (ACTION_PARAM_COUNT == 2 ? 1 : (1 + pr_cajump() % (ACTION_PARAM_COUNT - 1))), NULL);

		if(frame)
			self->SetState(frame);
	}
}

static FRandom pr_meleeattack("MeleeAccuracy");
ACTION_FUNCTION(A_MeleeAttack)
{
	ACTION_PARAM_INT(damage, 0);
	ACTION_PARAM_INT(accuracy, 1);

	if(CheckMeleeRange(self, players[0].mo))
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

	if(jump && (
		pr_monsterrefire() >= probability ||
		!(self->flags & FL_ATTACKMODE) || !target ||
		target->health <= 0 ||
		!CheckLine(self)
	))
	{
		self->SetState(jump);
	}
}

ACTION_FUNCTION(A_PlaySound)
{
	ACTION_PARAM_STRING(sound, 0);

	PlaySoundLocActor(sound, self);
}
