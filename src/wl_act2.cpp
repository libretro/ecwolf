// WL_ACT2.C

#include <stdio.h>
#include <math.h>
#include "wl_act.h"
#include "wl_def.h"
#include "wl_menu.h"
#include "id_ca.h"
#include "id_sd.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#include "language.h"
#include "thingdef.h"

static inline bool CheckDoorMovement(AActor *actor)
{
	MapTile::Side direction;
	switch(actor->dir)
	{
		default:
			return false;
		case north:
			direction = MapTile::South;
			break;
		case south:
			direction = MapTile::North;
			break;
		case east:
			direction = MapTile::West;
			break;
		case west:
			direction = MapTile::East;
			break;
	}

	if(actor->distance < 0) // Waiting for door?
	{
		MapSpot spot = map->GetSpot(actor->tilex, actor->tiley, 0);
		spot = spot->GetAdjacent(direction, true);
		if(spot->slideAmount[direction] != 0xffff)
			return true;
		// Door is open, go on
		actor->distance = TILEGLOBAL;
		TryWalk(actor);
	}
	return false;
}

/*
=============================================================================

							LOCAL CONSTANTS

=============================================================================
*/

#define PROJECTILESIZE  0xc000l

#define BJRUNSPEED      2048
#define BJJUMPSPEED     680


/*
=============================================================================

							GLOBAL VARIABLES

=============================================================================
*/



/*
=============================================================================

							LOCAL VARIABLES

=============================================================================
*/


dirtype dirtable[9] = {northwest,north,northeast,west,nodir,east,
	southwest,south,southeast};

short starthealth[4][NUMENEMIES] =
//
// BABY MODE
//
{
	{
		25,   // guards
		50,   // officer
		100,  // SS
		1,    // dogs
		850,  // Hans
		850,  // Schabbs
		200,  // fake hitler
		800,  // mecha hitler
		45,   // mutants
		25,   // ghosts
		25,   // ghosts
		25,   // ghosts
		25,   // ghosts

		850,  // Gretel
		850,  // Gift
		850,  // Fat
		5,    // en_spectre,
		1450, // en_angel,
		850,  // en_trans,
		1050, // en_uber,
		950,  // en_will,
		1250  // en_death
	},
	//
	// DON'T HURT ME MODE
	//
	{
		25,   // guards
		50,   // officer
		100,  // SS
		1,    // dogs
		950,  // Hans
		950,  // Schabbs
		300,  // fake hitler
		950,  // mecha hitler
		55,   // mutants
		25,   // ghosts
		25,   // ghosts
		25,   // ghosts
		25,   // ghosts

		950,  // Gretel
		950,  // Gift
		950,  // Fat
		10,   // en_spectre,
		1550, // en_angel,
		950,  // en_trans,
		1150, // en_uber,
		1050, // en_will,
		1350  // en_death
	},
	//
	// BRING 'EM ON MODE
	//
	{
		25,   // guards
		50,   // officer
		100,  // SS
		1,    // dogs

		1050, // Hans
		1550, // Schabbs
		400,  // fake hitler
		1050, // mecha hitler

		55,   // mutants
		25,   // ghosts
		25,   // ghosts
		25,   // ghosts
		25,   // ghosts

		1050, // Gretel
		1050, // Gift
		1050, // Fat
		15,   // en_spectre,
		1650, // en_angel,
		1050, // en_trans,
		1250, // en_uber,
		1150, // en_will,
		1450  // en_death
	},
	//
	// DEATH INCARNATE MODE
	//
	{
		25,   // guards
		50,   // officer
		100,  // SS
		1,    // dogs

		1200, // Hans
		2400, // Schabbs
		500,  // fake hitler
		1200, // mecha hitler

		65,   // mutants
		25,   // ghosts
		25,   // ghosts
		25,   // ghosts
		25,   // ghosts

		1200, // Gretel
		1200, // Gift
		1200, // Fat
		25,   // en_spectre,
		2000, // en_angel,
		1200, // en_trans,
		1400, // en_uber,
		1300, // en_will,
		1600  // en_death
	}
};

void    A_StartDeathCam (objtype *ob);


void    T_Path (objtype *ob);
void    T_Shoot (objtype *ob);
void    T_Bite (objtype *ob);
void    T_DogChase (objtype *ob);
void    T_Chase (objtype *ob);
void    T_Projectile (objtype *ob);

void    T_Schabb (objtype *ob);
void    T_SchabbThrow (objtype *ob);
void    T_Fake (objtype *ob);
void    T_FakeFire (objtype *ob);
void    T_Ghosts (objtype *ob);

void A_Slurpie (objtype *ob);
void A_HitlerMorph (objtype *ob);
void A_MechaSound (objtype *ob);

/*
=================
=
= A_Smoke
=
=================
*/

