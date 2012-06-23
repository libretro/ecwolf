// WL_INTER.C

#include "wl_def.h"
#include "wl_menu.h"
#include "id_ca.h"
#include "id_sd.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#include "language.h"
#include "wl_agent.h"
#include "wl_game.h"
#include "wl_inter.h"
#include "wl_text.h"
#include "g_mapinfo.h"

LRstruct LevelRatios[LRpack];
int32_t lastBreathTime = 0;

static void Write (int x, int y, const char *string, bool rightAlign=false);

//==========================================================================

/*
==================
=
= CLearSplitVWB
=
==================
*/

void
ClearSplitVWB (void)
{
	WindowX = 0;
	WindowY = 0;
	WindowW = 320;
	WindowH = 160;
}


//==========================================================================

#ifdef SPEAR
#ifndef SPEARDEMO
////////////////////////////////////////////////////////
//
// End of Spear of Destiny
//
////////////////////////////////////////////////////////

void
EndScreen (const char* palette, const char* screen)
{
	SDL_Color pal[256];
	CA_CacheScreen (screen);
	VW_UpdateScreen ();
	VL_ConvertPalette(palette, pal, 256);
	VL_FadeIn (0, 255, pal, 30);
	IN_ClearKeysDown ();
	IN_Ack ();
	VW_FadeOut ();
}


void
EndSpear (void)
{
	SDL_Color pal[256];

	EndScreen ("END1PAL", "ENDSCR11");

	CA_CacheScreen ("ENDSCR3");
	VW_UpdateScreen ();
	VL_ConvertPalette("END3PAL", pal, 256);
	VL_FadeIn (0, 255, pal, 30);
	//fontcolor = 0xd0;
	WindowX = 0;
	WindowW = 320;
	PrintX = 0;
	PrintY = 180;
	US_CPrint (language["STR_ENDGAME1"]);
	US_CPrint (language["STR_ENDGAME2"]);
	VW_UpdateScreen ();
	IN_UserInput(700);

	PrintX = 0;
	PrintY = 180;
	VWB_Bar (0, 180, 320, 20, 0);
	US_CPrint (language["STR_ENDGAME3"]);
	US_CPrint (language["STR_ENDGAME4"]);
	VW_UpdateScreen ();
	IN_UserInput(700);

	VW_FadeOut ();

	EndScreen ("END4PAL", "ENDSCR4");
	EndScreen ("END5PAL", "ENDSCR5");
	EndScreen ("END6PAL", "ENDSCR6");
	EndScreen ("END7PAL", "ENDSCR7");
	EndScreen ("END8PAL", "ENDSCR8");
	EndScreen ("END9PAL", "ENDSCR9");

	EndScreen ("END2PAL", "ENDSCR12");
}
#endif
#endif

//==========================================================================

/*
==================
=
= Victory
=
==================
*/

