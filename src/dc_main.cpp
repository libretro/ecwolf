/*
	dc_main.cpp
	This file is part of the Wolf4SDL\DC project.

 	This file is heavily based on Yabause, nxDoom and sdlWolf by
 	Lawrence Sebald, BlackAura and OneThirty8 respectively.

	Cyle Terry <cyle.terry@gmail.com>
*/

#ifdef _arch_dreamcast

#include <kos.h>
#include "wl_def.h"

#define DCROOT "/cd"

void DC_DrawString(int x, int y, char *string)
{
	bfont_draw_str(vram_s + ((y + 1) * 24 * 640) + (x * 12), 640, 0, string);
}

void DC_CLS()
{
	int x, y, ofs;
	for(y = 0; y < 480; y++)
	{
		ofs = (640 * y);
		for(x = 0; x < 640; x++)
			vram_s[ofs + x] = 0;
	}
}

void DC_CLA()
{
	int x, y, ofs;
	for(y = 168; y < 240; y++)
	{
		ofs = (640 * y);
		for(x = 24; x < 48; x++)
			vram_s[ofs + x] = 0;
	}
}

#ifdef SPEAR
#ifndef SPEARDEMO
int selectedMission;
int hasMission1 = 0;
int hasMission2 = 0;
int hasMission3 = 0;

int DC_SpearMenu()	{
	int i = 0;
	int yint;
	int thisMission = 1;
	int lastMission = -1;
	int firstPad = 0;
	cont_cond_t cond;

	i = hasMission1 + hasMission2 + hasMission3;

	yint = 6;
	DC_CLS();
	DC_DrawString(4, 1, "Wolf4SDL\\DC 1.4");
	if(hasMission1 == 1)	{
		DC_DrawString(4, yint, "Spear of Destiny (Original Mission)");
		yint++;
	}
	if(hasMission2 == 1)	{
		DC_DrawString(4, yint, "Mission 2 - Return to Danger");
		yint++;
	}
	if(hasMission3 == 1)	{
		DC_DrawString(4, yint, "Mission 3 - Ultimate Challenge");
	}

	while(1)	{
		SDL_Delay(5);
		firstPad = maple_first_controller();
		cont_get_cond(firstPad, &cond);

		if(!(cond.buttons & CONT_DPAD_DOWN))	{
			thisMission++;
			lastMission = -1;

			/* Jump to top mission */
			if(thisMission > i)	{
				thisMission = 1;
			}

			/* Don't go rapidly through */
			while(!(cond.buttons & CONT_DPAD_DOWN))	{
        		SDL_Delay(5);
				/* Get condition again */
				cont_get_cond(firstPad, &cond);
			}
		}
		if(!(cond.buttons & CONT_DPAD_UP))	{
			thisMission--;
			lastMission = -1;

			/* Jump to bottom mission */
			if(thisMission < 1)	{
				thisMission = i;
			}

			/* Don't go rapidly through */
			while(!(cond.buttons & CONT_DPAD_UP)) {
        		SDL_Delay(5);
				/* Get condition again */
				cont_get_cond(firstPad, &cond);
			}
		}

		if(lastMission != thisMission)	{
			DC_CLA();
			DC_DrawString(2, 6 + (thisMission - 1) , ">");
		}

		lastMission = thisMission;

		if (!(cond.buttons & CONT_A))	{
			if(thisMission == 1)	{
				if(hasMission1 == 1)
					return 1;
				else
					return 2;
			}
			else if(thisMission == 2)	{
				if(hasMission1 == 1)
					if(hasMission2 == 1)
						return 2;
					else
						return 3;
				else
					return 3;
			}
			else
				return 3;
		}
	}
}

int GetAvailableMissions(char *path)	{
	char filename[100];
	int Mission = 0;
	int Missions = 0;
	FILE *fp;

	sprintf(filename, "%smaphead.sod", path);
	fp = fopen(filename, "r");
	if(fp)	{
		fclose(fp);
		Mission = 1;
		hasMission1 = 1;
		Missions++;
	}

	sprintf(filename, "%smaphead.sd2", path);
	fp = fopen(filename, "r");
	if(fp)	{
		fclose(fp);
		Mission = 2;
		hasMission2 = 1;
		Missions++;
	}

	sprintf(filename, "%smaphead.sd3", path);
	fp = fopen(filename, "r");
	if(fp)	{
		fclose(fp);
		Mission = 3;
		hasMission3 = 1;
		Missions++;
	}

	if(Missions == 1)	{
		selectedMission = Mission;
	}

	return Missions;
}
#endif
#endif

