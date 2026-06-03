/* Shared Ogg Vorbis -> PCM decode helper. See id_sd_ogg.h. */

#include "id_sd_ogg.h"

#include <stdlib.h>
#include <string.h>

/* The one stb_vorbis entry point this core uses. Declared here to avoid
 * pulling in the full stb_vorbis header (and its stdio-conditional API);
 * the implementation is compiled in stb_vorbis_impl.c. Returns the number of
 * samples per channel, allocates an interleaved 16-bit buffer in *output, and
 * returns a negative value on error. */
extern int stb_vorbis_decode_memory(const unsigned char *mem, int len,
                                     int *channels, int *sample_rate,
                                     short **output);

int OggIsOgg(const uint8_t *mem, size_t size)
{
	return mem != NULL && size >= 4 && memcmp(mem, "OggS", 4) == 0;
}

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

int16_t *OggDecodeToMonoPCM(const uint8_t *mem, size_t size, int target_rate,
                            int *out_rate, int *out_samples)
{
	int channels = 0;
	int rate = 0;
	short *interleaved = NULL;
	int16_t *mono;
	int frames;

	if (!OggIsOgg(mem, size) || size > (size_t)0x7fffffff || target_rate <= 0)
		return NULL;

	/* frames = samples per channel; interleaved holds frames*channels shorts. */
	frames = stb_vorbis_decode_memory(mem, (int)size, &channels, &rate,
	                                  &interleaved);
	if (frames <= 0 || interleaved == NULL || channels < 1 || rate <= 0) {
		free(interleaved);
		return NULL;
	}

	if (channels == 1) {
		/* Already mono: reuse stb_vorbis's buffer. */
		mono = (int16_t *)interleaved;
	} else {
		/* Downmix to mono in place (output is never longer than input).
		 * Matches the stereo->mono averaging the WAV loader performs. */
		int i, c;
		mono = (int16_t *)interleaved;
		for (i = 0; i < frames; i++) {
			long acc = 0;
			for (c = 0; c < channels; c++)
				acc += interleaved[(size_t)i * channels + c];
			mono[i] = (int16_t)(acc / channels);
		}
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
