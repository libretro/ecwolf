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

#include "thingdef.h"

static TArray<ActionInfo *> *actionFunctions = NULL;
ActionInfo::ActionInfo(ActionPtr func, const FName &name) : func(func), name(name)
{
	if(actionFunctions == NULL)
		actionFunctions = new TArray<ActionInfo *>;
	printf("Adding %s\n", name.GetChars());
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

void InitFunctionTable()
{
	qsort(&(*actionFunctions)[0], actionFunctions->Size(), sizeof((*actionFunctions)[0]), FunctionTableComp);

	/*for(unsigned int i = 0;i < actionFunctions->Size();++i)
	{
		printf("%s\t%d\n", actionFunctions->operator[](i)->name.GetChars(), actionFunctions->operator[](i)->name.GetIndex());
	}*/
}

void ReleaseFunctionTable()
{
	delete actionFunctions;
}

ActionPtr FindFunction(const FName &func)
{
	ActionInfo *inf;
	unsigned int max = actionFunctions->Size()-1;
	unsigned int min = 0;
	unsigned int mid = max/2;
	do
	{
		inf = (*actionFunctions)[mid];
		if(inf->name == func)
			return inf->func;

		if(inf->name > func)
			max = mid-1;
		else if(inf->name < func)
			min = mid+1;
		mid = (max+min)/2;
	}
	while(max >= min);
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

ACTION_FUNCTION(A_Fall)
{
	self->flags &= ~FL_SOLID;
}