void A_Smoke (AActor *ob)
{
	static const ClassDef * const cls = ClassDef::FindClass("Smoke");
	if(cls == NULL)
		return;

	AActor *newobj = AActor::Spawn(cls, ob->x, ob->y, 0);
}


/*
===================
=
= ProjectileTryMove
=
= returns true if move ok
===================
*/

#define PROJSIZE        0x2000

boolean ProjectileTryMove (objtype *ob)
{
	int      xl,yl,xh,yh,x,y;
	objtype *check;

	xl = (ob->x-PROJSIZE) >> TILESHIFT;
	yl = (ob->y-PROJSIZE) >> TILESHIFT;

	xh = (ob->x+PROJSIZE) >> TILESHIFT;
	yh = (ob->y+PROJSIZE) >> TILESHIFT;

	//
	// check for solid walls
	//
	for (y=yl;y<=yh;y++)
		for (x=xl;x<=xh;x++)
		{
			check = actorat[x][y];
			if (check && !ISPOINTER(check))
				return false;
		}

		return true;
}



/*
=================
=
= T_Projectile
=
=================
*/

void T_Projectile (AActor *ob)
{
	int32_t deltax,deltay;
	int     damage;
	int32_t speed;

	speed = (int32_t)ob->speed*tics;

	deltax = FixedMul(speed,costable[ob->angle]);
	deltay = -FixedMul(speed,sintable[ob->angle]);

	if (deltax>0x10000l)
		deltax = 0x10000l;
	if (deltay>0x10000l)
		deltay = 0x10000l;

	ob->x += deltax;
	ob->y += deltay;

	deltax = LABS(ob->x - player->x);
	deltay = LABS(ob->y - player->y);

	if (!ProjectileTryMove (ob))
	{
#ifndef APOGEE_1_0          // actually the whole method is never reached in shareware 1.0
		if (ob->obclass == rocketobj)
		{
			PlaySoundLocActor("missile/hit",ob);
			ob->Die();
		}
#ifdef SPEAR
		else if (ob->obclass == hrocketobj)
		{
			PlaySoundLocActor("missile/hit",ob);
			ob->Die();
		}
#endif
		else
#endif
			ob->Die();

		return;
	}

	if (deltax < PROJECTILESIZE && deltay < PROJECTILESIZE)
	{       // hit the player
		switch (ob->obclass)
		{
		case needleobj:
			damage = (US_RndT() >>3) + 20;
			break;
		case rocketobj:
		case hrocketobj:
		case sparkobj:
			damage = (US_RndT() >>3) + 30;
			break;
		case fireobj:
			damage = (US_RndT() >>3);
			break;
		}

		TakeDamage (damage,ob);
		ob->state = NULL;               // mark for removal
		return;
	}

	ob->tilex = (short)(ob->x >> TILESHIFT);
	ob->tiley = (short)(ob->y >> TILESHIFT);
}



/*
==================
=
= A_DeathScream
=
==================
*/

void A_DeathScream (objtype *ob)
{
#ifndef UPLOAD
#ifndef SPEAR
	if (mapon==9 && !US_RndT())
#else
	if ((mapon==18 || mapon==19) && !US_RndT())
#endif
	{
		switch(ob->obclass)
		{
			case mutantobj:
			case guardobj:
			case officerobj:
			case ssobj:
			case dogobj:
				PlaySoundLocActor("guard/death6",ob);
				return;
		}
	}
#endif

	switch (ob->obclass)
	{
		case mutantobj:
			PlaySoundLocActor("mutant/death",ob);
			break;

		case guardobj:
		{
			const char* sounds[9]={ "guard/death1",
				"guard/death2",
				"guard/death3",
#ifndef APOGEE_1_0
				"guard/death4",
				"guard/death5",
				"guard/death7",
				"guard/death8",
				"guard/death9"
#endif
			};

#ifndef UPLOAD
			PlaySoundLocActor(sounds[US_RndT()%8],ob);
#else
			PlaySoundLocActor(sounds[US_RndT()%2],ob);
#endif
			break;
		}
		case officerobj:
			PlaySoundLocActor("officer/death",ob);
			break;
		case ssobj:
			PlaySoundLocActor("wolfss/death",ob); // JAB
			break;
		case dogobj:
			PlaySoundLocActor("dog/death",ob);      // JAB
			break;
#ifndef SPEAR
		case bossobj:
			SD_PlaySound("hans/death");                         // JAB
			break;
		case schabbobj:
			SD_PlaySound("schabbs/death");
			break;
		case fakeobj:
			SD_PlaySound("fake/death");
			break;
		case mechahitlerobj:
			SD_PlaySound("hitler/mechadeath");
			break;
		case realhitlerobj:
			SD_PlaySound("hitler/death");
			break;
#ifndef APOGEE_1_0
		case gretelobj:
			SD_PlaySound("gretel/death");
			break;
		case giftobj:
			SD_PlaySound("gift/death");
			break;
		case fatobj:
			SD_PlaySound("fat/death");
			break;
#endif
#else
		case spectreobj:
			SD_PlaySound("ghost/fade");
			break;
		case angelobj:
			SD_PlaySound("angel/death");
			break;
		case transobj:
			SD_PlaySound("trans/death");
			break;
		case uberobj:
			SD_PlaySound("uber/death");
			break;
		case willobj:
			SD_PlaySound("wilhelm/death");
			break;
		case deathobj:
			SD_PlaySound("deathknight/death");
			break;
#endif
	}
}


