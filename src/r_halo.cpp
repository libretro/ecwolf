/*
** r_halo.cpp - halo lighting implementation (see r_halo.h).
**
** Written C-style (flat growable arrays, no templates/STL) to suit the C89
** migration, though it is compiled as C++ for now because it reads AActor
** state (actor.h is among the last things to be de-C++'d).
*/

#include <stdlib.h>
#include <math.h>
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#endif

#include "r_halo.h"
#include "actor.h"
#include "thingdef/thingdef.h"

/* ---- class-declared halo lists (array of flat arrays) ------------------- */

typedef struct
{
	halodef_t *defs;
	int        count;
	int        cap;
} halolist_t;

static halolist_t *s_lists  = NULL;
static int         s_nlists = 0;
static int         s_listcap = 0;

/* Display Options toggle; default on. */
bool r_halolighting = true;

int Halo_NewList(void)
{
	if (s_nlists == s_listcap)
	{
		int newcap = s_listcap ? s_listcap * 2 : 8;
		halolist_t *p = (halolist_t *)realloc(s_lists, newcap * sizeof(halolist_t));
		if (!p)
			return -1;
		s_lists = p;
		s_listcap = newcap;
	}
	s_lists[s_nlists].defs  = NULL;
	s_lists[s_nlists].count = 0;
	s_lists[s_nlists].cap   = 0;
	return s_nlists++;
}

void Halo_ListAdd(int list, int id, int light, double radius)
{
	halolist_t *L;
	if (list < 0 || list >= s_nlists)
		return;
	L = &s_lists[list];
	if (L->count == L->cap)
	{
		int newcap = L->cap ? L->cap * 2 : 4;
		halodef_t *p = (halodef_t *)realloc(L->defs, newcap * sizeof(halodef_t));
		if (!p)
			return;
		L->defs = p;
		L->cap = newcap;
	}
	L->defs[L->count].id     = id;
	L->defs[L->count].light  = light;
	L->defs[L->count].radius = radius;
	L->count++;
}

/* ---- per-frame active halos --------------------------------------------- */

static haloinst_t *s_active   = NULL;
static int         s_nactive  = 0;
static int         s_activecap = 0;

/* Structure-of-arrays mirror of s_active, kept contiguous so the per-row span
** kernel (Halo_RowSpans) can load halo fields with packed SSE2 loads instead
** of gathering from the AoS struct. Grown in lockstep with s_active. */
static double *s_cx    = NULL;
static double *s_cy    = NULL;
static double *s_r2    = NULL;
static int    *s_lit   = NULL;

static void Halo_PushActive(double cx, double cy, double radius, int light)
{
	if (s_nactive == s_activecap)
	{
		int newcap = s_activecap ? s_activecap * 2 : 16;
		haloinst_t *p = (haloinst_t *)realloc(s_active, newcap * sizeof(haloinst_t));
		double *pcx, *pcy, *pr2;
		int *plit;
		if (!p)
			return;
		s_active = p;
		pcx = (double *)realloc(s_cx, newcap * sizeof(double));
		pcy = (double *)realloc(s_cy, newcap * sizeof(double));
		pr2 = (double *)realloc(s_r2, newcap * sizeof(double));
		plit = (int *)realloc(s_lit, newcap * sizeof(int));
		if (!pcx || !pcy || !pr2 || !plit)
			return;
		s_cx = pcx;
		s_cy = pcy;
		s_r2 = pr2;
		s_lit = plit;
		s_activecap = newcap;
	}
	s_active[s_nactive].cx     = cx;
	s_active[s_nactive].cy     = cy;
	s_active[s_nactive].radius = radius;
	s_active[s_nactive].r2     = radius * radius;
	s_active[s_nactive].light  = light;
	s_cx[s_nactive]  = cx;
	s_cy[s_nactive]  = cy;
	s_r2[s_nactive]  = radius * radius;
	s_lit[s_nactive] = light;
	s_nactive++;
}

void Halo_Populate(void)
{
	s_nactive = 0;

	for (AActor::Iterator check = AActor::GetIterator(); check.Next();)
	{
		int listIndex = check->GetClass()->Meta.GetMetaInt(AMETA_HaloLights, -1);
		if (listIndex < 0 || listIndex >= s_nlists)
			continue;

		{
			const halolist_t *L = &s_lists[listIndex];
			int i;
			for (i = 0; i < L->count; ++i)
			{
				const halodef_t *d = &L->defs[i];
				if ((check->haloLightMask & (1 << d->id)) == 0)
					continue;
				if (d->light == 0 || d->radius <= 0.0)
					continue;
				Halo_PushActive(FIXED2FLOAT(check->x), FIXED2FLOAT(check->y),
					d->radius, d->light << 3);
			}
		}
	}
}

int Halo_ActiveCount(void)
{
	if (!r_halolighting)
		return 0;
	return s_nactive;
}

haloinst_t *Halo_Active(int index)
{
	if (index < 0 || index >= s_nactive)
		return NULL;
	return &s_active[index];
}

