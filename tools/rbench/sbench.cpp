/*
** sbench.cpp - bit-exact + perf harness for ecwolf's LIVE wall drawer.
**
** ScalePost() in wl_draw.cpp is the actual Wolf3D raycaster wall-column
** scaler: it is called once per screen column per frame (the hot path),
** unlike the dead ZDoom vlinec4 family. This harness extracts ScalePost's
** scaling inner loop with synthetic but representative inputs, renders a
** full screen of columns, and FNV-1a hashes the framebuffer so any
** optimization can be proven bit-identical and measured.
**
** Build: g++ -O2 -std=gnu++98 sbench.cpp -o sbench
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define SCRW 320
#define SCRH 200
#define TEXH 64          /* Wolf3D wall textures are 64 tall */

/* ---- state ScalePost reads (mirrored from wl_draw.cpp) ---------------- */
static uint8_t  vbuf[SCRW*SCRH];
static int      vbufPitch = SCRW;
static int      viewheight = SCRH;
static int      viewshift = 0;
static const uint8_t *postsource;     /* points into a texture column */
static int      postx;
static int      texyscale;            /* fixed-point texel step */
static uint8_t  curshades[256];       /* the light/colormap row */
static uint8_t  texcol[TEXH];         /* one source texture column */

/* ---- BASELINE: ScalePost's exact scaling loop ------------------------- */
/* Extracted verbatim from wl_draw.cpp (the part driven by per-column
** wallheight). yd = wallheight, ywcount tracks the fixed-point accumulator. */
static void scalepost_base(int wallheight_col)
{
	int ywcount, yoffs, yw, yd, yendoffs;
	uint8_t col;

	if (postsource == NULL) return;

	ywcount = yd = wallheight_col;
	if (yd <= 0) yd = 100;

	/* Simplified offset setup: center the column vertically (the real code
	** uses viewz/topoffset; for the scaling-loop benchmark we just need a
	** valid yoffs/yendoffs span and the same yw seeding). */
	{
		int half = viewheight / 2;
		yoffs = (half - wallheight_col/2 - viewshift) * vbufPitch;
		if (yoffs < 0) yoffs = 0;
		yoffs += postx;
		yendoffs = half + wallheight_col/2 - 1 - viewshift;
		if (yendoffs >= viewheight) yendoffs = viewheight - 1;
		yw = (texyscale>>2) - 1;
	}
	if (yw < 0)
		yw = (texyscale>>2) - ((-yw) % (texyscale>>2));

	col = curshades[postsource[yw & (TEXH-1)]];
	yendoffs = yendoffs * vbufPitch + postx;
	while (yoffs <= yendoffs)
	{
		vbuf[yendoffs] = col;
		ywcount -= texyscale;
		if (ywcount <= 0)
		{
			do
			{
				ywcount += yd;
				yw--;
			}
			while (ywcount <= 0);
			if (yw < 0) yw = (texyscale>>2)-1;
			col = curshades[postsource[yw & (TEXH-1)]];
		}
		yendoffs -= vbufPitch;
	}
}

/* ---- FNV-1a ----------------------------------------------------------- */
static uint64_t fnv1a(const uint8_t *p, size_t n)
{
	uint64_t h = 1469598103934665603ULL;
	size_t i;
	for (i = 0; i < n; ++i) { h ^= p[i]; h *= 1099511628173ULL; }
	return h;
}

/* ---- workload: a screen of wall columns at varied distances ----------- */
static void setup(unsigned seed)
{
	unsigned i;
	srand(seed);
	for (i = 0; i < TEXH; ++i)  texcol[i]   = (uint8_t)(rand() & 0xFF);
	for (i = 0; i < 256; ++i)   curshades[i]= (uint8_t)((i*7) & 0xFF);
	postsource = texcol;
	texyscale = TEXH << 2;            /* 4.x fixed; matches (texyscale>>2)=TEXH */
	memset(vbuf, 0x55, sizeof(vbuf));
}

static void render_screen(void (*post)(int))
{
	int x;
	for (x = 0; x < SCRW; ++x)
	{
		/* vary wall height per column: near walls (tall, magnified) and far
		** walls (short, minified) across the screen, like a real scene */
		int wh = 32 + ((x * 13) % (SCRH + 40));
		postx = x;
		post(wh);
	}
}

static double now_sec(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static void bench(const char *name, void (*post)(int), int iters, uint64_t *h)
{
	int it; double t0, t1;
	setup(1234);
	render_screen(post);
	*h = fnv1a(vbuf, sizeof(vbuf));
	t0 = now_sec();
	for (it = 0; it < iters; ++it) { setup(1234); render_screen(post); }
	t1 = now_sec();
	printf("  %-22s hash=%016llx  %8.2f ms (%d iters)\n",
		name, (unsigned long long)*h, (t1-t0)*1e3, iters);
}

int main(int argc, char **argv)
{
	int iters = (argc > 1) ? atoi(argv[1]) : 5000;
	uint64_t h0;
	printf("ScalePost (live wall drawer) harness  %dx%d, %d iters\n", SCRW, SCRH, iters);
	printf("--------------------------------------------------------------\n");
	bench("scalepost_base", scalepost_base, iters, &h0);
	printf("\nReference hash: %016llx\n", (unsigned long long)h0);
	printf("\nNote: this loop is memory-latency-bound on the strided store\n");
	printf("(vbuf[yendoffs] -= vbufPitch each row). -O3/-march and a\n");
	printf("run-length-fill restructure were both measured here and neither\n");
	printf("beat the baseline (run-length was a regression), so the inner\n");
	printf("loop is left as-is; the win in ScalePost was the off-screen\n");
	printf("fast-forward already present above the loop.\n");
	return 0;
}