/*
=============================================================================

								SPEAR ACTORS

=============================================================================
*/

/*
===============
=
= T_UShoot
=
===============
*/

void T_UShoot (objtype *ob)
{
	int     dx,dy,dist;

	T_Shoot (ob);

	dx = abs(ob->tilex - player->tilex);
	dy = abs(ob->tiley - player->tiley);
	dist = dx>dy ? dx : dy;
	if (dist <= 1)
		TakeDamage (10,ob);
}


//
// will
//

/*
================
=
= T_Will
=
================
*/

void T_Will (objtype *ob)
{
	int32_t move;
	int     dx,dy,dist;
	boolean dodge;

	dodge = false;
	dx = abs(ob->tilex - player->tilex);
	dy = abs(ob->tiley - player->tiley);
	dist = dx>dy ? dx : dy;

	if (CheckLine(ob))                                              // got a shot at player?
	{
		ob->hidden = false;
		if ( (unsigned) US_RndT() < (tics<<3) && objfreelist)
		{
			const Frame *attack = ob->FindState("Missile");
			if(attack != NULL)
				ob->SetState(attack);
			return;
		}
		dodge = true;
	}
	else
		ob->hidden = true;

	if (ob->dir == nodir)
	{
		if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);
		if (ob->dir == nodir)
			return;                                                 // object is blocked in
	}

	move = ob->speed*tics;

	while (move)
	{
		if (CheckDoorMovement(ob))
			return;

		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		//
		// reached goal tile, so select another one
		//

		//
		// fix position to account for round off during moving
		//
		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= ob->distance;

		if (dist <4)
			SelectRunDir (ob);
		else if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);

		if (ob->dir == nodir)
			return;                                                 // object is blocked in
	}

}


//
// death
//

/*
===============
=
= T_Launch
=
===============
*/

void T_Launch (objtype *ob)
{
	int32_t deltax,deltay;
	float   angle;
	int     iangle;

	deltax = player->x - ob->x;
	deltay = ob->y - player->y;
	angle = (float) atan2 ((float) deltay, (float) deltax);
	if (angle<0)
		angle = (float) (M_PI*2+angle);
	iangle = (int) (angle/(M_PI*2)*ANGLES);
	if (ob->obclass == deathobj)
	{
		T_Shoot (ob);
	/*	if (ob->state == &s_deathshoot2)
		{
			iangle-=4;
			if (iangle<0)
				iangle+=ANGLES;
		}
		else
		{
			iangle+=4;
			if (iangle>=ANGLES)
				iangle-=ANGLES;
		}*/
	}

	static const ClassDef *cls = ClassDef::FindClass("Rocket");
	if(!cls)
		return;
	AActor *newobj = AActor::Spawn(cls, ob->x, ob->y, 0);
	/*switch(ob->obclass)
	{
		case deathobj:
			newobj->state = &s_hrocket;
			newobj->obclass = hrocketobj;
			PlaySoundLocActor ("deathknight/attack",newobj);
			break;
		case angelobj:
			newobj->state = &s_spark1;
			newobj->obclass = sparkobj;
			PlaySoundLocActor ("angel/attack",newobj);
			break;
		default:
			PlaySoundLocActor ("missile/fire",newobj);
	}*/

	newobj->dir = nodir;
	newobj->angle = iangle;
	newobj->speed = 0x2000l;
	newobj->active = ac_yes;
}



//
// angel
//


void A_Slurpie (objtype *)
{
	SD_PlaySound("misc/slurpie");
}

void A_Breathing (objtype *)
{
	SD_PlaySound("angel/breath");
}


/*
=================
=
= A_Victory
=
=================
*/

void A_Victory (objtype *)
{
	playstate = ex_victorious;
}


/*
=================
=
= A_StartAttack
=
=================
*/

void A_StartAttack (objtype *ob)
{
	ob->temp1 = 0;
}


/*
=================
=
= A_Relaunch
=
=================
*/

/*void A_Relaunch (objtype *ob)
{
	if (++ob->temp1 == 3)
	{
		NewState (ob,&s_angeltired);
		return;
	}

	if (US_RndT()&1)
	{
		NewState (ob,&s_angelchase1);
		return;
	}
}*/




//
// spectre
//


/*
===============
=
= A_Dormant
=
===============
*/

