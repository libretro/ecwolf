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

static void StatusDrawFace(FTexture *pic)
{
	VWB_DrawGraphic(pic, 136, 164);
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
	if(viewsize == 21 && ingame) return;

	if(!gamestate.faceframe.isValid())
	{
		facecount = 0;
		UpdateFace();
	}

	if (players[0].health)
		StatusDrawFace(TexMan(gamestate.faceframe));
	else
	{
		// TODO: Make this work based on damage types.
		static const ClassDef *needle = ClassDef::FindClass("Needle");
		if (players[0].killerobj && players[0].killerobj->GetClass() == needle)
			StatusDrawFace(TexMan("STFMUT0"));
		else
			StatusDrawFace(TexMan("STFDEAD0"));
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

void WeaponGrin ()
{
	static FTextureID grin = TexMan.CheckForTexture("STFEVL0", FTexture::TEX_Any);
	gamestate.faceframe = grin;
	facecount = 140;
}

void UpdateFace (bool damageUpdate)
{
	static int oldHealth = 100;
	static bool noGodFace = false;
	static FTextureID godmodeFace[3] = { TexMan.CheckForTexture("STFGOD0", FTexture::TEX_Any), TexMan.CheckForTexture("STFGOD1", FTexture::TEX_Any), TexMan.CheckForTexture("STFGOD2", FTexture::TEX_Any) };
	static FTextureID waitFace[2] = { TexMan.CheckForTexture("STFWAIT0", FTexture::TEX_Any), TexMan.CheckForTexture("STFWAIT1", FTexture::TEX_Any) };
	static FTextureID animations[7][3] =
	{
		{ TexMan.CheckForTexture("STFST00", FTexture::TEX_Any), TexMan.CheckForTexture("STFST01", FTexture::TEX_Any), TexMan.CheckForTexture("STFST02", FTexture::TEX_Any) },
		{ TexMan.CheckForTexture("STFST10", FTexture::TEX_Any), TexMan.CheckForTexture("STFST11", FTexture::TEX_Any), TexMan.CheckForTexture("STFST12", FTexture::TEX_Any) },
		{ TexMan.CheckForTexture("STFST20", FTexture::TEX_Any), TexMan.CheckForTexture("STFST21", FTexture::TEX_Any), TexMan.CheckForTexture("STFST22", FTexture::TEX_Any) },
		{ TexMan.CheckForTexture("STFST30", FTexture::TEX_Any), TexMan.CheckForTexture("STFST31", FTexture::TEX_Any), TexMan.CheckForTexture("STFST32", FTexture::TEX_Any) },
		{ TexMan.CheckForTexture("STFST40", FTexture::TEX_Any), TexMan.CheckForTexture("STFST41", FTexture::TEX_Any), TexMan.CheckForTexture("STFST42", FTexture::TEX_Any) },
		{ TexMan.CheckForTexture("STFST50", FTexture::TEX_Any), TexMan.CheckForTexture("STFST51", FTexture::TEX_Any), TexMan.CheckForTexture("STFST52", FTexture::TEX_Any) },
		{ TexMan.CheckForTexture("STFST60", FTexture::TEX_Any), TexMan.CheckForTexture("STFST61", FTexture::TEX_Any), TexMan.CheckForTexture("STFST62", FTexture::TEX_Any) },
	};

	if(damageUpdate)
	{
		// Update the face only if we've changed damage levels.
		if((100-players[0].health)/16 == (100-oldHealth)/16)
			return;
		facecount = 0;
	}
	oldHealth = players[0].health;

	// OK Wolf apparently did something more along the lines of ++facecount > M_Random()
	// This doesn't seem to work as well with the new random generator, so lets take a different approach.
	if (--facecount <= 0)
	{
		facecount = ((M_Random()>>3)|0xF);

		if (funnyticount > 301 * 70)
		{
			funnyticount = 0;
			FTextureID pickedID = waitFace[M_Random() & 1];
			if(pickedID.isValid())
			{
				gamestate.faceframe = pickedID;
				facecount = 17;
				return;
			}
		}

		unsigned int facePick = M_Random()%3;

		if(godmode && !noGodFace)
		{
			gamestate.faceframe = godmodeFace[facePick];

			if(!gamestate.faceframe.isValid())
			{
				if(!godmodeFace[0].isValid())
					noGodFace = true;
				godmodeFace[1] = godmodeFace[2] = godmodeFace[0];
			}
			else
				return;
		}

		if(players[0].mo)
			gamestate.faceframe = animations[(100-players[0].health)/16][facePick];
		else
			gamestate.faceframe = animations[0][0];
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

	static FTextureID ouchFace = TexMan.CheckForTexture("STFOUCH0", FTexture::TEX_Any);
	if(ouchFace.isValid() && points > 30 && players[0].health != 0)
	{
		gamestate.faceframe = ouchFace;
		facecount = 17;
	}
	else
		UpdateFace(true);
	DrawStatusBar();
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

void GiveExtraMan (int amount)
{
	players[0].lives += amount;
	if (players[0].lives < 0)
		players[0].lives = 0;
	else if(players[0].lives > 9)
		players[0].lives = 9;
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
		GiveExtraMan (1);
	}
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
	static bool extendedKeysGraphics = TexMan.CheckForTexture("STKEYS3", FTexture::TEX_Any).isValid();

	// Find keys in inventory
	int presentKeys = 0;
	if(players[0].mo)
	{
		for(AInventory *item = players[0].mo->inventory;item != NULL;item = item->inventory)
		{
			if(item->IsKindOf(NATIVE_CLASS(Key)))
			{
				int slot = static_cast<AKey *>(item)->KeyNumber;
				if(slot <= 4)
					presentKeys |= 1<<(slot-1);
				if(presentKeys == 15)
					break;
			}
		}
	}

	if (extendedKeysGraphics && (presentKeys & (1|4)) == (1|4))
		StatusDrawPic (30,4,"STKEYS5");
	else if(extendedKeysGraphics && (presentKeys & 4))
		StatusDrawPic (30,4,"STKEYS3");
	else if(presentKeys & 1)
		StatusDrawPic (30,4,"STKEYS1");
	else
		StatusDrawPic (30,4,"STKEYS0");

	if (extendedKeysGraphics && (presentKeys & (2|8)) == (2|8))
		StatusDrawPic (30,20,"STKEYS6");
	else if (extendedKeysGraphics && (presentKeys & 8))
		StatusDrawPic (30,20,"STKEYS4");
	else if (presentKeys & 2)
		StatusDrawPic (30,20,"STKEYS2");
	else
		StatusDrawPic (30,20,"STKEYS0");
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

static bool TryMove (AActor *ob)
{
	if (noclip)
	{
		return (ob->x-ob->radius >= 0 && ob->y-ob->radius >= 0
			&& ob->x+ob->radius < (((int32_t)(map->GetHeader().width))<<TILESHIFT)
			&& ob->y+ob->radius < (((int32_t)(map->GetHeader().height))<<TILESHIFT) );
	}

	int xl,yl,xh,yh,x,y;
	AActor *check;

	xl = (ob->x-ob->radius) >>TILESHIFT;
	yl = (ob->y-ob->radius) >>TILESHIFT;

	xh = (ob->x+ob->radius) >>TILESHIFT;
	yh = (ob->y+ob->radius) >>TILESHIFT;

	//
	// check for solid walls
	//
	for (y=yl;y<=yh;y++)
	{
		for (x=xl;x<=xh;x++)
		{
			const bool checkLines[4] =
			{
				(ob->x+ob->radius) > ((x+1)<<TILESHIFT),
				(ob->y-ob->radius) < (y<<TILESHIFT),
				(ob->x-ob->radius) < (x<<TILESHIFT),
				(ob->y+ob->radius) > ((y+1)<<TILESHIFT)
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
						if(spot->sideSolid[i] && spot->slideAmount[i] != 0xffff && checkLines[i])
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

static void ExecuteWalkTriggers(MapSpot spot, MapTrigger::Side dir)
{
	if(!spot)
		return;

	for(unsigned int i = spot->triggers.Size();i-- > 0;)
	{
		MapTrigger &trigger = spot->triggers[i];
		if(trigger.playerCross && trigger.activate[dir])
			map->ActivateTrigger(trigger, dir, players[0].mo);
	}
}

static void CheckWalkTriggers(AActor *ob, int32_t xmove, int32_t ymove)
{
	MapSpot spot;

	if(ob->fracx <= abs(xmove) || ob->fracx >= 0xFFFF-abs(xmove))
	{
		spot = map->GetSpot((ob->x-xmove)>>FRACBITS, ob->y>>FRACBITS, 0);
		if(xmove > 0)
			ExecuteWalkTriggers(spot->GetAdjacent(MapTile::East), MapTrigger::West);
		else if(xmove < 0)
			ExecuteWalkTriggers(spot->GetAdjacent(MapTile::West), MapTrigger::East);
	}

	if(ob->fracy <= abs(ymove) || ob->fracy >= 0xFFFF-abs(ymove))
	{
		spot = map->GetSpot(ob->x>>FRACBITS, (ob->y-ymove)>>FRACBITS, 0);
		if(ymove > 0)
			ExecuteWalkTriggers(spot->GetAdjacent(MapTile::South), MapTrigger::North);
		else if(ymove < 0)
			ExecuteWalkTriggers(spot->GetAdjacent(MapTile::North), MapTrigger::South);
	}
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
	fixed basex = ob->x;
	fixed basey = ob->y;

	ob->x = basex+xmove;
	ob->y = basey+ymove;

	if (TryMove (ob))
	{
		CheckWalkTriggers(ob, xmove, ymove);
		return;
	}

	if (!SD_SoundPlaying())
		SD_PlaySound ("world/hitwall");

	ob->x = basex+xmove;
	ob->y = basey;
	if (TryMove (ob))
	{
		CheckWalkTriggers(ob, xmove, 0);
		return;
	}

	ob->x = basex;
	ob->y = basey+ymove;
	if (TryMove (ob))
	{
		CheckWalkTriggers(ob, 0, ymove);
		return;
	}

	ob->x = basex;
	ob->y = basey;
}

//==========================================================================

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
	if (speed)
		funnyticount = 0;

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
				P_ChangeSwitchTexture(spot, static_cast<MapTile::Side>((direction+2)%4), trig.repeatable, trig.action);
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

player_t::player_t() : attackheld(false)
{
}

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
	GC::Mark(camera);
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
		<< camera
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

	if(arc.IsLoading())
	{
		mo->SetupWeaponSlots();
		CalcProjection(mo->radius);
	}
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
	players[0].mo = (APlayerPawn *) AActor::Spawn(gamestate.playerClass, ((int32_t)tilex<<TILESHIFT)+TILEGLOBAL/2, ((int32_t)tiley<<TILESHIFT)+TILEGLOBAL/2, 0, false);
	players[0].mo->angle = (450-dir)*ANGLE_1;
	players[0].mo->player = &players[0];
	Thrust (0,0); // set some variables

	if(players[0].state == player_t::PST_ENTER || players[0].state == player_t::PST_REBORN)
		players[0].Reborn();

	players[0].camera = players[0].mo;
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

ACTION_FUNCTION(A_FireCustomMissile)
{
	ACTION_PARAM_STRING(missiletype, 0);
	ACTION_PARAM_DOUBLE(angleOffset, 1);
	ACTION_PARAM_BOOL(useammo, 2);
	ACTION_PARAM_INT(spawnoffset, 3);
	ACTION_PARAM_INT(spawnheight, 4);
	ACTION_PARAM_BOOL(aim, 5);

	if(useammo && !players[0].ReadyWeapon->DepleteAmmo())
		return;

	fixed newx = self->x + spawnoffset*finesine[self->angle>>ANGLETOFINESHIFT]/64;
	fixed newy = self->y + spawnoffset*finecosine[self->angle>>ANGLETOFINESHIFT]/64;

	angle_t iangle = self->angle + (angle_t) ((angleOffset*ANGLE_45)/45);

	const ClassDef *cls = ClassDef::FindClass(missiletype);
	if(!cls)
		return;
	AActor *newobj = AActor::Spawn(cls, newx, newy, 0, true);
	newobj->flags |= FL_PLAYERMISSILE;
	newobj->angle = iangle;

	newobj->velx = FixedMul(newobj->speed,finecosine[iangle>>ANGLETOFINESHIFT]);
	newobj->vely = -FixedMul(newobj->speed,finesine[iangle>>ANGLETOFINESHIFT]);
}
