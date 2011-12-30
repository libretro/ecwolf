// WL_ACT2.C

#include <stdio.h>
#include <math.h>
#include "actor.h"
#include "m_random.h"
#include "wl_act.h"
#include "wl_def.h"
#include "wl_menu.h"
#include "id_ca.h"
#include "id_sd.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#include "language.h"
#include "thingdef/thingdef.h"
#include "wl_agent.h"

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

void    A_StartDeathCam (objtype *ob);


void    T_Path (objtype *ob);
void    T_Shoot (objtype *ob);
void    T_Chase (objtype *ob);
void    T_Projectile (objtype *ob);

void    T_SchabbThrow (objtype *ob);
void    T_FakeFire (objtype *ob);

void A_HitlerMorph (objtype *ob);

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

bool ProjectileTryMove (AActor *ob)
{
	int      xl,yl,xh,yh,x,y;
	MapSpot check;

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
			check = map->GetSpot(x, y, 0);
			if (check->tile)
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

void T_Projectile (AActor *self)
{
	int32_t deltax,deltay;
	int     damage;
	int32_t speed;

	speed = (int32_t)self->speed*tics;

	deltax = FixedMul(speed,costable[self->angle]);
	deltay = -FixedMul(speed,sintable[self->angle]);

	if (deltax>0x10000l)
		deltax = 0x10000l;
	if (deltay>0x10000l)
		deltay = 0x10000l;

	self->x += deltax;
	self->y += deltay;

	deltax = LABS(self->x - players[0].mo->x);
	deltay = LABS(self->y - players[0].mo->y);

	if (!ProjectileTryMove (self))
	{
		PlaySoundLocActor(self->deathsound, self);
		self->Die();
		return;
	}

	if (deltax < PROJECTILESIZE && deltay < PROJECTILESIZE)
	{       // hit the players[0].mo
		switch (self->obclass)
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

		TakeDamage (damage,self);
		self->state = NULL;               // mark for removal
		return;
	}

	self->tilex = (short)(self->x >> TILESHIFT);
	self->tiley = (short)(self->y >> TILESHIFT);
}



/*
==================
=
= A_DeathScream
=
==================
*/

ACTION_FUNCTION(A_DeathScream)
{
	PlaySoundLocActor(self->deathsound, self);
}


/*
=============================================================================

								SPEAR ACTORS

=============================================================================
*/


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

	deltax = players[0].mo->x - ob->x;
	deltay = ob->y - players[0].mo->y;
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

	deltax = ob->x - players[0].mo->x;
	if (deltax < -MINACTORDIST || deltax > MINACTORDIST)
		goto moveok;
	deltay = ob->y - players[0].mo->y;
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

	deltax = players[0].mo->x - ob->x;
	deltay = ob->y - players[0].mo->y;
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

	deltax = players[0].mo->x - ob->x;
	deltay = ob->y - players[0].mo->y;
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

	PlaySoundLocActor ("missile/fire",newobj);
}*/

#if 0
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

	deltax = players[0].mo->x - ob->x;
	deltay = ob->y - players[0].mo->y;
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

bool CheckMeleeRange(AActor *actor1, AActor *actor2)
{
	fixed r = actor1->radius + actor2->radius;
	return abs(actor2->x - actor1->x) <= r && abs(actor2->y - actor1->y) <= r;
}

ACTION_FUNCTION(T_Chase)
{
	enum
	{
		CHF_DONTDODGE = 1,
		CHF_BACKOFF = 2
	};

	ACTION_PARAM_INT(flags, 0);

	int32_t	move,target;
	int		dx,dy,dist,chance;
	bool	dodge = !(flags & CHF_DONTDODGE);

	self->flags &= ~FL_PATHING;

	if (gamestate.victoryflag)
		return;

	if(self->MissileState)
	{
		dodge = false;
		if (CheckLine(self))      // got a shot at players[0].mo?
		{
			self->hidden = false;
			dx = abs(self->tilex - players[0].mo->tilex);
			dy = abs(self->tiley - players[0].mo->tiley);
			dist = dx>dy ? dx : dy;

			if(!(flags & CHF_BACKOFF))
			{
				if (dist)
					chance = self->missilechance/dist;
				else
					chance = 300;

				if (dist == 1)
				{
					target = abs(self->x - players[0].mo->x);
					if (target < 0x14000l)
					{
						target = abs(self->y - players[0].mo->y);
						if (target < 0x14000l)
							chance = 300;
					}
				}
			}
			else
				chance = self->missilechance;

			if ( US_RndT()<chance)
			{
				if(self->MissileState)
					self->SetState(self->MissileState);
				return;
			}
			dodge = !(flags & CHF_DONTDODGE);
		}
		else
			self->hidden = true;
	}
	else
		self->hidden = !CheckMeleeRange(self, players[0].mo);

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

		//
		// check for melee range
		//
		if(self->MeleeState && CheckMeleeRange(self, players[0].mo))
		{
			PlaySoundLocActor(self->attacksound, self);
			self->SetState(self->MeleeState);
			return;
		}

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

		if ((flags & CHF_BACKOFF) && dist < 4)
			SelectRunDir (self);
		else if (dodge)
			SelectDodgeDir (self);
		else
			SelectChaseDir (self);

		if (self->dir == nodir)
			return; // object is blocked in
	}
}

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

