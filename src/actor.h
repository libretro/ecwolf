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

#define DECLARE_ABSTRACT_CLASS(name, parent) \
	friend class ClassDef; \
	private: \
		typedef parent Super; \
		typedef name ThisClass; \
	protected: \
		name(const ClassDef *classType) : parent(classType) {} \
		virtual const ClassDef *__StaticType() const { return __StaticClass; } \
	public: \
		static const ClassDef *__StaticClass; \
		static const size_t __PointerOffsets[];
#define DECLARE_CLASS(name, parent) \
	DECLARE_ABSTRACT_CLASS(name, parent) \
	protected: \
		static DObject *__InPlaceConstructor(const ClassDef *classType, void *mem); \
	public:
#define DECLARE_NATIVE_CLASS(name, parent) DECLARE_CLASS(A##name, A##parent)
#define HAS_OBJECT_POINTERS
#define __IMPCLS_ABSTRACT(cls, name) \
	const ClassDef *cls::__StaticClass = ClassDef::DeclareNativeClass<cls>(name, &Super::__StaticClass);
#define __IMPCLS(cls, name) \
	__IMPCLS_ABSTRACT(cls, name) \
	DObject *cls::__InPlaceConstructor(const ClassDef *classType, void *mem) { return new ((EInPlace *) mem) cls(classType); }
#define IMPLEMENT_ABSTRACT_CLASS(cls) \
	__IMPCLS_ABSTRACT(cls, #cls) \
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
typedef void (*ActionPtr)(AActor *, AActor *, const class Frame * const, const CallArguments &);

class Frame
{
	public:
		~Frame();
		int GetTics() const;

		union
		{
			char	sprite[4];
			uint32_t isprite;
		};
		uint8_t		frame;
		int			duration;
		unsigned	randDuration;
		bool		fullbright;
		class ActionCall
		{
			public:
				ActionPtr		pointer;
				CallArguments	*args;

				void operator() (AActor *self, AActor *stateOwner, const Frame * const caller) const;
		} action, thinker;
		const Frame	*next;
		unsigned int	index;

		unsigned int	spriteInf;

		bool	freeActionArgs;
};
FArchive &operator<< (FArchive &arc, const Frame *&frame);

// This class allows us to store pointers into the meta table and ensures that
// the pointers get deleted when the game exits.
template<class T>
class PointerIndexTable
{
public:
	~PointerIndexTable()
	{
		Clear();
	}

	void Clear()
	{
		for(unsigned int i = 0;i < objects.Size();++i)
			delete objects[i];
		objects.Clear();
	}

	unsigned int Push(T *object)
	{
		return objects.Push(object);
	}

	T *operator[] (unsigned int index)
	{
		return objects[index];
	}
private:
	TArray<T*>	objects;
};

enum
{
	AMETA_BASE = 0x12000,

	AMETA_Damage,
	AMETA_DropItems,
	AMETA_SecretDeathSound,
	AMETA_GibHealth
};

enum
{
	SPAWN_AllowReplacement = 1,
	SPAWN_Patrol = 2
};

class player_t;
class AActorProxy;
class ClassDef;
class AInventory;
class AActor : public DObject
{
	DECLARE_CLASS(AActor, DObject)
	HAS_OBJECT_POINTERS

	public:
		struct DropItem
		{
			public:
				FName			className;
				unsigned int	amount;
				uint8_t			probability;
		};
		typedef LinkedList<DropItem> DropList;

		void			AddInventory(AInventory *item);
		virtual void	BeginPlay() {}
		Thinker			*GetThinker();
		virtual void	Destroy();
		virtual void	Die();
		void			EnterZone(const MapZone *zone);
		AInventory		*FindInventory(const ClassDef *cls);
		const Frame		*FindState(const FName &name) const;
		static void		FinishSpawningActors();
		int				GetDamage();
		const AActor	*GetDefault() const;
		DropList		*GetDropList() const;
		const MapZone	*GetZone() const { return soundZone; }
		virtual void	PostBeginPlay() {}
		void			RemoveFromWorld();
		virtual void	RemoveInventory(AInventory *item);
		void			Serialize(FArchive &arc);
		void			SetState(const Frame *state, bool norun=false);
		static AActor	*Spawn(const ClassDef *type, fixed x, fixed y, fixed z, int flags);
		virtual void	Tick();
		virtual void	Touch(AActor *toucher) {}

		static PointerIndexTable<ExpressionNode> damageExpressions;
		static PointerIndexTable<DropList> dropItems;

		// Basic properties from objtype
		flagstype_t flags;

		int32_t	distance; // if negative, wait for that door to open
		dirtype	dir;

#pragma pack(push, 1)
// MSVC and older versions of GCC don't support constant union parts
// We do this instead of just using a regular word since writing to tilex/y
// indicates an error.
#if !defined(_MSC_VER) && (__GNUC__ > 4 || __GNUC_MINOR__ >= 6)
#define COORD_PART const word
#else
#define COORD_PART word
#endif
		union
		{
			fixed x;
#ifdef __BIG_ENDIAN__
			struct { COORD_PART tilex; COORD_PART fracx; };
#else
			struct { COORD_PART fracx; COORD_PART tilex; };
#endif
		};
		union
		{
			fixed y;
#ifdef __BIG_ENDIAN__
			struct { COORD_PART tiley; COORD_PART fracy; };
#else
			struct { COORD_PART fracy; COORD_PART tiley; };
#endif
		};
#pragma pack(pop)
		fixed	velx, vely;

		angle_t	angle;
		angle_t pitch;
		int32_t	health;
		short	defaultHealth[9];
		int32_t	speed, runspeed;
		int		points;
		fixed	radius;

		const Frame		*state;
		unsigned int	sprite;
		fixed			scaleX, scaleY;
		short			ticcount;

		short       viewx;
		word        viewheight;
		fixed       transx,transy;      // in global coord

		uint16_t	sighttime;
		uint8_t		sightrandom;
		fixed		missilefrequency;
		uint16_t	minmissilechance;
		short		movecount; // Emulation of Doom's movecount
		fixed		meleerange;
		uint16_t	painchance;
		FNameNoInit	activesound, attacksound, deathsound, painsound, seesound;

		const Frame *SpawnState, *SeeState, *PathState, *PainState, *MeleeState, *MissileState;
		short       temp1,hidden;
		fixed		killerx,killery; // For deathcam

		player_t	*player;	// Only valid with APlayerPawn

		TObjPtr<AInventory>	inventory;

		typedef LinkedList<AActor *>::Node Iterator;
		static LinkedList<AActor *>	actors;
		LinkedList<AActor *>::Node	*actorRef;
		static Iterator *GetIterator() { return actors.Head(); }
	protected:
		friend class AActorProxy;
		void	Init();

		const MapZone	*soundZone;
		TObjPtr<AActorProxy> thinker;
};

#endif
