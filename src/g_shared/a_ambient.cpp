/*
** a_ambient.cpp
**
** Ambient sound actor. Backported from LZWolf, rewritten for C++98 (the
** original used a C++11 lambda and auto).
**
*/

#include "a_ambient.h"
#include "id_sd.h"
#include "thinker.h"
#include "thingdef/thingdef.h"
#include "m_random.h"
#include "wl_def.h"
#include "wl_agent.h"
#include "id_ca.h"
#include "gamemap.h"

IMPLEMENT_CLASS(Ambient)

//===========================================================================
//
// AAmbient :: ZoneIndexOf
//
// The map-zone index at an actor's current tile, or 0 if it has no zone.
//
//===========================================================================

static int ZoneIndexOf(AActor *self)
{
	if(self == NULL)
		return 0;
	MapSpot spot = map->GetSpot(self->tilex, self->tiley, 0);
	if(spot != NULL && spot->zone != NULL)
		return spot->zone->index;
	return 0;
}

//===========================================================================
//
// AAmbient :: JumpState
//
// Jump to the given frame when the player's zone matches this actor's zone
// (enter == true), or when it stops matching (enter == false).
//
//===========================================================================

void AAmbient::JumpState(const Frame *frame, bool enter)
{
	AActor *player = players[0].mo;
	bool match = (ZoneIndexOf(player) == ZoneIndexOf(this));
	if(match == enter)
		SetState(frame);
}
