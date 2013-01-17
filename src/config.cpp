/*
** config.cpp
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

#include "config.h"
#include "scanner.h"

#include <fstream>
#include <cstdlib>
#include <cmath>
#include <cstring>
using namespace std;

#if defined(WINDOWS)
#include <direct.h>
#define mkdir(file,mode) _mkdir(file)
#elif defined(__APPLE__)
#include <CoreServices/CoreServices.h>
#endif
#include <sys/stat.h>

#include "zstring.h"

Config config;

Config::Config() : firstRun(false)
{
}

Config::~Config()
{
	TMap<FName, SettingsData *>::Pair *pair;
	for(TMap<FName, SettingsData *>::Iterator it(settings);it.NextPair(pair);)
		delete pair->Value;
}

void Config::LocateConfigFile(int argc, char* argv[])
{
	// Look for --config parameter
	for(int i = 0;i < argc-1;++i)
	{
		if(strcmp(argv[i], "--config") == 0)
		{
			configFile = argv[i+1];
			ReadConfig();
			return;
		}
	}

	// Nothing explicitly set so go to the home directory.
#if defined(WINDOWS)
	char *home = getenv("APPDATA");
	if(home == NULL || *home == '\0')
	{
		printf("APPDATA environment variable not set, falling back.\n");
		configDir = argv[0];
		int pos = configDir.LastIndexOfAny("/\\");
		configDir = configDir.Mid(0, pos);
	}
	else
		configDir.Format("%s\\ecwolf", home);
#elif defined(__APPLE__)
	UInt8 home[PATH_MAX];
	FSRef folder;

	if(FSFindFolder(kUserDomain, kPreferencesFolderType, kCreateFolder, &folder) != noErr ||
		FSRefMakePath(&folder, home, PATH_MAX) != noErr)
	{
		printf("Could not create your preferences files.\n");
		return;
	}
	configDir = reinterpret_cast<const char*>(home);	
#else
	char *home = getenv("HOME");
	if(home == NULL || *home == '\0')
	{
		printf("Please set your HOME environment variable.\n");
		return;
	}
	configDir.Format("%s/.config/ecwolf", home);
#endif

	struct stat dirStat;
	if(stat(configDir, &dirStat) == -1)
	{
		if(mkdir(configDir, S_IRWXU) == -1)
		{
			printf("Could not create settings directory, configuration will not be saved.\n");
			return;
		}
	}

#ifdef WINDOWS
	configFile = configDir + "\\ecwolf.cfg";
#else
	configFile = configDir + "/ecwolf.cfg";
#endif

	ReadConfig();
}

void Config::ReadConfig()
{
	// Check to see if we have located the config file.
	if(configFile.IsEmpty())
		return;

	fstream stream(configFile, ios_base::in | ios_base::binary);
	if(stream.is_open())
	{
		stream.seekg(0, ios_base::end);
		if(stream.fail())
			return;
		unsigned int size = static_cast<unsigned int>(stream.tellg());
		stream.seekg(0, ios_base::beg);
		if(stream.fail())
			return;
		char* data = new char[size];
		stream.read(data, size);
		// The eof flag seems to trigger fail on windows.
		if(!stream.eof() && stream.fail())
		{
			delete[] data;
			return;
		}
		stream.close();

		Scanner sc(data, size);
		sc.SetScriptIdentifier("Configuration");
		while(sc.TokensLeft())  // Go until there is nothing left to read.
		{
			sc.MustGetToken(TK_Identifier);
			FString index = sc->str;
			sc.MustGetToken('=');
			if(sc.CheckToken(TK_StringConst))
			{
				CreateSetting(index, "");
				GetSetting(index)->SetValue(sc->str);
			}
			else
			{
				bool negative = sc.CheckToken('-');
				sc.MustGetToken(TK_IntConst);
				CreateSetting(index, 0);
				GetSetting(index)->SetValue(negative ? -sc->number : sc->number);
			}
			sc.MustGetToken(';');
		}

		delete[] data;
	}

	if(settings.CountUsed() == 0)
		firstRun = true;
}

void Config::SaveConfig()
{
	// Check to see if we're saving the settings.
	if(configFile.IsEmpty())
		return;

	fstream stream(configFile, ios_base::out | ios_base::trunc);
	if(stream.is_open())
	{
		TMap<FName, SettingsData *>::Pair *pair;
		for(TMap<FName, SettingsData *>::Iterator it(settings);it.NextPair(pair);)
		{
			stream.write(pair->Key, strlen(pair->Key));
			if(stream.fail())
				return;
			SettingsData *data = pair->Value;
			if(data->GetType() == SettingsData::ST_INT)
			{
				// Determine size of number.
				unsigned int intLength = 1;
				while(data->GetInteger()/static_cast<int>(pow(10.0, static_cast<double>(intLength))) != 0)
					intLength++;

				char* value = new char[intLength + 7];
				sprintf(value, " = %d;\n", data->GetInteger());
				stream.write(value, strlen(value));
				delete[] value;
				if(stream.fail())
					return;
			}
			else
			{
				FString str = data->GetString(); // Make a non const copy of the string.
				Scanner::Escape(str);
				char* value = new char[str.Len() + 8];
				sprintf(value, " = \"%s\";\n", str.GetChars());
				stream.write(value, str.Len() + 7);
				delete[] value;
				if(stream.fail())
					return;
			}
		}
		stream.close();
	}
}

void Config::CreateSetting(const FName index, unsigned int defaultInt)
{
	SettingsData *data;
	if(!FindIndex(index, data))
	{
		data = new SettingsData(defaultInt);
		settings[index] = data;
	}
}

void Config::CreateSetting(const FName index, FString defaultString)
{
	SettingsData *data;
	if(!FindIndex(index, data))
	{
		data = new SettingsData(defaultString);
		settings[index] = data;
	}
}

SettingsData *Config::GetSetting(const FName index)
{
	SettingsData *data;
	if(FindIndex(index, data))
		return data;
	return NULL;
}

bool Config::FindIndex(const FName index, SettingsData *&data)
{
	SettingsData **setting = settings.CheckKey(index);
	if(setting == NULL)
		return false;
	data = *setting;
	return true;
}