/*void A_Dormant (objtype *ob)
{
	int32_t     deltax,deltay;
	int         xl,xh,yl,yh;
	int         x,y;
	uintptr_t   tile;

	deltax = ob->x - player->x;
	if (deltax < -MINACTORDIST || deltax > MINACTORDIST)
		goto moveok;
	deltay = ob->y - player->y;
	if (deltay < -MINACTORDIST || deltay > MINACTORDIST)
		goto moveok;

	return;
moveok:

	xl = (ob->x-MINDIST) >> TILESHIFT;
	xh = (ob->x+MINDIST) >> TILESHIFT;
	yl = (ob->y-MINDIST) >> TILESHIFT;
	yh = (ob->y+MINDIST) >> TILESHIFT;

	for (y=yl ; y<=yh ; y++)
		for (x=xl ; x<=xh ; x++)
		{
			tile = (uintptr_t)actorat[x][y];
			if (!tile)
				continue;
			if (!ISPOINTER(tile))
				return;
			if (((objtype *)tile)->flags&FL_SHOOTABLE)
				return;
		}

		ob->flags |= FL_AMBUSH | FL_SHOOTABLE;
		ob->flags &= ~FL_ATTACKMODE;
		ob->flags &= ~FL_NONMARK;      // stuck bugfix 1
		ob->dir = nodir;
		NewState (ob,&s_spectrewait1);
}*/


/*
=============================================================================

							SCHABBS / GIFT / FAT

=============================================================================
*/


/*
=================
=
= T_SchabbThrow
=
=================
*/

/*void T_SchabbThrow (objtype *ob)
{
	int32_t deltax,deltay;
	float   angle;
	int     iangle;

	deltax = player->x - ob->x;
	deltay = ob->y - player->y;
	angle = (float) atan2((float) deltay, (float) deltax);
	if (angle<0)
		angle = (float) (M_PI*2+angle);
	iangle = (int) (angle/(M_PI*2)*ANGLES);

	GetNewActor ();
	newobj->state = &s_needle1;
	newobj->ticcount = 1;

	newobj->tilex = ob->tilex;
	newobj->tiley = ob->tiley;
	newobj->x = ob->x;
	newobj->y = ob->y;
	newobj->obclass = needleobj;
	newobj->dir = nodir;
	newobj->angle = iangle;
	newobj->speed = 0x2000l;

	newobj->active = ac_yes;

	PlaySoundLocActor ("schabbs/throw",newobj);
}*/

/*
=================
=
= T_GiftThrow
=
=================
*/

/*void T_GiftThrow (objtype *ob)
{
	int32_t deltax,deltay;
	float   angle;
	int     iangle;

	deltax = player->x - ob->x;
	deltay = ob->y - player->y;
	angle = (float) atan2((float) deltay, (float) deltax);
	if (angle<0)
		angle = (float) (M_PI*2+angle);
	iangle = (int) (angle/(M_PI*2)*ANGLES);

	GetNewActor ();
	newobj->state = &s_rocket;
	newobj->ticcount = 1;

	newobj->tilex = ob->tilex;
	newobj->tiley = ob->tiley;
	newobj->x = ob->x;
	newobj->y = ob->y;
	newobj->obclass = rocketobj;
	newobj->dir = nodir;
	newobj->angle = iangle;
	newobj->speed = 0x2000l;
	newobj->active = ac_yes;

#ifndef APOGEE_1_0          // T_GiftThrow will never be called in shareware v1.0
	PlaySoundLocActor ("missile/fire",newobj);
#endif
}*/


/*
=================
=
= T_Schabb
=
=================
*/

/*void T_Schabb (objtype *ob)
{
	int32_t move;
	int     dx,dy,dist;
	boolean dodge;

	dodge = false;
	dx = abs(ob->tilex - player->tilex);
	dy = abs(ob->tiley - player->tiley);
	dist = dx>dy ? dx : dy;

	if (CheckLine(ob))                                              // got a shot at player?
	{
		ob->hidden = false;
		if ( (unsigned) US_RndT() < (tics<<3) && objfreelist)
		{
			//
			// go into attack frame
			//
			NewState (ob,&s_schabbshoot1);
			return;
		}
		dodge = true;
	}
	else
		ob->hidden = true;

	if (ob->dir == nodir)
	{
		if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);
		if (ob->dir == nodir)
			return;                                                 // object is blocked in
	}

	move = ob->speed*tics;

	while (move)
	{
		if (CheckDoorMovement(ob))
			return;

		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		//
		// reached goal tile, so select another one
		//

		//
		// fix position to account for round off during moving
		//
		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= ob->distance;

		if (dist <4)
			SelectRunDir (ob);
		else if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);

		if (ob->dir == nodir)
			return;                                                 // object is blocked in
	}
}*/


/*
=================
=
= T_Gift
=
=================
*/

