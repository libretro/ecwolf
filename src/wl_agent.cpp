// WL_AGENT.C

#include "wl_act.h"
#include "wl_def.h"
#include "id_ca.h"
#include "id_sd.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#include "actor.h"
#include "thingdef/thingdef.h"
#include "lnspec.h"

/*
=============================================================================

								LOCAL CONSTANTS

=============================================================================
*/

#define MAXMOUSETURN    10


#define MOVESCALE       150l
#define BACKMOVESCALE   100l
#define ANGLESCALE      20

/*
=============================================================================

								GLOBAL VARIABLES

=============================================================================
*/



//
// player state info
//
int32_t         thrustspeed;

word            plux,pluy;          // player coordinates scaled to unsigned

short           anglefrac;

objtype        *LastAttacker;

/*
=============================================================================

												LOCAL VARIABLES

=============================================================================
*/


struct atkinf
{
	int8_t    tics,attack,frame;              // attack is 1 for gun, 2 for knife
} attackinfo[4][14] =
{
	{ {6,0,1},{6,2,2},{6,0,3},{6,-1,4} },
	{ {6,0,1},{6,1,2},{6,0,3},{6,-1,4} },
	{ {6,0,1},{6,1,2},{6,3,3},{6,-1,4} },
	{ {6,0,1},{6,1,2},{6,4,3},{6,-1,4} },
};

//===========================================================================

//----------

void Attack (void);
void Use (void);
void Search (objtype *ob);
void SelectWeapon (void);
void SelectItem (void);

//----------

void ClipMove (objtype *ob, int32_t xmove, int32_t ymove);

/*
=============================================================================

								CONTROL STUFF

=============================================================================
*/

/*
======================
=
= CheckWeaponChange
=
= Keys 1-4 change weapons
=
======================
*/

void CheckWeaponChange (void)
{
	int newWeapon = -1;

	if (!gamestate.ammo)            // must use knife with no ammo
		return;

#ifdef _arch_dreamcast
	int joyx, joyy;
	IN_GetJoyFineDelta (&joyx, &joyy);
	if(joyx < -64)
		buttonstate[bt_prevweapon] = true;
	else if(joyx > 64)
		buttonstate[bt_nextweapon] = true;
#endif

	if(buttonstate[bt_nextweapon] && !buttonheld[bt_nextweapon])
	{
		newWeapon = gamestate.weapon + 1;
		if(newWeapon > gamestate.bestweapon) newWeapon = 0;
	}
	else if(buttonstate[bt_prevweapon] && !buttonheld[bt_prevweapon])
	{
		newWeapon = gamestate.weapon - 1;
		if(newWeapon < 0) newWeapon = gamestate.bestweapon;
	}
	else
	{
		for(int i = wp_knife; i <= gamestate.bestweapon; i++)
		{
			if (buttonstate[bt_readyknife + i - wp_knife])
			{
				newWeapon = i;
				break;
			}
		}
	}

	if(newWeapon != -1)
	{
		gamestate.weapon = gamestate.chosenweapon = (weapontype) newWeapon;
		DrawWeapon();
	}
}


/*
=======================
=
= ControlMovement
=
= Takes controlx,controly, and buttonstate[bt_strafe]
=
= Changes the player's angle and position
=
= There is an angle hack because when going 70 fps, the roundoff becomes
= significant
=
=======================
*/

