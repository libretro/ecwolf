// WL_STATE.C

#include "wl_act.h"
#include "wl_def.h"
#include "id_ca.h"
#include "id_sd.h"
#include "id_us.h"
#include "m_random.h"
#include "thingdef.h"

/*
=============================================================================

							LOCAL CONSTANTS

=============================================================================
*/


/*
=============================================================================

							GLOBAL VARIABLES

=============================================================================
*/


static const dirtype opposite[9] =
	{west,southwest,south,southeast,east,northeast,north,northwest,nodir};

static const dirtype diagonal[9][9] =
{
	/* east */  {nodir,nodir,northeast,nodir,nodir,nodir,southeast,nodir,nodir},
				{nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir},
	/* north */ {northeast,nodir,nodir,nodir,northwest,nodir,nodir,nodir,nodir},
				{nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir},
	/* west */  {nodir,nodir,northwest,nodir,nodir,nodir,southwest,nodir,nodir},
				{nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir},
	/* south */ {southeast,nodir,nodir,nodir,southwest,nodir,nodir,nodir,nodir},
				{nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir},
				{nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir}
};



void    SpawnNewObj (unsigned tilex, unsigned tiley, statetype *state);
void    NewState (objtype *ob, statetype *state);

bool TryWalk (AActor *ob);
void    MoveObj (objtype *ob, int32_t move);

boolean CheckLine (objtype *ob);
void    FirstSighting (AActor *ob);
boolean CheckSight (objtype *ob);

/*
=============================================================================

								LOCAL VARIABLES

=============================================================================
*/



//===========================================================================


/*
===================
=
= SpawnNewObj
=
= Spaws a new actor at the given TILE coordinates, with the given state, and
= the given size in GLOBAL units.
=
= newobj = a pointer to an initialized new actor
=
===================
*/

void SpawnNewObj (unsigned tilex, unsigned tiley, statetype *state)
{
	/*newobj->state = state;
	if (state->tictime)
		newobj->ticcount = DEMOCHOOSE_ORIG_SDL(
				US_RndT () % state->tictime,
				US_RndT () % state->tictime + 1);     // Chris' moonwalk bugfix ;D
	else
		newobj->ticcount = 0;*/

	newobj->tilex = (short) tilex;
	newobj->tiley = (short) tiley;
	newobj->x = ((int32_t)tilex<<TILESHIFT)+TILEGLOBAL/2;
	newobj->y = ((int32_t)tiley<<TILESHIFT)+TILEGLOBAL/2;
	newobj->dir = nodir;

	actorat[tilex][tiley] = newobj;
	newobj->EnterZone(map->GetSpot(newobj->tilex, newobj->tiley, 0)->zone);
}



/*
===================
=
= NewState
=
= Changes ob to a new state, setting ticcount to the max for that state
=
===================
*/

void NewState (objtype *ob, statetype *state)
{
//	ob->state = state;
//	ob->ticcount = state->tictime;
}



/*
=============================================================================

						ENEMY TILE WORLD MOVEMENT CODE

=============================================================================
*/


/*
==================================
=
= TryWalk
=
= Attempts to move ob in its current (ob->dir) direction.
=
= If blocked by either a wall or an actor returns FALSE
=
= If move is either clear or blocked only by a door, returns TRUE and sets
=
= ob->tilex         = new destination
= ob->tiley
= ob->distance      = TILEGLOBAl, or -doornumber if a door is blocking the way
=
= If a door is in the way, an OpenDoor call is made to start it opening.
= The actor code should wait until the door has been fully opened
=
==================================
*/