/*void T_Gift (objtype *ob)
{
	int32_t move;
	int     dx,dy,dist;
	boolean dodge;

	dodge = false;
	dx = abs(ob->tilex - player->tilex);
	dy = abs(ob->tiley - player->tiley);
	dist = dx>dy ? dx : dy;

	if (CheckLine(ob))                                              // got a shot at player?
	{
		ob->hidden = false;
		if ( (unsigned) US_RndT() < (tics<<3) && objfreelist)
		{
			//
			// go into attack frame
			//
			NewState (ob,&s_giftshoot1);
			return;
		}
		dodge = true;
	}
	else
		ob->hidden = true;

	if (ob->dir == nodir)
	{
		if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);
		if (ob->dir == nodir)
			return;                                                 // object is blocked in
	}

	move = ob->speed*tics;

	while (move)
	{
		if (CheckDoorMovement(ob))
			return;

		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		//
		// reached goal tile, so select another one
		//

		//
		// fix position to account for round off during moving
		//
		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= ob->distance;

		if (dist <4)
			SelectRunDir (ob);
		else if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);

		if (ob->dir == nodir)
			return;                                                 // object is blocked in
	}
}*/


/*
=================
=
= T_Fat
=
=================
*/
#if 0
void T_Fat (objtype *ob)
{
	int32_t move;
	int     dx,dy,dist;
	boolean dodge;

	dodge = false;
	dx = abs(ob->tilex - player->tilex);
	dy = abs(ob->tiley - player->tiley);
	dist = dx>dy ? dx : dy;

	if (CheckLine(ob))                                              // got a shot at player?
	{
		ob->hidden = false;
		if ( (unsigned) US_RndT() < (tics<<3) && objfreelist)
		{
			//
			// go into attack frame
			//
			NewState (ob,&s_fatshoot1);
			return;
		}
		dodge = true;
	}
	else
		ob->hidden = true;

	if (ob->dir == nodir)
	{
		if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);
		if (ob->dir == nodir)
			return;                                                 // object is blocked in
	}

	move = ob->speed*tics;

	while (move)
	{
		if (CheckDoorMovement(ob))
			return;

		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		//
		// reached goal tile, so select another one
		//

		//
		// fix position to account for round off during moving
		//
		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= ob->distance;

		if (dist <4)
			SelectRunDir (ob);
		else if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);

		if (ob->dir == nodir)
			return;                                                 // object is blocked in
	}
}


/*
=============================================================================

									HITLERS

=============================================================================
*/


//
// fake
//


/*
===============
=
= A_HitlerMorph
=
===============
*/

void A_HitlerMorph (objtype *ob)
{
	short health[4]={500,700,800,900};

	SpawnNewObj (ob->tilex,ob->tiley,&s_hitlerchase1);
	newobj->speed = SPDPATROL*5;

	newobj->x = ob->x;
	newobj->y = ob->y;

	newobj->distance = ob->distance;
	newobj->dir = ob->dir;
	newobj->flags = ob->flags | FL_SHOOTABLE;
	newobj->flags &= ~FL_NONMARK;   // hitler stuck with nodir fix

	newobj->obclass = realhitlerobj;
	newobj->health = health[gamestate.difficulty];
}


////////////////////////////////////////////////////////
//
// A_MechaSound
// A_Slurpie
//
////////////////////////////////////////////////////////
void A_MechaSound (objtype *ob)
{
	if (map->CheckLink(ob->GetZone(), player->GetZone(), true))
		PlaySoundLocActor ("hitler/active",ob);
}

void A_Slurpie (objtype *)
{
	SD_PlaySound("misc/slurpie");
}

/*
=================
=
= T_FakeFire
=
=================
*/

void T_FakeFire (objtype *ob)
{
	int32_t deltax,deltay;
	float   angle;
	int     iangle;

	if (!objfreelist)       // stop shooting if over MAXACTORS
	{
		NewState (ob,&s_fakechase1);
		return;
	}

	deltax = player->x - ob->x;
	deltay = ob->y - player->y;
	angle = (float) atan2((float) deltay, (float) deltax);
	if (angle<0)
		angle = (float)(M_PI*2+angle);
	iangle = (int) (angle/(M_PI*2)*ANGLES);

	GetNewActor ();
	newobj->state = &s_fire1;
	newobj->ticcount = 1;

	newobj->tilex = ob->tilex;
	newobj->tiley = ob->tiley;
	newobj->x = ob->x;
	newobj->y = ob->y;
	newobj->dir = nodir;
	newobj->angle = iangle;
	newobj->obclass = fireobj;
	newobj->speed = 0x1200l;
	newobj->active = ac_yes;

	PlaySoundLocActor ("fake/attack",newobj);
}



/*
=================
=
= T_Fake
=
=================
*/