void ControlMovement (objtype *ob)
{
	int32_t oldx,oldy;
	int     angle;
	int     angleunits;

	thrustspeed = 0;

	oldx = player->x;
	oldy = player->y;

	if(buttonstate[bt_strafeleft])
	{
		angle = ob->angle + ANGLES/4;
		if(angle >= ANGLES)
			angle -= ANGLES;
		if((!alwaysrun && buttonstate[bt_run]) || (alwaysrun && !buttonstate[bt_run]))
			Thrust(angle, RUNMOVE * MOVESCALE * tics);
		else
			Thrust(angle, BASEMOVE * MOVESCALE * tics);
	}

	if(buttonstate[bt_straferight])
	{
		angle = ob->angle - ANGLES/4;
		if(angle < 0)
			angle += ANGLES;
		if((!alwaysrun && buttonstate[bt_run]) || (alwaysrun && !buttonstate[bt_run]))
			Thrust(angle, RUNMOVE * MOVESCALE * tics );
		else
			Thrust(angle, BASEMOVE * MOVESCALE * tics);
	}

	//
	// side to side move
	//
	if (buttonstate[bt_strafe])
	{
		//
		// strafing
		//
		//
		if (controlx > 0)
		{
			angle = ob->angle - ANGLES/4;
			if (angle < 0)
				angle += ANGLES;
			Thrust (angle,controlx*MOVESCALE);      // move to left
		}
		else if (controlx < 0)
		{
			angle = ob->angle + ANGLES/4;
			if (angle >= ANGLES)
				angle -= ANGLES;
			Thrust (angle,-controlx*MOVESCALE);     // move to right
		}
	}
	else
	{
		//
		// not strafing
		//
		anglefrac += controlx;
		angleunits = anglefrac/ANGLESCALE;
		anglefrac -= angleunits*ANGLESCALE;
		ob->angle -= angleunits;

		if (ob->angle >= ANGLES)
			ob->angle -= ANGLES;
		if (ob->angle < 0)
			ob->angle += ANGLES;

	}

	//
	// forward/backwards move
	//
	if (controly < 0)
	{
		Thrust (ob->angle,-controly*MOVESCALE); // move forwards
	}
	else if (controly > 0)
	{
		angle = ob->angle + ANGLES/2;
		if (angle >= ANGLES)
			angle -= ANGLES;
		Thrust (angle,controly*BACKMOVESCALE);          // move backwards
	}

	if (gamestate.victoryflag)              // watching the BJ actor
		return;
}

/*
=============================================================================

							STATUS WINDOW STUFF

=============================================================================
*/


/*
==================
=
= StatusDrawPic
=
==================
*/

void StatusDrawPic (unsigned x, unsigned y, const char* pic)
{
	VWB_DrawPic((screenWidth-scaleFactor*320)/16 + scaleFactor*(x*8), screenHeight-scaleFactor*(STATUSLINES-y), pic, true);
//    LatchDrawPicScaledCoord ((screenWidth-scaleFactor*320)/16 + scaleFactor*x,
//        screenHeight-scaleFactor*(STATUSLINES-y),picnum);
}

void StatusDrawFace(const char* pic)
{
	StatusDrawPic(17, 4, pic);
	//StatusDrawLCD(picnum);
}


/*
==================
=
= DrawFace
=
==================
*/

void DrawFace (void)
{
	const char* godmode[3] = { "STFGOD0", "STFGOD1", "STFGOD2" };
	const char* animations[7][3] =
	{
		{ "STFST00", "STFST01", "STFST02" },
		{ "STFST10", "STFST11", "STFST12" },
		{ "STFST20", "STFST21", "STFST22" },
		{ "STFST30", "STFST31", "STFST32" },
		{ "STFST40", "STFST41", "STFST42" },
		{ "STFST50", "STFST51", "STFST52" },
		{ "STFST60", "STFST61", "STFST62" },
	};
	if(viewsize == 21 && ingame) return;
	if (GotChaingun())
		StatusDrawFace("STFEVL0");
	else if (gamestate.health)
	{
#ifdef SPEAR
		if (godmode)
			StatusDrawFace(godmode[gamestate.faceframe]);
		else
#endif
			StatusDrawFace(animations[(100-gamestate.health)/16][gamestate.faceframe]);
	}
	else
	{
#ifndef SPEAR
		if (LastAttacker && LastAttacker->obclass == needleobj)
			StatusDrawFace("STFMUT0");
		else
#endif
			StatusDrawFace("STFDEAD0");
	}
}

/*
===============
=
= UpdateFace
=
= Calls draw face if time to change
=
===============
*/

int facecount = 0;
int facetimes = 0;