void SelectPathDir (AActor *ob)
{
	ob->distance = TILEGLOBAL;

	if (!TryWalk (ob))
		ob->dir = nodir;
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


	self->flags |= FL_PATHING;
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

		if (self->tilex>map->GetHeader().width || self->tiley>map->GetHeader().height)
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
= Try to damage the players[0].mo, based on skill level and players[0].mo's speed
=
===============
*/

static FRandom pr_cabullet("CustomBullet");
ACTION_FUNCTION(A_WolfAttack)
{
	enum
	{
		WAF_NORANDOM = 1
	};

	ACTION_PARAM_INT(flags, 0);
	ACTION_PARAM_STRING(sound, 1);
	ACTION_PARAM_DOUBLE(snipe, 2);
	ACTION_PARAM_INT(maxdamage, 3);
	ACTION_PARAM_INT(blocksize, 4);
	ACTION_PARAM_INT(pointblank, 5);
	ACTION_PARAM_INT(longrange, 6);
	ACTION_PARAM_DOUBLE(runspeed, 7);

	int     dx,dy,dist;
	int     hitchance;

	runspeed *= 37.5;

	if (!map->CheckLink(self->GetZone(), players[0].mo->GetZone(), true))
		return;

	if (CheckLine (self))                    // players[0].mo is not behind a wall
	{
		dx = abs(self->x - players[0].mo->x);
		dy = abs(self->y - players[0].mo->y);
		dist = dx>dy ? dx:dy;

		dist = FixedMul(dist, snipe*FRACUNIT);
		dist /= blocksize<<9;

		if (thrustspeed >= runspeed)
		{
			if (self->flags&FL_VISABLE)
				hitchance = 160-dist*16;                // players[0].mo can see to dodge
			else
				hitchance = 160-dist*8;
		}
		else
		{
			if (self->flags&FL_VISABLE)
				hitchance = 256-dist*16;                // players[0].mo can see to dodge
			else
				hitchance = 256-dist*8;
		}

		// see if the shot was a hit

		if (US_RndT()<hitchance)
		{
			int damage = flags & WAF_NORANDOM ? maxdamage : (1 + (pr_cabullet()%maxdamage));
			if (dist>=pointblank)
				damage >>= 1;
			if (dist>=longrange)
				damage >>= 1;

			TakeDamage (damage,self);
		}
	}

	if(sound.Len() == 1 && sound[0] == '*')
		PlaySoundLocActor(self->attacksound, self);
	else
		PlaySoundLocActor(sound, self);
}
void T_Shoot(AActor *self)
{
	static CallArguments args;
	__AF_A_WolfAttack(self, args);
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
	SpawnNewObj (players[0].mo->tilex,players[0].mo->tiley+1,&s_bjrun1);
	newobj->x = players[0].mo->x;
	newobj->y = players[0].mo->y;
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

bool CheckPosition (AActor *ob)
{
	int x,y,xl,yl,xh,yh;
	MapSpot check;

	xl = (ob->x-players[0].mo->radius) >> TILESHIFT;
	yl = (ob->y-players[0].mo->radius) >> TILESHIFT;

	xh = (ob->x+players[0].mo->radius) >> TILESHIFT;
	yh = (ob->y+players[0].mo->radius) >> TILESHIFT;

	//
	// check for solid walls
	//
	for (y=yl;y<=yh;y++)
	{
		for (x=xl;x<=xh;x++)
		{
			check = map->GetSpot(x, y, 0);
			if (check->tile)
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
	NewState (players[0].mo,&s_deathcam);

	players[0].mo->x = gamestate.killx;
	players[0].mo->y = gamestate.killy;

	dx = ob->x - players[0].mo->x;
	dy = players[0].mo->y - ob->y;

	fangle = (float) atan2((float) dy, (float) dx);          // returns -pi to pi
	if (fangle<0)
		fangle = (float) (M_PI*2+fangle);

	players[0].mo->angle = (short) (fangle/(M_PI*2)*ANGLES);

	//
	// try to position as close as possible without being in a wall
	//
	dist = 0x14000l;
	do
	{
		xmove = FixedMul(dist,costable[players[0].mo->angle]);
		ymove = -FixedMul(dist,sintable[players[0].mo->angle]);

		players[0].mo->x = ob->x - xmove;
		players[0].mo->y = ob->y - ymove;
		dist += 0x1000;

	} while (!CheckPosition (players[0].mo));
	players[0].mo->tilex = (word)(players[0].mo->x >> TILESHIFT);         // scale to tile values
	players[0].mo->tiley = (word)(players[0].mo->y >> TILESHIFT);

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
