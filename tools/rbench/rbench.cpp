/*
** rbench.cpp - standalone bit-exact + perf harness for the ecwolf wall drawers
**
** The libretro-port wall renderer is now 100% portable C (the X86_ASM paths
** were removed in dec0d1c). This harness extracts the hot column/span drawers
** with their exact global-state ABI, drives them over a realistic
** 64x64-texture wall-rendering workload, FNV-1a hashes the output framebuffer
** for bit-exactness, and times each variant so optimizations can be proven
** identical-output and measured.
**
** Build:  g++ -O2 -std=gnu++98 rbench.cpp -o rbench
** Usage:  ./rbench [iters]
**
** Each drawer "variant" is compiled here from the same source shape as
** src/r_2d/r_draw.cpp so codegen comparisons (objdump) line up with the tree.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/* Drawer global-state ABI, mirrored verbatim from src/r_2d/r_draw.h  */
/* ------------------------------------------------------------------ */
typedef int32_t fixed_t;

uint8_t        *dc_dest;
int             dc_count;
int             dc_pitch;
fixed_t         dc_iscale;
fixed_t         dc_texturefrac;
const uint8_t  *dc_source;
uint8_t        *dc_colormap;

uint32_t        vplce[4];
uint32_t        vince[4];
uint8_t        *palookupoffse[4];
const uint8_t  *bufplce[4];

static int      vlinebits;
static int      mvlinebits;

/* ------------------------------------------------------------------ */
/* Baseline drawers - copied verbatim from src/r_2d/r_draw.cpp        */
/* (the current portable-C implementations, post-ASM-removal)         */
/* ------------------------------------------------------------------ */
uint32_t vlinec1_base(void)
{
	uint32_t fracstep = dc_iscale;
	uint32_t frac = dc_texturefrac;
	uint8_t *colormap = dc_colormap;
	int count = dc_count;
	const uint8_t *source = dc_source;
	uint8_t *dest = dc_dest;
	int bits = vlinebits;
	int pitch = dc_pitch;

	do
	{
		*dest = colormap[source[frac>>bits]];
		frac += fracstep;
		dest += pitch;
	} while (--count);

	return frac;
}

/* Verbatim from src/r_2d/r_draw.cpp. The engine guarantees place>>bits stays
** within the source column via the vince/start setup, not an in-loop mask, so
** the harness workload must honour that (see setup_workload). */
void vlinec4_base(void)
{
	uint8_t *dest = dc_dest;
	int count = dc_count;
	int bits = vlinebits;
	uint32_t place;

	do
	{
		dest[0] = palookupoffse[0][bufplce[0][(place=vplce[0])>>bits]]; vplce[0] = place+vince[0];
		dest[1] = palookupoffse[1][bufplce[1][(place=vplce[1])>>bits]]; vplce[1] = place+vince[1];
		dest[2] = palookupoffse[2][bufplce[2][(place=vplce[2])>>bits]]; vplce[2] = place+vince[2];
		dest[3] = palookupoffse[3][bufplce[3][(place=vplce[3])>>bits]]; vplce[3] = place+vince[3];
		dest += dc_pitch;
	} while (--count);
}

void mvlinec4_base(void)
{
	uint8_t *dest = dc_dest;
	int count = dc_count;
	int bits = mvlinebits;
	uint32_t place;

	do
	{
		uint8_t pix;
		pix = bufplce[0][(place=vplce[0])>>bits]; if(pix) dest[0] = palookupoffse[0][pix]; vplce[0] = place+vince[0];
		pix = bufplce[1][(place=vplce[1])>>bits]; if(pix) dest[1] = palookupoffse[1][pix]; vplce[1] = place+vince[1];
		pix = bufplce[2][(place=vplce[2])>>bits]; if(pix) dest[2] = palookupoffse[2][pix]; vplce[2] = place+vince[2];
		pix = bufplce[3][(place=vplce[3])>>bits]; if(pix) dest[3] = palookupoffse[3][pix]; vplce[3] = place+vince[3];
		dest += dc_pitch;
	} while (--count);
}