void UpdateFace (void)
{
	// don't make demo depend on sound playback
	if(demoplayback || demorecord)
	{
		if(facetimes > 0)
		{
			facetimes--;
			return;
		}
	}
	else if(GotChaingun())
		return;

	facecount += tics;
	if (facecount > US_RndT())
	{
		gamestate.faceframe = (US_RndT()>>6);
		if (gamestate.faceframe==3)
			gamestate.faceframe = 1;

		facecount = 0;
		DrawFace ();
	}
}



/*
===============
=
= LatchNumber
=
= right justifies and pads with blanks
=
===============
*/

static void LatchNumber (int x, int y, unsigned width, int32_t number)
{
	unsigned length,c;
	char    str[20];

	ltoa (number,str,10);

	length = (unsigned) strlen (str);

	while (length<width)
	{
		StatusDrawPic (x,y,"FONTN032");
		x++;
		width--;
	}

	c = length <= width ? 0 : length-width;

	const char* numerics[10] = { "FONTN048", "FONTN049", "FONTN050", "FONTN051", "FONTN052", "FONTN053", "FONTN054", "FONTN055", "FONTN056", "FONTN057" };
	while (c<length)
	{
		StatusDrawPic (x,y,numerics[str[c]-'0']);
		x++;
		c++;
	}
}


/*
===============
=
= DrawHealth
=
===============
*/

void DrawHealth (void)
{
	if(viewsize == 21 && ingame) return;
	LatchNumber (21,16,3,gamestate.health);
}


/*
===============
=
= TakeDamage
=
===============
*/

void TakeDamage (int points,objtype *attacker)
{
	LastAttacker = attacker;

	if (gamestate.victoryflag)
		return;
	if (gamestate.difficulty==gd_baby)
		points>>=2;

	if (!godmode)
		gamestate.health -= points;

	if (gamestate.health<=0)
	{
		gamestate.health = 0;
		playstate = ex_died;
		killerobj = attacker;
	}

	if (godmode != 2)
		StartDamageFlash (points);

	DrawHealth ();
	DrawFace ();

	//
	// MAKE BJ'S EYES BUG IF MAJOR DAMAGE!
	//
#ifdef SPEAR
	if (points > 30 && gamestate.health!=0 && !godmode && viewsize != 21)
	{
		StatusDrawFace("STFOUCH0");
		facecount = 0;
	}
#endif
}

/*
===============
=
= HealSelf
=
===============
*/

void HealSelf (int points)
{
	gamestate.health += points;
	if (gamestate.health>100)
		gamestate.health = 100;

	DrawHealth ();
	DrawFace ();
}


//===========================================================================


/*
===============
=
= DrawLevel
=
===============
*/

void DrawLevel (void)
{
	if(viewsize == 21 && ingame) return;
#ifdef SPEAR
	if (gamestate.mapon == 20)
		LatchNumber (2,16,2,18);
	else
#endif
		LatchNumber (2,16,2,gamestate.mapon+1);
}

//===========================================================================


/*
===============
=
= DrawLives
=
===============
*/

void DrawLives (void)
{
	if(viewsize == 21 && ingame) return;
	LatchNumber (14,16,1,gamestate.lives);
}


/*
===============
=
= GiveExtraMan
=
===============
*/

void GiveExtraMan (void)
{
	if (gamestate.lives<9)
		gamestate.lives++;
	DrawLives ();
	SD_PlaySound ("misc/end_bonus1");
}

//===========================================================================

/*
===============
=
= DrawScore
=
===============
*/

void DrawScore (void)
{
	if(viewsize == 21 && ingame) return;
	LatchNumber (6,16,6,gamestate.score);
}

/*
===============
=
= GivePoints
=
===============
*/

void GivePoints (int32_t points)
{
	gamestate.score += points;
	while (gamestate.score >= gamestate.nextextra)
	{
		gamestate.nextextra += EXTRAPOINTS;
		GiveExtraMan ();
	}
	DrawScore ();
}

//===========================================================================

