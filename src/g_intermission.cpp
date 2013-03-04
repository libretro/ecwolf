/*
** g_intermission.cpp
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

#include "wl_def.h"
#include "id_ca.h"
#include "id_in.h"
#include "id_vh.h"
#include "g_intermission.h"
#include "language.h"
#include "tarray.h"
#include "wl_inter.h"
#include "wl_menu.h"
#include "wl_play.h"

static TMap<FName, IntermissionInfo> intermissions;

IntermissionInfo &IntermissionInfo::Find(const FName &name)
{
	return intermissions[name];
}

////////////////////////////////////////////////////////////////////////////////

static void WaitIntermission(unsigned int time)
{
	if(time)
	{
		IN_UserInput(time);
	}
	else
	{
		IN_ClearKeysDown ();
		IN_Ack ();
	}
}

static void ShowImage(IntermissionAction *image, bool drawonly)
{
	if(!image->Music.IsEmpty())
		StartCPMusic(image->Music);

	if(!image->Palette.IsEmpty())
		VL_ReadPalette(image->Palette);

	static FTextureID background;
	static bool tileBackground = false;
	if(image->Background.isValid())
	{
		background = image->Background;
		tileBackground = image->BackgroundTile;
	}
	if(!tileBackground)
		CA_CacheScreen(TexMan(background));
	else
		VWB_DrawFill(TexMan(background), 0, 0, screenWidth, screenHeight);

	for(unsigned int i = 0;i < image->Draw.Size();++i)
	{
		VWB_DrawGraphic(TexMan(image->Draw[i].Image), image->Draw[i].X, image->Draw[i].Y);
	}

	if(!drawonly)
	{
		VW_UpdateScreen();
		WaitIntermission(image->Time);
	}
}

static void ShowFader(FaderIntermissionAction *fader)
{
	switch(fader->Fade)
	{
		case FaderIntermissionAction::FADEIN:
			ShowImage(fader, true);
			VL_FadeIn(0, 255, gamepal, fader->Time);
			break;
		case FaderIntermissionAction::FADEOUT:
			// We want to hold whatever may have been drawn in the previous page during the fade, so we don't need to draw.
			VL_FadeOut(0, 255, 0, 0, 0, fader->Time);
			break;
	}
}

static void ShowTextScreen(TextScreenIntermissionAction *textscreen)
{
	if(textscreen->TextSpeed)
		Printf("Warning: Text screen has a non-zero textspeed which isn't supported at this time.\n");

	ShowImage(textscreen, true);

	if(textscreen->TextDelay)
		WaitIntermission(textscreen->TextDelay);

	pa = MENU_BOTTOM;
	py = textscreen->PrintY;
	for(unsigned int i = 0;i < textscreen->Text.Size();++i)
	{
		FString line = textscreen->Text[i];
		if(line[0] == '$')
			line = language[line.Mid(1)];

		word width, height;
		VW_MeasurePropString(SmallFont, line, width, height);

		switch(textscreen->Alignment)
		{
			default:
				px = textscreen->PrintX;
				break;
			case TextScreenIntermissionAction::RIGHT:
				px = textscreen->PrintX - width;
				break;
			case TextScreenIntermissionAction::CENTER:
				px = textscreen->PrintX - width/2;
				break;
		}

		VWB_DrawPropString(SmallFont, line, textscreen->TextColor);

		py += SmallFont->GetHeight();
	}
	pa = MENU_CENTER;

	VW_UpdateScreen();
	WaitIntermission(textscreen->Time);
}

bool ShowIntermission(const IntermissionInfo &intermission)
{
	bool gototitle = false;
	for(unsigned int i = 0;!gototitle && i < intermission.Actions.Size();++i)
	{
		switch(intermission.Actions[i].type)
		{
			default:
			case IntermissionInfo::IMAGE:
				ShowImage(intermission.Actions[i].action, false);
				break;
			case IntermissionInfo::FADER:
				ShowFader((FaderIntermissionAction*)intermission.Actions[i].action);
				break;
			case IntermissionInfo::GOTOTITLE:
				gototitle = true;
				break;
			case IntermissionInfo::TEXTSCREEN:
				ShowTextScreen((TextScreenIntermissionAction*)intermission.Actions[i].action);
				break;
			case IntermissionInfo::VICTORYSTATS:
				Victory(true);
				break;
		}
	}

	if(!gototitle)
	{
		// Hold at the final page until esc is pressed
		IN_ClearKeysDown();

		ControlInfo ci;
		do
		{
			LastScan = 0;
			ReadAnyControl(&ci);
		}
		while(LastScan != sc_Escape && !ci.button1);
	}

	VL_ReadPalette(gameinfo.GamePalette);
	return !gototitle;
}