static inline short CheckSide(AActor *ob, unsigned int x, unsigned int y, MapTrigger::Side dir, bool canuse)
{
	AActor *temp = actorat[x][y];
	MapSpot spot = map->GetSpot(x, y, 0);
	if(spot->tile)
	{
		if(canuse)
		{
			bool used = false;
			for(unsigned int i = 0;i < spot->triggers.Size();++i)
			{
				if(spot->triggers[i].monsterUse && spot->triggers[i].activate[dir])
				{
					if(map->ActivateTrigger(spot->triggers[i], dir, ob))
						used = true;
				}
			}
			if(used)
			{
				ob->distance = -1;
				return 1;
			}
		}
		if(spot->slideAmount[dir] != 0xffff)
			return 0;
	}
	if(temp && ((uintptr_t)temp == 64 || temp->flags&FL_SHOOTABLE))
		return 0;
	return -1;
}
#define CHECKSIDE(x,y,dir) \
{ \
	short _cs; \
	if((_cs = CheckSide(ob, x, y, dir, true)) >= 0) \
		return _cs; \
}
#define CHECKDIAG(x,y) \
{ \
	short _cs; \
	if((_cs = CheckSide(ob, x, y, MapTrigger::North, false)) >= 0) \
		return _cs; \
}



bool TryWalk (AActor *ob)
{
	switch (ob->dir)
	{
		case north:
			if (!(ob->flags & FL_CANUSEWALLS))
			{
				CHECKDIAG(ob->tilex,ob->tiley-1);
			}
			else
			{
				CHECKSIDE(ob->tilex,ob->tiley-1,MapTrigger::South);
			}
			ob->tiley--;
			break;

		case northeast:
			CHECKDIAG(ob->tilex+1,ob->tiley-1);
			CHECKDIAG(ob->tilex+1,ob->tiley);
			CHECKDIAG(ob->tilex,ob->tiley-1);
			ob->tilex++;
			ob->tiley--;
			break;

		case east:
			if (!(ob->flags & FL_CANUSEWALLS))
			{
				CHECKDIAG(ob->tilex+1,ob->tiley);
			}
			else
			{
				CHECKSIDE(ob->tilex+1,ob->tiley,MapTrigger::West);
			}
			ob->tilex++;
			break;

		case southeast:
			CHECKDIAG(ob->tilex+1,ob->tiley+1);
			CHECKDIAG(ob->tilex+1,ob->tiley);
			CHECKDIAG(ob->tilex,ob->tiley+1);
			ob->tilex++;
			ob->tiley++;
			break;

		case south:
			if (!(ob->flags & FL_CANUSEWALLS))
			{
				CHECKDIAG(ob->tilex,ob->tiley+1);
			}
			else
			{
				CHECKSIDE(ob->tilex,ob->tiley+1,MapTrigger::North);
			}
			ob->tiley++;
			break;

		case southwest:
			CHECKDIAG(ob->tilex-1,ob->tiley+1);
			CHECKDIAG(ob->tilex-1,ob->tiley);
			CHECKDIAG(ob->tilex,ob->tiley+1);
			ob->tilex--;
			ob->tiley++;
			break;

		case west:
			if (!(ob->flags & FL_CANUSEWALLS))
			{
				CHECKDIAG(ob->tilex-1,ob->tiley);
			}
			else
			{
				CHECKSIDE(ob->tilex-1,ob->tiley,MapTrigger::East);
			}
			ob->tilex--;
			break;

		case northwest:
			CHECKDIAG(ob->tilex-1,ob->tiley-1);
			CHECKDIAG(ob->tilex-1,ob->tiley);
			CHECKDIAG(ob->tilex,ob->tiley-1);
			ob->tilex--;
			ob->tiley--;
			break;

		case nodir:
			return false;

		default:
			Quit ("Walk: Bad dir");
	}

	ob->EnterZone(map->GetSpot(ob->tilex, ob->tiley, 0)->zone);

	ob->distance = TILEGLOBAL;
	return true;
}


/*
==================================
=
= SelectDodgeDir
=
= Attempts to choose and initiate a movement for ob that sends it towards
= the player while dodging
=
= If there is no possible move (ob is totally surrounded)
=
= ob->dir           =       nodir
=
= Otherwise
=
= ob->dir           = new direction to follow
= ob->distance      = TILEGLOBAL or -doornumber
= ob->tilex         = new destination
= ob->tiley
=
==================================
*/