/*
==================
=
= DrawWeapon
=
==================
*/

void DrawWeapon (void)
{
	const char* weapons[] = { "KNIFE", "PISTOL", "MACHGUN", "GATLGUN" };
	if(viewsize == 21 && ingame) return;
	StatusDrawPic (32,8,weapons[gamestate.weapon]);
}


/*
==================
=
= DrawKeys
=
==================
*/

void DrawKeys (void)
{
	if(viewsize == 21 && ingame) return;
	if (gamestate.keys & 1)
		StatusDrawPic (30,4,"GOLDKEY");
	else
		StatusDrawPic (30,4,"NOKEY");

	if (gamestate.keys & 2)
		StatusDrawPic (30,20,"SILVRKEY");
	else
		StatusDrawPic (30,20,"NOKEY");
}

/*
==================
=
= GiveWeapon
=
==================
*/

void GiveWeapon (int weapon)
{
	GiveAmmo (6);

	if (gamestate.bestweapon<weapon)
		gamestate.bestweapon = gamestate.weapon
		= gamestate.chosenweapon = (weapontype) weapon;

	DrawWeapon ();
}

//===========================================================================

/*
===============
=
= DrawAmmo
=
===============
*/

void DrawAmmo (void)
{
	if(viewsize == 21 && ingame) return;
	LatchNumber (27,16,2,gamestate.ammo);
}

/*
===============
=
= GiveAmmo
=
===============
*/

void GiveAmmo (int ammo)
{
	if (!gamestate.ammo)                            // knife was out
	{
		if (!gamestate.attackframe)
		{
			gamestate.weapon = gamestate.chosenweapon;
			DrawWeapon ();
		}
	}
	gamestate.ammo += ammo;
	if (gamestate.ammo > 99)
		gamestate.ammo = 99;
	DrawAmmo ();
}

//===========================================================================

/*
==================
=
= GiveKey
=
==================
*/

void GiveKey (int key)
{
	gamestate.keys |= (1<<key);
	DrawKeys ();
}



/*
=============================================================================

								MOVEMENT

=============================================================================
*/


/*
===================
=
= GetBonus
=
===================
*/
void GetBonus (statobj_t *check)
{
	switch (check->itemnumber)
	{
		case    bo_firstaid:
			if (gamestate.health == 100)
				return;

			SD_PlaySound ("misc/medkit_pickup");
			HealSelf (25);
			break;

		case    bo_key1:
		case    bo_key2:
		case    bo_key3:
		case    bo_key4:
			GiveKey (check->itemnumber - bo_key1);
			SD_PlaySound ("misc/key_pickup");
			break;

		case    bo_cross:
			SD_PlaySound ("treasure/cross/pickup");
			GivePoints (100);
			gamestate.treasurecount++;
			break;
		case    bo_chalice:
			SD_PlaySound ("treasure/chalice/pickup");
			GivePoints (500);
			gamestate.treasurecount++;
			break;
		case    bo_bible:
			SD_PlaySound ("treasure/bible/pickup");
			GivePoints (1000);
			gamestate.treasurecount++;
			break;
		case    bo_crown:
			SD_PlaySound ("treasure/crown/pickup");
			GivePoints (5000);
			gamestate.treasurecount++;
			break;

		case    bo_clip:
			if (gamestate.ammo == 99)
				return;

			SD_PlaySound ("misc/ammo_pickup");
			GiveAmmo (8);
			break;
		case    bo_clip2:
			if (gamestate.ammo == 99)
				return;

			SD_PlaySound ("misc/ammo_pickup");
			GiveAmmo (4);
			break;

#ifdef SPEAR
		case    bo_25clip:
			if (gamestate.ammo == 99)
				return;

			SD_PlaySound ("misc/ammobox_pickup");
			GiveAmmo (25);
			break;
#endif

		case    bo_machinegun:
			SD_PlaySound ("weapon/machine/pickup");
			GiveWeapon (wp_machinegun);
			break;
		case    bo_chaingun:
			SD_PlaySound ("weapon/gatling/pickup");
			facetimes = 38;
			GiveWeapon (wp_chaingun);

			if(viewsize != 21)
				StatusDrawFace ("STFEVL0");
			facecount = 0;
			break;

		case    bo_fullheal:
			SD_PlaySound ("misc/1up");
			HealSelf (99);
			GiveAmmo (25);
			GiveExtraMan ();
			gamestate.treasurecount++;
			break;

		case    bo_food:
			if (gamestate.health == 100)
				return;

			SD_PlaySound ("misc/health_pickup");
			HealSelf (10);
			break;

		case    bo_alpo:
			if (gamestate.health == 100)
				return;

			SD_PlaySound ("misc/health_pickup");
			HealSelf (4);
			break;

		case    bo_gibs:
			if (gamestate.health >10)
				return;

			SD_PlaySound ("misc/slurpie");
			HealSelf (1);
			break;

#ifdef SPEAR
		case    bo_spear:
			spearflag = true;
			spearx = player->x;
			speary = player->y;
			spearangle = player->angle;
			playstate = ex_completed;
#endif
	}

	StartBonusFlash ();
	check->shapenum = -1;                   // remove from list
}