void DC_Main()	{
	int firstPad = 0;
	cont_cond_t cond;
	FILE *fp;

#ifdef SPEAR
#ifndef SPEARDEMO
	int Missions = GetAvailableMissions(DCROOT "/wolf3d/");
	if(Missions != 0) {
		if(Missions == 1)	{
			fs_chdir(DCROOT "/wolf3d");
			goto copy2ram;
		}
		else	{
			selectedMission = DC_SpearMenu();
			fs_chdir(DCROOT "/wolf3d");
			goto copy2ram;
		}
	}
#else
	fp = fopen(DCROOT "/wolf3d/vgahead.sdm", "r");
#endif
#else
#ifdef UPLOAD
	fp = fopen(DCROOT "/wolf3d/vgahead.wl1", "r");
#else
	fp = fopen(DCROOT "/wolf3d/vgahead.wl6", "r");
#endif
#endif
	if(fp)	{
		fclose(fp);
		fs_chdir(DCROOT "/wolf3d");
		goto copy2ram;
	}

	DC_CLS();
	DC_DrawString(4, 1, "Wolf4SDL\\DC 1.5");
	DC_DrawString(4, 6, "Please insert your Wolfenstein 3D CD");
	DC_DrawString(4, 7, "and press start.");

	firstPad = maple_first_controller();
	cont_get_cond(firstPad, &cond);
	while((cond.buttons & CONT_START))	{
		SDL_Delay(5);
		cont_get_cond(firstPad, &cond);
	}

	for(;;) {
#ifdef SPEAR
#ifndef SPEARDEMO
		/* Activision Wolfenstein 3D CD-ROM */
		Missions = GetAvailableMissions(DCROOT "/Install/data/SOD/");
		if(Missions != 0) {
			if(Missions == 1)	{
				fs_chdir(DCROOT "/Install/data/SOD");
				goto copy2ram;
			}
			else	{
				selectedMission = DC_SpearMenu();
				fs_chdir(DCROOT "/Install/data/SOD");
				goto copy2ram;
			}
		}

		/* Spear of Destiny Super CD CD-ROM */
		Missions = GetAvailableMissions(DCROOT "/");
		if(Missions != 0) {
			if(Missions == 1)	{
				fs_chdir(DCROOT);
				goto copy2ram;
			}
			else	{
				selectedMission = DC_SpearMenu();
				fs_chdir(DCROOT);
				goto copy2ram;
			}
		}
#endif
#else
#ifdef UPLOAD
		/* Random Shareware CD-ROM */
		fp = fopen(DCROOT "/vgahead.wl1", "r");
		if(fp)	{
			fclose(fp);
			fs_chdir(DCROOT);
			goto copy2ram;
		}
#else
		/* Activision Wolfenstein 3D CD-ROM */
		fp = fopen(DCROOT "/Install/data/WOLF3D/vgahead.wl6", "r");
		if(fp)	{
			fclose(fp);
			fs_chdir(DCROOT "/Install/data/WOLF3D");
			goto copy2ram;
		}

		/* Random Full Version CD-ROM */
		fp = fopen(DCROOT "/vgahead.wl6", "r");
		if(fp)	{
			fclose(fp);
			fs_chdir(DCROOT);
			goto copy2ram;
		}

		/* RTCW Game of the Year CD-ROM */
		fp = fopen(DCROOT "/Bonus/Wolf3D/PROGRAM FILES/Wolfenstein 3D/wolf3d/vgahead.wl6", "r");
		if(fp)	{
			fclose(fp);
			fs_chdir(DCROOT "/Bonus/Wolf3D/PROGRAM FILES/Wolfenstein 3D/wolf3d");
			goto copy2ram;
		}
#endif
#endif
	}

copy2ram:
	DC_CLS();

#ifdef SPEAR
#ifndef SPEARDEMO
	fs_copy("audiohed.sod", "/ram/audiohed.sod");
	fs_copy("audiot.sod", "/ram/audiot.sod");
	fs_copy("vgadict.sod", "/ram/vgadict.sod");
	fs_copy("vgagraph.sod", "/ram/vgagraph.sod");
	fs_copy("vgahead.sod", "/ram/vgahead.sod");
	switch(selectedMission)	{
		case 1:
			param_mission = 1;
			fs_copy("gamemaps.sod", "/ram/gamemaps.sod");
			fs_copy("maphead.sod", "/ram/maphead.sod");
			fs_copy("vswap.sod", "/ram/vswap.sod");
			break;
		case 2:
			param_mission = 2;
			fs_copy("gamemaps.sd2", "/ram/gamemaps.sd2");
			fs_copy("maphead.sd2", "/ram/maphead.sd2");
			fs_copy("vswap.sd2", "/ram/vswap.sd2");
			break;
		case 3:
			param_mission = 3;
			fs_copy("gamemaps.sd3", "/ram/gamemaps.sd3");
			fs_copy("maphead.sd3", "/ram/maphead.sd3");
			fs_copy("vswap.sd3", "/ram/vswap.sd3");
			break;
		default:
			dbglog(DBG_DEBUG, "Unknown: Mission %i\n", selectedMission);
			break;
	}
#else
	fs_copy("audiohed.sdm", "/ram/audiohed.sdm");
	fs_copy("audiot.sdm", "/ram/audiot.sdm");
	fs_copy("vgadict.sdm", "/ram/vgadict.sdm");
	fs_copy("vgagraph.sdm", "/ram/vgagraph.sdm");
	fs_copy("vgahead.sdm", "/ram/vgahead.sdm");
	fs_copy("gamemaps.sdm", "/ram/gamemaps.sdm");
	fs_copy("maphead.sdm", "/ram/maphead.sdm");
	fs_copy("vswap.sdm", "/ram/vswap.sdm");
#endif
#else
#ifndef UPLOAD
	fs_copy("audiohed.wl6", "/ram/audiohed.wl6");
	fs_copy("audiot.wl6", "/ram/audiot.wl6");
	fs_copy("vgadict.wl6", "/ram/vgadict.wl6");
	fs_copy("vgagraph.wl6", "/ram/vgagraph.wl6");
	fs_copy("vgahead.wl6", "/ram/vgahead.wl6");
	fs_copy("gamemaps.wl6", "/ram/gamemaps.wl6");
	fs_copy("maphead.wl6", "/ram/maphead.wl6");
	fs_copy("vswap.wl6", "/ram/vswap.wl6");
#else
	fs_copy("audiohed.wl1", "/ram/audiohed.wl1");
	fs_copy("audiot.wl1", "/ram/audiot.wl1");
	fs_copy("vgadict.wl1", "/ram/vgadict.wl1");
	fs_copy("vgagraph.wl1", "/ram/vgagraph.wl1");
	fs_copy("vgahead.wl1", "/ram/vgahead.wl1");
	fs_copy("gamemaps.wl1", "/ram/gamemaps.wl1");
	fs_copy("maphead.wl1", "/ram/maphead.wl1");
	fs_copy("vswap.wl1", "/ram/vswap.wl1");
#endif
#endif

	fs_chdir("/ram");
}

