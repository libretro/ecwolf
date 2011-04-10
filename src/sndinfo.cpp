/*
** sndinfo.cpp
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

#include "wl_def.h"
#include "m_swap.h"
#include "id_sd.h"
#include "w_wad.h"
#include "scanner.h"

////////////////////////////////////////////////////////////////////////////////
//
// Sound Index
//
////////////////////////////////////////////////////////////////////////////////

SoundIndex::SoundIndex() : priority(50)
{
	data[0] = data[1] = data[2] = NULL;
	lump[0] = lump[1] = lump[2] = -1;
}

SoundIndex::SoundIndex(const SoundIndex &other)
{
	*this = other;
}

SoundIndex::~SoundIndex()
{
	for(unsigned int i = 0;i < 3;i++)
	{
		if(data[i] != NULL)
			delete[] data[i];
	}
}

const SoundIndex &SoundIndex::operator= (const SoundIndex &other)
{
	priority = other.priority;
	for(unsigned int i = 0;i < 3;i++)
	{
		lump[i] = other.lump[i];

		if(lump[i] != -1)
		{
			data[i] = new byte[Wads.LumpLength(lump[i])];
			memcpy(data[i], other.data[i], Wads.LumpLength(lump[i]));
		}
		else
			data[i] = NULL;
	}
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
//
// SNDINFO
//
////////////////////////////////////////////////////////////////////////////////

SoundInformation SoundInfo;

SoundInformation::SoundInformation()
{
}

void SoundInformation::Init()
{
	printf("S_Init: Reading SNDINFO defintions.\n");

	int lastLump = 0;
	int lump = 0;
	while((lump = Wads.FindLump("SNDINFO", &lastLump)) != -1)
	{
		ParseSoundInformation(lump);
	}
}

void SoundInformation::ParseSoundInformation(int lumpNum)
{
	FWadLump lump = Wads.OpenLumpNum(lumpNum);
	char* data = new char[Wads.LumpLength(lumpNum)];
	lump.Read(data, Wads.LumpLength(lumpNum));

	Scanner sc(data, Wads.LumpLength(lumpNum));
	sc.SetScriptIdentifier(Wads.GetLumpFullName(lumpNum));
	delete[] data;

	while(sc.TokensLeft() != 0)
	{
		if(!sc.GetNextString())
			sc.ScriptMessage(Scanner::ERROR, "Expected logical name.\n");
		FName logicalName = sc->str;
		assert(logicalName[0] != '}');

		SoundIndex idx;
		bool hasAlternatives = false;

		if(sc.CheckToken('{'))
			hasAlternatives = true;

		unsigned int i = 0;
		do
		{
			if(sc.CheckToken('}') || !sc.GetNextString())
			{
				if(i == 0)
					sc.ScriptMessage(Scanner::ERROR, "Expected lump name for '%s'.\n", logicalName.GetChars());
				else
					break;
			}

			if(sc->str.Compare("NULL") == 0)
				continue;
			int sndLump = Wads.CheckNumForName(sc->str, ns_sounds);

			if(i == 2 && !sc.CheckToken('}'))
				sc.ScriptMessage(Scanner::ERROR, "Expected '}'.\n");
			if(sndLump == -1)
				continue;

			idx.lump[i] = sndLump;
			if(i == 0)
				idx.data[i] = SD_PrepareSound(sndLump);
			else
			{
				idx.data[i] = new byte[Wads.LumpLength(sndLump)];

				FWadLump soundReader = Wads.OpenLumpNum(sndLump);
				soundReader.Read(idx.data[i], Wads.LumpLength(sndLump));

				if(i == 1 || idx.lump[1] == -1)
					idx.priority = ReadLittleShort(&idx.data[i][4]);
			}
		}
		while(hasAlternatives && ++i < 3);

		if(!idx.IsNull())
			soundMap[logicalName] = idx;
	}
}

const SoundIndex &SoundInformation::operator[] (const char* logical) const
{
	const SoundIndex *it = soundMap.CheckKey(logical);
	if(it == NULL)
		return nullIndex;
	return *it;
}
