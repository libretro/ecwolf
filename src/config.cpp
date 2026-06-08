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

#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cstring>

#include "filesys.h"
#include "zstring.h"
#include "config.h"
#include "scanner.h"
#include "version.h"

typedef TMap<FName, TUniquePtr<SettingsData> > SettingsMap;

Config config;

Config::Config() : firstRun(false)
{
}

Config::~Config()
{
}


void Config::CreateSetting(const FName index, int defaultInt)
{
	SettingsData *data;
	if(!FindIndex(index, data))
	{
		data = new SettingsData(defaultInt);
		settings[index] = data;
	}
}

void Config::CreateSetting(const FName index, double defaultFloat)
{
	SettingsData *data;
	if(!FindIndex(index, data))
	{
		data = new SettingsData(defaultFloat);
		settings[index] = data;
	}
}

void Config::CreateSetting(const FName index, const char *defaultString)
{
	SettingsData *data;
	if(!FindIndex(index, data))
	{
		data = new SettingsData(defaultString);
		settings[index] = data;
	}
}

void Config::DeleteSetting(const FName index)
{
	settings.Remove(index);
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
	TUniquePtr<SettingsData> *setting = settings.CheckKey(index);
	if(setting == NULL)
		return false;
	data = *setting;
	return true;
}