int Halo_LightAtFixed(fixed xintercept, fixed yintercept)
{
	int light = 0;
	int i;
	double px, py;

	if (s_nactive == 0 || !r_halolighting)
		return 0;

	px = FIXED2FLOAT(xintercept);
	py = FIXED2FLOAT(yintercept);

	for (i = 0; i < s_nactive; ++i)
	{
		const haloinst_t *h = &s_active[i];
		double dx = px - h->cx;
		double dy = py - h->cy;
		if (dx * dx + dy * dy <= h->r2)
			light += h->light;
	}
	return light;
}

/*
** Per-scanline halo span accumulation (floor/ceiling drawer).
**
** For the ray P(t) = S + V*t, t in [0,1], find for each active halo the
** screen-column interval where the ray is inside the halo circle, and add the
** halo's light to halolight[] across that interval. This is the dominant cost
** in the floor/ceiling stage when lamps are in view (one quadratic solve per
** halo per scanline), so the per-halo discriminant/root math is done two halos
** at a time with SSE2 packed doubles. The span fill stays scalar (it is
** data-dependent) and the output is bit-identical to the scalar reference:
** the packed path performs the same operations (b = 2(V.sc), c = sc.sc - r2,
** disc = b*b - 4ac, roots (-b -+ sqrt)/2a) so each lane matches the scalar
** result exactly.
*/
void Halo_RowSpans(int *halolight, int viewwidth,
	double Sx, double Sy, double Vx, double Vy, double a)
{
	int hi = 0;

	if (a <= 0.0)
		return;

#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
	{
		const __m128d vSx = _mm_set1_pd(Sx);
		const __m128d vSy = _mm_set1_pd(Sy);
		const __m128d vVx = _mm_set1_pd(Vx);
		const __m128d vVy = _mm_set1_pd(Vy);
		const __m128d vTwo = _mm_set1_pd(2.0);
		const __m128d v4a = _mm_set1_pd(4.0 * a);
		const __m128d v2a = _mm_set1_pd(2.0 * a);

		for (; hi + 1 < s_nactive; hi += 2)
		{
			/* Load two halos' centers/r2 (contiguous SoA). */
			__m128d cx = _mm_loadu_pd(&s_cx[hi]);
			__m128d cy = _mm_loadu_pd(&s_cy[hi]);
			__m128d r2 = _mm_loadu_pd(&s_r2[hi]);

			__m128d scx = _mm_sub_pd(vSx, cx);
			__m128d scy = _mm_sub_pd(vSy, cy);
			/* b = 2*(Vx*scx + Vy*scy) */
			__m128d b = _mm_mul_pd(vTwo,
				_mm_add_pd(_mm_mul_pd(vVx, scx), _mm_mul_pd(vVy, scy)));
			/* c = scx*scx + scy*scy - r2 */
			__m128d c = _mm_sub_pd(
				_mm_add_pd(_mm_mul_pd(scx, scx), _mm_mul_pd(scy, scy)), r2);
			/* disc = b*b - 4a*c */
			__m128d disc = _mm_sub_pd(_mm_mul_pd(b, b), _mm_mul_pd(v4a, c));

			/* Common case: neither lane's ray crosses this halo on this row.
			** Skip the sqrt/divide/store entirely - this is what keeps the
			** packed path ahead of scalar, since sqrt is the dominant cost and
			** most halo/scanline pairs do not intersect. */
			if (_mm_movemask_pd(_mm_cmpgt_pd(disc, _mm_setzero_pd())) == 0)
				continue;

			/* At least one lane intersects. Sqrt both (the masked-out lane's
			** NaN is harmless - it is dropped by the per-lane disc>0 test).
			** Division (not reciprocal-multiply) matches scalar rounding. */
			{
			__m128d sq = _mm_sqrt_pd(disc);
			__m128d nb = _mm_sub_pd(_mm_setzero_pd(), b);
			__m128d t1v = _mm_div_pd(_mm_sub_pd(nb, sq), v2a);
			__m128d t2v = _mm_div_pd(_mm_add_pd(nb, sq), v2a);

			{
				double t1a[2], t2a[2], da[2];
				int lane;
				_mm_storeu_pd(da, disc);
				_mm_storeu_pd(t1a, t1v);
				_mm_storeu_pd(t2a, t2v);
				for (lane = 0; lane < 2; ++lane)
				{
					double t1, t2;
					int x1, x2, hx, lit;
					if (!(da[lane] > 0.0))
						continue;
					t1 = t1a[lane];
					t2 = t2a[lane];
					if (t1 < 0.0) t1 = 0.0;
					if (t2 > 1.0) t2 = 1.0;
					x1 = (int)(t1 * viewwidth);
					x2 = (int)(t2 * viewwidth);
					if (x1 < 0) x1 = 0;
					if (x2 > viewwidth) x2 = viewwidth;
					lit = s_lit[hi + lane];
					for (hx = x1; hx < x2; ++hx)
						halolight[hx] += lit;
				}
			}
			}
		}
	}
#endif

	/* Scalar tail (and the whole loop when SSE2 is unavailable). Identical math
	** to the packed path so results match lane-for-lane. */
	for (; hi < s_nactive; ++hi)
	{
		double scx = Sx - s_cx[hi];
		double scy = Sy - s_cy[hi];
		double b = 2.0 * (Vx * scx + Vy * scy);
		double c = (scx * scx + scy * scy) - s_r2[hi];
		double disc = b * b - 4.0 * a * c;
		if (disc > 0.0)
		{
			double sq = sqrt(disc);
			double t1 = (-b - sq) / (2.0 * a);
			double t2 = (-b + sq) / (2.0 * a);
			int x1, x2, hx, lit;
			if (t1 < 0.0) t1 = 0.0;
			if (t2 > 1.0) t2 = 1.0;
			x1 = (int)(t1 * viewwidth);
			x2 = (int)(t2 * viewwidth);
			if (x1 < 0) x1 = 0;
			if (x2 > viewwidth) x2 = viewwidth;
			lit = s_lit[hi];
			for (hx = x1; hx < x2; ++hx)
				halolight[hx] += lit;
		}
	}
}

