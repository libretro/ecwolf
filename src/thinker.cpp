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

struct ThinkerRef
{
	ThinkerRef(ThinkerRef *prev) : next(NULL), prev(prev)
	{
		if(prev != NULL)
			prev->next = this;
	}
	~ThinkerRef()
	{
		if(next)
			next->prev = prev;
		if(prev)
			prev->next = next;
	}

	Thinker *thinker;
	ThinkerRef *next;
	ThinkerRef *prev;
};

ThinkerList thinkerList;

ThinkerList::ThinkerList()
{
	head = new ThinkerRef(NULL);
	head->thinker = NULL;
	tail = head;

	destroyHead = new ThinkerRef(NULL);
	destroyHead->thinker = NULL;
	destroyTail = destroyHead;
}

ThinkerList::~ThinkerList()
{
	DestroyAll();

	// Get rid of the head ref, which isn't taken care of since it's NULL
	delete destroyHead;
	delete head;
}

void ThinkerList::CleanThinkers()
{
	ThinkerRef *pos = destroyHead->next;
	while(pos)
	{
		delete pos->thinker;
		if(pos->next != NULL)
		{
			pos = pos->next;
			delete pos->prev;
		}
		else
		{
			delete pos;
			break;
		}
	}
	destroyTail = destroyHead;
	assert(destroyHead->next == NULL);
}

void ThinkerList::DestroyAll()
{
	// Head is always pointing to NULL so we skip it.
	ThinkerRef *pos = head->next;
	while(pos)
	{
		if(pos->next == NULL)
		{
			// Now that we have no more thinker refs we shall delete the last
			// one now.
			pos->thinker->Destroy();
			break;
		}
		else
		{
			// Destroy the previous thinker, which should in turn delete the ref
			pos->prev->thinker->Destroy();
			pos = pos->next;
		}
	}
	CleanThinkers();
}

void ThinkerList::Tick()
{
	CleanThinkers();

	// Head is always pointing to NULL so we skip it.
	ThinkerRef *pos = head;
	while((pos = pos->next) != NULL)
	{
		pos->thinker->Tick();
	}
}

void ThinkerList::Register(Thinker *thinker)
{
	tail = new ThinkerRef(tail);
	tail->thinker = thinker;

	thinker->thinkerRef = tail;
}

void ThinkerList::Deregister(Thinker *thinker)
{
	if(thinker->thinkerRef == tail)
		tail = thinker->thinkerRef->prev;
	delete thinker->thinkerRef;
}

void ThinkerList::MarkForCollection(Thinker *thinker)
{
	destroyTail = new ThinkerRef(destroyTail);
	destroyTail->thinker = thinker;
}

////////////////////////////////////////////////////////////////////////////////

Thinker::Thinker()
{
	thinkerList.Register(this);
}

Thinker::~Thinker()
{
	thinkerList.Deregister(this);
}

void Thinker::Destroy()
{
	thinkerList.MarkForCollection(this);
}
