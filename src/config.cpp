// Emacs style mode select   -*- C++ -*- 
// =============================================================================
// ### ### ##   ## ###  #   ###  ##   #   #  ##   ## ### ##  ### ###  #  ###
// #    #  # # # # #  # #   #    # # # # # # # # # # #   # #  #   #  # # #  #
// ###  #  #  #  # ###  #   ##   # # # # # # #  #  # ##  # #  #   #  # # ###
//   #  #  #     # #    #   #    # # # # # # #     # #   # #  #   #  # # #  #
// ### ### #     # #    ### ###  ##   #   #  #     # ### ##  ###  #   #  #  #
//                                     --= http://bitowl.com/sde/ =--
// =============================================================================
// Copyright (C) 2008 "Blzut3" (admin@maniacsvault.net)
// The SDE Logo is a trademark of GhostlyDeath (ghostlydeath@gmail.com)
// =============================================================================
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
// =============================================================================
// Description:
// =============================================================================

#include "config.hpp"
#include "scanner.h"

#include <fstream>
#include <cstdlib>
#include <cmath>
#include <cstring>
using namespace std;

#ifdef WINDOWS
#include <direct.h>
#define mkdir(file,mode) _mkdir(file)
#else
#include <sys/stat.h>
#endif

#include "zstring.h"

Config *config = new Config();
void saveAtExit()
{
	config->SaveConfig();
	delete config;
}

Config::Config() : firstRun(false)
{
	atexit(saveAtExit);
}

Config::~Config()
{
	TMap<FName, SettingsData *>::Pair *pair;
	for(TMap<FName, SettingsData *>::Iterator it(settings);it.NextPair(pair);)
		delete pair->Value;
}

void Config::LocateConfigFile(int argc, char* argv[])
{
	FString configDir;
#ifdef WINDOWS
	configDir = argv[0];
	int pos = configDir.LastIndexOfAny("/\\");
	configDir = configDir.Mid(0, pos+1);
#else
	char *home = getenv("HOME");
	if(home == NULL || *home == '\0')
	{
		printf("Please set your HOME environment variable.\n");
		return;
	}
	configDir = FString(home) + "/.ecwolf/";
	struct stat dirStat;
	if(stat(configDir, &dirStat) == -1)
	{
		if(mkdir(configDir, S_IRWXU) == -1)
		{
			printf("Could not create settings directory, configuration will not be saved.\n");
			return;
		}
	}
#endif
	configFile = configDir + "ecwolf.cfg";

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
		unsigned int size = stream.tellg();
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
	FindIndex(index, data);
	return data;
}

bool Config::FindIndex(const FName index, SettingsData *&data)
{
	SettingsData **setting = settings.CheckKey(index);
	if(setting == NULL)
		return false;
	data = *setting;
	return true;
}