/*
===================
=
= TryMove
=
= returns true if move ok
= debug: use pointers to optimize
===================
*/

bool TryMove (AActor *ob)
{
	int xl,yl,xh,yh,x,y;
	AActor *check;

	xl = (ob->x-player->radius) >>TILESHIFT;
	yl = (ob->y-player->radius) >>TILESHIFT;

	xh = (ob->x+player->radius) >>TILESHIFT;
	yh = (ob->y+player->radius) >>TILESHIFT;

	//
	// check for solid walls
	//
	for (y=yl;y<=yh;y++)
	{
		for (x=xl;x<=xh;x++)
		{
			const bool checkLines[4] =
			{
				(ob->x+player->radius) > ((x+1)<<TILESHIFT),
				(ob->y-player->radius) < (y<<TILESHIFT),
				(ob->x-player->radius) < (x<<TILESHIFT),
				(ob->y+player->radius) > ((y+1)<<TILESHIFT)
			};
			MapSpot spot = map->GetSpot(x, y, 0);
			if(spot->tile)
			{
				// Check pushwall backs
				if(spot->pushAmount != 0)
				{
					switch(spot->pushDirection)
					{
						case MapTile::North:
							if(ob->y-ob->radius <= (y<<TILESHIFT)+((63-spot->pushAmount)<<10))
								return false;
							break;
						case MapTile::West:
							if(ob->x-ob->radius <= (x<<TILESHIFT)+((63-spot->pushAmount)<<10))
								return false;
							break;
						case MapTile::East:
							if(ob->x+ob->radius >= (x<<TILESHIFT)+(spot->pushAmount<<10))
								return false;
							break;
						case MapTile::South:
							if(ob->y+ob->radius >= (y<<TILESHIFT)+(spot->pushAmount<<10))
								return false;
							break;
					}
				}
				else
				{
					for(unsigned short i = 0;i < 4;++i)
					{
						if(spot->slideAmount[i] != 0xffff && checkLines[i])
							return false;
					}
				}
			}
		}
	}

	//
	// check for actors
	//
	for(AActor::Iterator *iter = AActor::actors.Head();iter;iter = iter->Next())
	{
		if(iter->Item() == ob)
			continue;

		check = iter->Item();
		
		if(check->flags & FL_SOLID)
		{
			fixed r = check->radius + ob->radius;
			if(abs(ob->x - check->x) > r ||
				abs(ob->y - check->y) > r)
				continue;
			return false;
		}
	}

	return true;
}


/*
===================
=
= ClipMove
=
===================
*/

