// dds_rdds.h
//
// Thin, engine-independent glue between ecwolf's FDDSTexture and
// RetroArch libretro-common's rdds DDS decoder.  Kept free of ecwolf
// headers so the exact code that ships can also be exercised directly
// by a standalone sanitizer harness.

#ifndef ECWOLF_DDS_RDDS_H
#define ECWOLF_DDS_RDDS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Decode mip level 0 of a DDS image (raw file bytes, including the
// 128-byte header) to a freshly malloc()'d, row-major RGBA8 buffer
// (memory byte order R,G,B,A).  Returns NULL on any decode/parse error.
// On success stores the base dimensions in *w and *h; the caller owns
// the buffer and must release it with free().
uint8_t *DDS_DecodeRGBA(const void *data, size_t size,
                        unsigned *w, unsigned *h);

// Convert a row-major RGBA8 image into ecwolf's column-major, 8-bit
// paletted Pixels layout (pixels[x*h + y]).  'rgb32k' is the engine's
// 15-bit RGB -> palette-index lookup, flattened to 32*32*32 entries and
// indexed as rgb32k[((r>>3)*32 + (g>>3))*32 + (b>>3)].  A texel whose
// alpha is below 128 becomes palette index 0 and sets *masked (this
// mirrors ecwolf's original top-alpha-bit test for 1/4/8-bit alpha).
// pixels must hold w*h bytes.
void DDS_PalettizeColumnMajor(const uint8_t *rgba, unsigned w, unsigned h,
                              const uint8_t *rgb32k, uint8_t *pixels,
                              int *masked);

#ifdef __cplusplus
}
#endif

#endif
