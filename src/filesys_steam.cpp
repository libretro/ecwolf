/*
** filesys_steam.cpp
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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define USE_WINDOWS_DWORD
#define USE_WINDOWS_BOOLEAN
#include <windows.h>
#undef ERROR
#endif

#include "doomerrors.h"
#include "filesys.h"
#include "scanner.h"

namespace FileSys {

#ifdef _WIN32
/*
** Bits and pieces
**
**---------------------------------------------------------------------------
** Copyright 1998-2009 Randy Heit
** All rights reserved.
**
** License conditions same as BSD above.
**---------------------------------------------------------------------------
**
*/
//==========================================================================
//
// QueryPathKey
//
// Returns the value of a registry key into the output variable value.
//
//==========================================================================

static bool QueryPathKey(HKEY key, const char *keypath, const char *valname, FString &value)
{
	HKEY steamkey;
	DWORD pathtype;
	DWORD pathlen;
	LONG res;

	if(ERROR_SUCCESS == RegOpenKeyEx(key, keypath, 0, KEY_QUERY_VALUE, &steamkey))
	{
		if (ERROR_SUCCESS == RegQueryValueEx(steamkey, valname, 0, &pathtype, NULL, &pathlen) &&
			pathtype == REG_SZ && pathlen != 0)
		{
			// Don't include terminating null in count
			char *chars = value.LockNewBuffer(pathlen - 1);
			res = RegQueryValueEx(steamkey, valname, 0, NULL, (LPBYTE)chars, &pathlen);
			value.UnlockBuffer();
			if (res != ERROR_SUCCESS)
			{
				value = "";
			}
		}
		RegCloseKey(steamkey);
	}
	return value.IsNotEmpty();
}
#else
static TMap<int, FString> SteamAppInstallPath;
static void PSR_FindEndBlock(Scanner &sc)
{
	int depth = 1;
	do
	{
		if(sc.CheckToken('}'))
			--depth;
		else if(sc.CheckToken('{'))
			++depth;
		else
			sc.GetNextToken();
	}
	while(depth);
}
static void PSR_SkipBlock(Scanner &sc)
{
	sc.MustGetToken('{');
	PSR_FindEndBlock(sc);
}
static bool PSR_FindAndEnterBlock(Scanner &sc, const char* keyword)
{
	// Finds a block with a given keyword and then enter it (opening brace)
	// Should be closed with PSR_FindEndBlock
	while(sc.TokensLeft())
	{
		if(sc.CheckToken('}'))
		{
			sc.Rewind();
			return false;
		}

		sc.MustGetToken(TK_StringConst);
		if(sc->str.CompareNoCase(keyword) != 0)
		{
			if(!sc.CheckToken(TK_StringConst))
				PSR_SkipBlock(sc);
		}
		else
		{
			sc.MustGetToken('{');
			return true;
		}
	}
	return false;
}
static void PSR_ReadApps(Scanner &sc)
{
	// Inside of each app, the only thing we care about is the installdir
	// If the files we want are inside will be determined later.
	while(sc.TokensLeft())
	{
		if(sc.CheckToken('}'))
			break;

		sc.MustGetToken(TK_StringConst);
		int appid = atoi(sc->str);
		if(sc.CheckToken(TK_StringConst))
			continue;

		sc.MustGetToken('{');
		while(!sc.CheckToken('}'))
		{
			sc.MustGetToken(TK_StringConst);
			bool installdir = sc->str.CompareNoCase("installdir") == 0;

			if(sc.CheckToken('{'))
				PSR_FindEndBlock(sc);
			else
			{
				sc.MustGetToken(TK_StringConst);
				if(installdir && !sc->str.IsEmpty())
				{
					SteamAppInstallPath[appid] = sc->str;
				}
			}
		}
	}
	sc.Rewind();
}
static void ParseSteamRegistry(const char* path)
{
	char* data;
	long size;

	// Read registry data
	FILE* registry = fopen(path, "rb");
	if(!registry)
		return;

	fseek(registry, 0, SEEK_END);
	size = ftell(registry);
	fseek(registry, 0, SEEK_SET);
	data = new char[size];
	fread(data, 1, size, registry);
	fclose(registry);

	Scanner sc(data, size);
	delete[] data;

	// Find the app listing
	if(PSR_FindAndEnterBlock(sc, "InstallConfigStore"))
	{
		if(PSR_FindAndEnterBlock(sc, "Software"))
		{
			if(PSR_FindAndEnterBlock(sc, "Valve"))
			{
				if(PSR_FindAndEnterBlock(sc, "Steam"))
				{
					if(PSR_FindAndEnterBlock(sc, "apps"))
					{
						PSR_ReadApps(sc);
						PSR_FindEndBlock(sc);
					}
					PSR_FindEndBlock(sc);
				}
				PSR_FindEndBlock(sc);
			}
			PSR_FindEndBlock(sc);
		}
		PSR_FindEndBlock(sc);
	}
}
#endif

FString GetSteamPath(ESteamApp game)
{
	static struct SteamAppInfo
	{
		const char* const BasePath;
		const int AppID;
	} AppInfo[NUM_STEAM_APPS] =
	{
		{"Wolfenstein 3D", 2270},
		{"Spear of Destiny", 9000},
		{"The Apogee Throwback Pack", 238050}
	};

#if defined(_WIN32)
	FString path;

	//==========================================================================
	//
	// I_GetSteamPath
	//
	// Check the registry for the path to Steam, so that we can search for
	// IWADs that were bought with Steam.
	//
	//==========================================================================

	if (!QueryPathKey(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "SteamPath", path))
	{
		if(!QueryPathKey(HKEY_LOCAL_MACHINE, "Software\\Valve\\Steam", "InstallPath", path))
			path = "";
	}
	if(!path.IsEmpty())
		path += "\\SteamApps\\common";

	if(path.IsEmpty())
		return path;

	return path + PATH_SEPARATOR + AppInfo[game].BasePath;
#else
	// Linux and OS X actually allow the user to install to any location, so
	// we need to figure out on an app-by-app basis where the game is installed.
	// To do so, we read the virtual registry.
	if(SteamAppInstallPath.CountUsed() == 0)
	{
#ifdef __APPLE__
		FString regPath = OSX_FindFolder(DIR_ApplicationSupport) + "/Steam/config/config.vdf";
		try
		{
			
			ParseSteamRegistry(regPath);
		}
		catch(class CDoomError &error)
		{
			// If we can't parse for some reason just pretend we can't find anything.
			return FString();
		}
#else
		char* home = getenv("HOME");
		if(home != NULL && *home != '\0')
		{
			FString regPath;
			regPath.Format("%s/.steam/steam/config/config.vdf", home);
			try
			{
				ParseSteamRegistry(regPath);
			}
			catch(class CDoomError &error)
			{
				// If we can't parse for some reason just pretend we can't find anything.
				return FString();
			}
		}
#endif
	}
	const FString *installPath = SteamAppInstallPath.CheckKey(AppInfo[game].AppID);
	if(installPath)
		return *installPath;
	return FString();
#endif
}

}
