/*
** fstring_c.h
**
** A C string type, layout- and semantics-compatible with FString (zstring.h),
** intended as the eventual C replacement for it. Like FString, an FString_C is
** ABI-identical to a single 'char *Chars' that points at the character data;
** the reference count, length and allocation size live in an FStringData_C
** header in the bytes immediately *before* Chars. So FString_C_GetChars() is a
** single pointer load, exactly like FString::operator const char*(), and the
** copy-on-write reference counting matches FString's, keeping copies cheap.
**
** This header only provides the storage and the core operations needed to
** prove the model is codegen-compatible with FString. The full operation set
** (concatenation, formatting, comparisons, case folding, ...) and the call-site
** migration are done in later, separately verified steps.
**
** Valid as C89 and C++.
*/

#ifndef __FSTRING_C_H__
#define __FSTRING_C_H__

#include <stdlib.h>
#include <string.h>

/* Header stored immediately before the character data, identical in layout to
** FStringData (Len, AllocLen, RefCount), so the two models are interchangeable
** at the storage level. */
typedef struct FStringData_C
{
	unsigned int Len;		/* length excluding terminating null */
	unsigned int AllocLen;	/* bytes allocated for the string */
	int RefCount;			/* < 0 means locked for writing */
} FStringData_C;

/* The string itself is just a char* to the data, mirroring FString. */
typedef struct FString_C
{
	char *Chars;
} FString_C;

/* Reach the header sitting just before the character data. */
#define FSTRING_C_DATA(s) (((FStringData_C *)(s)->Chars) - 1)

/* Single pointer load - matches FString::operator const char* / GetChars(). */
#define FSTRING_C_GETCHARS(s) ((const char *)(s)->Chars)
#define FSTRING_C_LEN(s)      (FSTRING_C_DATA(s)->Len)

/* Forward declaration: every operation that assigns Chars releases the prior
** buffer first, so it is also defined here for use before its definition. */
static void FString_C_Release(FString_C *s);

/* Allocate a data block holding room for 'len' chars (plus header and null),
** padded as FStringData::Alloc does, and return a pointer to the char area. */
static char *FString_C_AllocBuffer(size_t len)
{
	size_t total = len + 1 + sizeof(FStringData_C);
	FStringData_C *block;
	total = (total + 7) & ~(size_t)7;	/* pad up, as FStringData::Alloc */
	block = (FStringData_C *)malloc(total);
	if (block == NULL)
		return NULL;
	block->Len = (unsigned int)len;
	block->AllocLen = (unsigned int)(total - sizeof(FStringData_C) - 1);
	block->RefCount = 1;
	return (char *)(block + 1);
}

/* Construct from a C string (FString(const char*)). */
static void FString_C_InitCStr(FString_C *s, const char *src)
{
	size_t len = (src != NULL) ? strlen(src) : 0;
	FString_C_Release(s);
	s->Chars = FString_C_AllocBuffer(len);
	if (s->Chars != NULL)
	{
		if (len != 0)
			memcpy(s->Chars, src, len);
		s->Chars[len] = '\0';
	}
}

/* Share another string's buffer, bumping the reference count (FString's
** AttachToOther / copy constructor). Cheap copy, no allocation. */
static void FString_C_Copy(FString_C *dst, const FString_C *src)
{
	dst->Chars = src->Chars;
	FSTRING_C_DATA(dst)->RefCount++;
}

/* Release this string's hold on its buffer, freeing it at zero refs
** (FStringData::Release + Dealloc). A NULL Chars (never-initialized or already
** released) is treated as empty and is a no-op, so the convention throughout is
** that Chars == NULL means "no buffer held". */
static void FString_C_Release(FString_C *s)
{
	if (s->Chars != NULL)
	{
		FStringData_C *data = FSTRING_C_DATA(s);
		if (--data->RefCount <= 0)
			free(data);
		s->Chars = NULL;
	}
}

/* Zero-initialize to the empty/no-buffer state. A local 'FString_C s;' must be
** FString_C_Init'd (or assigned by an Init, Format or Concat op) before use,
** since unlike FString there is no default constructor. */
#define FSTRING_C_INIT(s) ((s)->Chars = NULL)

/*
** Query and comparison operations. These mirror the inline FString members of
** the same names; each reduces to the same str* call or length test, so they
** are codegen-neutral with FString. The CompareNoCase/Compare variants assume
** the same stricmp/strnicmp/strcmp the build maps in (e.g. strcasecmp).
*/
#define FSTRING_C_ISEMPTY(s)    (FSTRING_C_LEN(s) == 0)
#define FSTRING_C_ISNOTEMPTY(s) (FSTRING_C_LEN(s) != 0)

static int FString_C_Compare(const FString_C *s, const char *other)
{
	return strcmp(s->Chars, other);
}

static int FString_C_CompareNoCase(const FString_C *s, const char *other)
{
	return stricmp(s->Chars, other);
}