void T_Fake (objtype *ob)
{
	int32_t move;

	if (CheckLine(ob))                      // got a shot at player?
	{
		ob->hidden = false;
		if ( (unsigned) US_RndT() < (tics<<1) && objfreelist)
		{
			//
			// go into attack frame
			//
			NewState (ob,&s_fakeshoot1);
			return;
		}
	}
	else
		ob->hidden = true;

	if (ob->dir == nodir)
	{
		SelectDodgeDir (ob);
		if (ob->dir == nodir)
			return;                                                 // object is blocked in
	}

	move = ob->speed*tics;

	while (move)
	{
		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		//
		// reached goal tile, so select another one
		//

		//
		// fix position to account for round off during moving
		//
		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= ob->distance;

		SelectDodgeDir (ob);

		if (ob->dir == nodir)
			return;                                                 // object is blocked in
	}
}
#endif

/*
============================================================================

STAND

============================================================================
*/


/*
===============
=
= T_Stand
=
===============
*/

ACTION_FUNCTION(T_Stand)
{
	SightPlayer (self);
}


/*
============================================================================

CHASE

============================================================================
*/

/*
=================
=
= T_Chase
=
=================
*/

ACTION_FUNCTION(T_Chase)
{
	int32_t move,target;
	int     dx,dy,dist,chance;
	boolean dodge;

	if (gamestate.victoryflag)
		return;

	dodge = false;
	if (CheckLine(self))      // got a shot at player?
	{
		self->hidden = false;
		dx = abs(self->tilex - player->tilex);
		dy = abs(self->tiley - player->tiley);
		dist = dx>dy ? dx : dy;

		{
			if (dist)
				chance = (tics<<4)/dist;
			else
				chance = 300;

			if (dist == 1)
			{
				target = abs(self->x - player->x);
				if (target < 0x14000l)
				{
					target = abs(self->y - player->y);
					if (target < 0x14000l)
						chance = 300;
				}
			}
		}

		if ( US_RndT()<chance)
		{
			const Frame *attack = self->FindState("Missile");
			if(attack != NULL)
				self->SetState(attack);
			return;
		}
		dodge = true;
	}
	else
		self->hidden = true;

	if (self->dir == nodir)
	{
		if (dodge)
			SelectDodgeDir (self);
		else
			SelectChaseDir (self);
		if (self->dir == nodir)
			return; // object is blocked in
	}

	move = self->speed;

	while (move)
	{
		if (CheckDoorMovement(self))
			return;

		if (move < self->distance)
		{
			MoveObj (self,move);
			break;
		}

		//
		// reached goal tile, so select another one
		//

		//
		// fix position to account for round off during moving
		//
		self->x = ((int32_t)self->tilex<<TILESHIFT)+TILEGLOBAL/2;
		self->y = ((int32_t)self->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= self->distance;

		if (dodge)
			SelectDodgeDir (self);
		else
			SelectChaseDir (self);

		if (self->dir == nodir)
			return; // object is blocked in
	}
}


/*
=================
=
= T_Ghosts
=
=================
*/

void T_Ghosts (objtype *ob)
{
	int32_t move;

	if (ob->dir == nodir)
	{
		SelectChaseDir (ob);
		if (ob->dir == nodir)
			return;                                                 // object is blocked in
	}

	move = ob->speed*tics;

	while (move)
	{
		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		//
		// reached goal tile, so select another one
		//

		//
		// fix position to account for round off during moving
		//
		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= ob->distance;

		SelectChaseDir (ob);

		if (ob->dir == nodir)
			return;                                                 // object is blocked in
	}
}

/*
=================
=
= T_DogChase
=
=================
*/
#if 0
void T_DogChase (objtype *ob)
{
	int32_t    move;
	int32_t    dx,dy;


	if (ob->dir == nodir)
	{
		SelectDodgeDir (ob);
		if (ob->dir == nodir)
			return;                                                 // object is blocked in
	}

	move = ob->speed*tics;

	while (move)
	{
		//
		// check for byte range
		//
		dx = player->x - ob->x;
		if (dx<0)
			dx = -dx;
		dx -= move;
		if (dx <= MINACTORDIST)
		{
			dy = player->y - ob->y;
			if (dy<0)
				dy = -dy;
			dy -= move;
			if (dy <= MINACTORDIST)
			{
				NewState (ob,&s_dogjump1);
				return;
			}
		}

		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		//
		// reached goal tile, so select another one
		//

		//
		// fix position to account for round off during moving
		//
		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= ob->distance;

		SelectDodgeDir (ob);

		if (ob->dir == nodir)
			return;                                                 // object is blocked in
	}
}
#endif



/*
============================================================================

									PATH

============================================================================
*/


/*
===============
=
= SelectPathDir
=
===============
*/

void SelectPathDir (objtype *ob)
{
#if 0
	unsigned spot;

	spot = MAPSPOT(ob->tilex,ob->tiley,1)-ICONARROWS;

	if (spot<8)
	{
		// new direction
		ob->dir = (dirtype)(spot);
	}

	ob->distance = TILEGLOBAL;

	if (!TryWalk (ob))
		ob->dir = nodir;
#endif
}


/*
===============
=
= T_Path
=
===============
*/

ACTION_FUNCTION(T_Path)
{
	int32_t    move;

	if (SightPlayer (self))
		return;

	if (self->dir == nodir)
	{
		SelectPathDir (self);
		if (self->dir == nodir)
			return;                                 // all movement is blocked
	}


	move = self->speed;

	while (move)
	{
		if (CheckDoorMovement(self))
			return;

		if (move < self->distance)
		{
			MoveObj (self,move);
			break;
		}

		if (self->tilex>MAPSIZE || self->tiley>MAPSIZE)
		{
			sprintf (str, "T_Path hit a wall at %u,%u, dir %u",
				self->tilex,self->tiley,self->dir);
			Quit (str);
		}

		self->x = ((int32_t)self->tilex<<TILESHIFT)+TILEGLOBAL/2;
		self->y = ((int32_t)self->tiley<<TILESHIFT)+TILEGLOBAL/2;
		move -= self->distance;

		SelectPathDir (self);

		if (self->dir == nodir)
			return;                                 // all movement is blocked
	}
}


/*
=============================================================================

									FIGHT

=============================================================================
*/


/*
===============
=
= T_Shoot
=
= Try to damage the player, based on skill level and player's speed
=
===============
*/

ACTION_FUNCTION(T_Shoot)
{
	int     dx,dy,dist;
	int     hitchance,damage;

	hitchance = 128;

	if (!map->CheckLink(self->GetZone(), player->GetZone(), true))
		return;

	if (CheckLine (self))                    // player is not behind a wall
	{
		dx = abs(self->tilex - player->tilex);
		dy = abs(self->tiley - player->tiley);
		dist = dx>dy ? dx:dy;

		if (self->obclass == ssobj || self->obclass == bossobj)
			dist = dist*2/3;                                        // ss are better shots

		if (thrustspeed >= RUNSPEED)
		{
			if (self->flags&FL_VISABLE)
				hitchance = 160-dist*16;                // player can see to dodge
			else
				hitchance = 160-dist*8;
		}
		else
		{
			if (self->flags&FL_VISABLE)
				hitchance = 256-dist*16;                // player can see to dodge
			else
				hitchance = 256-dist*8;
		}

		// see if the shot was a hit

		if (US_RndT()<hitchance)
		{
			if (dist<2)
				damage = US_RndT()>>2;
			else if (dist<4)
				damage = US_RndT()>>3;
			else
				damage = US_RndT()>>4;

			TakeDamage (damage,self);
		}
	}

#if 0
	switch(ob->obclass)
	{
		case ssobj:
			PlaySoundLocActor("wolfss/attack",ob);
			break;
#ifndef SPEAR
#ifndef APOGEE_1_0
		case giftobj:
		case fatobj:
			PlaySoundLocActor("missile/fire",ob);
			break;
#endif
		case mechahitlerobj:
		case realhitlerobj:
		case bossobj:
			PlaySoundLocActorBoss("hans/attack",ob);
			break;
		case schabbobj:
			PlaySoundLocActor("schabbs/throw",ob);
			break;
		case fakeobj:
			PlaySoundLocActor("fake/attack",ob);
			break;
#endif
		default:
			PlaySoundLocActor("guard/attack",ob);
	}
#endif
}
void T_Shoot(AActor *self) { __AF_T_Shoot(self); }

/*
===============
=
= T_Bite
=
===============
*/

void T_Bite (objtype *ob)
{
	int32_t    dx,dy;

	PlaySoundLocActor("dog/attack",ob);     // JAB

	dx = player->x - ob->x;
	if (dx<0)
		dx = -dx;
	dx -= TILEGLOBAL;
	if (dx <= MINACTORDIST)
	{
		dy = player->y - ob->y;
		if (dy<0)
			dy = -dy;
		dy -= TILEGLOBAL;
		if (dy <= MINACTORDIST)
		{
			if (US_RndT()<180)
			{
				TakeDamage (US_RndT()>>4,ob);
				return;
			}
		}
	}
}


#ifndef SPEAR
/*
============================================================================

									BJ VICTORY

============================================================================
*/


//
// BJ victory
//


/*
===============
=
= SpawnBJVictory
=
===============
*/
#if 0
void SpawnBJVictory (void)
{
	SpawnNewObj (player->tilex,player->tiley+1,&s_bjrun1);
	newobj->x = player->x;
	newobj->y = player->y;
	newobj->obclass = bjobj;
	newobj->dir = north;
	newobj->temp1 = 6;                      // tiles to run forward
}



/*
===============
=
= T_BJRun
=
===============
*/

void T_BJRun (objtype *ob)
{
	int32_t    move;

	move = BJRUNSPEED*tics;

	while (move)
	{
		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}


		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;
		move -= ob->distance;

		SelectPathDir (ob);

		if ( !(--ob->temp1) )
		{
			NewState (ob,&s_bjjump1);
			return;
		}
	}
}