/* ------------------------------------------------------------------ */
/* Tier 1: latency-restructured drawers. Same arithmetic, but the four */
/* independent columns are split into phases - gather all four texels, */
/* then all four colormap lookups, then all four stores - so the       */
/* out-of-order core can overlap the four independent dependent-load   */
/* chains instead of serializing them. Bit-identical output.           */
/* ------------------------------------------------------------------ */
void vlinec4_t1(void)
{
	uint8_t *dest = dc_dest;
	int count = dc_count;
	int bits = vlinebits;
	uint32_t p0 = vplce[0], p1 = vplce[1], p2 = vplce[2], p3 = vplce[3];
	const uint32_t i0 = vince[0], i1 = vince[1], i2 = vince[2], i3 = vince[3];
	const uint8_t *b0 = bufplce[0], *b1 = bufplce[1], *b2 = bufplce[2], *b3 = bufplce[3];
	const uint8_t *c0 = palookupoffse[0], *c1 = palookupoffse[1], *c2 = palookupoffse[2], *c3 = palookupoffse[3];
	int pitch = dc_pitch;

	do
	{
		/* phase 1: four independent texture loads issue together */
		uint8_t t0 = b0[p0>>bits];
		uint8_t t1 = b1[p1>>bits];
		uint8_t t2 = b2[p2>>bits];
		uint8_t t3 = b3[p3>>bits];
		/* phase 2: four independent colormap loads */
		dest[0] = c0[t0];
		dest[1] = c1[t1];
		dest[2] = c2[t2];
		dest[3] = c3[t3];
		p0 += i0; p1 += i1; p2 += i2; p3 += i3;
		dest += pitch;
	} while (--count);

	vplce[0] = p0; vplce[1] = p1; vplce[2] = p2; vplce[3] = p3;
}

void mvlinec4_t1(void)
{
	uint8_t *dest = dc_dest;
	int count = dc_count;
	int bits = mvlinebits;
	uint32_t p0 = vplce[0], p1 = vplce[1], p2 = vplce[2], p3 = vplce[3];
	const uint32_t i0 = vince[0], i1 = vince[1], i2 = vince[2], i3 = vince[3];
	const uint8_t *b0 = bufplce[0], *b1 = bufplce[1], *b2 = bufplce[2], *b3 = bufplce[3];
	const uint8_t *c0 = palookupoffse[0], *c1 = palookupoffse[1], *c2 = palookupoffse[2], *c3 = palookupoffse[3];
	int pitch = dc_pitch;

	do
	{
		uint8_t t0 = b0[p0>>bits];
		uint8_t t1 = b1[p1>>bits];
		uint8_t t2 = b2[p2>>bits];
		uint8_t t3 = b3[p3>>bits];
		if (t0) dest[0] = c0[t0];
		if (t1) dest[1] = c1[t1];
		if (t2) dest[2] = c2[t2];
		if (t3) dest[3] = c3[t3];
		p0 += i0; p1 += i1; p2 += i2; p3 += i3;
		dest += pitch;
	} while (--count);

	vplce[0] = p0; vplce[1] = p1; vplce[2] = p2; vplce[3] = p3;
}

/* ------------------------------------------------------------------ */
/* FNV-1a over the framebuffer                                        */
/* ------------------------------------------------------------------ */
static uint64_t fnv1a(const uint8_t *p, size_t n)
{
	uint64_t h = 1469598103934665603ULL;
	size_t i;
	for (i = 0; i < n; ++i)
	{
		h ^= p[i];
		h *= 1099511628173ULL;
	}
	return h;
}

/* ------------------------------------------------------------------ */
/* Workload: a screen of wall columns. Wolf3D textures are 64x64, so  */
/* texture height is 64 (HeightBits=6) -> vlinebits is the fixed-point */
/* fraction width. Mirror the real setupvline(): frac is 16.16 into a  */
/* 64-tall texture column, so bits == FRACBITS == 16 in the tall path. */
/* ------------------------------------------------------------------ */
#define SCRW   320
#define SCRH   200
#define TEXH   64
#define TEXMASK (TEXH-1)
#define FRACBITS 16
/* Backing source columns are sized so place>>FRACBITS stays in range for all
** SCRH rows given the vince values chosen below (max index < COLBUF). */
#define COLBUF 512

