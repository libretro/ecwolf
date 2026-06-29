#ifndef __CONFIG_TYPES_H__
#define __CONFIG_TYPES_H__

/* Vendored, hand-written replacement for the autotools/cmake-generated
 * config_types.h. ogg/os_types.h only falls through to this header on the
 * generic branch (i.e. not Windows/MinGW/Cygwin/Apple/Haiku/BeOS and the other
 * explicitly-handled toolchains).
 *
 * Use the same plain-int typing Tremor itself assumes rather than <stdint.h>'s
 * exact-width types: the decoder freely mixes `int` and `ogg_int32_t` (e.g. the
 * point/exponent locals in sharedbook.c's _book_unquantize that are passed to
 * the VFLOAT_* helpers taking ogg_int32_t*), so ogg_int32_t MUST be `int` for
 * that to type-check. On some targets <stdint.h> defines int32_t as `long`,
 * which is a distinct type from `int` even when both are 32-bit and breaks
 * those calls. `int` is 32-bit on every platform that reaches this branch, and
 * avoiding <stdint.h> also keeps the MSVC C89 path clean. */
typedef short          ogg_int16_t;
typedef unsigned short ogg_uint16_t;
typedef int            ogg_int32_t;
typedef unsigned int   ogg_uint32_t;
typedef long long      ogg_int64_t;
typedef unsigned long long ogg_uint64_t;

#endif