/*
===============
=
= T_BJJump
=
===============
*/

void T_BJJump (objtype *ob)
{
	int32_t    move;

	move = BJJUMPSPEED*tics;
	MoveObj (ob,move);
}


/*
===============
=
= T_BJYell
=
===============
*/

void T_BJYell (objtype *ob)
{
	PlaySoundLocActor("misc/yeah",ob);  // JAB
}


/*
===============
=
= T_BJDone
=
===============
*/

void T_BJDone (objtype *)
{
	playstate = ex_victorious;                              // exit castle tile
}
#endif



//===========================================================================


/*
===============
=
= CheckPosition
=
===============
*/

boolean CheckPosition (objtype *ob)
{
	int     x,y,xl,yl,xh,yh;
	objtype *check;

	xl = (ob->x-PLAYERSIZE) >> TILESHIFT;
	yl = (ob->y-PLAYERSIZE) >> TILESHIFT;

	xh = (ob->x+PLAYERSIZE) >> TILESHIFT;
	yh = (ob->y+PLAYERSIZE) >> TILESHIFT;

	//
	// check for solid walls
	//
	for (y=yl;y<=yh;y++)
	{
		for (x=xl;x<=xh;x++)
		{
			check = actorat[x][y];
			if (check && !ISPOINTER(check))
				return false;
		}
	}

	return true;
}


