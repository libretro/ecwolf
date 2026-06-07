/*
** r_halo.h - halo lighting (radial actor light), backported from LZWolf.
**
** A "halo" is a circular region of boosted light level centered on an actor.
** An actor class declares one or more halos via the DECORATE "halolight"
** property; each is identified by an id (0..31) and is toggled at runtime per
** actor instance via A_EnableHaloLight, gated through the actor's
** haloLightMask bitfield.
**
** This is the C89-style port: no templates, no STL. Class-declared halo
** definitions live in a single flat growable array; each actor class records
** a [start,count) slice of it via the AMETA_HaloLights class-meta int. Per
** frame the renderer collects the active halos (those whose actor has the
** corresponding mask bit set) into a second flat array, which the floor/
** ceiling drawer queries with a ray-circle intersection per scanline.
**
** The colored-light ("littype"/Lit) extension from LZWolf is intentionally
** omitted; halos here are monochrome light-level boosts.
*/

#ifndef __R_HALO_H__
#define __R_HALO_H__

#include "wl_def.h"

/* A halo declared on an actor class (template definition). */
typedef struct
{
	int    id;        /* 0..31, indexes the per-actor haloLightMask */
	int    light;     /* light-level boost when active */
	double radius;    /* radius in map units */
} halodef_t;

/* An active halo for the current frame (world-space instance). */
typedef struct
{
	double cx, cy;    /* center, map units */
	double radius;
	double r2;        /* radius*radius, precomputed for the per-column test */
	int    light;
} haloinst_t;

/*
** Class-declaration side (called from DECORATE property parsing).
**
** Each actor class that declares halos owns a halo "list" (a flat growable
** array of halodef_t), mirroring LZWolf's per-class HaloLightList. A class
** stores the handle of its list in the AMETA_HaloLights class-meta int.
**
** Halo_NewList allocates an empty list and returns its handle (>= 0). This is
** called on the first halolight line of a class, or again when a subclass
** adds its own halos (so it does not mutate an inherited parent's list).
** Halo_ListAdd appends a definition to the given list.
*/
int          Halo_NewList(void);
void         Halo_ListAdd(int list, int id, int light, double radius);

/*
** Per-frame render side. Halo_Populate walks the actor list and fills the
** active-halo array; the floor/ceiling drawer then reads it.
*/
void         Halo_Populate(void);
int          Halo_ActiveCount(void);
haloinst_t  *Halo_Active(int index);

/*
** Light boost at a world point (x,y in fixed-point map coords): the sum of
** the light of every active halo whose circle contains the point. Used by the
** wall/intercept lighting path. Returns 0 when no halo covers the point.
*/
int          Halo_LightAtFixed(fixed xintercept, fixed yintercept);

/* ---- zone lighting -------------------------------------------------------
**
** A "zonelight" boosts the light level of the entire map zone an actor stands
** in (zones are the connected regions already used for sound propagation),
** rather than a radius around the actor. Same class-list + per-actor-mask
** model as halos; the renderer queries a per-zone-index light accumulator.
*/

/* A zonelight declared on an actor class. */
typedef struct
{
	int id;       /* 0..31, indexes the per-actor zoneLightMask */
	int light;    /* light-level boost when active */
} zonedef_t;

/* Class-declaration side (DECORATE "zonelight" property). */
int          Zone_NewList(void);
void         Zone_ListAdd(int list, int id, int light);

/*
** Per-frame: walk the actors, and for each active zonelight add its light to
** the accumulator for the zone the actor occupies. zoneCount is the map's
** zone count (sizes the accumulator). Zone_LightForIndex returns the summed
** boost for a zone index (0 if none / out of range).
*/
void         Zone_Populate(int zoneCount);
int          Zone_LightForIndex(int zoneIndex);
int          Zone_AnyActive(void);

#endif
