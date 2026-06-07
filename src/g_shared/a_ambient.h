/*
** a_ambient.h
**
** Ambient sound actor. Plays a looping positional sound while the player is
** in the same map zone as the actor (see LoopedAudio). Backported from LZWolf.
**
*/

#ifndef __A_AMBIENT_H__
#define __A_AMBIENT_H__

#include "actor.h"

class AAmbient : public AActor
{
	DECLARE_NATIVE_CLASS(Ambient, Actor)

	public:
		// Jump to frame when the player's zone matches this actor's zone
		// (enter == true) or stops matching (enter == false).
		void JumpState(const Frame *frame, bool enter);
};

#endif
