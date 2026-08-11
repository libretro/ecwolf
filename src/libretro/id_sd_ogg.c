/* Shared Ogg Vorbis -> PCM decode helper. See id_sd_ogg.h.
 *
 * Decoding goes through libretro-common's rvorbis using its s16 pipeline,
 * which -- like the Tremor decoder this replaces -- runs the whole Vorbis
 * chain in integer/fixed-point arithmetic (Q20 residue front-end, Q28
 * inverse MDCT, Q27 windowing) and emits native-endian signed 16-bit PCM
 * directly. There is therefore still no float->int16 conversion anywhere on
 * this path and the output remains bit-identical across every platform the
 * core targets. The downstream mixer is int16 end to end.
 *
 * Known divergence from the Tremor path: chained (multi-link) Ogg streams
 * decode their first logical stream only, where Tremor followed link
 * boundaries (and this file re-read the channel count at each). Chained
 * files are vanishingly rare as game assets; recorded as accepted.
 */

#include "id_sd_ogg.h"

#include <stdlib.h>
#include <string.h>

#include "formats/rvorbis.h"

int OggIsOgg(const uint8_t *mem, size_t size)
{
	return mem != NULL && size >= 4 && memcmp(mem, "OggS", 4) == 0;
}

/* ---- integer linear resampler ------------------------------------------- */

/* Resample mono int16 PCM from in_rate to out_rate with 16.16 fixed-point
 * linear interpolation. Consumes (frees) src and returns a fresh buffer;
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
	rvorbis      *rv;
	rvorbis_info  vi;
	int           error = 0;
	int           channels, rate;
	int16_t      *mono = NULL;     /* growable mono accumulator */
	int           mono_cap = 0;    /* capacity in frames */
	int           frames = 0;      /* frames written so far */
	int16_t       block[4096];     /* interleaved decode scratch */

	if (!OggIsOgg(mem, size) || size > (size_t)0x7fffffff || target_rate <= 0)
		return NULL;

	rv = rvorbis_open_memory(mem, (int)size, &error, NULL);
	if (rv == NULL)
		return NULL;

	vi = rvorbis_get_info(rv);
	if (vi.channels < 1 || vi.sample_rate == 0) {
		rvorbis_close(rv);
		return NULL;
	}
	channels = vi.channels;
	rate     = (int)vi.sample_rate;

	for (;;) {
		/* Request whole frames only; the return value is frames per
		 * channel actually produced (0 at end of stream). */
		int want_shorts = (int)(sizeof(block) / sizeof(block[0]));
		int got_frames = rvorbis_get_samples_s16_interleaved(rv, channels,
		                                                     block,
		                                                     want_shorts);
		int i, ch;

		if (got_frames <= 0)   /* clean EOF (rvorbis skips holes itself) */
			break;

		if (frames + got_frames > mono_cap) {
			int new_cap = mono_cap ? mono_cap : OGG_GROW_FRAMES;
			int16_t *tmp;
			while (frames + got_frames > new_cap)
				new_cap *= 2;
			tmp = (int16_t *)realloc(mono,
			                         (size_t)new_cap * sizeof(int16_t));
			if (tmp == NULL) {
				free(mono);
				rvorbis_close(rv);
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
		frames += got_frames;
	}

	rvorbis_close(rv);

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
