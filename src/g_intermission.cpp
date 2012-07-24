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
#include "id_vh.h"
#include "g_intermission.h"
#include "tarray.h"

static TMap<FName, IntermissionInfo> intermissions;

IntermissionInfo &IntermissionInfo::Find(const FName &name)
{
	return intermissions[name];
}

////////////////////////////////////////////////////////////////////////////////

static void ShowImage(IntermissionAction *image, bool nowait)
{
	CA_CacheScreen(TexMan(image->Background));
	for(unsigned int i = 0;i < image->Draw.Size();++i)
	{
		VWB_DrawGraphic(TexMan(image->Draw[i].Image), image->Draw[i].X, image->Draw[i].Y);
	}
	VW_UpdateScreen();

	if(!nowait)
		VL_WaitVBL(image->Time);
}

static void ShowFader(FaderIntermissionAction *fader)
{
	ShowImage(fader, true);

	switch(fader->Fade)
	{
		case FaderIntermissionAction::FADEIN:
			VW_FadeIn();
			VL_WaitVBL(fader->Time);
			break;
		case FaderIntermissionAction::FADEOUT:
			VL_WaitVBL(fader->Time);
			VW_FadeOut();
			break;
	}
}

void ShowIntermission(const IntermissionInfo &intermission)
{
	for(unsigned int i = 0;i < intermission.Actions.Size();++i)
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
		}
	}
}
