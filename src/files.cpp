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

extern "C"
{
#include "7z/r7z_lzma.h"
}
#include "tarray.h"

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

// The decoder state lives behind an opaque pointer (a pattern kept from the
// LZMA-SDK days, when its headers could not be included from files.h); it
// now also carries the whole-stream decode result, since rlzma_dec_decode
// is a one-shot whole-block decoder rather than an incremental one. The
// only real-world caller (FZipLump::FillCache) performs a single full-size
// Read anyway; partial reads are still honoured from the decoded buffer.
struct FileReaderLZMA::StreamPointer
{
	rlzma_dec_t Dec;
	TArray<uint8_t> Out;
	size_t OutPos;
	bool Decoded;
};

FileReaderLZMA::FileReaderLZMA (FileReader &file, size_t uncompressed_size, bool zip)
: File(file), SawEOF(false)
{
	uint8_t header[4 + RLZMA_PROPS_SIZE];

	assert(zip == true);

	Size = uncompressed_size;
	OutProcessed = 0;

	// Zip method-14 payload: 2-byte version, 2-byte props size, then the
	// LZMA properties, then the raw stream.
	if (File.Read(header, sizeof(header)) < (long)sizeof(header))
	{
		I_Error("FileReaderLZMA: File too short\n");
	}
	if (header[2] + header[3] * 256 != RLZMA_PROPS_SIZE)
	{
		I_Error("FileReaderLZMA: LZMA props size is %d (expected %d)\n",
			header[2] + header[3] * 256, RLZMA_PROPS_SIZE);
	}

	Streamp = new StreamPointer;
	Streamp->OutPos = 0;
	Streamp->Decoded = false;

	if (rlzma_dec_init(&Streamp->Dec, header + 4) != RLZMA_OK)
	{
		I_Error("FileReaderLZMA: unsupported LZMA properties\n");
	}
}

FileReaderLZMA::~FileReaderLZMA ()
{
	delete Streamp;
}

long FileReaderLZMA::Read (void *buffer, long len)
{
	if (!Streamp->Decoded)
	{
		// Whole-stream decode. Zip gives no compressed length at this
		// layer, so read a generous slice of the remaining container and
		// widen it if the decoder reports truncated input: LZMA streams
		// smaller than their plaintext are the overwhelmingly common case,
		// and the ladder caps at the container end.
		long remain = File.GetLength() - File.Tell();
		if (remain < 0)
			remain = 0;
		long want = (long)(Size + Size / 4 + 1024);
		if (want > remain)
			want = remain;

		TArray<uint8_t> in;
		in.Resize(want);
		long got = want > 0 ? File.Read(&in[0], want) : 0;

		Streamp->Out.Resize(Size);
		for (;;)
		{
			int r = rlzma_dec_decode(&Streamp->Dec,
				Size ? &Streamp->Out[0] : NULL, Size,
				got ? &in[0] : NULL, (size_t)got);
			if (r == RLZMA_OK)
				break;
			if (got >= remain)
				I_Error ("Corrupt LZMA stream");
			// Truncated input: widen toward the container end and retry.
			long more = got ? got : 1024;
			if (got + more > remain)
				more = remain - got;
			in.Resize(got + more);
			long extra = File.Read(&in[got], more);
			if (extra <= 0)
				I_Error ("Corrupt LZMA stream");
			got += extra;
		}
		Streamp->Decoded = true;
	}

	if ((size_t)len > Size - Streamp->OutPos)
		len = (long)(Size - Streamp->OutPos);
	if (len > 0)
	{
		memcpy(buffer, &Streamp->Out[Streamp->OutPos], len);
		Streamp->OutPos += len;
	}
	return len;
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
