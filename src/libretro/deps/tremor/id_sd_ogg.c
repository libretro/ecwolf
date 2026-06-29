/* Shared Ogg Vorbis -> PCM decode helper. See id_sd_ogg.h.
 *
 * Decoding goes through Tremor (libvorbisidec), the fixed-point Ogg Vorbis
 * decoder, rather than a floating-point decoder. Every stage of the Vorbis
 * pipeline -- the IMDCT, the floor curves, the residue vectors and the final
 * windowing -- runs in integer/fixed-point arithmetic, and Tremor's ov_read()
 * emits native-endian signed 16-bit PCM directly. There is therefore no
 * float->int16 conversion anywhere on this path and the output is bit-identical
 * across every platform the core targets (no x87/SSE rounding divergence, no
 * FMA contraction differences). The downstream mixer is int16 end to end, so
 * the whole sound path is now deterministic integer audio.
 */

#include "id_sd_ogg.h"

#include <stdlib.h>
#include <string.h>

#include "ivorbisfile.h"

int OggIsOgg(const uint8_t *mem, size_t size)
{
	return mem != NULL && size >= 4 && memcmp(mem, "OggS", 4) == 0;
}

/* ---- in-memory datasource for Tremor's ov_open_callbacks ---------------- */

typedef struct {
	const uint8_t *data;
	size_t         size;
	size_t         pos;
} mem_stream;

static size_t mem_read(void *ptr, size_t size, size_t nmemb, void *src)
{
	mem_stream *m = (mem_stream *)src;
	size_t want, avail, n;

	if (size == 0 || nmemb == 0)
		return 0;

	/* Guard the size*nmemb multiply against overflow. */
	want = size * nmemb;
	if (want / size != nmemb)
		want = m->size; /* clamp; the avail check below caps it anyway */

	avail = m->size - m->pos;
	n = want < avail ? want : avail;
	/* Hand back whole elements only, mirroring fread() semantics. */
	n -= n % size;

	if (n > 0) {
		memcpy(ptr, m->data + m->pos, n);
		m->pos += n;
	}
	return n / size;
}

static int mem_seek(void *src, ogg_int64_t offset, int whence)
{
	mem_stream *m = (mem_stream *)src;
	ogg_int64_t base;

	switch (whence) {
	case SEEK_SET: base = 0;                 break;
	case SEEK_CUR: base = (ogg_int64_t)m->pos;  break;
	case SEEK_END: base = (ogg_int64_t)m->size; break;
	default:       return -1;
	}

	base += offset;
	if (base < 0 || base > (ogg_int64_t)m->size)
		return -1;

	m->pos = (size_t)base;
	return 0;
}

static int mem_close(void *src)
{
	(void)src;
	return 0;
}

static long mem_tell(void *src)
{
	mem_stream *m = (mem_stream *)src;
	return (long)m->pos;
}

/* ---- mono 16-bit resampler (integer 16.16 linear interpolation) --------- */

/* Resample mono 16-bit PCM from in_rate to out_rate with linear interpolation.
 * Returns a new malloc()'d buffer of (*out_frames) samples and frees src; on
 * allocation failure returns NULL after freeing src. */
static int16_t *resample_mono(int16_t *src, int in_frames, int in_rate,
                              int out_rate, int *out_frames)
{
	int16_t *dst;
	long long n;
	int i;

	if (in_rate == out_rate) {
		*out_frames = in_frames;
		return src;
	}

	/* out_frames = in_frames * out_rate / in_rate, rounded down but at
	 * least 1 so a very short clip is not lost entirely. */
	n = (long long)in_frames * out_rate / in_rate;
	if (n < 1)
		n = 1;
	if (n > 0x7fffffff) {
		free(src);
		return NULL;
	}

	dst = (int16_t *)malloc((size_t)n * sizeof(int16_t));
	if (dst == NULL) {
		free(src);
		return NULL;
	}

	for (i = 0; i < (int)n; i++) {
		/* Position in source samples, in 16.16 fixed point. */
		long long pos = (long long)i * in_rate * 65536 / out_rate;
		int idx = (int)(pos >> 16);
		int frac = (int)(pos & 0xffff);
		int s0, s1;
		if (idx >= in_frames - 1) {
			dst[i] = src[in_frames - 1];
			continue;
		}
		s0 = src[idx];
		s1 = src[idx + 1];
		dst[i] = (int16_t)(s0 + (((s1 - s0) * frac) >> 16));
	}

	free(src);
	*out_frames = (int)n;
	return dst;
}

