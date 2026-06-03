/* Single translation unit that compiles the vendored stb_vorbis decoder.
 *
 * The upstream stb_vorbis.c is kept pristine (so it can be updated by simply
 * dropping in a newer copy); all build-time configuration lives here.
 *
 * We only ever decode Ogg Vorbis from an in-memory lump, so the stdio/file
 * pull API is disabled. The push API is disabled too; the libretro core uses
 * the one-shot stb_vorbis_decode_memory() entry point exclusively.
 */

#define STB_VORBIS_NO_STDIO 1
#define STB_VORBIS_NO_PUSHDATA_API 1

/* Quiet a few -Wall warnings that the upstream single-file decoder is known to
 * emit under strict flags; these are in third-party code we do not modify. */
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#include "stb_vorbis.c"
