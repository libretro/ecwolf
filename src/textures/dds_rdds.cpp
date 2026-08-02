// dds_rdds.cpp
//
// See dds_rdds.h.  Depends only on the vendored libretro-common rdds
// decoder (and the C runtime) so it links unchanged into both ecwolf and
// the standalone sanitizer harness.

#include "dds_rdds.h"

#include <stdlib.h>

#include <formats/image.h>
#include <formats/rdds.h>

uint8_t *DDS_DecodeRGBA(const void *data, size_t size,
                        unsigned *w, unsigned *h)
{
	rdds_t   *hd  = rdds_alloc();
	void     *out = NULL;
	unsigned  dw  = 0, dh = 0;
	int       r;

	if (!hd)
		return NULL;

	// rdds reads through the pointer; it does not take ownership of the
	// input bytes, which stay valid for the whole decode.
	if (!rdds_set_buf_ptr(hd, (void *)data))
	{
		rdds_free(hd);
		return NULL;
	}

	do
	{
		r = rdds_process_image(hd, &out, size, &dw, &dh, true);
	}
	while (r == IMAGE_PROCESS_NEXT);

	rdds_free(hd);

	if (r != IMAGE_PROCESS_END || !out)
	{
		free(out);
		return NULL;
	}

	if (w)
		*w = dw;
	if (h)
		*h = dh;
	return (uint8_t *)out;
}

void DDS_PalettizeColumnMajor(const uint8_t *rgba, unsigned w, unsigned h,
                              const uint8_t *rgb32k, uint8_t *pixels,
                              int *masked)
{
	unsigned x, y;
	int      m = 0;

	for (x = 0; x < w; ++x)
	{
		for (y = 0; y < h; ++y)
		{
			const uint8_t *p = rgba + 4 * ((size_t)y * w + x);
			uint8_t        a = p[3];

			if (a < 128)
			{
				pixels[(size_t)x * h + y] = 0;
				m = 1;
			}
			else
				pixels[(size_t)x * h + y] =
					rgb32k[(((p[0] >> 3) * 32) + (p[1] >> 3)) * 32
					       + (p[2] >> 3)];
		}
	}

	if (masked)
		*masked = m;
}
