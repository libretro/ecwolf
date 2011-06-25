/*
** thinker.h
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
** A thinker is an object which is to be informed of tick events.
**
*/

#ifndef __THINKER_H__
#define __THINKER_H__

#include "wl_def.h"
#include "name.h"
#include "linkedlist.h"

class Thinker;
class ThinkerList;

#define DECLARE_THINKER(type) \
	public: \
		static Thinker::__ThinkerInfo __StaticThinkerType; \
	protected: \
		virtual Thinker::__ThinkerInfo &__ThinkerType() const { return __StaticThinkerType; }
#define IMPLEMENT_THINKER(type) \
	Thinker::__ThinkerInfo type::__StaticThinkerType(#type);

class Thinker
{
	public:
		typedef const FName __ThinkerInfo;

		Thinker();
		virtual ~Thinker();

		virtual void	Destroy();
		template<class T>
		bool			IsThinkerType() { __ThinkerType() == T::__StaticThinkerType; }
		virtual void	Tick()=0;

	protected:
		// Use DECLARE_THINKER to implement
		virtual __ThinkerInfo &__ThinkerType() const=0;

	private:
		friend class ThinkerList;

		LinkedList<Thinker *>::Node	*thinkerRef;
};

extern class ThinkerList
{
	public:
		ThinkerList();
		~ThinkerList();

		void	DestroyAll();
		void	Tick();
	protected:
		void	CleanThinkers();

		friend class Thinker;
		void	Register(Thinker *thinker);
		void	Deregister(Thinker *thinker);
		void	MarkForCollection(Thinker *thinker);

	private:
		LinkedList<Thinker *>	thinkers;
		LinkedList<Thinker *>	toDestroy;
} thinkerList;

#endif