/*
===============
=
= A_StartDeathCam
=
===============
*/
#if 0
void    A_StartDeathCam (objtype *ob)
{
	int32_t dx,dy;
	float   fangle;
	int32_t xmove,ymove;
	int32_t dist;

	FinishPaletteShifts ();

	VW_WaitVBL (100);

	if (gamestate.victoryflag)
	{
		playstate = ex_victorious;                              // exit castle tile
		return;
	}

	if(usedoublebuffering) VH_UpdateScreen();

	gamestate.victoryflag = true;
	unsigned fadeheight = viewsize != 21 ? screenHeight-scaleFactor*STATUSLINES : screenHeight;
	VL_BarScaledCoord (0, 0, screenWidth, fadeheight, bordercol);
	FizzleFade(screenBuffer, 0, 0, screenWidth, fadeheight, 70, false);

	if (bordercol != VIEWCOLOR)
	{
		fontnumber = 1;
		SETFONTCOLOR(15,bordercol);
		PrintX = 68; PrintY = 45;
		US_Print (language["STR_SEEAGAIN"]);
	}
	else
	{
#ifdef JAPAN
#ifndef JAPDEMO
		CA_CacheScreen(C_LETSSEEPIC);
#endif
#else
		Write(0,7,language["STR_SEEAGAIN"]);
#endif
	}

	VW_UpdateScreen ();
	if(usedoublebuffering) VH_UpdateScreen();

	IN_UserInput(300);

	//
	// line angle up exactly
	//
	NewState (player,&s_deathcam);

	player->x = gamestate.killx;
	player->y = gamestate.killy;

	dx = ob->x - player->x;
	dy = player->y - ob->y;

	fangle = (float) atan2((float) dy, (float) dx);          // returns -pi to pi
	if (fangle<0)
		fangle = (float) (M_PI*2+fangle);

	player->angle = (short) (fangle/(M_PI*2)*ANGLES);

	//
	// try to position as close as possible without being in a wall
	//
	dist = 0x14000l;
	do
	{
		xmove = FixedMul(dist,costable[player->angle]);
		ymove = -FixedMul(dist,sintable[player->angle]);

		player->x = ob->x - xmove;
		player->y = ob->y - ymove;
		dist += 0x1000;

	} while (!CheckPosition (player));
	plux = (word)(player->x >> UNSIGNEDSHIFT);                      // scale to fit in unsigned
	pluy = (word)(player->y >> UNSIGNEDSHIFT);
	player->tilex = (word)(player->x >> TILESHIFT);         // scale to tile values
	player->tiley = (word)(player->y >> TILESHIFT);

	//
	// go back to the game
	//

	DrawPlayBorder ();

	fizzlein = true;

	switch (ob->obclass)
	{
#ifndef SPEAR
		case schabbobj:
			NewState (ob,&s_schabbdeathcam);
			break;
		case realhitlerobj:
			NewState (ob,&s_hitlerdeathcam);
			break;
		case giftobj:
			NewState (ob,&s_giftdeathcam);
			break;
		case fatobj:
			NewState (ob,&s_fatdeathcam);
			break;
#endif
	}
}
#endif

#endif