void ClipMove (objtype *ob, int32_t xmove, int32_t ymove)
{
	int32_t    basex,basey;

	basex = ob->x;
	basey = ob->y;

	ob->x = basex+xmove;
	ob->y = basey+ymove;
	if (TryMove (ob))
		return;

#ifndef REMDEBUG
	if (noclip && ob->x > 2*TILEGLOBAL && ob->y > 2*TILEGLOBAL
		&& ob->x < (((int32_t)(map->GetHeader().width-1))<<TILESHIFT)
		&& ob->y < (((int32_t)(map->GetHeader().height-1))<<TILESHIFT) )
		return;         // walk through walls
#endif

	if (!SD_SoundPlaying())
		SD_PlaySound ("world/hitwall");

	ob->x = basex+xmove;
	ob->y = basey;
	if (TryMove (ob))
		return;

	ob->x = basex;
	ob->y = basey+ymove;
	if (TryMove (ob))
		return;

	ob->x = basex;
	ob->y = basey;
}

//==========================================================================

/*
===================
=
= VictoryTile
=
===================
*/

void VictoryTile (void)
{
#ifndef SPEAR
//	SpawnBJVictory ();
#endif

	gamestate.victoryflag = true;
}

/*
===================
=
= Thrust
=
===================
*/

// For player movement in demos exactly as in the original Wolf3D v1.4 source code
static fixed FixedByFracOrig(fixed a, fixed b)
{
	int sign = 0;
	if(b == 65536) b = 65535;
	else if(b == -65536) b = 65535, sign = 1;
	else if(b < 0) b = (-b), sign = 1;

	if(a < 0)
	{
		a = -a;
		sign = !sign;
	}
	fixed res = (fixed)(((int64_t) a * b) >> 16);
	if(sign)
		res = -res;
	return res;
}

void Thrust (int angle, int32_t speed)
{
	int32_t xmove,ymove;

	//
	// ZERO FUNNY COUNTER IF MOVED!
	//
#ifdef SPEAR
	if (speed)
		funnyticount = 0;
#endif

	thrustspeed += speed;
	//
	// moving bounds speed
	//
	if (speed >= MINDIST*2)
		speed = MINDIST*2-1;

	xmove = DEMOCHOOSE_ORIG_SDL(
				FixedByFracOrig(speed, costable[angle]),
				FixedMul(speed,costable[angle]));
	ymove = DEMOCHOOSE_ORIG_SDL(
				-FixedByFracOrig(speed, sintable[angle]),
				-FixedMul(speed,sintable[angle]));

	ClipMove(player,xmove,ymove);

	player->tilex = (short)(player->x >> TILESHIFT);                // scale to tile values
	player->tiley = (short)(player->y >> TILESHIFT);

	player->EnterZone(map->GetSpot(player->tilex, player->tiley, 0)->zone);
}


/*
=============================================================================

								ACTIONS

=============================================================================
*/


/*
===============
=
= Cmd_Fire
=
===============
*/

void Cmd_Fire (void)
{
	buttonheld[bt_attack] = true;

	gamestate.weaponframe = 0;

	player->SetState(player->MissileState);

	gamestate.attackframe = 0;
	gamestate.attackcount =
		attackinfo[gamestate.weapon][gamestate.attackframe].tics;
	gamestate.weaponframe =
		attackinfo[gamestate.weapon][gamestate.attackframe].frame;
}

//===========================================================================

/*
===============
=
= Cmd_Use
=
===============
*/

void Cmd_Use (void)
{
	int     checkx,checky;
	MapTrigger::Side direction;

	//
	// find which cardinal direction the player is facing
	//
	if (player->angle < ANGLES/8 || player->angle > 7*ANGLES/8)
	{
		checkx = player->tilex + 1;
		checky = player->tiley;
		direction = MapTrigger::East;
	}
	else if (player->angle < 3*ANGLES/8)
	{
		checkx = player->tilex;
		checky = player->tiley-1;
		direction = MapTrigger::North;
	}
	else if (player->angle < 5*ANGLES/8)
	{
		checkx = player->tilex - 1;
		checky = player->tiley;
		direction = MapTrigger::West;
	}
	else
	{
		checkx = player->tilex;
		checky = player->tiley + 1;
		direction = MapTrigger::South;
	}

	bool doNothing = true;
	MapSpot spot = map->GetSpot(checkx, checky, 0);
	for(unsigned int i = 0;i < spot->triggers.Size();++i)
	{
		MapTrigger &trig = spot->triggers[i];
		if(trig.activate[direction] && trig.playerUse)
		{
			map->ActivateTrigger(trig, direction, player);
			doNothing = false;
		}
	}

	if(doNothing)
		SD_PlaySound ("misc/do_nothing");
}

