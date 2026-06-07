#ifndef __WL_GAME_H__
#define __WL_GAME_H__

#include "textures/textures.h"
class FArchive;

/*
=============================================================================

						WL_GAME DEFINITIONS

=============================================================================
*/

//---------------
//
// gamestate structure
//
//---------------

extern struct gametype
{
	char		mapname[9];
	const class SkillInfo *difficulty;
	const class ClassDef *playerClass;

	FTextureID  faceframe;

	short       secretcount,treasurecount,killcount,
				secrettotal,treasuretotal,killtotal;
	int32_t     TimeCount;
	bool        victoryflag;            // set during victory animations
	bool		fullmap;
} gamestate;

extern  char            demoname[13];

void    SetupGameLevel (void);
bool    GameLoop (void);
void    DrawPlayScreen (bool noborder=false);
void    DrawPlayBorderSides (void);

void    PlayDemo (int demonumber);
void    RecordDemo (void);

enum
{
	NEWMAP_KEEPFACING = 1,
	NEWMAP_KEEPPOSITION = 2
};
extern struct NewMap_t
{
	fixed x;
	fixed y;
	angle_t angle;
	int newmap;
	int flags;
} NewMap;

// JAB
#define PlaySoundLocMapSpot(s,spot)     PlaySoundLocGlobal(s,(((int32_t)spot->GetX() << TILESHIFT) + (1L << (TILESHIFT - 1))),(((int32_t)spot->GetY() << TILESHIFT) + (1L << (TILESHIFT - 1))),SD_GENERIC)
#define PlaySoundLocTile(s,tx,ty)       PlaySoundLocGlobal(s,(((int32_t)(tx) << TILESHIFT) + (1L << (TILESHIFT - 1))),(((int32_t)ty << TILESHIFT) + (1L << (TILESHIFT - 1))),SD_GENERIC)
#define PlaySoundLocActor(s,ob)         PlaySoundLocGlobal(s,(ob)->x,(ob)->y,SD_GENERIC)
#define PlaySoundLocActorBoss(s,ob)     PlaySoundLocGlobal(s,(ob)->x,(ob)->y,SD_BOSSWEAPONS)
int     PlaySoundLocGlobal(const char* s,fixed gx,fixed gy,int chan,unsigned int objId=0,bool looping=false,double volume=1.0);
void UpdateSoundLoc(void);

// Looping/ambient positional audio. Sounds are registered against the spawnid
// of the actor that owns them; updateSoundPos (called once per tic) keeps the
// nearest sources playing on the limited generic channels, re-homing or
// stopping them as the player moves, and tracks their world position so the
// mixer can pan them. See a_ambient.cpp and the ambient DECORATE class.
namespace LoopedAudio
{
	typedef unsigned int ObjId;
	typedef int SndChannel;

	bool has(ObjId objId);
	void add(ObjId objId, SndChannel channel, const char *sound, double attenuation, double volume);
	bool claimed(SndChannel channel);
	void finished(SndChannel channel);
	void updateSoundPos(void);
	void stopSoundFrom(ObjId objId, bool halt_only = false);
	void setVolume(ObjId objId, double volume);
	void stopActiveSounds(bool halt_only = false);
	void Serialize(FArchive &arc);
}

// Per-spawn unique id allocator for actors (keys LoopedAudio entries).
namespace ActorSpawnID
{
	void Serialize(FArchive &arc);
}

#endif
