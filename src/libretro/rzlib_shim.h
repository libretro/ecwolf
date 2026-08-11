/*
** rzlib_shim.h
**
** A minimal zlib-compatible API implemented on libretro-common's
** rinflate/rdeflate (encodings/deflate.h). The engine's zlib consumers
** (FileReaderZ, m_png) carry carefully-tuned streaming loops around the
** z_stream contract; reimplementing those loops against a new API risks
** subtle behaviour changes, while this shim lets their logic stay
** byte-for-byte identical and swaps only the codec underneath.
**
** Only the surface those consumers use is provided: inflateInit,
** inflateInit2 (raw window), inflate with Z_SYNC_FLUSH/Z_FINISH,
** inflateEnd, deflateInit, deflate, deflateEnd, and the handful of
** constants and typedefs they touch.
*/

#ifndef __RZLIB_SHIM_H__
#define __RZLIB_SHIM_H__

#include <stdint.h>
#include <stdlib.h>

extern "C"
{
#include "encodings/deflate.h"
}

typedef unsigned char Bytef;
typedef unsigned int uInt;
typedef unsigned long uLong;

#define Z_NULL 0
#define Z_OK 0
#define Z_STREAM_END 1
#define Z_SYNC_FLUSH 2
#define Z_FINISH 4
#define Z_STREAM_ERROR (-2)
#define Z_DATA_ERROR (-3)
#define Z_BUF_ERROR (-5)
#define MAX_WBITS 15

typedef struct z_stream_s
{
	const Bytef *next_in;
	uInt avail_in;
	Bytef *next_out;
	uInt avail_out;

	/* zalloc/zfree exist only so callers' Z_NULL assignments compile. */
	void *zalloc;
	void *zfree;

	/* shim state */
	void *rs;
	int deflating;
	int finish_signalled;
	int saw_end;
} z_stream;

static inline int inflateInit2 (z_stream *s, int window_bits)
{
	s->rs = rinflate_new (window_bits);
	s->deflating = 0;
	s->finish_signalled = 0;
	s->saw_end = 0;
	return s->rs ? Z_OK : Z_STREAM_ERROR;
}

static inline int inflateInit (z_stream *s)
{
	return inflateInit2 (s, MAX_WBITS);
}

static inline int inflate (z_stream *s, int flush)
{
	size_t rd = 0, wr = 0;
	int r;
	(void)flush;
	if (s->saw_end)
		return Z_STREAM_END;
	rinflate_set_in (s->rs, s->next_in, s->avail_in);
	rinflate_set_out (s->rs, s->next_out, s->avail_out);
	r = rinflate_process (s->rs, &rd, &wr);
	s->next_in += rd;
	s->avail_in -= (uInt)rd;
	s->next_out += wr;
	s->avail_out -= (uInt)wr;
	if (r == RDEFLATE_PROCESS_ERROR)
		return Z_DATA_ERROR;
	if (r == RDEFLATE_PROCESS_END)
	{
		s->saw_end = 1;
		return Z_STREAM_END;
	}
	/* zlib returns Z_BUF_ERROR when no forward progress is possible;
	   FileReaderZ's refill loop depends on a non-Z_OK result to stop
	   spinning once the input is exhausted. */
	if (rd == 0 && wr == 0 && s->avail_in == 0)
		return Z_BUF_ERROR;
	return Z_OK;
}

static inline int inflateEnd (z_stream *s)
{
	if (s->rs)
		rinflate_free (s->rs);
	s->rs = NULL;
	return Z_OK;
}

static inline int deflateInit (z_stream *s, int level)
{
	s->rs = rdeflate_new (level, MAX_WBITS);
	s->deflating = 1;
	s->finish_signalled = 0;
	s->saw_end = 0;
	return s->rs ? Z_OK : Z_STREAM_ERROR;
}

static inline int deflate (z_stream *s, int flush)
{
	size_t rd = 0, wr = 0;
	int r;
	if (s->saw_end)
		return Z_STREAM_END;
	if (flush == Z_FINISH && !s->finish_signalled)
	{
		rdeflate_finish (s->rs);
		s->finish_signalled = 1;
	}
	rdeflate_set_in (s->rs, s->next_in, s->avail_in);
	rdeflate_set_out (s->rs, s->next_out, s->avail_out);
	r = rdeflate_process (s->rs, &rd, &wr);
	s->next_in += rd;
	s->avail_in -= (uInt)rd;
	s->next_out += wr;
	s->avail_out -= (uInt)wr;
	if (r == RDEFLATE_PROCESS_ERROR)
		return Z_DATA_ERROR;
	if (r == RDEFLATE_PROCESS_END)
	{
		s->saw_end = 1;
		return Z_STREAM_END;
	}
	return Z_OK;
}

static inline int deflateEnd (z_stream *s)
{
	if (s->rs)
		rdeflate_free (s->rs);
	s->rs = NULL;
	return Z_OK;
}

typedef unsigned char Byte;

/* One-shot helpers (farchive): zlib-wrapped whole-buffer compress and
   uncompress with zlib's exact return contract for the sizes the callers
   check. */
static inline int compress (Bytef *dest, uLong *destLen,
                            const Bytef *source, uLong sourceLen)
{
	z_stream s;
	int r;
	if (deflateInit (&s, 9) != Z_OK)
		return Z_STREAM_ERROR;
	s.next_in = source;
	s.avail_in = (uInt)sourceLen;
	s.next_out = dest;
	s.avail_out = (uInt)*destLen;
	do
		r = deflate (&s, Z_FINISH);
	while (r == Z_OK && s.avail_out != 0);
	*destLen = (uLong)(s.next_out - dest);
	deflateEnd (&s);
	if (r == Z_STREAM_END)
		return Z_OK;
	return (r == Z_OK) ? Z_BUF_ERROR : r;
}

static inline int uncompress (Bytef *dest, uLong *destLen,
                              const Bytef *source, uLong sourceLen)
{
	z_stream s;
	int r;
	if (inflateInit (&s) != Z_OK)
		return Z_STREAM_ERROR;
	s.next_in = source;
	s.avail_in = (uInt)sourceLen;
	s.next_out = dest;
	s.avail_out = (uInt)*destLen;
	do
		r = inflate (&s, Z_SYNC_FLUSH);
	while (r == Z_OK && s.avail_out != 0);
	*destLen = (uLong)(s.next_out - dest);
	inflateEnd (&s);
	if (r == Z_STREAM_END)
		return Z_OK;
	if (r == Z_OK || r == Z_BUF_ERROR)
		return Z_BUF_ERROR;
	return r;
}

#endif /* __RZLIB_SHIM_H__ */
