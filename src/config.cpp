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
#include "scanner.hpp"

#include <string>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <cstring>

#ifdef WINDOWS
#include <direct.h>
#define mkdir(file,mode) _mkdir(file)
#else
#include <sys/stat.h>
#endif

using namespace std;

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
	for(map<string, SettingsData *>::iterator it=settings.begin();it != settings.end();it++)
		delete (*it).second;
	settings.clear();
}

void Config::LocateConfigFile(int argc, char* argv[])
{
	string configDir;
#ifdef WINDOWS
	configDir = argv[0];
	int pos = static_cast<int> (configDir.find_last_of('\\')) > static_cast<int> (configDir.find_last_of('/')) ? configDir.find_last_of('\\') : configDir.find_last_of('/');
	configDir = configDir.substr(0, pos+1);
#else
	char *home = getenv("HOME");
	if(home == NULL || *home == '\0')
	{
		printf("Please set your HOME environment variable.\n");
		return;
	}
	configDir = string(home) + "/.ecwolf/";
	struct stat dirStat;
	if(stat(configDir.c_str(), &dirStat) == -1)
	{
		if(mkdir(configDir.c_str(), S_IRWXU) == -1)
		{
			printf("Could not create settings directory, configuration will not be saved.\n");
			return;
		}
	}
#endif
	configFile = configDir + "ecwolf.cfg";

	ReadConfig();
}

// NOTE: Be sure that '\\' is the first thing in the array otherwise it will re-escape.
static char escapeCharacters[] = {'\\', '"', 0};
const string &Config::Escape(string &str)
{
	for(unsigned int i = 0;escapeCharacters[i] != 0;i++)
	{
		// += 2 because we'll be inserting 1 character.
		for(size_t p = 0;p < str.length() && (p = str.find_first_of(escapeCharacters[i], p)) != string::npos;p += 2)
		{
			str.insert(p, 1, '\\');
		}
	}
	return str;
}
const string &Config::Unescape(string &str)
{
	for(unsigned int i = 0;escapeCharacters[i] != 0;i++)
	{
		string sequence = string("\\").append(1, escapeCharacters[i]);
		for(size_t p = 0;p < str.length() && (p = str.find_first_of(sequence, p)) != string::npos;p++)
		{
			str.replace(str.find_first_of(sequence, p), 2, 1, escapeCharacters[i]);
		}
	}
	return str;
}

void Config::ReadConfig()
{
	// Check to see if we have located the config file.
	if(configFile.empty())
		return;

	fstream stream(configFile.c_str(), ios_base::in | ios_base::binary);
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
		while(sc.TokensLeft())  // Go until there is nothing left to read.
		{
			sc.MustGetToken(TK_Identifier);
			string index = sc.str;
			sc.MustGetToken('=');
			if(sc.CheckToken(TK_StringConst))
			{
				CreateSetting(index, "");
				GetSetting(index)->SetValue(sc.str);
			}
			else
			{
				bool negative = sc.CheckToken('-');
				sc.MustGetToken(TK_IntConst);
				CreateSetting(index, 0);
				GetSetting(index)->SetValue(negative ? -sc.number : sc.number);
			}
			sc.MustGetToken(';');
		}

		delete[] data;
	}

	if(settings.size() == 0)
		firstRun = true;
}

void Config::SaveConfig()
{
	// Check to see if we're saving the settings.
	if(configFile.empty())
		return;

	fstream stream(configFile.c_str(), ios_base::out | ios_base::trunc);
	if(stream.is_open())
	{
		for(map<string, SettingsData *>::iterator it=settings.begin();it != settings.end();it++)
		{
			stream.write((*it).first.c_str(), (*it).first.length());
			if(stream.fail())
				return;
			SettingsData *data = (*it).second;
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
				string str = data->GetString(); // Make a non const copy of the string.
				Escape(str);
				char* value = new char[str.length() + 8];
				sprintf(value, " = \"%s\";\n", str.c_str());
				stream.write(value, str.length() + 7);
				delete[] value;
				if(stream.fail())
					return;
			}
		}
		stream.close();
	}
}

void Config::CreateSetting(const string index, unsigned int defaultInt)
{
	SettingsData *data;
	if(!FindIndex(index, data))
	{
		data = new SettingsData(defaultInt);
		settings[index] = data;
	}
}

void Config::CreateSetting(const string index, string defaultString)
{
	SettingsData *data;
	if(!FindIndex(index, data))
	{
		data = new SettingsData(defaultString);
		settings[index] = data;
	}
}

SettingsData *Config::GetSetting(const string index)
{
	SettingsData *data;
	FindIndex(index, data);
	return data;
}

bool Config::FindIndex(const string index, SettingsData *&data)
{
	map<string, SettingsData *>::iterator it = settings.find(index);
	if(it != settings.end())
	{
		data = (*it).second;
		return true;
	}
	return false;
}
