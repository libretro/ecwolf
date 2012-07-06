// WL_AGENT.C

#include "wl_def.h"
#include "id_ca.h"
#include "id_sd.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#include "actor.h"
#include "thingdef/thingdef.h"
#include "lnspec.h"
#include "wl_agent.h"
#include "a_inventory.h"
#include "a_keys.h"
#include "m_random.h"
#include "g_mapinfo.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_state.h"
#include "wl_play.h"

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
int32_t			thrustspeed;

AActor			*LastAttacker;

player_t		players[1];

void ClipMove (AActor *ob, int32_t xmove, int32_t ymove);

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
	AWeapon *newWeapon = NULL;

	if(buttonstate[bt_nextweapon] && !buttonheld[bt_nextweapon])
	{
		newWeapon = players[0].PendingWeapon = players[0].weapons.PickNextWeapon(&players[0]);
	}
	else if(buttonstate[bt_prevweapon] && !buttonheld[bt_prevweapon])
	{
		newWeapon = players[0].PendingWeapon = players[0].weapons.PickPrevWeapon(&players[0]);
	}
	else
	{
		for(int i = 0;i <= 9;++i)
		{
			if(buttonstate[bt_slot0 + i] && !buttonheld[bt_slot0 + i])
			{
				newWeapon = players[0].weapons.Slots[i].PickWeapon(&players[0]);
				break;
			}
		}
	}

	if(newWeapon && newWeapon != players[0].ReadyWeapon)
		players[0].PendingWeapon = newWeapon;
}


/*
=======================
=
= ControlMovement
=
= Takes controlx,controly, and buttonstate[bt_strafe]
=
= Changes the players[0].mo's angle and position
=
= There is an angle hack because when going 70 fps, the roundoff becomes
= significant
=
=======================
*/

