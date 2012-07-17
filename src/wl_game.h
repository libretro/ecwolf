#ifndef __WL_GAME_H__
#define __WL_GAME_H__

#include "textures/textures.h"

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
	short       difficulty;
	short       mapon;

	FTextureID  faceframe;

	short       episode,secretcount,treasurecount,killcount,
				secrettotal,treasuretotal,killtotal;
	int32_t     TimeCount;
	bool        victoryflag;            // set during victory animations
} gamestate;

extern  byte            bordercol;
extern  char            demoname[13];

void    SetupGameLevel (void);
void    GameLoop (void);
void    DrawPlayBorder (void);
void    DrawPlayScreen (bool noborder=false);
void    DrawPlayBorderSides (void);
void    ShowActStatus();

void    PlayDemo (int demonumber);
void    RecordDemo (void);


#ifdef SPEAR
extern  int32_t            spearx,speary;
extern  unsigned        spearangle;
extern  bool            spearflag;
#endif


#define ClearMemory SD_StopDigitized


// JAB
#define PlaySoundLocMapSpot(s,spot)     PlaySoundLocGlobal(s,(((int32_t)spot->GetX() << TILESHIFT) + (1L << (TILESHIFT - 1))),(((int32_t)spot->GetY() << TILESHIFT) + (1L << (TILESHIFT - 1))),SD_GENERIC)
#define PlaySoundLocTile(s,tx,ty)       PlaySoundLocGlobal(s,(((int32_t)(tx) << TILESHIFT) + (1L << (TILESHIFT - 1))),(((int32_t)ty << TILESHIFT) + (1L << (TILESHIFT - 1))),SD_GENERIC)
#define PlaySoundLocActor(s,ob)         PlaySoundLocGlobal(s,(ob)->x,(ob)->y,SD_GENERIC)
#define PlaySoundLocActorBoss(s,ob)     PlaySoundLocGlobal(s,(ob)->x,(ob)->y,SD_BOSSWEAPONS)
void    PlaySoundLocGlobal(const char* s,fixed gx,fixed gy,int chan);
void UpdateSoundLoc(void);

#endif
