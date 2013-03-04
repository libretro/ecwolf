/*
** g_intermission.h
**
**---------------------------------------------------------------------------
** Copyright 2012 Braden Obrzut
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

#include "g_mapinfo.h"
#include "tarray.h"
#include "zstring.h"
#include "textures/textures.h"

class IntermissionAction
{
public:
	struct DrawData
	{
	public:
		FTextureID		Image;
		unsigned int	X;
		unsigned int	Y;
	};

	IntermissionAction() : Time(0), BackgroundTile(false)
	{
		// Invalid background means use previous.
		Background.SetInvalid();
	}

	FTextureID			Background;
	TArray<DrawData>	Draw;
	FString				Music;
	FString				Palette;
	unsigned int		Time;
	bool				BackgroundTile;
};

class FaderIntermissionAction : public IntermissionAction
{
public:
	enum FadeType
	{
		FADEIN,
		FADEOUT
	};

	FadeType	Fade;
};

class TextScreenIntermissionAction : public IntermissionAction
{
public:
	enum Alignment
	{
		LEFT,
		CENTER,
		RIGHT
	};

	TextScreenIntermissionAction() : IntermissionAction(),
		Alignment(LEFT), TextColor(CR_UNTRANSLATED), TextDelay(20), TextSpeed(4)
	{
	}

	unsigned int	PrintX;
	unsigned int	PrintY;
	Alignment		Alignment;
	TArray<FString>	Text;
	EColorRange		TextColor;
	unsigned int	TextDelay;
	unsigned int	TextSpeed;
};

class IntermissionInfo
{
public:
	static IntermissionInfo &Find(const FName &name);

	enum ActionType
	{
		IMAGE,
		FADER,
		GOTOTITLE,
		TEXTSCREEN,
		VICTORYSTATS
	};

	struct Action
	{
		ActionType			type;
		IntermissionAction	*action;
	};
	TArray<Action>	Actions;
};

// Returns true if the game should return to the menu instead of the title screen.
bool ShowIntermission(const IntermissionInfo &intermission);
