/* Single translation unit that compiles the vendored stb_vorbis decoder.
 *
 * The upstream stb_vorbis.c is kept pristine (so it can be updated by simply
 * dropping in a newer copy); all build-time configuration lives here.
 *
 * We only ever decode Ogg Vorbis from an in-memory lump, so the stdio/file
 * pull API is disabled. The push API is disabled too; the libretro core uses
 * the one-shot stb_vorbis_decode_memory() entry point exclusively.
 *
 * Symbol isolation: libretro-common's audio_mixer.c embeds its own private
 * copy of the same stb_vorbis decoder. On platforms that link the core into a
 * single static image alongside libretro-common (Emscripten/wasm-ld, Vita and
 * other console toolchains), the two copies collide at link time with
 * "multiple definition" errors. Most native linkers silently pick one copy,
 * which is why this only surfaces on statically-linked targets.
 *
 * To avoid the clash without modifying the vendored decoder, every public
 * stb_vorbis_* function is macro-renamed to an ecwolf_-prefixed name before the
 * decoder is included, giving the whole API internal-to-this-core linkage. The
 * single entry point the core actually uses is then re-exported below under a
 * stable name for id_sd_ogg.c. Keep this list in sync if the decoder is
 * updated and gains new public functions.
 */

#define STB_VORBIS_NO_STDIO 1
#define STB_VORBIS_NO_PUSHDATA_API 1

/* Namespace every public API function so it cannot collide with the copy of
 * stb_vorbis that lives inside libretro-common's audio_mixer translation unit. */
#define stb_vorbis_close                          ecwolf_stb_vorbis_close
#define stb_vorbis_decode_filename                ecwolf_stb_vorbis_decode_filename
#define stb_vorbis_decode_frame_pushdata          ecwolf_stb_vorbis_decode_frame_pushdata
#define stb_vorbis_decode_memory                  ecwolf_stb_vorbis_decode_memory
#define stb_vorbis_flush_pushdata                 ecwolf_stb_vorbis_flush_pushdata
#define stb_vorbis_get_comment                    ecwolf_stb_vorbis_get_comment
#define stb_vorbis_get_error                      ecwolf_stb_vorbis_get_error
#define stb_vorbis_get_file_offset                ecwolf_stb_vorbis_get_file_offset
#define stb_vorbis_get_frame_float                ecwolf_stb_vorbis_get_frame_float
#define stb_vorbis_get_frame_short                ecwolf_stb_vorbis_get_frame_short
#define stb_vorbis_get_frame_short_interleaved    ecwolf_stb_vorbis_get_frame_short_interleaved
#define stb_vorbis_get_info                       ecwolf_stb_vorbis_get_info
#define stb_vorbis_get_sample_offset              ecwolf_stb_vorbis_get_sample_offset
#define stb_vorbis_get_samples_float              ecwolf_stb_vorbis_get_samples_float
#define stb_vorbis_get_samples_float_interleaved  ecwolf_stb_vorbis_get_samples_float_interleaved
#define stb_vorbis_get_samples_short              ecwolf_stb_vorbis_get_samples_short
#define stb_vorbis_get_samples_short_interleaved  ecwolf_stb_vorbis_get_samples_short_interleaved
#define stb_vorbis_open_memory                    ecwolf_stb_vorbis_open_memory
#define stb_vorbis_seek                           ecwolf_stb_vorbis_seek
#define stb_vorbis_seek_frame                     ecwolf_stb_vorbis_seek_frame
#define stb_vorbis_seek_start                     ecwolf_stb_vorbis_seek_start
#define stb_vorbis_stream_length_in_samples       ecwolf_stb_vorbis_stream_length_in_samples
#define stb_vorbis_stream_length_in_seconds       ecwolf_stb_vorbis_stream_length_in_seconds

/* Quiet a few -Wall warnings that the upstream single-file decoder is known to
 * emit under strict flags; these are in third-party code we do not modify. */
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#include "stb_vorbis.c"

/* Undo the rename for the one entry point the core consumes, then provide a
 * real external definition that forwards to the namespaced implementation.
 * id_sd_ogg.c declares this prototype itself, so no header is required. */
#undef stb_vorbis_decode_memory

int stb_vorbis_decode_memory(const unsigned char *mem, int len,
                             int *channels, int *sample_rate, short **output)
{
   return ecwolf_stb_vorbis_decode_memory(mem, len, channels, sample_rate,
                                          output);
}
