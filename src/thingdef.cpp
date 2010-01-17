#include <string>

#include "scanner.hpp"
#include "w_wad.h"
#include "wl_def.h"
#include "thingdef.h"

using namespace std;

#define DEFINE_FLAG(prefix, flag, type, variable) { prefix##_##flag, #flag, (int)(size_t)&((type*)1)->variable - 1 }
const FlagDef flags[] =
{
	DEFINE_FLAG(FL, SHOOTABLE, Actor, flags),
	DEFINE_FLAG(FL, BONUS, Actor, flags),
	DEFINE_FLAG(FL, NEVERMARK, Actor, flags),
	DEFINE_FLAG(FL, VISABLE, Actor, flags)
};

void ClassDef::LoadActors()
{
	printf("ClassDef: Loading actor definitions.\n");

	int lastLump = 0;
	int lump = 0;
	while((lump = Wads.FindLump("DECORATE", &lastLump)) != -1)
	{
		ParseDecorateLump(lump);
	}
}

void ClassDef::ParseActor(Scanner &sc)
{
	ClassDef *newClass = new ClassDef();

	sc.MustGetToken(TK_Identifier);
	newClass->name = sc.str;
	sc.MustGetToken('{');
	while(!sc.CheckToken('}'))
	{
		if(sc.CheckToken('+') || sc.CheckToken('-'))
		{
			string flagName;
			sc.MustGetToken(TK_Identifier);
			flagName = sc.str;
			if(sc.CheckToken('.'))
			{
				sc.MustGetToken(TK_Identifier);
				flagName += string(".") + sc.str;
			}
			printf("Warning: Unkown flag '%s' for actor '%s'.\n", flagName.c_str(), newClass->name.c_str());
		}
		else
		{
			sc.MustGetToken(TK_Identifier);
			if(stricmp(sc.str.c_str(), "states") == 0)
			{
				sc.MustGetToken('{');
				sc.MustGetToken(TK_Identifier); // We should already have grabbed the identifier in all other cases.
				while(sc.lastToken != '}')
				{
					if(sc.CheckToken(':'))
					{
						// New state
						if(sc.CheckToken('}'))
							break;
						sc.MustGetToken(TK_Identifier);
					}
					else
					{
						if(sc.str.length() != 4)
							sc.ScriptError("Frame must be exactly 4 characters long.");
						if(stricmp(sc.str.c_str(), "goto") == 0)
						{
							sc.MustGetToken(TK_Identifier);
							if(sc.CheckToken('}'))
								break;
							sc.MustGetToken(TK_Identifier);
							continue;
						}
						else if(stricmp(sc.str.c_str(), "wait") == 0)
						{
							if(sc.CheckToken('}'))
								break;
							sc.MustGetToken(TK_Identifier);
							continue;
						}
						else if(stricmp(sc.str.c_str(), "loop") == 0)
						{
							if(sc.CheckToken('}'))
								break;
							sc.MustGetToken(TK_Identifier);
							continue;
						}
						else if(stricmp(sc.str.c_str(), "stop") == 0)
						{
							if(sc.CheckToken('}'))
								break;
							sc.MustGetToken(TK_Identifier);
							continue;
						}
						sc.MustGetToken(TK_Identifier);
						sc.CheckToken('-');
						sc.MustGetToken(TK_IntConst);
						if(sc.CheckToken('}'))
							break;
						for(int func = 0;func < 3;func++)
						{
							sc.MustGetToken(TK_Identifier);
							if(sc.str.length() == 4 || func == 2) // Assuming we won't have any 4 character length function names.
								break; // Second condition is just a dummy so we satisfy our need for an indentifer before the next loop.
							if(sc.CheckToken('}'))
								break;
							if(sc.CheckToken('('))
								sc.MustGetToken(')');
							if(sc.CheckToken('}'))
								break;
						}
					}
				}
			}
			else
			{
				string propertyName = sc.str;
				if(sc.CheckToken('.'))
				{
					sc.MustGetToken(TK_Identifier);
					propertyName = string(".") + sc.str;
				}
				do
				{
					sc.GetNextToken();
				}
				while(sc.CheckToken(','));
				printf("Warning: Unkown property '%s' for actor '%s'.\n", propertyName.c_str(), newClass->name.c_str());
			}
		}
	}
}

void ClassDef::ParseDecorateLump(int lumpNum)
{
	FWadLump lump = Wads.OpenLumpNum(lumpNum);
	char* data = new char[Wads.LumpLength(lumpNum)];
	lump.Read(data, Wads.LumpLength(lumpNum));
	Scanner sc(data, Wads.LumpLength(lumpNum));

	while(sc.TokensLeft())
	{
		sc.MustGetToken(TK_Identifier);
		if(stricmp(sc.str.c_str(), "actor") == 0)
		{
			ParseActor(sc);
		}
		else
			sc.ScriptError("Unknown thing section '%s'.", sc.str.c_str());
	}
}
