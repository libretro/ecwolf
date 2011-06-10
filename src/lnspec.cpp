/*
** lnspec.cpp
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

#include "id_sd.h"
#include "lnspec.h"
using namespace Specials;

#define DEFINE_SPECIAL(name,num,args) \
	static int LN_##name(MapSpot spot, AActor *activator);
#define FUNC(name) \
	static int LN_##name(MapSpot spot, AActor *activator)

DEFINE_SPECIAL(NOP, 0, 0)
#include "lnspecials.h"
#undef DEFINE_SPECIAL

LineSpecialFunction lnspecFunctions[NUM_POSSIBLE_SPECIALS] =
{
	LN_NOP,
	LN_Door_Open,
	LN_NOP,
	LN_Exit_Normal,
	LN_Exit_Secret,
	LN_NOP
};

#define DEFINE_SPECIAL(name,num,args) { #name, num, args},
const struct LineSpecialMeta
{
	const char* const		name;
	const unsigned int		num;
	const unsigned short	args;
} lnspecMeta[] = {
#include "lnspecials.h"
	{ NULL, 0, 0 }
};

LineSpecialFunction Specials::LookupFunction(const char* const function)
{
	const LineSpecialMeta *func = lnspecMeta;
	do
	{
		if(stricmp(func->name, function) == 0)
			return lnspecFunctions[func->num];
	}
	while((++func)->name != NULL);
	return LN_NOP;
}
LineSpecialFunction Specials::LookupFunction(LineSpecials function)
{
	return lnspecFunctions[function];
}

////////////////////////////////////////////////////////////////////////////////

#include "thinker.h"

FUNC(NOP)
{
	return 0;
}

class EVDoor : public Thinker
{
	public:
		EVDoor(MapSpot spot) : Thinker(), state(Opening), spot(spot), amount(0) {}

		void Tick()
		{
			switch(state)
			{
				case Opening:
					if(amount < 0xffff)
						amount += 0xff;
					spot->slideAmount[1] = spot->slideAmount[3] = amount;
					if(amount == 0xffff)
						state = Opened;
					break;
				case Opened:
					state = Closing;
					break;
				case Closing:
					if(amount > 0)
						amount -= 0xff;
					spot->slideAmount[1] = spot->slideAmount[3] = amount;
					if(amount == 0)
						Destroy();
					break;
			}
		}

	private:
		enum State { Opening, Opened, Closing } state;

		MapSpot spot;
		unsigned short amount;
};

FUNC(Door_Open)
{
	new EVDoor(spot);
	return 0;
}

FUNC(Exit_Normal)
{
	if(buttonheld[bt_use])
		return 0;
	buttonheld[bt_use] = true;

	playstate = ex_completed;
	SD_PlaySound ("world/level_done");
	SD_WaitSoundDone();
	return 1;
}

FUNC(Exit_Secret)
{
	if(buttonheld[bt_use])
		return 0;
	buttonheld[bt_use] = true;

	playstate = ex_secretlevel;
	SD_PlaySound ("world/level_done");
	SD_WaitSoundDone();
	return 1;
}
