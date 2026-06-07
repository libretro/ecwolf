/*
** r_halo.cpp - halo lighting implementation (see r_halo.h).
**
** Written C-style (flat growable arrays, no templates/STL) to suit the C89
** migration, though it is compiled as C++ for now because it reads AActor
** state (actor.h is among the last things to be de-C++'d).
*/

#include <stdlib.h>
#include <math.h>

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

static void Halo_PushActive(double cx, double cy, double radius, int light)
{
	if (s_nactive == s_activecap)
	{
		int newcap = s_activecap ? s_activecap * 2 : 16;
		haloinst_t *p = (haloinst_t *)realloc(s_active, newcap * sizeof(haloinst_t));
		if (!p)
			return;
		s_active = p;
		s_activecap = newcap;
	}
	s_active[s_nactive].cx     = cx;
	s_active[s_nactive].cy     = cy;
	s_active[s_nactive].radius = radius;
	s_active[s_nactive].r2     = radius * radius;
	s_active[s_nactive].light  = light;
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

	if (s_nactive == 0)
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
	return s_zoneany;
}
