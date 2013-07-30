#include "wl_def.h"
#include "wl_agent.h"
#include "wl_game.h"
#include "wl_play.h"
#include "textures/textures.h"
#include "id_ca.h"
#include "id_us.h"
#include "id_vh.h"
#include "scanner.h"
#include "w_wad.h"
#include "m_random.h"
#include "thingdef/thingdef.h"
#include "g_mapinfo.h"
#include "a_inventory.h"
#include "a_keys.h"

#include <cmath>
#include <climits>

/*
=============================================================================

							STATUS WINDOW STUFF

=============================================================================
*/

struct LatchConfig
{
	unsigned int Enabled;
	unsigned int Digits;
	unsigned int X;
	unsigned int Y;
};
static struct StatusBarConfig_t
{
	LatchConfig Floor, Score, Lives, Health, Ammo;

	// The following don't use the digits
	LatchConfig Mugshot, Keys, Weapon;
} StatusBarConfig = {
	{1, 2, 16, 16},
	{1, 6, 48, 16},
	{1, 1, 112, 16},
	{1, 3, 168, 16},
	{1, 3, 208, 16},
	{1, 0, 136, 4},
	{1, 0, 240, 4},
	{1, 0, 256, 8}
};

/*
==================
=
= StatusDrawPic
=
==================
*/

void StatusDrawPic (unsigned x, unsigned y, const char* pic)
{
	VWB_DrawGraphic(TexMan(pic), x, 200-(STATUSLINES-y));
}

