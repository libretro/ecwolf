/*
** dobject.h
**
**---------------------------------------------------------------------------
** Copyright 1998-2008 Randy Heit
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
*/

#ifndef __DOBJECT_H__
#define __DOBJECT_H__

#include <stdlib.h>
#include "wl_def.h"
#include "m_alloc.h"

class ClassDef;

class FArchive;
class DObject;
class AActor;

enum EInPlace { EC_InPlace };


enum EObjectFlags
{
	// GC flags
	OF_EuthanizeMe		= 1 << 5,		// Object wants to die
	OF_YesReallyDelete	= 1 << 7,		// Object is being deleted outside the collector, and this is okay, so don't print a warning

	// Other flags
	OF_JustSpawned		= 1 << 8,		// Thinker was spawned this tic
	OF_SerialSuccess	= 1 << 9,		// For debugging Serialize() calls
};

template<class T> class TObjPtr;

namespace GC
{
	// Number of bytes currently allocated through M_Malloc/M_Realloc.
	extern size_t AllocBytes;

	// Amount of memory to allocate before triggering a collection.
	extern size_t Threshold;

	// List of every object.
	extern DObject *Root;

	// Size of GC pause.
	extern int Pause;

	// Does a complete collection.
	void FullGC();

	// Handles a read barrier.
	template<class T> inline T *ReadBarrier(T *&obj)
	{
		if (obj == NULL || !(obj->ObjectFlags & OF_EuthanizeMe))
		{
			return obj;
		}
		return obj = NULL;
	}

	// Check if it's time to collect, and do a collection step if it is.
	static inline void CheckGC()
	{
		if (AllocBytes >= Threshold)
			FullGC();
	}
}

// A template class to help with handling read barriers. It does not
// handle write barriers, because those can be handled more efficiently
// with knowledge of the object that holds the pointer.
template<class T>
class TObjPtr
{
	union
	{
		T *p;
		DObject *o;
	};
public:
	TObjPtr() throw()
	{
	}
	TObjPtr(T *q) throw()
		: p(q)
	{
	}
	TObjPtr(const TObjPtr<T> &q) throw()
		: p(q.p)
	{
	}
	T *operator=(T *q) throw()
	{
		return p = q;
		// The caller must now perform a write barrier.
	}
	operator T*() throw()
	{
		return GC::ReadBarrier(p);
	}
	T &operator*()
	{
		T *q = GC::ReadBarrier(p);
		assert(q != NULL);
		return *q;
	}
	T **operator&() throw()
	{
		// Does not perform a read barrier. The only real use for this is with
		// the DECLARE_POINTER macro, where a read barrier would be a very bad
		// thing.
		return &p;
	}
	T *operator->() throw()
	{
		return GC::ReadBarrier(p);
	}
	bool operator<(T *u) throw()
	{
		return GC::ReadBarrier(p) < u;
	}
	bool operator<=(T *u) throw()
	{
		return GC::ReadBarrier(p) <= u;
	}
	bool operator>(T *u) throw()
	{
		return GC::ReadBarrier(p) > u;
	}
	bool operator>=(T *u) throw()
	{
		return GC::ReadBarrier(p) >= u;
	}
	bool operator!=(T *u) throw()
	{
		return GC::ReadBarrier(p) != u;
	}
	bool operator==(T *u) throw()
	{
		return GC::ReadBarrier(p) == u;
	}

	template<class U> friend inline FArchive &operator<<(FArchive &arc, TObjPtr<U> &o);
	friend class DObject;
};

template<class T> inline FArchive &operator<<(FArchive &arc, TObjPtr<T> &o)
{
	return arc << o.p;
}

// Use barrier_cast instead of static_cast when you need to cast
// the contents of a TObjPtr to a related type.
template<class T,class U> inline T barrier_cast(TObjPtr<U> &o)
{
	return static_cast<T>(static_cast<U *>(o));
}

#define RUNTIME_CLASS(cls) cls::__StaticClass

class DObject
{
friend class ClassDef;
public:
	static const ClassDef *__StaticClass;
	static const size_t __PointerOffsets[];
protected:
	virtual const ClassDef *__StaticType() const { return __StaticClass; }
	virtual size_t __GetSize() const { return sizeof(DObject); }
	static DObject *__InPlaceConstructor(const ClassDef *classType, void *mem);
	typedef DObject ThisClass;

	// Per-instance variables. There are four.
private:
	const ClassDef *Class;				// This object's type
public:
	DObject *ObjNext;			// Keep track of all allocated objects
	uint32_t ObjectFlags;			// Flags for this object

public:
	DObject ();
	DObject (const ClassDef *inClass);
	virtual ~DObject ();

	inline bool IsSameKindOf (const ClassDef *base, const ClassDef *other) const;
	inline bool IsKindOf (const ClassDef *base) const;
	inline bool IsA (const ClassDef *type) const;

	void SerializeUserVars(FArchive &arc);
	virtual void Serialize (FArchive &arc);

	// For catching Serialize functions in derived classes
	// that don't call their base class.
	void CheckIfSerialized () const;

	virtual void Destroy ();

	// If you need to replace one object with another and want to
	// change any pointers from the old object to the new object,
	// use this method.
	virtual size_t PointerSubstitution (DObject *old, DObject *notOld);
	static size_t StaticPointerSubstitution (DObject *old, DObject *notOld);
	
	const ClassDef *GetClass() const
	{
		if (Class == NULL)
		{
			// Save a little time the next time somebody wants this object's type
			// by recording it now.
			const_cast<DObject *>(this)->Class = __StaticType();
		}
		return Class;
	}

	void SetClass (ClassDef *inClass)
	{
		Class = inClass;
	}

	void *operator new(size_t len)
	{
		return M_Malloc(len);
	}

	void operator delete (void *mem)
	{
		M_Free(mem);
	}

protected:
	// This form of placement new and delete is for use *only* by ClassDef's
	// CreateNew() method. Do not use them for some other purpose.
	void *operator new(size_t, EInPlace *mem)
	{
		return (void *)mem;
	}

	void operator delete (void *mem, EInPlace *)
	{
		M_Free (mem);
	}

	virtual void	Init() {}
};

#endif //__DOBJECT_H__
