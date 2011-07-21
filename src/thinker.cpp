/*
** thinker.cpp
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

#include "thinker.h"
#include "wl_def.h"

ThinkerList *thinkerList;

void DeinitThinkerList()
{
	delete thinkerList;
}

void InitThinkerList()
{
	thinkerList = new ThinkerList();
	atexit(DeinitThinkerList);
}

ThinkerList::ThinkerList()
{
}

ThinkerList::~ThinkerList()
{
	DestroyAll();
}

void ThinkerList::CleanThinkers()
{
	LinkedList<Thinker *>::Node *iter = toDestroy.Head();
	while(iter)
	{
		delete iter->Item();
		iter = iter->Next();
	}
	toDestroy.Clear();
}

void ThinkerList::DestroyAll()
{
	LinkedList<Thinker *>::Node *iter = thinkers.Head();
	while(iter)
	{
		iter->Item()->Destroy();
		iter = iter->Next();
	}
	CleanThinkers();
}

void ThinkerList::Tick()
{
	CleanThinkers();

	LinkedList<Thinker *>::Node *iter = thinkers.Head();
	while(iter)
	{
		iter->Item()->Tick();
		iter = iter->Next();
	}
}

void ThinkerList::Register(Thinker *thinker)
{
	thinker->thinkerRef = thinkers.Push(thinker);
}

void ThinkerList::Deregister(Thinker *thinker)
{
	thinkers.Remove(thinker->thinkerRef);
}

void ThinkerList::MarkForCollection(Thinker *thinker)
{
	toDestroy.Push(thinker);
}

////////////////////////////////////////////////////////////////////////////////

Thinker::Thinker()
{
	thinkerList->Register(this);
}

Thinker::~Thinker()
{
	thinkerList->Deregister(this);
}

void Thinker::Destroy()
{
	thinkerList->MarkForCollection(this);
}