void DC_CheckParameters()	{
	FILE 	*fp;
	char 	*buf;
	char 	*result = NULL;
    bool 	sampleRateGiven = false;
    bool 	audioBufferGiven = false;
	int		length = 0;

	fp = fopen(DCROOT "/params.txt", "r");

	if(!fp)
		return;

	// Get file length
	fseek(fp, 0, SEEK_END);
	length = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// Read file
	buf = (char *)malloc(length + 2);
	fread(buf, 1, length, fp);
	buf[length]=0;
	fclose(fp);

	result = strtok(buf, " ");

	while(result != NULL) {
#ifndef SPEAR
		if(!strcmp(result, "--goobers"))
#else
		if(!strcmp(result, "--debugmode"))
#endif
			param_debugmode = true;
		else if(!strcmp(result, "--baby"))
            param_difficulty = 0;
		else if(!strcmp(result, "--easy"))
            param_difficulty = 1;
		else if(!strcmp(result, "--normal"))
            param_difficulty = 2;
		else if(!strcmp(result, "--hard"))
            param_difficulty = 3;
		else if(!strcmp(result, "--nowait"))
            param_nowait = true;
		else if(!strcmp(result, "--tedlevel"))
		{
			result = strtok(NULL, " ");
			param_tedlevel = atoi(result);
		}
		else if(!strcmp(result, "--res"))
		{
			result = strtok(NULL, " ");
			screenWidth = atoi(result);
			result = strtok(NULL, " ");
			screenHeight = atoi(result);
			if(screenWidth % 320)
			{
				dbglog(DBG_DEBUG, "Screen width must be a multiple of 320!\n");
				dbglog(DBG_DEBUG, "Defaulting to 320x200.\n");
				screenWidth = 320;
				screenHeight = 200;
			}
			if(screenHeight % 200)
			{
				dbglog(DBG_DEBUG, "Screen height must be a multiple of 200!");
				dbglog(DBG_DEBUG, "Defaulting to 320x200.\n");
				screenWidth = 320;
				screenHeight = 200;
			}
		}
		else if(!strcmp(result, "--samplerate"))
		{
			result = strtok(NULL, " ");
			param_samplerate = atoi(result);
            sampleRateGiven = true;
		}
		else if(!strcmp(result, "--audiobuffer"))
		{
			result = strtok(NULL, " ");
            param_audiobuffer = atoi(result);
            audioBufferGiven = true;
		}
		else if(!strcmp(result, "--goodtimes"))
            param_goodtimes = true;

		result = strtok(NULL, " ");
	}

	free(buf);

	if(sampleRateGiven && !audioBufferGiven)
		param_audiobuffer = 4096 / (44100 / param_samplerate);
}

int DC_MousePresent() {
	return maple_first_mouse() != 0;
}

#endif // _arch_dreamcast
