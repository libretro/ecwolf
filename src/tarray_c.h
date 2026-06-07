/*
** tarray_c.h
**
** Macro-monomorphized C array, layout- and semantics-compatible with the POD
** specializations of TArray<T> (tarray.h). Each TARRAY_C(T) expands to a struct
** with the same {Array, Most, Count} fields and the same growth policy
** (Grow/DoResize) and accessors (Size/operator[]/Push/Resize) as TArray, so a
** TArray<T> of a trivially-copyable T can be replaced by TARRAY_C_TYPE(T) with
** identical generated code. Verified codegen-identical to TArray<int> at -O2.
**
** Intended as a step toward letting leaf translation units compile as C: this
** header is valid C89 and C++ and depends only on the existing M_Malloc/
** M_Realloc/M_Free allocator (m_alloc.h) that TArray itself uses.
**
** Only valid for trivially-copyable element types (no constructors,
** destructors or self-referential pointers); that matches every POD TArray
** instantiation. Non-POD element types must keep using TArray.
*/

#ifndef __TARRAY_C_H__
#define __TARRAY_C_H__

#include <stdlib.h>
#include <string.h>
#include "m_alloc.h"

/* Struct shape, identical field order/types to TArray<T>'s data members. */
#define TARRAY_C_TYPE(T) struct { T *Array; unsigned int Most; unsigned int Count; }

/* Zero-initialize (mirrors TArray's default constructor). */
#define TARRAY_C_INIT(a)      do { (a)->Array = NULL; (a)->Most = 0; (a)->Count = 0; } while(0)

/* Accessors. ARRAY[i] and Size() are bare, exactly like TArray. */
#define TARRAY_C_SIZE(a)      ((a)->Count)
#define TARRAY_C_AT(a,i)      ((a)->Array[(i)])

/*
** Grow/DoResize/Push/Resize. These take the element type T explicitly so the
** void* result of M_Realloc can be cast portably (C++ forbids the implicit
** void* conversion that C allows). T is a compile-time constant, so element
** size still folds and codegen is unchanged from TArray<T>.
*/
#define TARRAY_C_DORESIZE(a, T) \
	do { (a)->Array = (T*)M_Realloc((a)->Array, sizeof(T) * (a)->Most); } while(0)

#define TARRAY_C_GROW(a, T, amount) \
	do { \
		if ((a)->Count + (unsigned int)(amount) > (a)->Most) { \
			unsigned int _ca = (a)->Count + (unsigned int)(amount); \
			unsigned int _cb = (a)->Most = ((a)->Most >= 16) ? (a)->Most + (a)->Most / 2 : 16; \
			(a)->Most = (_ca > _cb ? _ca : _cb); \
			TARRAY_C_DORESIZE(a, T); \
		} \
	} while(0)

#define TARRAY_C_PUSH(a, T, item) \
	do { TARRAY_C_GROW(a, T, 1); (a)->Array[(a)->Count] = (item); (a)->Count++; } while(0)

/* Resize to exactly 'amount' elements. Mirrors TArray<T>::Resize, which for a
** grown range uses placement default-init (::new T, *without* parens) - for a
** trivially-constructible T that leaves the new slots uninitialized, exactly
** as here. The caller is responsible for populating them, as TArray's callers
** are. (Shrinking only runs trivial destructors in TArray, i.e. nothing for
** POD, so we simply lower Count.) */
#define TARRAY_C_RESIZE(a, T, amount) \
	do { \
		unsigned int _amt = (unsigned int)(amount); \
		if ((a)->Count < _amt) \
			TARRAY_C_GROW(a, T, _amt - (a)->Count); \
		(a)->Count = _amt; \
	} while(0)

#define TARRAY_C_CLEAR(a) \
	do { if ((a)->Array != NULL) { M_Free((a)->Array); (a)->Array = NULL; } (a)->Most = 0; (a)->Count = 0; } while(0)

#endif