static int FString_C_CompareNoCaseN(const FString_C *s, const char *other, int len)
{
	return strnicmp(s->Chars, other, (size_t)len);
}

/* Construct from the first 'len' chars of src (FString(const char*, size_t)).
** Used by Left/Mid below. */
static void FString_C_InitSubstr(FString_C *s, const char *src, size_t len)
{
	FString_C_Release(s);
	s->Chars = FString_C_AllocBuffer(len);
	if (s->Chars != NULL)
	{
		if (len != 0)
			memcpy(s->Chars, src, len);
		s->Chars[len] = '\0';
	}
}

/* dst = first 'numChars' chars of src (FString::Left). dst must be unused. */
static void FString_C_Left(FString_C *dst, const FString_C *src, size_t numChars)
{
	size_t len = FSTRING_C_LEN(src);
	if (len < numChars)
		numChars = len;
	FString_C_InitSubstr(dst, src->Chars, numChars);
}

/* dst = up to 'numChars' chars of src starting at 'pos' (FString::Mid). Passing
** numChars = ~(size_t)0 takes the rest of the string, as FString's default. */
static void FString_C_Mid(FString_C *dst, const FString_C *src, size_t pos, size_t numChars)
{
	size_t len = FSTRING_C_LEN(src);
	if (pos >= len)
	{
		FString_C_InitSubstr(dst, "", 0);
		return;
	}
	if (pos + numChars > len || pos + numChars < pos)
		numChars = len - pos;
	FString_C_InitSubstr(dst, src->Chars + pos, numChars);
}

/*
** printf-style formatting (FString::Format / VFormat). FString uses its own
** StringFormat::VWorker, but only standard printf specifiers are used in the
** codebase (%d, %s, %X, width/precision/flags, %*d, ...), for which vsnprintf
** produces identical output; this delegates to vsnprintf with the usual
** size-probe-then-allocate pattern rather than porting the 662-line custom
** formatter. dst must be unused (it is freshly allocated here).
*/
#include <stdarg.h>
#include <stdio.h>

/* Old MSVC (pre-2013) lacks va_copy; the build's compat layer defines it, but
** define a fallback so this header is self-sufficient. */
#if !defined(va_copy) && !defined(__cplusplus)
#	if defined(__va_copy)
#		define va_copy(d, s) __va_copy(d, s)
#	else
#		define va_copy(d, s) ((d) = (s))
#	endif
#endif

static void FString_C_VFormat(FString_C *dst, const char *fmt, va_list arglist)
{
	va_list ap;
	int needed;
	FString_C_Release(dst);		/* drop any prior buffer, like FString::VFormat */
	va_copy(ap, arglist);
	needed = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);
	if (needed < 0)
		needed = 0;
	dst->Chars = FString_C_AllocBuffer((size_t)needed);
	if (dst->Chars != NULL)
		vsnprintf(dst->Chars, (size_t)needed + 1, fmt, arglist);
}

static void FString_C_Format(FString_C *dst, const char *fmt, ...)
{
	va_list arglist;
	va_start(arglist, fmt);
	FString_C_VFormat(dst, fmt, arglist);
	va_end(arglist);
}

/*
** Concatenation (FString::operator+ / the (head,tail) constructors). Mirrors
** the ctor: allocate len1+len2, copy head then tail. dst must be unused; it is
** freshly allocated and is independent of head/tail (no shared buffer), so the
** caller owns it and must Release it. In C the temporary that C++ created and
** destroyed for 'a + b' becomes this explicit dst that the caller frees - that
** lifetime is the call site's responsibility, the crux of the migration.
*/
static void FString_C_Concat(FString_C *dst, const FString_C *head, const FString_C *tail)
{
	size_t len1 = FSTRING_C_LEN(head);
	size_t len2 = FSTRING_C_LEN(tail);
	char *buf = FString_C_AllocBuffer(len1 + len2);
	if (buf != NULL)
	{
		if (len1 != 0) memcpy(buf, head->Chars, len1);
		if (len2 != 0) memcpy(buf + len1, tail->Chars, len2);
		buf[len1 + len2] = '\0';
	}
	FString_C_Release(dst);		/* free prior AFTER reading head/tail (may alias dst) */
	dst->Chars = buf;
}

/* As FString_C_Concat but the tail is a plain C string
** (FString::operator+(const char*) / FString(const FString&, const char*)). */
static void FString_C_ConcatCStr(FString_C *dst, const FString_C *head, const char *tail)
{
	size_t len1 = FSTRING_C_LEN(head);
	size_t len2 = (tail != NULL) ? strlen(tail) : 0;
	char *buf = FString_C_AllocBuffer(len1 + len2);
	if (buf != NULL)
	{
		if (len1 != 0) memcpy(buf, head->Chars, len1);
		if (len2 != 0) memcpy(buf + len1, tail, len2);
		buf[len1 + len2] = '\0';
	}
	FString_C_Release(dst);		/* free prior AFTER reading head (may alias dst) */
	dst->Chars = buf;
}

#endif