void SelectDodgeDir (objtype *ob)
{
	int         deltax,deltay,i;
	unsigned    absdx,absdy;
	dirtype     dirtry[5];
	dirtype     turnaround,tdir;

	if (ob->flags & FL_FIRSTATTACK)
	{
		//
		// turning around is only ok the very first time after noticing the
		// player
		//
		turnaround = nodir;
		ob->flags &= ~FL_FIRSTATTACK;
	}
	else
		turnaround=opposite[ob->dir];

	deltax = player->tilex - ob->tilex;
	deltay = player->tiley - ob->tiley;

	//
	// arange 5 direction choices in order of preference
	// the four cardinal directions plus the diagonal straight towards
	// the player
	//

	if (deltax>0)
	{
		dirtry[1]= east;
		dirtry[3]= west;
	}
	else
	{
		dirtry[1]= west;
		dirtry[3]= east;
	}

	if (deltay>0)
	{
		dirtry[2]= south;
		dirtry[4]= north;
	}
	else
	{
		dirtry[2]= north;
		dirtry[4]= south;
	}

	//
	// randomize a bit for dodging
	//
	absdx = abs(deltax);
	absdy = abs(deltay);

	if (absdx > absdy)
	{
		tdir = dirtry[1];
		dirtry[1] = dirtry[2];
		dirtry[2] = tdir;
		tdir = dirtry[3];
		dirtry[3] = dirtry[4];
		dirtry[4] = tdir;
	}

	if (US_RndT() < 128)
	{
		tdir = dirtry[1];
		dirtry[1] = dirtry[2];
		dirtry[2] = tdir;
		tdir = dirtry[3];
		dirtry[3] = dirtry[4];
		dirtry[4] = tdir;
	}

	dirtry[0] = diagonal [ dirtry[1] ] [ dirtry[2] ];

	//
	// try the directions util one works
	//
	for (i=0;i<5;i++)
	{
		if ( dirtry[i] == nodir || dirtry[i] == turnaround)
			continue;

		ob->dir = dirtry[i];
		if (TryWalk(ob))
			return;
	}

	//
	// turn around only as a last resort
	//
	if (turnaround != nodir)
	{
		ob->dir = turnaround;

		if (TryWalk(ob))
			return;
	}

	ob->dir = nodir;
}


/*
============================
=
= SelectChaseDir
=
= As SelectDodgeDir, but doesn't try to dodge
=
============================
*/

void SelectChaseDir (objtype *ob)
{
	int     deltax,deltay;
	dirtype d[3];
	dirtype tdir, olddir, turnaround;


	olddir=ob->dir;
	turnaround=opposite[olddir];

	deltax=player->tilex - ob->tilex;
	deltay=player->tiley - ob->tiley;

	d[1]=nodir;
	d[2]=nodir;

	if (deltax>0)
		d[1]= east;
	else if (deltax<0)
		d[1]= west;
	if (deltay>0)
		d[2]=south;
	else if (deltay<0)
		d[2]=north;

	if (abs(deltay)>abs(deltax))
	{
		tdir=d[1];
		d[1]=d[2];
		d[2]=tdir;
	}

	if (d[1]==turnaround)
		d[1]=nodir;
	if (d[2]==turnaround)
		d[2]=nodir;


	if (d[1]!=nodir)
	{
		ob->dir=d[1];
		if (TryWalk(ob))
			return;     /*either moved forward or attacked*/
	}

	if (d[2]!=nodir)
	{
		ob->dir=d[2];
		if (TryWalk(ob))
			return;
	}

	/* there is no direct path to the player, so pick another direction */

	if (olddir!=nodir)
	{
		ob->dir=olddir;
		if (TryWalk(ob))
			return;
	}

	if (US_RndT()>128)      /*randomly determine direction of search*/
	{
		for (tdir=north; tdir<=west; tdir=(dirtype)(tdir+1))
		{
			if (tdir!=turnaround)
			{
				ob->dir=tdir;
				if ( TryWalk(ob) )
					return;
			}
		}
	}
	else
	{
		for (tdir=west; tdir>=north; tdir=(dirtype)(tdir-1))
		{
			if (tdir!=turnaround)
			{
				ob->dir=tdir;
				if ( TryWalk(ob) )
					return;
			}
		}
	}

	if (turnaround !=  nodir)
	{
		ob->dir=turnaround;
		if (ob->dir != nodir)
		{
			if ( TryWalk(ob) )
				return;
		}
	}

	ob->dir = nodir;                // can't move
}