void Victory (void)
{
#ifndef SPEARDEMO
	int32_t sec;
	int i, min, kr, sr, tr, x;
	char tempstr[8];

#define RATIOX  22
#define RATIOY  14
#define TIMEX   14
#define TIMEY   8


#ifdef SPEAR
	StartCPMusic (XTHEEND_MUS);

	VWB_Bar (0, 0, 320, 200, VIEWCOLOR);
	VWB_DrawPic (124, 44, "BJCOLPS1");
	VW_UpdateScreen ();
	VW_FadeIn ();
	VW_WaitVBL (2 * 70);
	VWB_DrawPic (124, 44, "BJCOLPS2");
	VW_UpdateScreen ();
	VW_WaitVBL (105);
	VWB_DrawPic (124, 44, "BJCOLPS3");
	VW_UpdateScreen ();
	VW_WaitVBL (105);
	VWB_DrawPic (124, 44, "BJCOLPS4");
	VW_UpdateScreen ();
	VW_WaitVBL (3 * 70);

	VL_FadeOut (0, 255, 0, 68, 68, 5);
#endif

	StartCPMusic ("URAHERO");
	VWB_Clear(VIEWCOLOR, 0, 0, screenWidth, screenHeight);
	DrawPlayScreen(true);

	if (bordercol != VIEWCOLOR)
		DrawStatusBorder (VIEWCOLOR);

#ifdef JAPAN
#ifndef JAPDEMO
	CA_CacheGrChunk (C_ENDRATIOSPIC);
	VWB_DrawPic (0, 0, C_ENDRATIOSPIC);
	UNCACHEGRCHUNK (C_ENDRATIOSPIC);
#endif
#else
	Write (18, 2, language["STR_YOUWIN"]);

	Write (TIMEX, TIMEY - 2, language["STR_TOTALTIME"]);

	Write (12, RATIOY - 2, language["STR_AVERAGES"]);

	Write (RATIOX, RATIOY, language["STR_RATKILL"], true);
	Write (RATIOX, RATIOY + 2, language["STR_RATSECRET"], true);
	Write (RATIOX, RATIOY + 4, language["STR_RATTREASURE"], true);
	Write (RATIOX+8, RATIOY, "%");
	Write (RATIOX+8, RATIOY + 2, "%");
	Write (RATIOX+8, RATIOY + 4, "%");

#endif

	VWB_DrawGraphic (TexMan("L_BJWINS"), 8, 4);


	for (kr = sr = tr = sec = i = 0; i < LRpack; i++)
	{
		sec += LevelRatios[i].time;
		kr += LevelRatios[i].kill;
		sr += LevelRatios[i].secret;
		tr += LevelRatios[i].treasure;
	}

#ifndef SPEAR
	kr /= LRpack;
	sr /= LRpack;
	tr /= LRpack;
#else
	kr /= 14;
	sr /= 14;
	tr /= 14;
#endif

	min = sec / 60;
	sec %= 60;

	if (min > 99)
		min = sec = 99;

	FString timeString;
	timeString.Format("%02d:%02d", min, sec);
	Write (TIMEX, TIMEY, timeString);

	itoa (kr, tempstr, 10);
	x = RATIOX + 8 - (int) strlen(tempstr) * 2;
	Write (x, RATIOY, tempstr);

	itoa (sr, tempstr, 10);
	x = RATIOX + 8 - (int) strlen(tempstr) * 2;
	Write (x, RATIOY + 2, tempstr);

	itoa (tr, tempstr, 10);
	x = RATIOX + 8 - (int) strlen(tempstr) * 2;
	Write (x, RATIOY + 4, tempstr);

	VW_UpdateScreen ();
	VW_FadeIn ();

	IN_Ack ();

	VW_FadeOut ();
	if(screenHeight % 200 != 0)
		VL_ClearScreen(0);

#ifndef SPEAR
	EndText ();
#else
	EndSpear ();
#endif

#endif // SPEARDEMO
}

//==========================================================================

static void Write (int x, int y, const char *string, bool rightAlign)
{
	static FFont *IntermissionFont = NULL;
	static FRemapTable *remap = NULL;
	if(!IntermissionFont)
	{
		IntermissionFont = V_GetFont("IntermissionFont");
		remap = IntermissionFont->GetColorTranslation(CR_UNTRANSLATED);
	}

	int nx = x*8;
	int ny = y*8;

	if(rightAlign)
	{
		word width, height;
		VW_MeasurePropString(IntermissionFont, string, width, height);
		nx -= width;
	}

	int width;
	while(*string != '\0')
	{
		if(*string != '\n')
		{
			FTexture *glyph = IntermissionFont->GetChar(*string, &width);
			if(glyph)
				VWB_DrawGraphic(glyph, nx, ny, MENU_NONE, remap);
			nx += width;
		}
		else
		{
			nx = x*8;
			ny += IntermissionFont->GetHeight();
		}
		++string;
	}
}


