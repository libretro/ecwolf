/*
** a_inventory.h
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#ifndef __A_INVENTORY_H__
#define __A_INVENTORY_H__

#include "actor.h"
#include "textures/textures.h"

class AInventory : public AActor
{
	DECLARE_NATIVE_CLASS(Inventory, Actor)
	HAS_OBJECT_POINTERS

	public:
		virtual void	AttachToOwner(AActor *owner);
		virtual void	DetachFromOwner();
		virtual void	Destroy();
		virtual bool	HandlePickup(AInventory *item, bool &good);
		void			Serialize(FArchive &arc);
		void			Touch(AActor *toucher);
		virtual bool	TryPickup(AActor *toucher);
		virtual bool	Use();

		flagstype_t		itemFlags;
		TObjPtr<AActor>	owner;

		FNameNoInit		pickupsound;
		unsigned int	amount;
		unsigned int	maxamount;
		unsigned int	interhubamount;
		FTextureID		icon;
	protected:
		virtual AInventory	*CreateCopy(AActor *holder);
		void				GoAwayAndDie();
		bool				GoesAway();
};

class AAmmo : public AInventory
{
	DECLARE_NATIVE_CLASS(Ammo, Inventory)

	public:
		const ClassDef	*GetAmmoType();
		bool			HandlePickup(AInventory *item, bool &good);

	protected:
		AInventory		*CreateCopy(AActor *holder);
};

class ACustomInventory : public AInventory
{
	DECLARE_NATIVE_CLASS(CustomInventory, Inventory)

	public:
		bool	TryPickup(AActor *toucher);

	protected:
		bool	ExecuteState(AActor *context, const Frame *frame);
};

class AHealth : public AInventory
{
	DECLARE_NATIVE_CLASS(Health, Inventory)

	protected:
		bool	TryPickup(AActor *toucher);
};

enum
{
	AWMETA_Start = 0x01000,

	AWMETA_SelectionOrder,
	AWMETA_SlotNumber,
	AWMETA_SlotPriority
};

class AWeapon : public AInventory
{
	DECLARE_NATIVE_CLASS(Weapon, Inventory)
	HAS_OBJECT_POINTERS

	public:
		enum FireMode
		{
			PrimaryFire,
			AltFire,
			EitherFire
		};

		enum EBobStyle
		{
			BobNormal,
			BobInverse,
			BobAlpha,
			BobInverseAlpha,
			BobSmooth,
			BobInverseSmooth
		};

		void		AttachToOwner(AActor *owner);
		bool		CheckAmmo(FireMode fireMode, bool autoSwitch, bool requireAmmo=false);
		bool		DepleteAmmo();

		const Frame	*GetAtkState(bool hold) const;
		const Frame	*GetDownState() const;
		const Frame	*GetReadyState() const;
		const Frame	*GetUpState() const;

		bool		HandlePickup(AInventory *item, bool &good);
		void		Serialize(FArchive &arc);

		flagstype_t		weaponFlags;
		const ClassDef	*ammotype1;
		int				ammogive1;
		unsigned int	ammouse1;
		fixed			yadjust;

		// Inventory instance variables
		FireMode		mode;
		TObjPtr<AAmmo>	ammo1;

		// Bob
		EBobStyle	BobStyle;
		fixed		BobRangeX, BobRangeY;
		fixed		BobSpeed;

	protected:
		bool	UseForAmmo(AWeapon *owned);
};

#endif