/*
============================
=
= SelectRunDir
=
= Run Away from player
=
============================
*/

void SelectRunDir (objtype *ob)
{
	int deltax,deltay;
	dirtype d[3];
	dirtype tdir;


	deltax=player->tilex - ob->tilex;
	deltay=player->tiley - ob->tiley;

	if (deltax<0)
		d[1]= east;
	else
		d[1]= west;
	if (deltay<0)
		d[2]=south;
	else
		d[2]=north;

	if (abs(deltay)>abs(deltax))
	{
		tdir=d[1];
		d[1]=d[2];
		d[2]=tdir;
	}

	ob->dir=d[1];
	if (TryWalk(ob))
		return;     /*either moved forward or attacked*/

	ob->dir=d[2];
	if (TryWalk(ob))
		return;

	/* there is no direct path to the player, so pick another direction */

	if (US_RndT()>128)      /*randomly determine direction of search*/
	{
		for (tdir=north; tdir<=west; tdir=(dirtype)(tdir+1))
		{
			ob->dir=tdir;
			if ( TryWalk(ob) )
				return;
		}
	}
	else
	{
		for (tdir=west; tdir>=north; tdir=(dirtype)(tdir-1))
		{
			ob->dir=tdir;
			if ( TryWalk(ob) )
				return;
		}
	}

	ob->dir = nodir;                // can't move
}


/*
=================
=
= MoveObj
=
= Moves ob be move global units in ob->dir direction
= Actors are not allowed to move inside the player
= Does NOT check to see if the move is tile map valid
=
= ob->x                 = adjusted for new position
= ob->y
=
=================
*/

void MoveObj (objtype *ob, int32_t move)
{
	int32_t    deltax,deltay;

	switch (ob->dir)
	{
		case north:
			ob->y -= move;
			break;
		case northeast:
			ob->x += move;
			ob->y -= move;
			break;
		case east:
			ob->x += move;
			break;
		case southeast:
			ob->x += move;
			ob->y += move;
			break;
		case south:
			ob->y += move;
			break;
		case southwest:
			ob->x -= move;
			ob->y += move;
			break;
		case west:
			ob->x -= move;
			break;
		case northwest:
			ob->x -= move;
			ob->y -= move;
			break;

		case nodir:
			return;

		default:
			Quit ("MoveObj: bad dir!");
	}

	//
	// check to make sure it's not on top of player
	//
	if (map->CheckLink(ob->GetZone(), player->GetZone(), true))
	{
		deltax = ob->x - player->x;
		if (deltax < -MINACTORDIST || deltax > MINACTORDIST)
			goto moveok;
		deltay = ob->y - player->y;
		if (deltay < -MINACTORDIST || deltay > MINACTORDIST)
			goto moveok;

		if (ob->hidden)          // move closer until he meets CheckLine
			goto moveok;

		if (ob->obclass == ghostobj || ob->obclass == spectreobj)
			TakeDamage (tics*2,ob);

		//
		// back up
		//
		switch (ob->dir)
		{
			case north:
				ob->y += move;
				break;
			case northeast:
				ob->x -= move;
				ob->y += move;
				break;
			case east:
				ob->x -= move;
				break;
			case southeast:
				ob->x -= move;
				ob->y -= move;
				break;
			case south:
				ob->y -= move;
				break;
			case southwest:
				ob->x += move;
				ob->y -= move;
				break;
			case west:
				ob->x += move;
				break;
			case northwest:
				ob->x += move;
				ob->y += move;
				break;

			case nodir:
				return;
		}
		return;
	}
moveok:
	ob->distance -=move;
}

/*
=============================================================================

								STUFF

=============================================================================
*/

/*
===============
=
= DropItem
=
= Tries to drop a bonus item somewhere in the tiles surrounding the
= given tilex/tiley
=
===============
*/