/* ---- zone lighting ------------------------------------------------------ */

#include "gamemap.h"

extern GameMap *map;

typedef struct
{
	zonedef_t *defs;
	int        count;
	int        cap;
} zonelist_t;

static zonelist_t *s_zlists  = NULL;
static int         s_nzlists = 0;
static int         s_zlistcap = 0;

int Zone_NewList(void)
{
	if (s_nzlists == s_zlistcap)
	{
		int newcap = s_zlistcap ? s_zlistcap * 2 : 8;
		zonelist_t *p = (zonelist_t *)realloc(s_zlists, newcap * sizeof(zonelist_t));
		if (!p)
			return -1;
		s_zlists = p;
		s_zlistcap = newcap;
	}
	s_zlists[s_nzlists].defs  = NULL;
	s_zlists[s_nzlists].count = 0;
	s_zlists[s_nzlists].cap   = 0;
	return s_nzlists++;
}

void Zone_ListAdd(int list, int id, int light)
{
	zonelist_t *L;
	if (list < 0 || list >= s_nzlists)
		return;
	L = &s_zlists[list];
	if (L->count == L->cap)
	{
		int newcap = L->cap ? L->cap * 2 : 4;
		zonedef_t *p = (zonedef_t *)realloc(L->defs, newcap * sizeof(zonedef_t));
		if (!p)
			return;
		L->defs = p;
		L->cap = newcap;
	}
	L->defs[L->count].id    = id;
	L->defs[L->count].light = light;
	L->count++;
}

/* Per-zone-index light accumulator for the current frame. */
static int *s_zonelight   = NULL;
static int  s_zonelightcap = 0;
static int  s_zonecount    = 0;
static int  s_zoneany      = 0;   /* nonzero if any zone has light this frame */

void Zone_Populate(int zoneCount)
{
	int i;
	unsigned int mapwidth, mapheight;

	s_zonecount = zoneCount;
	s_zoneany = 0;
	if (zoneCount <= 0)
		return;

	if (zoneCount > s_zonelightcap)
	{
		int *p = (int *)realloc(s_zonelight, zoneCount * sizeof(int));
		if (!p)
		{
			s_zonecount = 0;
			return;
		}
		s_zonelight = p;
		s_zonelightcap = zoneCount;
	}
	for (i = 0; i < zoneCount; ++i)
		s_zonelight[i] = 0;

	mapwidth  = map->GetHeader().width;
	mapheight = map->GetHeader().height;

	for (AActor::Iterator check = AActor::GetIterator(); check.Next();)
	{
		int listIndex = check->GetClass()->Meta.GetMetaInt(AMETA_ZoneLights, -1);
		if (listIndex < 0 || listIndex >= s_nzlists)
			continue;

		{
			const zonelist_t *L = &s_zlists[listIndex];
			unsigned int curx = (unsigned int)(check->x >> TILESHIFT);
			unsigned int cury = (unsigned int)(check->y >> TILESHIFT);
			MapSpot spot = map->GetSpot(curx % mapwidth, cury % mapheight, 0);
			int zi;
			if (spot->zone == NULL)
				continue;
			zi = spot->zone->index;
			if (zi < 0 || zi >= zoneCount)
				continue;
			{
				int k;
				for (k = 0; k < L->count; ++k)
				{
					const zonedef_t *d = &L->defs[k];
					if ((check->zoneLightMask & (1 << d->id)) == 0)
						continue;
					if (d->light == 0)
						continue;
					s_zonelight[zi] += d->light << 3;
					s_zoneany = 1;
				}
			}
		}
	}
}

int Zone_LightForIndex(int zoneIndex)
{
	if (zoneIndex < 0 || zoneIndex >= s_zonecount || s_zonelight == NULL)
		return 0;
	return s_zonelight[zoneIndex];
}

int Zone_AnyActive(void)
{
	if (!r_halolighting)
		return 0;
	return s_zoneany;
}
