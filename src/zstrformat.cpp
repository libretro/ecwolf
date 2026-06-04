/*
** zstrformat.cpp
** Routines for generic printf-style formatting.
**
**---------------------------------------------------------------------------
** Copyright 2005-2008 Randy Heit
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
** Portions of this file relating to printing floating point numbers
** are covered by the following copyright:
**
**---------------------------------------------------------------------------
** Copyright (c) 1990, 1993
**	The Regents of the University of California.  All rights reserved.
**
** This code is derived from software contributed to Berkeley by
** Chris Torek.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 4. Neither the name of the University nor the names of its contributors
**    may be used to endorse or promote products derived from this software
**    without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
**
**---------------------------------------------------------------------------
**
** Even though the standard C library has a function to do printf-style
** formatting in a generic way, there is no standard interface to this
** function. So if you want to do some printf formatting that doesn't fit in
** the context of the provided functions, you need to roll your own. Why is
** that?
**
** Maybe Microsoft wants you to write a better one yourself? When used as
** part of a sprintf replacement, this function is significantly faster than
** Microsoft's offering. When used as part of a fprintf replacement, this
** function turns out to be slower, but that's probably because the CRT's
** fprintf can interact with the FILE object on a low level for better
** perfomance. If you sprintf into a buffer and then fwrite that buffer, this
** routine wins again, though the difference isn't great.
*/

#include <limits.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#include "zstring.h"

#ifndef _MSC_VER
#include <stdint.h>
#else
typedef unsigned __int64 uint64_t;
typedef signed __int64 int64_t;
#endif

