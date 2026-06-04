/*
** fbench.cpp - bit-exact + perf harness for the LIVE floor/ceiling drawer.
**
** R_DrawPlane() in wl_floorceiling.cpp is ECWolf's perspective floor/ceiling
** mapper, run twice per frame (floor + ceiling) over O(viewwidth*viewheight)
** pixels - the other half of the per-frame raster cost alongside ScalePost.
**
** This extracts the inner row/column loop (the useOptimized 64x64-flat fast
** path, which is the common Wolf3D case) with representative inputs, renders a
** screenful, and FNV-1a hashes the framebuffer so any optimization can be
** proven bit-identical and measured.
**
** Build: g++ -O2 -std=gnu++98 fbench.cpp -o fbench
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define FRACBITS 16
#define FRACUNIT (1<<FRACBITS)
#define TILESHIFT 16
typedef int32_t fixed;

#define SCRW 320
#define SCRH 200

static uint8_t  vbuf[SCRW*SCRH];
static int      viewwidth = SCRW;
static int      viewheight = SCRH;
static int      viewshift = 0;
static fixed    viewx = 5*FRACUNIT + 0x4000, viewy = 7*FRACUNIT + 0x2000;
static fixed    viewsin = 0x0000B505, viewcos = 0x0000B505; /* ~45deg */
static int      scale = 0x10000;
static uint8_t  flat64[64*64];     /* a 64x64 floor texture */
static uint8_t  curshades[256];    /* colormap row */

#define MINZ 1
static fixed FixedMul(fixed a, fixed b){ return (fixed)(((int64_t)a*b)>>FRACBITS); }

/* The useOptimized inner loop, extracted from R_DrawPlane. We isolate the
** per-row + per-pixel cost for a single textured plane; tile/map-lookup is
** modelled as "texture stays the same" (the common run between tile crossings)
** so we measure the steady-state pixel work the loop spends most time in. */
static void drawplane_base(int halfheight, fixed planeheight)
{
	const fixed heightFactor = (planeheight<0?-planeheight:planeheight)>>8;
	int y0 = ((24*heightFactor)>>FRACBITS) - (viewshift<0?-viewshift:viewshift);
	if (y0 > halfheight) return;
	if (y0 <= 0) y0 = 1;

	fixed planenumerator = FixedMul(0x7fff0000, planeheight);
	const int floor = planenumerator < 0;
	int tex_offsetPitch;
	uint8_t *tex_offset;
	if (floor) {
		tex_offset = vbuf + SCRW * (halfheight + y0);
		tex_offsetPitch = SCRW-viewwidth;
		planenumerator = -planenumerator;
	} else {
		tex_offset = vbuf + SCRW * (halfheight - y0 - 1);
		tex_offsetPitch = -viewwidth-SCRW;
	}

	const int viewxTile = viewx>>FRACBITS;
	const int viewxFrac = (viewx&(FRACUNIT-1))<<8;
	const int viewyTile = viewy>>FRACBITS;
	const int viewyFrac = (viewy&(FRACUNIT-1))<<8;

	static int wallclip[SCRW];
	for (int x = 0; x < viewwidth; ++x)
		wallclip[x] = 8 + ((x*5) % (halfheight)); /* varied wall clip */

	const uint8_t *tex = flat64;

	for (int y = y0; floor ? y+halfheight < viewheight : y < halfheight; ++y, tex_offset += tex_offsetPitch)
	{
		if (floor ? (y+halfheight < 0) : (y < halfheight - viewheight)) { tex_offset += viewwidth; continue; }

		fixed dist = (planenumerator / (y + 1))<<8;
		fixed gu =  viewxFrac + FixedMul(dist, viewcos);
		fixed gv = -viewyFrac + FixedMul(dist, viewsin);
		fixed tex_step = dist / scale;
		fixed du =  FixedMul(tex_step, viewsin);
		fixed dv = -FixedMul(tex_step, viewcos);
		gu -= (viewwidth >> 1) * du;
		gv -= (viewwidth >> 1) * dv;

		for (unsigned x = 0; x < (unsigned)viewwidth; ++x, ++tex_offset)
		{
			if (wallclip[x] <= y)
			{
				const int u = (gu>>18) & 63;
				const int v = (-gv>>18) & 63;
				const unsigned texoffs = (u * 64) + v;
				*tex_offset = curshades[tex[texoffs]];
			}
			gu += du;
			gv += dv;
		}
	}
}

static uint64_t fnv1a(const uint8_t *p, size_t n){uint64_t h=1469598103934665603ULL;for(size_t i=0;i<n;i++){h^=p[i];h*=1099511628173ULL;}return h;}
static void setup(unsigned seed){srand(seed);for(int i=0;i<64*64;i++)flat64[i]=rand()&0xFF;for(int i=0;i<256;i++)curshades[i]=(uint8_t)((i*5)&0xFF);memset(vbuf,0x33,sizeof(vbuf));}
static double now_sec(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}

static void bench(const char *name, void(*fn)(int,fixed), int iters, uint64_t *h){
	const int halfheight = (viewheight>>1) - viewshift;
	setup(7);
	fn(halfheight, 0x8000);              /* ceiling-ish */
	fn(halfheight, -0x8000);             /* floor-ish */
	*h = fnv1a(vbuf, sizeof(vbuf));
	double t0=now_sec();
	for(int i=0;i<iters;i++){ setup(7); fn(halfheight,0x8000); fn(halfheight,-0x8000); }
	double t1=now_sec();
	printf("  %-20s hash=%016llx  %8.2f ms (%d iters)\n", name,(unsigned long long)*h,(t1-t0)*1e3,iters);
}

int main(int argc,char**argv){
	int iters=(argc>1)?atoi(argv[1]):5000;
	uint64_t h0;
	printf("R_DrawPlane (live floor/ceiling) harness %dx%d, %d iters\n",SCRW,SCRH,iters);
	printf("--------------------------------------------------------------\n");
	bench("drawplane_base", drawplane_base, iters, &h0);
	printf("\nReference hash: %016llx\n",(unsigned long long)h0);
	printf("\nNote: like ScalePost, this loop is memory-latency-bound on the\n");
	printf("dependent curshades[tex[texoffs]] double-gather. -O2/-O3/-march are\n");
	printf("flat within noise, and a negate-hoisting restructure (tracking -gv\n");
	printf("to drop the per-pixel negate) was bit-exact but a regression. The\n");
	printf("loop is left as-is; the real wins already present are the wallclip[]\n");
	printf("per-column hoist and the useOptimized 64x64-flat fast path.\n");
	return 0;
}