//
// Breathe Mr. BJ!!!
//
void BJ_Breathe (bool drawOnly=false)
{
	static int which = 0, max = 10;
	static FTexture* const pics[2] = { TexMan("L_GUY1"), TexMan("L_GUY2") };

	if(drawOnly)
	{
		VWB_DrawGraphic(pics[which], 0, 16);
		return;
	}

	SDL_Delay(5);

	if ((int32_t) GetTimeCount () - lastBreathTime > max)
	{
		which ^= 1;
		VWB_DrawGraphic(pics[which], 0, 16);
		VW_UpdateScreen ();
		lastBreathTime = GetTimeCount();
		max = 35;
	}
}



/*
==================
=
= LevelCompleted
=
= Entered with the screen faded out
= Still in split screen mode with the status bar
=
= Exit with the screen faded out
=
==================
*/

void
LevelCompleted (void)
{
#define VBLWAIT 30
#define PAR_AMOUNT      500
#define PERCENT100AMT   10000

	int x, i, min, sec, ratio, kr, sr, tr;
	char tempstr[10];
	int32_t bonus, timeleft = 0;

	ClearSplitVWB ();           // set up for double buffering in split screen
	VWB_Clear(VIEWCOLOR, 0, 0, screenWidth, screenHeight);
	DrawPlayScreen(true);

	if (bordercol != VIEWCOLOR)
		DrawStatusBorder (VIEWCOLOR);

	StartCPMusic (gameinfo.IntermissionMusic);

//
// do the intermission
//
	IN_ClearKeysDown ();
	IN_StartAck ();

#ifdef JAPAN
	CA_CacheGrChunk (C_INTERMISSIONPIC);
	VWB_DrawPic (0, 0, C_INTERMISSIONPIC);
	UNCACHEGRCHUNK (C_INTERMISSIONPIC);
#endif
	BJ_Breathe(true);

	int mapon = 0;
#ifndef SPEAR
	if (mapon < 8)
#else
	if (mapon != 4 && mapon != 9 && mapon != 15 && mapon < 17)
#endif
	{
		Write (14, 2, language["STR_FLOORCOMPLETED"]);

		Write (24, 7, language["STR_BONUS"], true);
		Write (24, 10, language["STR_TIME"], true);
		Write (24, 12, language["STR_PAR"], true);

		Write (37, 14, "%");
		Write (37, 16, "%");
		Write (37, 18, "%");
		Write (29, 14, language["STR_RAT2KILL"], true);
		Write (29, 16, language["STR_RAT2SECRET"], true);
		Write (29, 18, language["STR_RAT2TREASURE"], true);

		Write (26, 2, itoa (gamestate.mapon + 1, tempstr, 10));

		FString timeString;
		timeString.Format("%02d:%02d", levelInfo->Par/60, levelInfo->Par%60);
		Write (26, 12, timeString);

		//
		// PRINT TIME
		//
		sec = gamestate.TimeCount / 70;

		if (sec > 99 * 60)      // 99 minutes max
			sec = 99 * 60;

		if ((unsigned)gamestate.TimeCount < levelInfo->Par * 70)
			timeleft = (int32_t) (levelInfo->Par - sec);

		min = sec / 60;
		sec %= 60;
		timeString.Format("%02d:%02d", min, sec);
		Write(26, 10, timeString);

		VW_UpdateScreen ();
		VW_FadeIn ();


		//
		// FIGURE RATIOS OUT BEFOREHAND
		//
		kr = sr = tr = 100;
		if (gamestate.killtotal)
			kr = (gamestate.killcount * 100) / gamestate.killtotal;
		if (gamestate.secrettotal)
			sr = (gamestate.secretcount * 100) / gamestate.secrettotal;
		if (gamestate.treasuretotal)
			tr = (gamestate.treasurecount * 100) / gamestate.treasuretotal;


		//
		// PRINT TIME BONUS
		//
		bonus = timeleft * PAR_AMOUNT;
		if (bonus)
		{
			for (i = 0; i <= timeleft; i++)
			{
				ltoa ((int32_t) i * PAR_AMOUNT, tempstr, 10);
				x = 36 - (int) strlen(tempstr) * 2;
				Write (x, 7, tempstr);
				if (!(i % (PAR_AMOUNT / 10)))
					SD_PlaySound ("misc/end_bonus1");
				if(!usedoublebuffering || !(i % (PAR_AMOUNT / 50))) VW_UpdateScreen ();
				while(SD_SoundPlaying ())
					BJ_Breathe ();
				if (IN_CheckAck ())
					goto done;
			}

			VW_UpdateScreen ();

			SD_PlaySound ("misc/end_bonus2");
			while (SD_SoundPlaying ())
				BJ_Breathe ();
		}


#ifdef SPANISH
#define RATIOXX                33
#else
#define RATIOXX                37
#endif
		//
		// KILL RATIO
		//
		ratio = kr;
		for (i = 0; i <= ratio; i++)
		{
			itoa (i, tempstr, 10);
			x = RATIOXX - (int) strlen(tempstr) * 2;
			Write (x, 14, tempstr);
			if (!(i % 10))
				SD_PlaySound ("misc/end_bonus1");
			if(!usedoublebuffering || !(i & 1)) VW_UpdateScreen ();
			while (SD_SoundPlaying ())
				BJ_Breathe ();

			if (IN_CheckAck ())
				goto done;
		}
		if (ratio >= 100)
		{
			VW_WaitVBL (VBLWAIT);
			SD_StopSound ();
			bonus += PERCENT100AMT;
			ltoa (bonus, tempstr, 10);
			x = (RATIOXX - 1) - (int) strlen(tempstr) * 2;
			Write (x, 7, tempstr);
			VW_UpdateScreen ();
			SD_PlaySound ("misc/100percent");
		}
		else if (!ratio)
		{
			VW_WaitVBL (VBLWAIT);
			SD_StopSound ();
			SD_PlaySound ("misc/no_bonus");
		}
		else
			SD_PlaySound ("misc/end_bonus2");

		VW_UpdateScreen ();
		while (SD_SoundPlaying ())
			BJ_Breathe ();

		//
		// SECRET RATIO
		//
		ratio = sr;
		for (i = 0; i <= ratio; i++)
		{
			itoa (i, tempstr, 10);
			x = RATIOXX - (int) strlen(tempstr) * 2;
			Write (x, 16, tempstr);
			if (!(i % 10))
				SD_PlaySound ("misc/end_bonus1");
			if(!usedoublebuffering || !(i & 1)) VW_UpdateScreen ();
			while (SD_SoundPlaying ())
				BJ_Breathe ();

			if (IN_CheckAck ())
				goto done;
		}
		if (ratio >= 100)
		{
			VW_WaitVBL (VBLWAIT);
			SD_StopSound ();
			bonus += PERCENT100AMT;
			ltoa (bonus, tempstr, 10);
			x = (RATIOXX - 1) - (int) strlen(tempstr) * 2;
			Write (x, 7, tempstr);
			VW_UpdateScreen ();
			SD_PlaySound ("misc/100percent");
		}
		else if (!ratio)
		{
			VW_WaitVBL (VBLWAIT);
			SD_StopSound ();
			SD_PlaySound ("misc/no_bonus");
		}
		else
			SD_PlaySound ("misc/end_bonus2");
		VW_UpdateScreen ();
		while (SD_SoundPlaying ())
			BJ_Breathe ();

		//
		// TREASURE RATIO
		//
		ratio = tr;
		for (i = 0; i <= ratio; i++)
		{
			itoa (i, tempstr, 10);
			x = RATIOXX - (int) strlen(tempstr) * 2;
			Write (x, 18, tempstr);
			if (!(i % 10))
				SD_PlaySound ("misc/end_bonus1");
			if(!usedoublebuffering || !(i & 1)) VW_UpdateScreen ();
			while (SD_SoundPlaying ())
				BJ_Breathe ();
			if (IN_CheckAck ())
				goto done;
		}
		if (ratio >= 100)
		{
			VW_WaitVBL (VBLWAIT);
			SD_StopSound ();
			bonus += PERCENT100AMT;
			ltoa (bonus, tempstr, 10);
			x = (RATIOXX - 1) - (int) strlen(tempstr) * 2;
			Write (x, 7, tempstr);
			VW_UpdateScreen ();
			SD_PlaySound ("misc/100percent");
		}
		else if (!ratio)
		{
			VW_WaitVBL (VBLWAIT);
			SD_StopSound ();
			SD_PlaySound ("misc/no_bonus");
		}
		else
			SD_PlaySound ("misc/end_bonus2");
		VW_UpdateScreen ();
		while (SD_SoundPlaying ())
			BJ_Breathe ();


		//
		// JUMP STRAIGHT HERE IF KEY PRESSED
		//
done:   itoa (kr, tempstr, 10);
		x = RATIOXX - (int) strlen(tempstr) * 2;
		Write (x, 14, tempstr);

		itoa (sr, tempstr, 10);
		x = RATIOXX - (int) strlen(tempstr) * 2;
		Write (x, 16, tempstr);

		itoa (tr, tempstr, 10);
		x = RATIOXX - (int) strlen(tempstr) * 2;
		Write (x, 18, tempstr);

		bonus = (int32_t) timeleft *PAR_AMOUNT +
			(PERCENT100AMT * (kr >= 100)) +
			(PERCENT100AMT * (sr >= 100)) + (PERCENT100AMT * (tr >= 100));

		GivePoints (bonus);
		ltoa (bonus, tempstr, 10);
		x = 36 - (int) strlen(tempstr) * 2;
		Write (x, 7, tempstr);

		//
		// SAVE RATIO INFORMATION FOR ENDGAME
		//
		LevelRatios[mapon].kill = kr;
		LevelRatios[mapon].secret = sr;
		LevelRatios[mapon].treasure = tr;
		LevelRatios[mapon].time = min * 60 + sec;
	}
	else
	{
#ifdef SPEAR
#ifndef SPEARDEMO
		switch (mapon)
		{
			case 4:
				Write (14, 4, " trans\n" " grosse\n" "defeated!");
				break;
			case 9:
				Write (14, 4, "barnacle\n" "wilhelm\n" "defeated!");
				break;
			case 15:
				Write (14, 4, "ubermutant\n" "defeated!");
				break;
			case 17:
				Write (14, 4, " death\n" " knight\n" "defeated!");
				break;
			case 18:
				Write (13, 4, "secret tunnel\n" "    area\n" "  completed!");
				break;
			case 19:
				Write (13, 4, "secret castle\n" "    area\n" "  completed!");
				break;
		}
#endif
#else
		Write (14, 4, "secret floor\n completed!");
#endif

		Write (10, 16, "15000 bonus!");

		VW_UpdateScreen ();
		VW_FadeIn ();

		GivePoints (15000);
	}


	DrawStatusBar();
	VW_UpdateScreen ();

	lastBreathTime = GetTimeCount();
	IN_StartAck ();
	while (!IN_CheckAck ())
		BJ_Breathe ();

//
// done
//
#ifdef SPEARDEMO
	if (gamestate.mapon == 1)
	{
		SD_PlaySound ("misc/1up");

		Message ("This concludes your demo\n"
				"of Spear of Destiny! Now,\n" "go to your local software\n" "store and buy it!");

		IN_ClearKeysDown ();
		IN_Ack ();
	}
#endif

#ifdef JAPDEMO
	if (gamestate.mapon == 3)
	{
		SD_PlaySound ("misc/1up");

		Message ("This concludes your demo\n"
				"of Wolfenstein 3-D! Now,\n" "go to your local software\n" "store and buy it!");

		IN_ClearKeysDown ();
		IN_Ack ();
	}
#endif

	VW_FadeOut ();
	DrawPlayBorder();
}