static const char hexits[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
static const char HEXits[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
static const char spaces[16] = {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
static const char zeroes[17] = {'0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','.'};
static const char dotchar = '.';

namespace StringFormat
{
	static int writepad (OutputFunc output, void *outputData, const char *pad, int padsize, int spaceToFill);

	int Worker (OutputFunc output, void *outputData, const char *fmt, ...)
	{
		va_list arglist;
		int len;

		va_start (arglist, fmt);
		len = VWorker (output, outputData, fmt, arglist);
		va_end (arglist);
		return len;
	}

	int VWorker (OutputFunc output, void *outputData, const char *fmt, va_list arglist)
	{
		const char *c;
		const char *base;
		int len = 0;
		int width;
		int precision;
		int flags;

		base = c = fmt;
		for (;;)
		{
			while (*c && *c != '%')
			{
				++c;
			}
			if (*c == '\0')
			{
				return len + output (outputData, base, int(c - base));
			}

			if (c - base > 0)
			{
				len += output (outputData, base, int(c - base));
			}
			c++;

			// Gather the flags, if any
			for (flags = 0;; ++c)
			{
				if (*c == '-')
				{
					flags |= F_MINUS;		// bit 0
				}
				else if (*c == '+')
				{
					flags |= F_PLUS;		// bit 1
				}
				else if (*c == '0')
				{
					flags |= F_ZERO;		// bit 2
				}
				else if (*c == ' ')
				{
					flags |= F_BLANK;		// bit 3
				}
				else if (*c == '#')
				{
					flags |= F_HASH;		// bit 4
				}
				else
				{
					break;
				}
			}

			width = precision = -1;

			// Read the width, if any
			if (*c == '*')
			{
				++c;
				width = va_arg (arglist, int);
				if (width < 0)
				{ // Negative width means minus flag and positive width
					flags |= F_MINUS;
					width = -width;
				}
			}
			else if (*c >= '0' && *c <= '9')
			{
				width = *c++ - '0';
				while (*c >= '0' && *c <= '9')
				{
					width = width * 10 + *c++ - '0';
				}
			}

			// If 0 and - both appear, 0 is ignored.
			// If the blank and + both appear, the blank is ignored.
			flags &= ~((flags & 3) << 2);

			// Read the precision, if any
			if (*c == '.')
			{
				precision = 0;
				if (*++c == '*')
				{
					++c;
					precision = va_arg (arglist, int);
				}
				else if (*c >= '0' && *c <= '9')
				{
					precision = *c++ - '0';
					while (*c >= '0' && *c <= '9')
					{
						precision = precision * 10 + *c++ - '0';
					}
				}
			}

			// Read the size prefix, if any
			if (*c == 'h')
			{
				if (*++c == 'h')
				{
					flags |= F_HALFHALF;
					++c;
				}
				else
				{
					flags |= F_HALF;
				}
			}
			else if (*c == 'l')
			{
				if (*++c == 'l')
				{
					flags |= F_LONGLONG;
					++c;
				}
				else
				{
					flags |= F_LONG;
				}
			}
			else if (*c == 'I')
			{
				if (*++c == '6')
				{
					if (*++c == '4')
					{
						flags |= F_LONGLONG;
                        ++c;
					}
				}
				else
				{
					flags |= F_BIGI;
				}
			}
			else if (*c == 't')
			{
				flags |= F_PTRDIFF;
				++c;
			}
			else if (*c == 'z')
			{
				flags |= F_SIZE;
				++c;
			}

			base = c+1;

			// Now that that's all out of the way, we should be pointing at the type specifier
	{
		char prefix[3];
		int prefixlen;
		char hexprefix = '\0';
		char sign = '\0';
		int postprefixzeros = 0;
		int size = flags & 0xF000;
		char buffer[80], *ibuff;
		const char *obuff = 0;
		char type = *c++;
		int bufflen = 0;
		int outlen = 0;
		unsigned int intarg = 0;
		uint64_t int64arg = 0;
		const void *voidparg;
		const char *charparg;
		const char *xits = hexits;
		int inlen = len;

		// Using a bunch of if/else if statements is faster than a switch, because a switch generates
		// a jump table. A jump table means a possible data cache miss and a hefty penalty while the
		// cache line is loaded.

		if (type == 'x' || type == 'X' ||
			type == 'p' ||
			type == 'd' || type == 'u' || type == 'i' ||
			type == 'o' ||
			type == 'B')
		{
			if (type == 'X' || type == 'p')
			{
				xits = HEXits;
			}
			if (type == 'p')
			{
				type = 'X';
				voidparg = va_arg (arglist, void *);
				if (sizeof(void*) == sizeof(int))
				{
					intarg = (unsigned int)(size_t)voidparg;
					precision = 8;
					size = 0;
				}
				else
				{
					int64arg = (uint64_t)(size_t)voidparg;
					precision = 16;
					size = F_LONGLONG;
				}
			}
			else
			{
				if (size == 0)
				{
					intarg = va_arg (arglist, int);
				}
				else if (size == F_HALFHALF)
				{
					intarg = va_arg (arglist, int);
					intarg = (signed char)intarg;
				}
				else if (size == F_HALF)
				{
					intarg = va_arg (arglist, int);
					intarg = (short)intarg;
				}
				else if (size == F_LONG)
				{
					if (sizeof(long) == sizeof(int)) intarg = va_arg (arglist, int);
					else { int64arg = va_arg (arglist, int64_t); size = F_LONGLONG; }
				}
				else if (size == F_BIGI)
				{
					if (sizeof(void*) == sizeof(int)) intarg = va_arg (arglist, int);
					else { int64arg = va_arg (arglist, int64_t); size = F_LONGLONG; }
				}
				else if (size == F_LONGLONG)
				{
					int64arg = va_arg (arglist, int64_t);
				}
				else if (size == F_PTRDIFF)
				{
					if (sizeof(ptrdiff_t) == sizeof(int)) intarg = va_arg (arglist, int);
					else { int64arg = va_arg (arglist, int64_t); size = F_LONGLONG; }
				}
				else if (size == F_SIZE)
				{
					if (sizeof(size_t) == sizeof(int)) intarg = va_arg (arglist, int);
					else { int64arg = va_arg (arglist, int64_t); size = F_LONGLONG; }
				}
				else
				{
					intarg = va_arg (arglist, int);
				}
			}

			if (precision < 0) precision = 1;

			ibuff = &buffer[sizeof(buffer)];

			if (size == F_LONGLONG)
			{
				if (int64arg == 0)
				{
					flags |= F_ZEROVALUE;
				}
				else
				{
					if (type == 'o')
					{ // Octal: Dump digits until it fits in an unsigned int
						while (int64arg > UINT_MAX)
						{
							*--ibuff = char(int64arg & 7) + '0'; int64arg >>= 3;
						}
						intarg = int(int64arg);
					}
					else if (type == 'x' || type == 'X')
					{ // Hexadecimal: Dump digits until it fits in an unsigned int
						while (int64arg > UINT_MAX)
						{
							*--ibuff = xits[int64arg & 15]; int64arg >>= 4;
						}
						intarg = int(int64arg);
					}
					else if (type == 'B')
					{ // Binary: Dump digits until it fits in an unsigned int
						while (int64arg > UINT_MAX)
						{
							*--ibuff = char(int64arg & 1) + '0'; int64arg >>= 1;
						}
						intarg = int(int64arg);
					}
					else
					{
						if (type != 'u')
						{
							// If a signed number is negative, set the negative flag and make it positive.
							int64_t sint64arg = (int64_t)int64arg;
							if (sint64arg < 0)
							{
								flags |= F_NEGATIVE;
								sint64arg = -sint64arg;
								int64arg = sint64arg;
							}
							flags |= F_SIGNED;
							type = 'u';
						}
						// If an unsigned int64 is too big to fit in an unsigned int, dump out
						// digits until it is sufficiently small.
						while (int64arg > INT_MAX)
						{
							*--ibuff = char(int64arg % 10) + '0'; int64arg /= 10;
						}
						intarg = (unsigned int)(int64arg);
					}
				}
			}
			else
			{
				if (intarg == 0)
				{
					flags |= F_ZEROVALUE;
				}
				else if (type == 'i' || type == 'd')
				{ // If a signed int is negative, set the negative flag and make it positive.
					signed int sintarg = (signed int)intarg;
					if (sintarg < 0)
					{
						flags |= F_NEGATIVE;
						sintarg = -sintarg;
						intarg = sintarg;
					}
					flags |= F_SIGNED;
					type = 'u';
				}
			}
			if (flags & F_ZEROVALUE)
			{
				if (precision != 0)
				{
					*--ibuff = '0';
				}
			}
			else if (type == 'u')
			{ // Decimal
				int i;

				// Unsigned division is typically slower than signed division.
				// Do it at most once.
				if (intarg > INT_MAX)
				{
					*--ibuff = char(intarg % 10) + '0'; intarg /= 10;
				}
				i = (int)intarg;
				while (i != 0)
				{
					*--ibuff = char(i % 10) + '0'; i /= 10;
				}
			}
			else if (type == 'o')
			{ // Octal
				while (intarg != 0)
				{
					*--ibuff = char(intarg & 7) + '0'; intarg >>= 3;
				}
			}
			else if (type == 'B')
			{ // Binary
				while (intarg != 0)
				{
					*--ibuff = char(intarg & 1) + '0'; intarg >>= 1;
				}
			}
			else
			{ // Hexadecimal
				while (intarg != 0)
				{
					*--ibuff = xits[intarg & 15]; intarg >>= 4;
				}
			}
			// Check for prefix (only for non-decimal, which are always unsigned)
			if ((flags & (F_HASH|F_ZEROVALUE)) == F_HASH)
			{
				if (type == 'o')
				{
					if (bufflen >= precision)
					{
						sign = '0';
					}
				}
				else if (type == 'x' || type == 'X')
				{
					hexprefix = type;
				}
				else if (type == 'B')
				{
					hexprefix = '!';
				}
			}
			bufflen = (int)(ptrdiff_t)(&buffer[sizeof(buffer)] - ibuff);
			obuff = ibuff;
			if (precision >= 0)
			{
				postprefixzeros = precision - bufflen;
				if (postprefixzeros < 0) postprefixzeros = 0;
//				flags &= ~F_ZERO;
			}
		}
		else if (type == 'c')
		{
			intarg = va_arg (arglist, int);
			buffer[0] = char(intarg);
			bufflen = 1;
			obuff = buffer;
		}
		else if (type == 's')
		{
			charparg = va_arg (arglist, const char *);
			if (charparg == NULL)
			{
				obuff = "(null)";
				bufflen = 6;
			}
			else
			{
				obuff = charparg;
				if (precision < 0)
				{
					bufflen = (int)strlen (charparg);
				}
				else
				{
					for (bufflen = 0; bufflen < precision && charparg[bufflen] != '\0'; ++bufflen)
					{ /* empty */ }
				}
			}
		}
		else if (type == '%')
		{ // Just print a '%': Output it with the next stage.
			base--;
			continue;
		}
		else if (type == 'n')
		{
			if (size == F_HALFHALF)
			{
				*va_arg (arglist, char *) = (char)inlen;
			}
			else if (size == F_HALF)
			{
				*va_arg (arglist, short *) = (short)inlen;
			}
			else if (size == F_LONG)
			{
				*va_arg (arglist, long *) = inlen;
			}
			else if (size == F_LONGLONG)
			{
				*va_arg (arglist, int64_t *) = inlen;
			}
			else if (size == F_BIGI)
			{
				*va_arg (arglist, ptrdiff_t *) = inlen;
			}
			else
			{
				*va_arg (arglist, int *) = inlen;
			}
		}
		
		// Check for sign prefix (only for signed numbers)
		if (flags & F_SIGNED)
		{
			if (flags & F_NEGATIVE)
			{
				sign = '-';
			}
			else if (flags & F_PLUS)
			{
				sign = '+';
			}
			else if (flags & F_BLANK)
			{
				sign = ' ';
			}
		}

		// Construct complete prefix from sign and hex prefix character
		prefixlen = 0;
		if (sign != '\0')
		{
			prefix[0] = sign;
			prefixlen = 1;
		}
		if (hexprefix != '\0')
		{
			prefix[prefixlen] = '0';
			prefix[prefixlen + 1] = hexprefix;
			prefixlen += 2;
		}

		// Pad the output to the field width, if needed
		int fieldlen = prefixlen + postprefixzeros + bufflen;
		const char *pad = (flags & F_ZERO) ? zeroes : spaces;

		// If the output is right aligned and zero-padded, then the prefix must come before the padding.
		if ((flags & (F_ZERO|F_MINUS)) == F_ZERO && prefixlen > 0)
		{
			outlen += output (outputData, prefix, prefixlen);
			prefixlen = 0;
		}
        if (!(flags & F_MINUS) && fieldlen < width)
		{ // Field is right-justified, so padding comes first
			outlen += writepad (output, outputData, pad, sizeof(spaces), width - fieldlen);
			width = -1;
		}

		// Output field: Prefix, post-prefix zeros, buffer text
		if (prefixlen > 0)
		{
			outlen += output (outputData, prefix, prefixlen);
		}
		outlen += writepad (output, outputData, zeroes, sizeof(spaces), postprefixzeros);
		if (bufflen > 0)
		{
			outlen += output (outputData, obuff, bufflen);
		}

        if ((flags & F_MINUS) && fieldlen < width)
		{ // Field is left-justified, so padding comes last
			outlen += writepad (output, outputData, pad, sizeof(spaces), width - fieldlen);
		}
		len += outlen;
	}
	}
	}

	static int writepad (OutputFunc output, void *outputData, const char *pad, int padsize, int spaceToFill)
	{
		int outlen = 0;
		while (spaceToFill > 0)
		{
			int count = spaceToFill > padsize ? padsize : spaceToFill;
			outlen += output (outputData, pad, count);
			spaceToFill -= count;
		}
		return outlen;
	}
};
