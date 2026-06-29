#ifndef __CONFIG_TYPES_H__
#define __CONFIG_TYPES_H__

/* Vendored, hand-written replacement for the autotools/cmake-generated
 * config_types.h. ogg/os_types.h only falls through to this header on the
 * generic branch (i.e. not Windows/MinGW/Cygwin/Apple/Haiku/BeOS and the
 * other explicitly-handled toolchains); every such target the libretro core
 * builds for has a working <stdint.h>, so use it directly rather than relying
 * on a configure run. */
#include <stdint.h>

typedef int16_t  ogg_int16_t;
typedef uint16_t ogg_uint16_t;
typedef int32_t  ogg_int32_t;
typedef uint32_t ogg_uint32_t;
typedef int64_t  ogg_int64_t;
typedef uint64_t ogg_uint64_t;

#endif