//==========================================================================


/*
=================
=
= PreloadGraphics
=
= Fill the cache up
=
=================
*/

bool PreloadUpdate (unsigned current, unsigned total)
{

	double x = 53;
	double y = 101;
	double w = 214.0*current/total;
	double h = 3;
	double ow = w - 1;
	double oh = h - 1;
	double ox = x, oy = y;
	VirtualToRealCoords(x, y, w, h, 320, 200, false, true);
	VirtualToRealCoords(ox, oy, ow, oh, 320, 200, false, true);

	if (current)
	{
		VWB_Clear(0x37, x, y, x+w, y+h);
		VWB_Clear(0x32, ox, oy, ox+ow, oy+oh);

	}
	VW_UpdateScreen ();
	return (false);
}

void PreloadGraphics (void)
{
	ClearSplitVWB ();           // set up for double buffering in split screen

	VWB_Clear(bordercol, 0, 0, screenWidth, screenHeight);
	ShowActStatus();
	VWB_DrawGraphic(TexMan("GETPSYCH"), 48, 56);

	WindowX = (screenWidth - scaleFactor*224)/2;
	WindowY = (screenHeight - scaleFactor*(STATUSLINES+48))/2;
	WindowW = scaleFactor * 28 * 8;
	WindowH = scaleFactor * 48;

	VW_UpdateScreen ();
	VW_FadeIn ();

	PreloadUpdate (5, 10);

	TexMan.PrecacheLevel();

//      PM_Preload (PreloadUpdate);
	PreloadUpdate (10, 10);
	IN_UserInput (70);
	VW_FadeOut ();

	DrawPlayScreen ();
	VW_UpdateScreen ();
}


