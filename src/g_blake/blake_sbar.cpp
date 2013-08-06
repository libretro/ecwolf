/*
** blake_sbar.cpp
**
**---------------------------------------------------------------------------
** Copyright 2013 Braden Obrzut
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
#include "a_inventory.h"
#include "id_ca.h"
#include "id_vh.h"
#include "g_mapinfo.h"
#include "v_font.h"
#include "v_video.h"
#include "wl_agent.h"
#include "wl_def.h"
#include "wl_play.h"

enum
{
	STATUSLINES = 48,
	STATUSTOPLINES = 16
};

class BlakeStatusBar : public DBaseStatusBar
{
public:
	BlakeStatusBar() : CurrentScore(0) {}

	void DrawStatusBar();
	unsigned int GetHeight(bool top)
	{
		if(viewsize == 21)
			return 0;
		return top ? STATUSTOPLINES : STATUSLINES;
	}

	void NewGame()
	{
		CurrentScore = players[0].score;
	}

	void Tick();

protected:
	void DrawString(FFont *font, const char* string, double x, double y, bool shadow, EColorRange color=CR_UNTRANSLATED, bool center=false);

private:
	int CurrentScore;
};

DBaseStatusBar *CreateStatusBar_Blake() { return new BlakeStatusBar(); }

void BlakeStatusBar::DrawStatusBar()
{
	static FFont *IndexFont = V_GetFont("INDEXFON");
	static FFont *HealthFont = V_GetFont("BlakeHealthFont");
	static FFont *ScoreFont = V_GetFont("BlakeScoreFont");

	static FTextureID STBar = TexMan.GetTexture("STBAR", FTexture::TEX_Any);
	static FTextureID STBarTop = TexMan.GetTexture("STTOP", FTexture::TEX_Any);

	double stx = 0;
	double sty = 200-STATUSLINES;
	double stw = 320;
	double sth = STATUSLINES;
	screen->VirtualToRealCoords(stx, sty, stw, sth, 320, 200, true, true);

	screen->DrawTexture(TexMan(STBar), stx, sty,
		DTA_DestWidthF, stw,
		DTA_DestHeightF, sth,
		TAG_DONE);

	stx = 0;
	sty = 0;
	stw = 320;
	sth = STATUSTOPLINES;
	screen->VirtualToRealCoords(stx, sty, stw, sth, 320, 200, true, true);

	screen->DrawTexture(TexMan(STBarTop), stx, 0.0,
		DTA_DestWidthF, stw,
		DTA_DestHeightF, sth,
		TAG_DONE);

	// Draw the top information
	FString lives, area;
	// TODO: Don't depend on LevelNumber for this switch
	if(levelInfo->LevelNumber > 20)
		area = "SECRET";
	else
		area.Format("AREA: %d", levelInfo->LevelNumber);
	lives.Format("LIVES: %d", players[0].lives);
	DrawString(IndexFont, area, 18, 5, true, CR_WHITE);
	DrawString(IndexFont, levelInfo->GetName(map), 160, 5, true, CR_WHITE, true);
	DrawString(IndexFont, lives, 267, 5, true, CR_WHITE);

	// Draw bottom information
	FString health;
	health.Format("%3d", players[0].health);
	DrawString(HealthFont, health, 128, 162, false);

	FString score;
	score.Format("%7d", CurrentScore);
	DrawString(ScoreFont, score, 256, 155, false);

	if(players[0].ReadyWeapon)
	{
		FTexture *weapon = TexMan(players[0].ReadyWeapon->icon);
		if(weapon)
		{
			stx = 248;
			sty = 176;
			stw = weapon->GetScaledWidthDouble();
			sth = weapon->GetScaledHeightDouble();
			screen->VirtualToRealCoords(stx, sty, stw, sth, 320, 200, true, true);
			screen->DrawTexture(weapon, stx, sty,
				DTA_DestWidthF, stw,
				DTA_DestHeightF, sth,
				TAG_DONE);
		}

		// TODO: Fix color
		unsigned int amount = players[0].ReadyWeapon->ammo[AWeapon::PrimaryFire]->amount;
		FString ammo;
		ammo.Format("%3d%%", amount);
		DrawString(IndexFont, ammo, 252, 190, false, CR_LIGHTBLUE);
	}
}

void BlakeStatusBar::DrawString(FFont *font, const char* string, double x, double y, bool shadow, EColorRange color, bool center)
{
	Tick();

	word strWidth, strHeight;
	VW_MeasurePropString(font, string, strWidth, strHeight);

	if(center)
		x -= strWidth/2.0;

	const double startX = x;
	FRemapTable *remap = font->GetColorTranslation(color);

	while(*string != '\0')
	{
		char ch = *string++;
		if(ch == '\n')
		{
			y += font->GetHeight();
			x = startX;
			continue;
		}

		int chWidth;
		FTexture *tex = font->GetChar(ch, &chWidth);
		if(tex)
		{
			double tx, ty, tw, th;

			if(shadow)
			{
				tx = x + 1, ty = y + 1, tw = tex->GetScaledWidthDouble(), th = tex->GetScaledHeightDouble();
				screen->VirtualToRealCoords(tx, ty, tw, th, 320, 200, true, true);
				screen->DrawTexture(tex, tx, ty,
					DTA_DestWidthF, tw,
					DTA_DestHeightF, th,
					DTA_FillColor, GPalette.BlackIndex,
					TAG_DONE);
			}

			tx = x, ty = y, tw = tex->GetScaledWidthDouble(), th = tex->GetScaledHeightDouble();
			screen->VirtualToRealCoords(tx, ty, tw, th, 320, 200, true, true);
			screen->DrawTexture(tex, tx, ty,
				DTA_DestWidthF, tw,
				DTA_DestHeightF, th,
				DTA_Translation, remap,
				TAG_DONE);
		}
		x += chWidth;
	}
}

void BlakeStatusBar::Tick()
{
	int scoreDelta = players[0].score - CurrentScore;
	if(scoreDelta > 1500)
		CurrentScore += scoreDelta/4;
	else
		CurrentScore += clamp<int>(scoreDelta, 0, 8);
}