/*
=============================================================================

								PLAYER CONTROL

=============================================================================
*/



/*
===============
=
= SpawnPlayer
=
===============
*/

void SpawnPlayer (int tilex, int tiley, int dir)
{
	static const ClassDef * const playerClass = ClassDef::FindClass("BJPlayer");
	player = AActor::Spawn(playerClass, ((int32_t)tilex<<TILESHIFT)+TILEGLOBAL/2, ((int32_t)tiley<<TILESHIFT)+TILEGLOBAL/2, 0);
	player->angle = (270+dir)%360;
	if (player->angle<0)
		player->angle += ANGLES;
	Thrust (0,0); // set some variables
}


//===========================================================================

/*
===============
=
= T_KnifeAttack
=
= Update player hands, and try to do damage when the proper frame is reached
=
===============
*/

void    KnifeAttack (objtype *ob)
{
	AActor *closest;
	int32_t dist;

	SD_PlaySound ("weapon/knife/attack", SD_WEAPONS);
	// actually fire
	dist = 0x7fffffff;
	closest = NULL;
	for(AActor::Iterator *check = AActor::GetIterator();check;check = check->Next())
	{
		if(check->Item() == ob)
			continue;

		if ( (check->Item()->flags & FL_SHOOTABLE) && (check->Item()->flags & FL_VISABLE)
			&& abs(check->Item()->viewx-centerx) < shootdelta)
		{
			if (check->Item()->transx < dist)
			{
				dist = check->Item()->transx;
				closest = check->Item();
			}
		}
	}

	if (!closest || dist > 0x18000l)
	{
		// missed
		return;
	}

	// hit something
	DamageActor (closest,US_RndT() >> 4);
}



void GunAttack (AActor *ob)
{
	AActor *closest=NULL,*oldclosest=NULL;
	int      damage;
	int      dx,dy,dist;
	int32_t  viewdist;

	switch (gamestate.weapon)
	{
		case wp_pistol:
			SD_PlaySound ("weapon/pistol/attack", SD_WEAPONS);
			break;
		case wp_machinegun:
			SD_PlaySound ("weapon/machine/attack", SD_WEAPONS);
			break;
		case wp_chaingun:
			SD_PlaySound ("weapon/gatling/fire", SD_WEAPONS);
			break;
	}

	madenoise = true;

	//
	// find potential targets
	//
	viewdist = 0x7fffffffl;
	closest = NULL;

	while (1)
	{
		oldclosest = closest;

		for(AActor::Iterator *check = AActor::GetIterator();check;check = check->Next())
		{
			if(check->Item() == ob)
				continue;

			if ((check->Item()->flags & FL_SHOOTABLE) && (check->Item()->flags & FL_VISABLE)
				&& abs(check->Item()->viewx-centerx) < shootdelta)
			{
				if (check->Item()->transx < viewdist)
				{
					viewdist = check->Item()->transx;
					closest = check->Item();
				}
			}
		}

		if (closest == oldclosest)
			return; // no more targets, all missed

		//
		// trace a line from player to enemey
		//
		if (CheckLine(closest))
			break;
	}

	//
	// hit something
	//
	dx = ABS(closest->tilex - player->tilex);
	dy = ABS(closest->tiley - player->tiley);
	dist = dx>dy ? dx:dy;
	if (dist<2)
		damage = US_RndT() / 4;
	else if (dist<4)
		damage = US_RndT() / 6;
	else
	{
		if ( (US_RndT() / 12) < dist)           // missed
			return;
		damage = US_RndT() / 6;
	}
	DamageActor (closest,damage);
}