//==========================================================================

/*
==================
=
= DrawHighScores
=
==================
*/

void
DrawHighScores (void)
{
	char buffer[16];
#ifndef SPEAR
	char *str;
#ifndef UPLOAD
	char buffer1[5];
#endif
#endif
	word i, w, h;
	HighScore *s;

	ClearMScreen ();

	FTexture *highscores = TexMan("HGHSCORE");
	if(highscores->GetScaledWidth() < 320)
	{
		DrawStripes (10);
		VWB_DrawGraphic(highscores, 160-highscores->GetScaledWidth()/2, 0, MENU_TOP);
	}
	else
		VWB_DrawGraphic(highscores, 0, 0);

#ifndef SPEAR
	static FTextureID texName = TexMan.CheckForTexture("M_NAME", FTexture::TEX_Any);
	static FTextureID texLevel = TexMan.CheckForTexture("M_LEVEL", FTexture::TEX_Any);
	static FTextureID texScore = TexMan.CheckForTexture("M_SCORE", FTexture::TEX_Any);
	if(texName.isValid())
		VWB_DrawGraphic(TexMan(texName), 32, 68);
	if(texLevel.isValid())
		VWB_DrawGraphic(TexMan(texLevel), 160, 68);
	if(texScore.isValid())
		VWB_DrawGraphic(TexMan(texScore), 224, 68);
#endif


/*#ifndef SPEAR
	SETFONTCOLOR (15, 0x29);
#else
	SETFONTCOLOR (HIGHLIGHT, 0x29);
#endif*/

	for (i = 0, s = Scores; i < MaxScores; i++, s++)
	{
		PrintY = 76 + (16 * i);

		//
		// name
		//
#ifndef SPEAR
		PrintX = 4 * 8;
#else
		PrintX = 16;
#endif
		US_Print (SmallFont, s->name, gameinfo.FontColors[GameInfo::HIGHSCORES]);

		//
		// level
		//
		itoa (s->completed, buffer, 10);
#ifndef SPEAR
		for (str = buffer; *str; str++)
			*str = *str + (129 - '0');  // Used fixed-width numbers (129...)
		VW_MeasurePropString (BigFont, buffer, w, h);
		PrintX = (22 * 8) - w;
#else
		VW_MeasurePropString (BigFont, buffer, w, h);
		PrintX = 194 - w;
#endif

#ifndef UPLOAD
#ifndef SPEAR
		PrintX -= 6;
		itoa (s->episode + 1, buffer1, 10);
		US_Print (SmallFont, "E", gameinfo.FontColors[GameInfo::HIGHSCORES]);
		US_Print (SmallFont, buffer1, gameinfo.FontColors[GameInfo::HIGHSCORES]);
		US_Print (SmallFont, "/L", gameinfo.FontColors[GameInfo::HIGHSCORES]);
#endif
#endif

#ifdef SPEAR
		if (s->completed == 21)
			VWB_DrawPic (PrintX + 8, PrintY - 1, "M_WONSPR");
		else
#endif
			US_Print (SmallFont, buffer, gameinfo.FontColors[GameInfo::HIGHSCORES]);

		//
		// score
		//
		itoa (s->score, buffer, 10);
#ifndef SPEAR
		for (str = buffer; *str; str++)
			*str = *str + (129 - '0');  // Used fixed-width numbers (129...)
		VW_MeasurePropString (BigFont, buffer, w, h);
		PrintX = (34 * 8) - 8 - w;
#else
		VW_MeasurePropString (BigFont, buffer, w, h, gameinfo.FontColors[GameInfo::HIGHSCORES]);
		PrintX = 292 - w;
#endif
		US_Print (SmallFont, buffer, gameinfo.FontColors[GameInfo::HIGHSCORES]);

/*#ifdef APOGEE_1_0
//#ifndef UPLOAD
#ifndef SPEAR
		//
		// verification #
		//
		if (!i)
		{
			char temp = (((s->score >> 28) & 0xf) ^ ((s->score >> 24) & 0xf)) + 'A';
			char temp1 = (((s->score >> 20) & 0xf) ^ ((s->score >> 16) & 0xf)) + 'A';
			char temp2 = (((s->score >> 12) & 0xf) ^ ((s->score >> 8) & 0xf)) + 'A';
			char temp3 = (((s->score >> 4) & 0xf) ^ ((s->score >> 0) & 0xf)) + 'A';

			SETFONTCOLOR (0x49, 0x29);
			PrintX = 35 * 8;
			buffer[0] = temp;
			buffer[1] = temp1;
			buffer[2] = temp2;
			buffer[3] = temp3;
			buffer[4] = 0;
			US_Print (buffer);
			SETFONTCOLOR (15, 0x29);
		}
#endif
//#endif
#endif*/
	}

	VW_UpdateScreen ();
}