void ControlMovement (AActor *ob)
{
	int32_t oldx,oldy;
	angle_t angle;
	int strafe = 0;

	thrustspeed = 0;

	oldx = players[0].mo->x;
	oldy = players[0].mo->y;

	if(buttonstate[bt_strafeleft])
	{
		if((!alwaysrun && buttonstate[bt_run]) || (alwaysrun && !buttonstate[bt_run]))
			strafe -= RUNMOVE;
		else
			strafe -= BASEMOVE;
	}

	if(buttonstate[bt_straferight])
	{
		if((!alwaysrun && buttonstate[bt_run]) || (alwaysrun && !buttonstate[bt_run]))
			strafe += RUNMOVE;
		else
			strafe += BASEMOVE;
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
		strafe += controlx;
	}
	else
	{
		//
		// not strafing
		//
		ob->angle -= controlx*(ANGLE_1/ANGLESCALE);
	}

	if(strafe)
	{
		// Cap the speed
		if (strafe > 100)
			strafe = 100;
		else if (strafe < -100)
			strafe = -100;

		if (strafe > 0)
		{
			angle = ob->angle - ANGLE_90;
			Thrust (angle,strafe*MOVESCALE);      // move to left
		}
		else if (strafe < 0)
		{
			angle = ob->angle + ANGLE_90;
			Thrust (angle,-strafe*MOVESCALE);     // move to right
		}
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
		angle = ob->angle + ANGLE_180;
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
	VWB_DrawGraphic(TexMan(pic), x*8, 200-(STATUSLINES-y));
}

void StatusDrawFace(const char* pic)
{
	VWB_DrawGraphic(TexMan(pic), 136, 164);
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
	else if (players[0].health)
	{
#ifdef SPEAR
		if (godmode)
			StatusDrawFace(godmode[gamestate.faceframe]);
		else
#endif
			StatusDrawFace(animations[(100-players[0].health)/16][gamestate.faceframe]);
	}
	else
	{
#ifndef SPEAR
		//if (LastAttacker && LastAttacker->obclass == needleobj)
		//	StatusDrawFace("STFMUT0");
		//else
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

	++facecount;
	if (facecount > M_Random())
	{
		gamestate.faceframe = (M_Random()>>6);
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
	static FFont *HudFont = NULL;
	if(!HudFont)
	{
		HudFont = V_GetFont("HudFont");
	}

	x *= 8;
	y = 200-(STATUSLINES-y);// + HudFont->GetHeight();

	FString str;
	str.Format("%*d", width, number);

	int cwidth;
	FRemapTable *remap = HudFont->GetColorTranslation(CR_UNTRANSLATED);
	for(unsigned int i = 0;i < str.Len();++i)
	{
		VWB_DrawGraphic(HudFont->GetChar(str[i], &cwidth), x, y, MENU_NONE, remap);
		x += cwidth;
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
	LatchNumber (21,16,3,players[0].health);
}


/*
===============
=
= TakeDamage
=
===============
*/

void TakeDamage (int points,AActor *attacker)
{
	LastAttacker = attacker;

	if (gamestate.victoryflag)
		return;
	if (gamestate.difficulty==gd_baby)
		points>>=2;

	if (!godmode)
		players[0].health -= points;

	if (players[0].health<=0)
	{
		players[0].health = 0;
		playstate = ex_died;
		players[0].killerobj = attacker;
	}

	if (godmode != 2)
		StartDamageFlash (points);

	DrawHealth ();
	DrawFace ();

	//
	// MAKE BJ'S EYES BUG IF MAJOR DAMAGE!
	//
#ifdef SPEAR
	if (points > 30 && players[0].health!=0 && !godmode && viewsize != 21)
	{
		StatusDrawFace("STFOUCH0");
		facecount = 0;
	}
#endif
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
	LatchNumber (2,16,2,levelInfo->FloorNumber);
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
	LatchNumber (14,16,1,players[0].lives);
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
	if (players[0].lives<9)
		players[0].lives++;
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
	LatchNumber (6,16,6,players[0].score);
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
	players[0].score += points;
	while (players[0].score >= players[0].nextextra)
	{
		players[0].nextextra += EXTRAPOINTS;
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
	if((viewsize == 21 && ingame) ||
		players[0].ReadyWeapon == NULL ||
		players[0].ReadyWeapon->icon.isNull()
	)
		return;

	VWB_DrawGraphic(TexMan(players[0].ReadyWeapon->icon), 256, 168);
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

	// Find keys in inventory
	int presentKeys = 0;
	if(players[0].mo)
	{
		for(AInventory *item = players[0].mo->inventory;item != NULL;item = item->inventory)
		{
			if(item->IsKindOf(NATIVE_CLASS(Key)))
			{
				int slot = static_cast<AKey *>(item)->KeyNumber;
				if(slot == 1 || slot == 2)
					presentKeys |= 1<<(slot-1);
				if(presentKeys == 3)
					break;
			}
		}
	}

	if (presentKeys & 1)
		StatusDrawPic (30,4,"GOLDKEY");
	else
		StatusDrawPic (30,4,"NOKEY");

	if (presentKeys & 2)
		StatusDrawPic (30,20,"SILVRKEY");
	else
		StatusDrawPic (30,20,"NOKEY");
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
	if((viewsize == 21 && ingame) ||
		!players[0].ReadyWeapon || !players[0].ReadyWeapon->ammo1)
		return;

	unsigned int amount = players[0].ReadyWeapon->ammo1->amount;
	LatchNumber (26,16,3,amount);
}

//===========================================================================

void DrawStatusBar()
{
	if(viewsize == 21 && ingame)
		return;

	VWB_DrawGraphic(TexMan("STBAR"), 0, 160);
	DrawFace ();
	DrawHealth ();
	DrawLives ();
	DrawLevel ();
	DrawAmmo ();
	DrawKeys ();
	DrawWeapon ();
	DrawScore ();
}


/*
=============================================================================

								MOVEMENT

=============================================================================
*/

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

	xl = (ob->x-players[0].mo->radius) >>TILESHIFT;
	yl = (ob->y-players[0].mo->radius) >>TILESHIFT;

	xh = (ob->x+players[0].mo->radius) >>TILESHIFT;
	yh = (ob->y+players[0].mo->radius) >>TILESHIFT;

	//
	// check for solid walls
	//
	for (y=yl;y<=yh;y++)
	{
		for (x=xl;x<=xh;x++)
		{
			const bool checkLines[4] =
			{
				(ob->x+players[0].mo->radius) > ((x+1)<<TILESHIFT),
				(ob->y-players[0].mo->radius) < (y<<TILESHIFT),
				(ob->x-players[0].mo->radius) < (x<<TILESHIFT),
				(ob->y+players[0].mo->radius) > ((y+1)<<TILESHIFT)
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
							if(ob->y-ob->radius <= static_cast<fixed>((y<<TILESHIFT)+((63-spot->pushAmount)<<10)))
								return false;
							break;
						case MapTile::West:
							if(ob->x-ob->radius <= static_cast<fixed>((x<<TILESHIFT)+((63-spot->pushAmount)<<10)))
								return false;
							break;
						case MapTile::East:
							if(ob->x+ob->radius >= static_cast<fixed>((x<<TILESHIFT)+(spot->pushAmount<<10)))
								return false;
							break;
						case MapTile::South:
							if(ob->y+ob->radius >= static_cast<fixed>((y<<TILESHIFT)+(spot->pushAmount<<10)))
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
	for(AActor::Iterator *next = NULL, *iter = AActor::GetIterator();iter;iter = next)
	{
		next = iter->Next();
		if(iter->Item() == ob)
			continue;

		check = iter->Item();

		fixed r = check->radius + ob->radius;
		if(check->flags & FL_SOLID)
		{
			if(abs(ob->x - check->x) > r ||
				abs(ob->y - check->y) > r)
				continue;
			return false;
		}
		else
		{
			if(abs(ob->x - check->x) <= r &&
				abs(ob->y - check->y) <= r)
				check->Touch(ob);
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

void ClipMove (AActor *ob, int32_t xmove, int32_t ymove)
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

void Thrust (angle_t angle, int32_t speed)
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

	xmove = FixedMul(speed,finecosine[angle>>ANGLETOFINESHIFT]);
	ymove = -FixedMul(speed,finesine[angle>>ANGLETOFINESHIFT]);

	ClipMove(players[0].mo,xmove,ymove);

	players[0].mo->EnterZone(map->GetSpot(players[0].mo->tilex, players[0].mo->tiley, 0)->zone);
}


/*
=============================================================================

								ACTIONS

=============================================================================
*/

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
	if (players[0].mo->angle < ANGLE_45 || players[0].mo->angle > 7*ANGLE_45)
	{
		checkx = players[0].mo->tilex + 1;
		checky = players[0].mo->tiley;
		direction = MapTrigger::East;
	}
	else if (players[0].mo->angle < 3*ANGLE_45)
	{
		checkx = players[0].mo->tilex;
		checky = players[0].mo->tiley-1;
		direction = MapTrigger::North;
	}
	else if (players[0].mo->angle < 5*ANGLE_45)
	{
		checkx = players[0].mo->tilex - 1;
		checky = players[0].mo->tiley;
		direction = MapTrigger::West;
	}
	else
	{
		checkx = players[0].mo->tilex;
		checky = players[0].mo->tiley + 1;
		direction = MapTrigger::South;
	}

	bool doNothing = true;
	MapSpot spot = map->GetSpot(checkx, checky, 0);
	for(unsigned int i = 0;i < spot->triggers.Size();++i)
	{
		MapTrigger &trig = spot->triggers[i];
		if(trig.activate[direction] && trig.playerUse)
		{
			if(map->ActivateTrigger(trig, direction, players[0].mo))
			{
				//buttonstate[bt_use] = false;
				doNothing = false;
			}
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

const fixed RAISERANGE = 96*FRACUNIT;
const fixed RAISESPEED = FRACUNIT*6;

void player_t::BringUpWeapon()
{
	psprite.sy = RAISERANGE;
	psprite.sx = 0;

	if(PendingWeapon != WP_NOCHANGE)
	{
		ReadyWeapon = PendingWeapon;
		PendingWeapon = WP_NOCHANGE;
		SetPSprite(ReadyWeapon->GetUpState());
	}
	else
	{
		ReadyWeapon = NULL;
		SetPSprite(NULL);
	}
}
ACTION_FUNCTION(A_Lower)
{
	player_t *player = self->player;

	player->psprite.sy += RAISESPEED;
	if(player->psprite.sy < RAISERANGE)
		return;
	player->psprite.sy = RAISERANGE;

	player->BringUpWeapon();
}
ACTION_FUNCTION(A_Raise)
{
	player_t *player = self->player;

	if(player->PendingWeapon != WP_NOCHANGE)
	{
		player->SetPSprite(player->ReadyWeapon->GetDownState());
		return;
	}

	player->psprite.sy -= RAISESPEED;
	if(player->psprite.sy > 0)
		return;
	player->psprite.sy = 0;

	if(player->ReadyWeapon)
		player->SetPSprite(player->ReadyWeapon->GetReadyState());
	else
		player->psprite.frame = NULL;
}

size_t player_t::PropagateMark()
{
	GC::Mark(mo);
	GC::Mark(ReadyWeapon);
	if(PendingWeapon != WP_NOCHANGE)
		GC::Mark(PendingWeapon);
	return sizeof(*this);
}

void player_t::Reborn()
{
	ReadyWeapon = NULL;
	PendingWeapon = WP_NOCHANGE;
	flags = 0;

	if(state == PST_ENTER)
	{
		lives = 3;
		score = oldscore = 0;
		nextextra = EXTRAPOINTS;
	}

	mo->GiveStartingInventory();
	health = mo->maxhealth;

	// Recalculate the projection here so that player classes with differing radii are supported.
	CalcProjection(mo->radius);

	DrawStatusBar();
}

void player_t::Serialize(FArchive &arc)
{
	BYTE state = this->state;
	arc << state;
	this->state = static_cast<State>(state);

	arc << mo
		<< killerobj
		<< oldscore
		<< score
		<< nextextra
		<< lives
		<< health
		<< ReadyWeapon
		<< PendingWeapon
		<< flags;

	arc << psprite.frame
		<< psprite.ticcount
		<< psprite.sx
		<< psprite.sy;

	mo->SetupWeaponSlots();
	CalcProjection(mo->radius);
}

void player_t::SetPSprite(const Frame *frame)
{
	flags &= ~player_t::PF_WEAPONREADY;
	psprite.frame = frame;

	if(frame)
	{
		psprite.ticcount = frame->duration;
		frame->action(mo);
	}
}

FArchive &operator<< (FArchive &arc, player_t *&player)
{
	return arc.SerializePointer(players, (BYTE**)&player, sizeof(players[0]));
}

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
	players[0].mo = (APlayerPawn *) AActor::Spawn(playerClass, ((int32_t)tilex<<TILESHIFT)+TILEGLOBAL/2, ((int32_t)tiley<<TILESHIFT)+TILEGLOBAL/2, 0);
	players[0].mo->angle = (450-dir)*ANGLE_1;
	players[0].mo->player = &players[0];
	Thrust (0,0); // set some variables

	if(players[0].state == player_t::PST_ENTER || players[0].state == player_t::PST_REBORN)
		players[0].Reborn();

	players[0].state = player_t::PST_LIVE;
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

static FRandom pr_cwpunch("CustomWpPunch");
ACTION_FUNCTION(A_CustomPunch)
{
	enum
	{
		CPF_USEAMMO = 1,
		CPF_ALWAYSPLAYSOUND = 2
	};

	ACTION_PARAM_INT(damage, 0);
	ACTION_PARAM_BOOL(norandom, 1);
	ACTION_PARAM_INT(flags, 2);
	ACTION_PARAM_STRING(pufftype, 3);
	ACTION_PARAM_DOUBLE(range, 4);
	ACTION_PARAM_DOUBLE(lifesteal, 5);

	player_t *player = self->player;

	if(flags & CPF_ALWAYSPLAYSOUND)
		SD_PlaySound(player->ReadyWeapon->attacksound, SD_WEAPONS);
	if(range == 0)
		range = 64;

	// actually fire
	int dist = 0x7fffffff;
	AActor *closest = NULL;
	for(AActor::Iterator *check = AActor::GetIterator();check;check = check->Next())
	{
		if(check->Item() == self)
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

	if (!closest || dist-(FRACUNIT/2) > (range/64)*FRACUNIT)
	{
		// missed
		return;
	}

	if(!norandom)
		damage *= pr_cwpunch()%8 + 1;

	// hit something
	if(!(flags & CPF_ALWAYSPLAYSOUND))
		SD_PlaySound(player->ReadyWeapon->attacksound, SD_WEAPONS);
	DamageActor(closest, damage);

	// Ammo is only used when hit
	if(flags & CPF_USEAMMO)
	{
		if(!player->ReadyWeapon->DepleteAmmo())
			return;
	}

	if(lifesteal > 0 && player->health < self->health)
	{
		damage *= lifesteal;
		player->health += damage;
		if(player->health > self->health)
			player->health = self->health;
		DrawHealth();
		DrawFace();
	}
}

static FRandom pr_cwbullet("CustomWpBullet");
ACTION_FUNCTION(A_GunAttack)
{
	enum
	{
		WAF_NORANDOM = 1
	};

	player_t *player = self->player;
	AActor *closest=NULL,*oldclosest=NULL;
	int      dx,dy,dist;
	int32_t  viewdist;

	ACTION_PARAM_INT(flags, 0);
	ACTION_PARAM_STRING(sound, 1);
	ACTION_PARAM_DOUBLE(snipe, 2);
	ACTION_PARAM_INT(maxdamage, 3);
	ACTION_PARAM_INT(blocksize, 4);
	ACTION_PARAM_INT(pointblank, 5);
	ACTION_PARAM_INT(longrange, 6);
	ACTION_PARAM_INT(maxrange, 7);

	if(!player->ReadyWeapon->DepleteAmmo())
		return;

	if(sound.Len() == 1 && sound[0] == '*')
		SD_PlaySound(player->ReadyWeapon->attacksound, SD_WEAPONS);
	else
		SD_PlaySound(sound, SD_WEAPONS);

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
			if(check->Item() == self)
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
	dx = ABS(closest->x - players[0].mo->x);
	dy = ABS(closest->y - players[0].mo->y);
	dist = dx>dy ? dx:dy;

	dist = FixedMul(dist, snipe*FRACUNIT);
	dist /= blocksize<<9;

	int damage = flags & WAF_NORANDOM ? maxdamage : (1 + (pr_cwbullet()%maxdamage));
	if (dist >= pointblank)
		damage = damage * 2 / 3;
	if (dist >= longrange)
	{
		if ( (pr_cwbullet() % maxrange) < dist)           // missed
			return;
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

	if (players[0].mo->angle > 270)
	{
		players[0].mo->angle -= (short)(tics * 3);
		if (players[0].mo->angle < 270)
			players[0].mo->angle = 270;
	}
	else if (players[0].mo->angle < 270)
	{
		players[0].mo->angle += (short)(tics * 3);
		if (players[0].mo->angle > 270)
			players[0].mo->angle = 270;
	}

	desty = (((int32_t)players[0].mo->tiley-5)<<TILESHIFT)-0x3000;

	if (players[0].mo->y > desty)
	{
		players[0].mo->y -= tics*4096;
		if (players[0].mo->y < desty)
			players[0].mo->y = desty;
	}
}