static void StatusDrawFace(FTexture *pic)
{
	VWB_DrawGraphic(pic, StatusBarConfig.Mugshot.X, 200-(STATUSLINES-StatusBarConfig.Mugshot.Y));
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
	if((viewsize == 21 && ingame) || !StatusBarConfig.Mugshot.Enabled) return;

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
	static int oldDamageLevel = 0;
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

	const int maxHealth = players[0].mo ? players[0].mo->maxhealth : 100;
	const int damageLevel = MIN(6, players[0].health > maxHealth ? 0 : (maxHealth-players[0].health)/(maxHealth/6));
	if(damageUpdate)
	{
		// Update the face only if we've changed damage levels.
		if(damageLevel == oldDamageLevel)
			return;
		facecount = 0;
	}
	oldDamageLevel = damageLevel;

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
			gamestate.faceframe = animations[damageLevel][facePick];
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

static void LatchNumber (int x, int y, unsigned width, int32_t number, bool cap=false)
{
	static FFont *HudFont = NULL;
	if(!HudFont)
	{
		HudFont = V_GetFont("HudFont");
	}

	y = 200-(STATUSLINES-y);// + HudFont->GetHeight();

	FString str;
	str.Format("%*d", width, number);
	if(str.Len() > width && cap)
	{
		int maxval = width <= 9 ? (int) ceil(pow(10., width))-1 : INT_MAX;
		str.Format("%d", maxval);
	}

	int cwidth;
	FRemapTable *remap = HudFont->GetColorTranslation(CR_UNTRANSLATED);
	for(size_t i = MAX<size_t>(0, str.Len()-width);i < str.Len();++i)
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
	if((viewsize == 21 && ingame) || !StatusBarConfig.Health.Enabled) return;
	LatchNumber (StatusBarConfig.Health.X,StatusBarConfig.Health.Y,StatusBarConfig.Health.Digits,players[0].health,true);
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
	if((viewsize == 21 && ingame) || !StatusBarConfig.Floor.Enabled) return;
	LatchNumber (StatusBarConfig.Floor.X,StatusBarConfig.Floor.Y,StatusBarConfig.Floor.Digits,levelInfo->FloorNumber);
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
	if((viewsize == 21 && ingame) || !StatusBarConfig.Lives.Enabled) return;
	LatchNumber (StatusBarConfig.Lives.X,StatusBarConfig.Lives.Y,StatusBarConfig.Lives.Digits,players[0].lives);
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
	if((viewsize == 21 && ingame) || !StatusBarConfig.Score.Enabled) return;
	LatchNumber (StatusBarConfig.Score.X,StatusBarConfig.Score.Y,StatusBarConfig.Score.Digits,players[0].score);
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
	if((viewsize == 21 && ingame) || !StatusBarConfig.Weapon.Enabled ||
		players[0].ReadyWeapon == NULL ||
		players[0].ReadyWeapon->icon.isNull()
	)
		return;

	VWB_DrawGraphic(TexMan(players[0].ReadyWeapon->icon), StatusBarConfig.Weapon.X, 200-(STATUSLINES-StatusBarConfig.Weapon.Y));
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
	if((viewsize == 21 && ingame) || !StatusBarConfig.Keys.Enabled) return;
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

	const unsigned int x = StatusBarConfig.Keys.X;
	const unsigned int y = StatusBarConfig.Keys.Y;
	if (extendedKeysGraphics && (presentKeys & (1|4)) == (1|4))
		StatusDrawPic (x,y,"STKEYS5");
	else if(extendedKeysGraphics && (presentKeys & 4))
		StatusDrawPic (x,y,"STKEYS3");
	else if(presentKeys & 1)
		StatusDrawPic (x,y,"STKEYS1");
	else
		StatusDrawPic (x,y,"STKEYS0");

	if (extendedKeysGraphics && (presentKeys & (2|8)) == (2|8))
		StatusDrawPic (x,y+16,"STKEYS6");
	else if (extendedKeysGraphics && (presentKeys & 8))
		StatusDrawPic (x,y+16,"STKEYS4");
	else if (presentKeys & 2)
		StatusDrawPic (x,y+16,"STKEYS2");
	else
		StatusDrawPic (x,y+16,"STKEYS0");
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
	if((viewsize == 21 && ingame) || !StatusBarConfig.Ammo.Enabled ||
		!players[0].ReadyWeapon || !players[0].ReadyWeapon->ammo[AWeapon::PrimaryFire])
		return;

	unsigned int amount = players[0].ReadyWeapon->ammo[AWeapon::PrimaryFire]->amount;
	LatchNumber (StatusBarConfig.Ammo.X,StatusBarConfig.Ammo.Y,StatusBarConfig.Ammo.Digits,amount,true);
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

//===========================================================================

void SetupStatusbar()
{
	// Temporary configuration lump so that some mods can be ported to ECWolf
	// before a proper solution is created.
	// ---> WILL BE REMOVED <---

	int lastLump = 0;
	int lumpnum = 0;
	while((lumpnum = Wads.FindLump("LATCHCFG", &lastLump)) != -1)
	{
		FMemLump lump = Wads.ReadLump(lumpnum);
		Scanner sc((const char*)(lump.GetMem()), lump.GetSize());
		sc.SetScriptIdentifier(Wads.GetLumpFullName(lumpnum));
		sc.ScriptMessage(Scanner::WARNING, "Utilizing temporary status bar configuration script.");

		while(sc.TokensLeft())
		{
			sc.MustGetToken(TK_Identifier);
			FString key = sc->str;
			key.ToLower();
			sc.MustGetToken('=');
			sc.MustGetToken(TK_IntConst);
			unsigned int value = sc->number;

			LatchConfig *var = NULL;
			FString extrakey;
			if(key.IndexOf("ammo") == 0)
			{
				extrakey = key.Mid(4);
				var = &StatusBarConfig.Ammo;
			}
			else if(key.IndexOf("floor") == 0)
			{
				extrakey = key.Mid(5);
				var = &StatusBarConfig.Floor;
			}
			else if(key.IndexOf("health") == 0)
			{
				extrakey = key.Mid(6);
				var = &StatusBarConfig.Health;
			}
			else if(key.IndexOf("keys") == 0)
			{
				extrakey = key.Mid(4);
				var = &StatusBarConfig.Keys;
			}
			else if(key.IndexOf("lives") == 0)
			{
				extrakey = key.Mid(5);
				var = &StatusBarConfig.Lives;
			}
			else if(key.IndexOf("mugshot") == 0)
			{
				extrakey = key.Mid(7);
				var = &StatusBarConfig.Mugshot;
			}
			else if(key.IndexOf("score") == 0)
			{
				extrakey = key.Mid(5);
				var = &StatusBarConfig.Score;
			}
			else if(key.IndexOf("weapon") == 0)
			{
				extrakey = key.Mid(6);
				var = &StatusBarConfig.Weapon;
			}
			else
				sc.ScriptMessage(Scanner::ERROR, "Unknown key '%s'.\n", key.GetChars());

			if(extrakey.Compare("enabled") == 0)
				var->Enabled = value;
			else if(extrakey.Compare("digits") == 0)
				var->Digits = value;
			else if(extrakey.Compare("x") == 0)
				var->X = value;
			else if(extrakey.Compare("y") == 0)
				var->Y = value;
			else
				sc.ScriptMessage(Scanner::ERROR, "Unknown key '%s'.\n", key.GetChars());
		}
	}
}
