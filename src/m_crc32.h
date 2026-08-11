/*
** m_crc32.h
** Simple interface to zlib's CRC table
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
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

#ifndef __M_CRC32__
#define __M_CRC32__

#include "wl_def.h"
extern "C"
{
#include "encodings/crc32.h"
}

// CRC32 via libretro-common's encoding_crc32 (identical polynomial and
// pre/post conditioning to zlib's crc32()). The byte-stepping table for
// CRC1 is generated locally once, since the library does not export its
// internal table.

inline const uint32_t *GetCRCTable ()
{
	static uint32_t table[256];
	static bool built = false;
	if (!built)
	{
		for (uint32_t i = 0; i < 256; i++)
		{
			uint32_t c = i;
			for (int k = 0; k < 8; k++)
				c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
			table[i] = c;
		}
		built = true;
	}
	return table;
}
inline uint32_t CalcCRC32 (const uint8_t *buf, unsigned int len)
{
	return encoding_crc32 (0, buf, len);
}
inline uint32_t AddCRC32 (uint32_t crc, const uint8_t *buf, unsigned int len)
{
	return encoding_crc32 (crc, buf, len);
}
inline uint32_t CRC1 (uint32_t crc, const uint8_t c, const uint32_t *crcTable)
{
	return crcTable[(crc & 0xff) ^ c] ^ (crc >> 8);
}

#endif