//===========================================================================


/*
=======================
=
= CheckHighScore
=
=======================
*/

void
CheckHighScore (int32_t score, word other)
{
	word i, j;
	int n;
	HighScore myscore;

	strcpy (myscore.name, "");
	myscore.score = score;
	myscore.episode = gamestate.episode;
	myscore.completed = other;

	for (i = 0, n = -1; i < MaxScores; i++)
	{
		if ((myscore.score > Scores[i].score)
			|| ((myscore.score == Scores[i].score) && (myscore.completed > Scores[i].completed)))
		{
			for (j = MaxScores; --j > i;)
				Scores[j] = Scores[j - 1];
			Scores[i] = myscore;
			n = i;
			break;
		}
	}

	StartCPMusic (gameinfo.ScoresMusic);
	DrawHighScores ();

	VW_FadeIn ();

	if (n != -1)
	{
		//
		// got a high score
		//
		PrintY = 76 + (16 * n);
#ifndef SPEAR
		PrintX = 4 * 8;
		US_LineInput (PrintX, PrintY, Scores[n].name, 0, true, MaxHighName, 100);
#else
		PrintX = 16;
		VWB_Bar (PrintX - 2, PrintY - 2, 145, 15, 0x9c);
		VW_UpdateScreen ();
		US_LineInput (PrintX, PrintY, Scores[n].name, 0, true, MaxHighName, 130);
#endif
	}
	else
	{
		IN_ClearKeysDown ();
		IN_UserInput (500);
	}

}