//===========================================================================

/*
===============
=
= VictorySpin
=
===============
*/

void VictorySpin (void)
{
	int32_t    desty;

	if (player->angle > 270)
	{
		player->angle -= (short)(tics * 3);
		if (player->angle < 270)
			player->angle = 270;
	}
	else if (player->angle < 270)
	{
		player->angle += (short)(tics * 3);
		if (player->angle > 270)
			player->angle = 270;
	}

	desty = (((int32_t)player->tiley-5)<<TILESHIFT)-0x3000;

	if (player->y > desty)
	{
		player->y -= tics*4096;
		if (player->y < desty)
			player->y = desty;
	}
}


//===========================================================================

/*
===============
=
= T_Attack
=
===============
*/

ACTION_FUNCTION(T_Attack)
{
	struct  atkinf  *cur;

	UpdateFace ();

	if (gamestate.victoryflag)              // watching the BJ actor
	{
		VictorySpin ();
		return;
	}

	if ( buttonstate[bt_use] && !buttonheld[bt_use] )
		buttonstate[bt_use] = false;

	if ( buttonstate[bt_attack] && !buttonheld[bt_attack])
		buttonstate[bt_attack] = false;

	ControlMovement (self);
	if (gamestate.victoryflag)              // watching the BJ actor
		return;

	plux = (word) (player->x >> UNSIGNEDSHIFT);                     // scale to fit in unsigned
	pluy = (word) (player->y >> UNSIGNEDSHIFT);
	player->tilex = (short)(player->x >> TILESHIFT);                // scale to tile values
	player->tiley = (short)(player->y >> TILESHIFT);

	//
	// change frame and fire
	//
	gamestate.attackcount -= (short) tics;
	while (gamestate.attackcount <= 0)
	{
		cur = &attackinfo[gamestate.weapon][gamestate.attackframe];
		switch (cur->attack)
		{
			case -1:
				player->SetState(player->SpawnState);
				if (!gamestate.ammo)
				{
					gamestate.weapon = wp_knife;
					DrawWeapon ();
				}
				else
				{
					if (gamestate.weapon != gamestate.chosenweapon)
					{
						gamestate.weapon = gamestate.chosenweapon;
						DrawWeapon ();
					}
				}
				gamestate.attackframe = gamestate.weaponframe = 0;
				return;

			case 4:
				if (!gamestate.ammo)
					break;
				if (buttonstate[bt_attack])
					gamestate.attackframe -= 2;
			case 1:
				if (!gamestate.ammo)
				{       // can only happen with chain gun
					gamestate.attackframe++;
					break;
				}
				GunAttack (self);
				if (!ammocheat)
					gamestate.ammo--;
				DrawAmmo ();
				break;

			case 2:
				KnifeAttack (self);
				break;

			case 3:
				if (gamestate.ammo && buttonstate[bt_attack])
					gamestate.attackframe -= 2;
				break;
		}

		gamestate.attackcount += cur->tics;
		gamestate.attackframe++;
		gamestate.weaponframe =
			attackinfo[gamestate.weapon][gamestate.attackframe].frame;
	}
}



//===========================================================================

/*
===============
=
= T_Player
=
===============
*/

ACTION_FUNCTION(T_Player)
{
	if (gamestate.victoryflag)              // watching the BJ actor
	{
		VictorySpin ();
		return;
	}

	UpdateFace ();
	CheckWeaponChange ();

	if ( buttonstate[bt_use] )
		Cmd_Use ();

	if ( buttonstate[bt_attack] && !buttonheld[bt_attack])
		Cmd_Fire ();

	ControlMovement (self);
	if (gamestate.victoryflag)              // watching the BJ actor
		return;

	plux = (word) (player->x >> UNSIGNEDSHIFT);                     // scale to fit in unsigned
	pluy = (word) (player->y >> UNSIGNEDSHIFT);
	player->tilex = (short)(player->x >> TILESHIFT);                // scale to tile values
	player->tiley = (short)(player->y >> TILESHIFT);
}