/* ---- decode ------------------------------------------------------------- */

/* Number of frames the mono accumulator grows by when it fills. Picked so a
 * typical sound effect or short jingle decodes in a handful of reallocs. */
#define OGG_GROW_FRAMES 8192

int16_t *OggDecodeToMonoPCM(const uint8_t *mem, size_t size, int target_rate,
                            int *out_rate, int *out_samples)
{
	mem_stream      ms;
	ov_callbacks    cbs;
	OggVorbis_File  vf;
	vorbis_info    *vi;
	int             channels, rate;
	int16_t        *mono = NULL;     /* growable mono accumulator */
	int             mono_cap = 0;    /* capacity in frames */
	int             frames = 0;      /* frames written so far */
	short           block[4096];     /* interleaved decode scratch */

	if (!OggIsOgg(mem, size) || size > (size_t)0x7fffffff || target_rate <= 0)
		return NULL;

	ms.data = mem;
	ms.size = size;
	ms.pos  = 0;

	cbs.read_func  = mem_read;
	cbs.seek_func  = mem_seek;
	cbs.close_func = mem_close;
	cbs.tell_func  = mem_tell;

	if (ov_open_callbacks(&ms, &vf, NULL, 0, cbs) < 0)
		return NULL;

	vi = ov_info(&vf, -1);
	if (vi == NULL || vi->channels < 1 || vi->rate <= 0) {
		ov_clear(&vf);
		return NULL;
	}
	channels = vi->channels;
	rate     = (int)vi->rate;

	for (;;) {
		int bitstream = 0;
		long ret = ov_read(&vf, (char *)block, (int)sizeof(block),
		                   &bitstream);
		long got_frames, i;
		int ch;

		if (ret == 0)        /* clean EOF */
			break;
		if (ret < 0)         /* OV_HOLE / OV_EBADLINK: skip the glitch */
			continue;

		/* Channel count can in principle change at a link boundary; always
		 * read it back so the downmix divisor matches this block. */
		vi = ov_info(&vf, -1);
		if (vi != NULL && vi->channels >= 1)
			channels = vi->channels;

		got_frames = ret / (2 * channels);
		if (got_frames <= 0)
			continue;

		if (frames + got_frames > mono_cap) {
			int new_cap = mono_cap ? mono_cap : OGG_GROW_FRAMES;
			int16_t *tmp;
			while (frames + got_frames > new_cap)
				new_cap *= 2;
			tmp = (int16_t *)realloc(mono,
			                         (size_t)new_cap * sizeof(int16_t));
			if (tmp == NULL) {
				free(mono);
				ov_clear(&vf);
				return NULL;
			}
			mono = tmp;
			mono_cap = new_cap;
		}

		/* Downmix this block to mono by averaging, matching the WAV loader. */
		if (channels == 1) {
			for (i = 0; i < got_frames; i++)
				mono[frames + i] = block[i];
		} else {
			for (i = 0; i < got_frames; i++) {
				long acc = 0;
				for (ch = 0; ch < channels; ch++)
					acc += block[i * channels + ch];
				mono[frames + i] = (int16_t)(acc / channels);
			}
		}
		frames += (int)got_frames;
	}

	ov_clear(&vf);

	if (mono == NULL || frames <= 0) {
		free(mono);
		return NULL;
	}

	/* The mixer only upsamples, so deliver PCM at the engine rate. */
	mono = resample_mono(mono, frames, rate, target_rate, &frames);
	if (mono == NULL)
		return NULL;

	if (out_rate)
		*out_rate = target_rate;
	if (out_samples)
		*out_samples = frames;
	return mono;
}
