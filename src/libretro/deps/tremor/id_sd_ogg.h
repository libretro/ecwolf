/* Shared Ogg Vorbis -> PCM decode helper for the libretro ECWolf core.
 *
 * Both the digital-sound path (Mix_Chunk_Digital::loadSound) and the music
 * path (SynthesizeAdlibIMFOrN3D) accept Ogg Vorbis lumps; this is the single
 * decoder both call so the decode/downmix logic lives in one place.
 */

#ifndef ECWOLF_ID_SD_OGG_H
#define ECWOLF_ID_SD_OGG_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returns true if the buffer begins with the Ogg capture pattern ("OggS"). */
int OggIsOgg(const uint8_t *mem, size_t size);

/* Decode an in-memory Ogg Vorbis stream to mono 16-bit native-endian PCM,
 * resampled to target_rate.
 *
 * The core's software mixer only upsamples (it cannot downsample), and Ogg
 * files are commonly 44.1 or 48 kHz, so the decoder resamples to the mixer's
 * fixed rate here. Pass the engine sample rate (44100) as target_rate; the
 * returned chunk then mixes through the equal-rate fast path.
 *
 * On success returns a malloc()'d buffer of (*out_samples) int16_t samples at
 * target_rate (caller frees with free()) and writes target_rate to
 * (*out_rate); returns NULL on any failure (not Ogg, decode error, empty
 * stream, allocation failure) leaving the outputs untouched. Multi-channel
 * input is downmixed to mono to match the core's mono mixing path.
 */
int16_t *OggDecodeToMonoPCM(const uint8_t *mem, size_t size, int target_rate,
                            int *out_rate, int *out_samples);

#ifdef __cplusplus
}
#endif

#endif /* ECWOLF_ID_SD_OGG_H */