void DropItem (wl_stat_t itemtype, int tilex, int tiley)
{
#if 0
	int     x,y,xl,xh,yl,yh;

	//
	// find a free spot to put it in
	//
	if (!actorat[tilex][tiley])
	{
		PlaceItemType (itemtype, tilex,tiley);
		return;
	}

	xl = tilex-1;
	xh = tilex+1;
	yl = tiley-1;
	yh = tiley+1;

	for (x=xl ; x<= xh ; x++)
	{
		for (y=yl ; y<= yh ; y++)
		{
			if (!actorat[x][y])
			{
				PlaceItemType (itemtype, x,y);
				return;
			}
		}
	}
#endif
}


/*
===================
=
= DamageActor
=
= Called when the player succesfully hits an enemy.
=
= Does damage points to enemy ob, either putting it into a stun frame or
= killing it.
=
===================
*/

void DamageActor (AActor *ob, unsigned damage)
{
	madenoise = true;

	//
	// do double damage if shooting a non attack mode actor
	//
	if ( !(ob->flags & FL_ATTACKMODE) )
		damage <<= 1;

	ob->health -= (short)damage;

	if (ob->health<=0)
		ob->Die();
	else
	{
		if (! (ob->flags & FL_ATTACKMODE) )
			FirstSighting (ob);             // put into combat mode

		if(ob->PainState)
			ob->SetState(ob->PainState);
	}
}

/*
=============================================================================

								CHECKSIGHT

=============================================================================
*/


/*
=====================
=
= CheckLine
=
= Returns true if a straight line between the player and ob is unobstructed
=
=====================
*/

boolean CheckLine (objtype *ob)
{
	int         x1,y1,xt1,yt1,x2,y2,xt2,yt2;
	int         x,y;
	int         xdist,ydist,xstep,ystep;
	int         partial,delta;
	int32_t     ltemp;
	int         xfrac,yfrac,deltafrac;
	unsigned    intercept;
	MapTile::Side	direction;

	x1 = ob->x >> UNSIGNEDSHIFT;            // 1/256 tile precision
	y1 = ob->y >> UNSIGNEDSHIFT;
	xt1 = x1 >> 8;
	yt1 = y1 >> 8;

	x2 = plux;
	y2 = pluy;
	xt2 = player->tilex;
	yt2 = player->tiley;

	xdist = abs(xt2-xt1);

	if (xdist > 0)
	{
		if (xt2 > xt1)
		{
			partial = 256-(x1&0xff);
			xstep = 1;
			direction = MapTile::East;
		}
		else
		{
			partial = x1&0xff;
			xstep = -1;
			direction = MapTile::West;
		}

		deltafrac = abs(x2-x1);
		delta = y2-y1;
		ltemp = ((int32_t)delta<<8)/deltafrac;
		if (ltemp > 0x7fffl)
			ystep = 0x7fff;
		else if (ltemp < -0x7fffl)
			ystep = -0x7fff;
		else
			ystep = ltemp;
		yfrac = y1 + (((int32_t)ystep*partial) >>8);

		x = xt1+xstep;
		xt2 += xstep;
		do
		{
			y = yfrac>>8;
			yfrac += ystep;

			MapSpot spot = map->GetSpot(x, y, 0);
			x += xstep;

			if (!spot->tile)
				continue;
			if (spot->tile && spot->slideAmount[direction] == 0)
				return false;

			//
			// see if the door is open enough
			//
			intercept = yfrac-ystep/2;

			if (intercept>spot->slideAmount[direction])
				return false;

		} while (x != xt2);
	}

	ydist = abs(yt2-yt1);

	if (ydist > 0)
	{
		if (yt2 > yt1)
		{
			partial = 256-(y1&0xff);
			ystep = 1;
			direction = MapTile::South;
		}
		else
		{
			partial = y1&0xff;
			ystep = -1;
			direction = MapTile::North;
		}

		deltafrac = abs(y2-y1);
		delta = x2-x1;
		ltemp = ((int32_t)delta<<8)/deltafrac;
		if (ltemp > 0x7fffl)
			xstep = 0x7fff;
		else if (ltemp < -0x7fffl)
			xstep = -0x7fff;
		else
			xstep = ltemp;
		xfrac = x1 + (((int32_t)xstep*partial) >>8);

		y = yt1 + ystep;
		yt2 += ystep;
		do
		{
			x = xfrac>>8;
			xfrac += xstep;

			MapSpot spot = map->GetSpot(x, y, 0);
			y += ystep;

			if (!spot->tile)
				continue;
			if (spot->tile && spot->slideAmount[direction] == 0)
				return false;

			//
			// see if the door is open enough
			//
			intercept = xfrac-xstep/2;

			if (intercept>spot->slideAmount[direction])
				return false;
		} while (y != yt2);
	}

	return true;
}


