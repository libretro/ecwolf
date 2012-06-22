/*
** actor.h
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

#ifndef __ACTOR_H__
#define __ACTOR_H__

#include "wl_def.h"
#include "gamemap.h"
#include "linkedlist.h"
#include "name.h"
#include "dobject.h"

#define DECLARE_CLASS(name, parent) \
	friend class ClassDef; \
	private: \
		typedef parent Super; \
		typedef name ThisClass; \
	protected: \
		name(const ClassDef *classType) : parent(classType) {} \
		virtual const ClassDef *__StaticType() const { return __StaticClass; } \
		static DObject *__InPlaceConstructor(const ClassDef *classType, void *mem); \
	public: \
		static const ClassDef *__StaticClass; \
		static const size_t __PointerOffsets[];
#define DECLARE_NATIVE_CLASS(name, parent) DECLARE_CLASS(A##name, A##parent)
#define HAS_OBJECT_POINTERS
#define __IMPCLS_ABSTRACT(cls, name) \
	const ClassDef *cls::__StaticClass = ClassDef::DeclareNativeClass<cls>(name, Super::__StaticClass);
#define __IMPCLS(cls, name) \
	__IMPCLS_ABSTRACT(cls, name) \
	DObject *cls::__InPlaceConstructor(const ClassDef *classType, void *mem) { return new ((EInPlace *) mem) cls(classType); }
#define IMPLEMENT_ABSTRACT_CLASS(cls) \
	__IMPCLS_ABSTRACT(cls, #cls) \
	DObject *cls::__InPlaceConstructor(const ClassDef *classType, void *mem) { return Super::__InPlaceConstructor(classType, mem); } \
	const size_t cls::__PointerOffsets[] = { ~(size_t)0 };
#define IMPLEMENT_INTERNAL_CLASS(cls) \
	__IMPCLS(cls, #cls) \
	const size_t cls::__PointerOffsets[] = { ~(size_t)0 };
#define IMPLEMENT_INTERNAL_POINTY_CLASS(cls) \
	__IMPCLS(cls, #cls) \
	const size_t cls::__PointerOffsets[] = {
#define IMPLEMENT_CLASS(name) \
	__IMPCLS(A##name, #name) \
	const size_t A##name::__PointerOffsets[] = { ~(size_t)0 };
#define IMPLEMENT_POINTY_CLASS(name) \
	__IMPCLS(A##name, #name) \
	const size_t A##name::__PointerOffsets[] = {
// Similar to typeoffsetof, but doesn't cast to int.
#define DECLARE_POINTER(ptr) \
	(size_t)&((ThisClass*)1)->ptr - 1,
#define END_POINTERS ~(size_t)0 };
#define NATIVE_CLASS(name) A##name::__StaticClass

class AActor;
class CallArguments;
class ExpressionNode;

typedef uint32_t flagstype_t;
typedef void (*ActionPtr)(AActor *, const CallArguments &args);

class Frame
{
	public:
		~Frame();

		union
		{
			char	sprite[4];
			uint32_t isprite;
		};
		uint8_t		frame;
		int			duration;
		bool		fullbright;
		class ActionCall
		{
			public:
				ActionPtr		pointer;
				CallArguments	*args;

				void operator() (AActor *self) const;
		} action, thinker;
		const Frame	*next;
		unsigned int	index;

		unsigned int	spriteInf;

		bool	freeActionArgs;
};

class player_t;
class AActorProxy;
class ClassDef;
class AInventory;
class AActor : public DObject
{
	DECLARE_CLASS(AActor, DObject)

	public:
		struct DropItem
		{
			public:
				FName			className;
				unsigned int	amount;
				uint8_t			probabilty;
		};
		typedef LinkedList<DropItem> DropList;

		virtual ~AActor();

		void			AddInventory(AInventory *item);
		Thinker			*GetThinker() const;
		virtual void	Destroy();
		void			Die();
		void			EnterZone(const MapZone *zone);
		AInventory		*FindInventory(const ClassDef *cls) const;
		const Frame		*FindState(const FName &name) const;
		const AActor	*GetDefault() const;
		const MapZone	*GetZone() const { return soundZone; }
		void			RemoveFromWorld();
		void			RemoveInventory(AInventory *item);
		void			SetState(const Frame *state, bool notic=false);
		static AActor	*Spawn(const ClassDef *type, fixed x, fixed y, fixed z);
		virtual void	Tick();
		virtual void	Touch(AActor *toucher) {}

		// Basic properties from objtype
		flagstype_t flags;

		int32_t	distance; // if negative, wait for that door to open
		dirtype	dir;

		fixed	x, y;
		fixed	velx, vely;
		word	tilex, tiley;

		angle_t	angle;
		int32_t	health;
		short	defaultHealth[9];
		int32_t	speed, runspeed;
		int		points;
		fixed	radius;
		ExpressionNode *damage;

		short       ticcount;
		const Frame	*state;
		unsigned int sprite;

		short       viewx;
		word        viewheight;
		fixed       transx,transy;      // in global coord

		uint16_t	sighttime;
		uint8_t		sightrandom;
		uint16_t	missilechance;
		FNameNoInit	attacksound, deathsound, seesound;

		const Frame *SpawnState, *SeeState, *PathState, *PainState, *MeleeState, *MissileState, *DeathState;
		short       temp1,hidden;

		player_t	*player;	// Only valid with APlayerPawn

		AInventory	*inventory;

		DropList	*dropitems;

		typedef LinkedList<AActor *>::Node Iterator;
		static LinkedList<AActor *>	actors;
		LinkedList<AActor *>::Node	*actorRef;
		static Iterator *GetIterator() { return actors.Head(); }
	protected:
		friend class AActorProxy;
		void	Init();

		const MapZone	*soundZone;
		AActorProxy		*thinker;
};

#endif
