/*
** files.cpp
** Implements classes for reading from files or memory blocks
**
**---------------------------------------------------------------------------
** Copyright 1998-2008 Randy Heit
** Copyright 2005-2008 Christoph Oelckers
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

#include "LzmaDec.h"

#include "files.h"
#include "filesys.h"
#include "templates.h"
#include "zdoomsupport.h"
#include "doomerrors.h"

//==========================================================================
//
// FileReader
//
// reads data from an uncompressed file or part of it
//
//==========================================================================


char *FileReader::GetsFromBuffer(const char * bufptr, char *strbuf, int len)
{
	if (len>Length-FilePos) len=Length-FilePos;
	if (len <= 0) return NULL;

	char *p = strbuf;
	while (len > 1)
	{
		if (bufptr[FilePos] == 0)
		{
			FilePos++;
			break;
		}
		if (bufptr[FilePos] != '\r')
		{
			*p++ = bufptr[FilePos];
			len--;
			if (bufptr[FilePos] == '\n') 
			{
				FilePos++;
				break;
			}
		}
		FilePos++;
	}
	if (p==strbuf) return NULL;
	*p++=0;
	return strbuf;
}


//==========================================================================
//
// FileReaderZ
//
// The zlib wrapper
// reads data from a ZLib compressed stream
//
//==========================================================================

FileReaderZ::FileReaderZ (FileReader &file, bool zip)
: File(file), SawEOF(false)
{
	int err;

	FillBuffer ();

	Stream.zalloc = Z_NULL;
	Stream.zfree = Z_NULL;

	if (!zip) err = inflateInit (&Stream);
	else err = inflateInit2 (&Stream, -MAX_WBITS);

	if (err != Z_OK)
	{
		I_Error ("FileReaderZ: inflateInit failed: %d\n", err);
	}
}

FileReaderZ::~FileReaderZ ()
{
	inflateEnd (&Stream);
}

long FileReaderZ::Read (void *buffer, long len)
{
	int err;

	Stream.next_out = (Bytef *)buffer;
	Stream.avail_out = len;

	do
	{
		err = inflate (&Stream, Z_SYNC_FLUSH);
		if (Stream.avail_in == 0 && !SawEOF)
		{
			FillBuffer ();
		}
	} while (err == Z_OK && Stream.avail_out != 0);

	if (err != Z_OK && err != Z_STREAM_END)
	{
		I_Error ("Corrupt zlib stream");
	}

	if (Stream.avail_out != 0)
	{
		I_Error ("Ran out of data in zlib stream");
	}

	return len - Stream.avail_out;
}

void FileReaderZ::FillBuffer ()
{
	long numread = File.Read (InBuff, BUFF_SIZE);

	if (numread < BUFF_SIZE)
	{
		SawEOF = true;
	}
	Stream.next_in = InBuff;
	Stream.avail_in = numread;
}

//==========================================================================
//
// FileReaderZ
//
// The bzip2 wrapper
// reads data from a libbzip2 compressed stream
//
//==========================================================================

FileReaderBZ2::FileReaderBZ2 (FileReader &file)
: File(file), SawEOF(false)
{
	int err;

	FillBuffer ();

	Stream.bzalloc = NULL;
	Stream.bzfree = NULL;
	Stream.opaque = NULL;

	err = BZ2_bzDecompressInit(&Stream, 0, 0);

	if (err != BZ_OK)
	{
		I_Error ("FileReaderBZ2: bzDecompressInit failed: %d\n", err);
	}
}

FileReaderBZ2::~FileReaderBZ2 ()
{
	BZ2_bzDecompressEnd (&Stream);
}

long FileReaderBZ2::Read (void *buffer, long len)
{
	int err;

	Stream.next_out = (char *)buffer;
	Stream.avail_out = len;

	do
	{
		err = BZ2_bzDecompress(&Stream);
		if (Stream.avail_in == 0 && !SawEOF)
		{
			FillBuffer ();
		}
	} while (err == BZ_OK && Stream.avail_out != 0);

	if (err != BZ_OK && err != BZ_STREAM_END)
	{
		I_Error ("Corrupt bzip2 stream");
	}

	if (Stream.avail_out != 0)
	{
		I_Error ("Ran out of data in bzip2 stream");
	}

	return len - Stream.avail_out;
}

void FileReaderBZ2::FillBuffer ()
{
	long numread = File.Read(InBuff, BUFF_SIZE);

	if (numread < BUFF_SIZE)
	{
		SawEOF = true;
	}
	Stream.next_in = (char *)InBuff;
	Stream.avail_in = numread;
}

//==========================================================================
//
// bz_internal_error
//
// libbzip2 wants this, since we build it with BZ_NO_STDIO set.
//
//==========================================================================

extern "C" void bz_internal_error (int errcode)
{
	I_FatalError("libbzip2: internal error number %d\n", errcode);
}

//==========================================================================
//
// FileReaderLZMA
//
// The lzma wrapper
// reads data from a LZMA compressed stream
//
//==========================================================================

// This is retarded but necessary to work around the inclusion of windows.h in recent
// LZMA versions, meaning it's no longer possible to include the LZMA headers in files.h. 
// As a result we cannot declare the CLzmaDec member in the header so we work around
// it my wrapping it into another struct that can be declared anonymously in the header.
struct FileReaderLZMA::StreamPointer
{
	CLzmaDec Stream;
};

static void *SzAlloc(void *, size_t size) {
	void *ret = malloc(size);
	CHECKMALLOCRESULT(ret);
	return ret;
}
static void SzFree(void *, void *address) { free(address); }
ISzAlloc g_Alloc = { SzAlloc, SzFree };

FileReaderLZMA::FileReaderLZMA (FileReader &file, size_t uncompressed_size, bool zip)
: File(file), SawEOF(false)
{
	uint8_t header[4 + LZMA_PROPS_SIZE];
	int err;

	assert(zip == true);

	Size = uncompressed_size;
	OutProcessed = 0;

	// Read zip LZMA properties header
	if (File.Read(header, sizeof(header)) < (long)sizeof(header))
	{
		I_Error("FileReaderLZMA: File too shart\n");
	}
	if (header[2] + header[3] * 256 != LZMA_PROPS_SIZE)
	{
		I_Error("FileReaderLZMA: LZMA props size is %d (expected %d)\n",
			header[2] + header[3] * 256, LZMA_PROPS_SIZE);
	}

	FillBuffer();

	Streamp = new StreamPointer;
	LzmaDec_Construct(&Streamp->Stream);
	err = LzmaDec_Allocate(&Streamp->Stream, header + 4, LZMA_PROPS_SIZE, &g_Alloc);

	if (err != SZ_OK)
	{
		I_Error("FileReaderLZMA: LzmaDec_Allocate failed: %d\n", err);
	}

	LzmaDec_Init(&Streamp->Stream);
}

FileReaderLZMA::~FileReaderLZMA ()
{
	LzmaDec_Free(&Streamp->Stream, &g_Alloc);
	delete Streamp;
}

long FileReaderLZMA::Read (void *buffer, long len)
{
	int err;
	Byte *next_out = (Byte *)buffer;

	do
	{
		ELzmaFinishMode finish_mode = LZMA_FINISH_ANY;
		ELzmaStatus status;
		size_t out_processed = len;
		size_t in_processed = InSize;

		err = LzmaDec_DecodeToBuf(&Streamp->Stream, next_out, &out_processed, InBuff + InPos, &in_processed, finish_mode, &status);
		InPos += in_processed;
		InSize -= in_processed;
		next_out += out_processed;
		len = (long)(len - out_processed);
		if (err != SZ_OK)
		{
			I_Error ("Corrupt LZMA stream");
		}
		if (in_processed == 0 && out_processed == 0)
		{
			if (status != LZMA_STATUS_FINISHED_WITH_MARK)
			{
				I_Error ("Corrupt LZMA stream");
			}
		}
		if (InSize == 0 && !SawEOF)
		{
			FillBuffer ();
		}
	} while (err == SZ_OK && len != 0);

	if (err != Z_OK && err != Z_STREAM_END)
	{
		I_Error ("Corrupt LZMA stream");
	}

	if (len != 0)
	{
		I_Error ("Ran out of data in LZMA stream");
	}

	return (long)(next_out - (Byte *)buffer);
}

void FileReaderLZMA::FillBuffer ()
{
	long numread = File.Read(InBuff, BUFF_SIZE);

	if (numread < BUFF_SIZE)
	{
		SawEOF = true;
	}
	InPos = 0;
	InSize = numread;
}

//==========================================================================
//
// MemoryReader
//
// reads data from a block of memory
//
//==========================================================================

MemoryReader::MemoryReader (const char *buffer, long length)
{
	bufptr=buffer;
	Length=length;
	FilePos=0;
}

MemoryReader::~MemoryReader ()
{
}

long MemoryReader::Tell () const
{
	return FilePos;
}

long MemoryReader::Seek (long offset, int origin)
{
	switch (origin)
	{
	case SEEK_CUR:
		offset+=FilePos;
		break;

	case SEEK_END:
		offset+=Length;
		break;

	}
	FilePos=clamp<long>(offset,0,Length);
	return 0;
}

long MemoryReader::Read (void *buffer, long len)
{
	if (len>Length-FilePos) len=Length-FilePos;
	if (len<0) len=0;
	memcpy(buffer,bufptr+FilePos,len);
	FilePos+=len;
	return len;
}

char *MemoryReader::Gets(char *strbuf, int len)
{
	return GetsFromBuffer(bufptr, strbuf, len);
}