/*
================
=
= CheckSight
=
= Checks a straight line between player and current object
=
= If the sight is ok, check alertness and angle to see if they notice
=
= returns true if the player has been spoted
=
================
*/

#define MINSIGHT        0x18000l

boolean CheckSight (objtype *ob)
{
	int32_t deltax,deltay;

	//
	// don't bother tracing a line if the area isn't connected to the player's
	//
	if (!map->CheckLink(ob->GetZone(), player->GetZone(), true))
		return false;

	//
	// if the player is real close, sight is automatic
	//
	deltax = player->x - ob->x;
	deltay = player->y - ob->y;

	if (deltax > -MINSIGHT && deltax < MINSIGHT
		&& deltay > -MINSIGHT && deltay < MINSIGHT)
		return true;

	//
	// see if they are looking in the right direction
	//
	switch (ob->dir)
	{
		case north:
			if (deltay > 0)
				return false;
			break;

		case east:
			if (deltax < 0)
				return false;
			break;

		case south:
			if (deltay < 0)
				return false;
			break;

		case west:
			if (deltax > 0)
				return false;
			break;

		// check diagonal moving guards fix

		case northwest:
			if (DEMOCOND_SDL && deltay > -deltax)
				return false;
			break;

		case northeast:
			if (DEMOCOND_SDL && deltay > deltax)
				return false;
			break;

		case southwest:
			if (DEMOCOND_SDL && deltax > deltay)
				return false;
			break;

		case southeast:
			if (DEMOCOND_SDL && -deltax > deltay)
				return false;
			break;
	}

	//
	// trace a line to check for blocking tiles (corners)
	//
	return CheckLine (ob);
}


/*
===============
=
= FirstSighting
=
= Puts an actor into attack mode and possibly reverses the direction
= if the player is behind it
=
===============
*/

void FirstSighting (AActor *ob)
{
	if(ob->SeeState)
		ob->SetState(ob->SeeState);

	PlaySoundLocActor(ob->seesound, ob);
	ob->speed = ob->runspeed;

	if (ob->distance < 0)
		ob->distance = 0;       // ignore the door opening command

	ob->flags |= FL_ATTACKMODE|FL_FIRSTATTACK;
}



/*
===============
=
= SightPlayer
=
= Called by actors that ARE NOT chasing the player.  If the player
= is detected (by sight, noise, or proximity), the actor is put into
= it's combat frame and true is returned.
=
= Incorporates a random reaction delay
=
===============
*/

static FRandom pr_sight("SightPlayer");
bool SightPlayer (AActor *ob)
{
	if (ob->flags & FL_ATTACKMODE)
		Quit ("An actor in ATTACKMODE called SightPlayer!");

	if (ob->sighttime != ob->defaults->sighttime)
	{
		//
		// count down reaction time
		//
		if (ob->sightrandom)
		{
			--ob->sightrandom;
			return false;
		}

		if (ob->sighttime > 0)
		{
			--ob->sighttime;
			return false;
		}
	}
	else
	{
		if (!map->CheckLink(ob->GetZone(), player->GetZone(), true))
			return false;

		if (ob->flags & FL_AMBUSH)
		{
			if (!CheckSight (ob))
				return false;
			ob->flags &= ~FL_AMBUSH;
		}
		else
		{
			if (!madenoise && !CheckSight (ob))
				return false;
		}

		--ob->sighttime; // We need to somehow mark we started.
		ob->sightrandom = 1; // Account for tic.
		if(ob->defaults->sightrandom)
			ob->sightrandom += pr_sight(255/ob->defaults->sightrandom);
		return false;
	}

	FirstSighting (ob);

	return true;
}
