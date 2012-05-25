#ifndef __WL_AGENT__
#define __WL_AGENT__

#include "wl_def.h"
#include "a_playerpawn.h"
#include "weaponslots.h"

/*
=============================================================================

							WL_AGENT DEFINITIONS

=============================================================================
*/

extern  int      facecount, facetimes;
extern  int32_t  thrustspeed;
extern  AActor   *LastAttacker;

void    Cmd_Use ();
void    Thrust (angle_t angle, int32_t speed);
void    SpawnPlayer (int tilex, int tiley, int dir);
void    TakeDamage (int points,AActor *attacker);
void    GivePoints (int32_t points);
void    GetBonus (statobj_t *check);
void    VictorySpin ();

//
// player state info
//

void	DrawStatusBar();
void    StatusDrawFace(unsigned picnum);
void    GiveExtraMan (void);
void    UpdateFace ();
void    CheckWeaponChange ();
void    ControlMovement (AActor *self);

////////////////////////////////////////////////////////////////////////////////

class AWeapon;

#define WP_NOCHANGE ((AWeapon*)~0)

extern class player_t
{
	public:
		void	BringUpWeapon();
		void	Reborn();
		void	SetPSprite(const Frame *frame);

		enum State
		{
			PST_ENTER,
			PST_DEAD,
			PST_REBORN,
			PST_LIVE
		};

		enum PlayerFlags
		{
			PF_WEAPONREADY = 0x1
		};

		APlayerPawn	*mo;

		int32_t		oldscore,score,nextextra;
		short		lives;
		int32_t		health;

		FWeaponSlots	weapons;
		AWeapon			*ReadyWeapon;
		AWeapon			*PendingWeapon;
		struct
		{
			const Frame	*frame;
			short		ticcount;
			fixed		sx;
			fixed		sy;
		} psprite;

		int32_t		flags;
		State		state;
} players[];

#endif