static uint8_t  framebuffer[SCRW*SCRH];
static uint8_t  texcol[4][COLBUF];   /* 4 source texture columns */
static uint8_t  colormap[4][256];    /* 4 light/colormap tables  */

static void setup_workload(unsigned seed)
{
	unsigned i, c;
	srand(seed);
	for (c = 0; c < 4; ++c)
	{
		for (i = 0; i < COLBUF; ++i)
		{
			uint8_t v = (uint8_t)(rand() & 0xFF);
			/* ~25% transparent (index 0) for the masked path */
			if ((rand() & 3) == 0) v = 0;
			texcol[c][i] = v;
		}
		for (i = 0; i < 256; ++i)
			colormap[c][i] = (uint8_t)((i * (c+3)) & 0xFF);
	}
	/* Non-zero background so masked transparency produces a visibly distinct
	** framebuffer hash from the opaque path. */
	memset(framebuffer, 0x55, sizeof(framebuffer));
}

/* Render the whole screen in 4-column groups using a drawer fn. */
static void render_screen_4col(void (*draw4)(void))
{
	int x;
	dc_pitch = SCRW;
	vlinebits = FRACBITS;
	mvlinebits = FRACBITS;

	for (x = 0; x < SCRW; x += 4)
	{
		int c;
		for (c = 0; c < 4; ++c)
		{
			bufplce[c]       = texcol[c];
			palookupoffse[c] = colormap[c];
			/* start within first texel, step keeps max index
			** (SCRH-1)*step>>16 < COLBUF for all columns */
			vplce[c] = (uint32_t)(((x + c) & 7) << FRACBITS);
			vince[c] = (uint32_t)(0x18000 + ((x + c) & 15) * 0x800);
		}
		dc_dest  = framebuffer + x;
		dc_count = SCRH;
		draw4();
	}
}

static double now_sec(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec * 1e-9;
}

typedef void (*draw4_fn)(void);

static void bench(const char *name, draw4_fn fn, int iters, uint64_t *out_hash)
{
	int it;
	double t0, t1;
	setup_workload(1234);

	/* one render to capture the canonical hash */
	render_screen_4col(fn);
	*out_hash = fnv1a(framebuffer, sizeof(framebuffer));

	/* timed loop (re-setup each iter so the masked transparency pattern
	   and starting fractions are identical run to run) */
	t0 = now_sec();
	for (it = 0; it < iters; ++it)
	{
		setup_workload(1234);
		render_screen_4col(fn);
	}
	t1 = now_sec();

	printf("  %-22s  hash=%016llx  %8.2f ms  (%d iters, %.1f Mpix/s)\n",
		name, (unsigned long long)*out_hash,
		(t1 - t0) * 1e3, iters,
		(double)iters * SCRW * SCRH / (t1 - t0) / 1e6);
}

int main(int argc, char **argv)
{
	int iters = (argc > 1) ? atoi(argv[1]) : 2000;
	uint64_t h_v4, h_mv4;

	printf("ecwolf wall-drawer harness  (%dx%d, %d iters)\n", SCRW, SCRH, iters);
	printf("--------------------------------------------------------------\n");
	printf("BASELINE (current portable C from r_draw.cpp):\n");
	bench("vlinec4 (opaque)",  vlinec4_base,  iters, &h_v4);
	bench("mvlinec4 (masked)", mvlinec4_base, iters, &h_mv4);

	{
		uint64_t h_v4_t1, h_mv4_t1;
		printf("\nTIER 1 (latency-restructured, phased loads):\n");
		bench("vlinec4_t1 (opaque)",  vlinec4_t1,  iters, &h_v4_t1);
		bench("mvlinec4_t1 (masked)", mvlinec4_t1, iters, &h_mv4_t1);

		printf("\nBit-exactness check:\n");
		printf("  vlinec4  : %s\n", h_v4_t1  == h_v4  ? "MATCH" : "*** MISMATCH ***");
		printf("  mvlinec4 : %s\n", h_mv4_t1 == h_mv4 ? "MATCH" : "*** MISMATCH ***");
	}

	printf("\nReference hashes (any optimized variant must match):\n");
	printf("  vlinec4  = %016llx\n", (unsigned long long)h_v4);
	printf("  mvlinec4 = %016llx\n", (unsigned long long)h_mv4);
	return 0;
}
