#include "language.h"
#include "w_wad.h"
#include "scanner.hpp"
using namespace std;

Language language;

void Language::SetupStrings(const char* language)
{
	int lastLump = 0;
	int lump = 0;
	while((lump = Wads.FindLump("LANGUAGE", &lastLump)) != -1)
	{
		// Read the lump defaulting to english
		ReadLump(lump, language == NULL ? "enu" : language);
	}
}

void Language::ReadLump(int lump, const char* language)
{
	FWadLump wadLump = Wads.OpenLumpNum(lump);
	char* data = new char[wadLump.GetLength()];
	wadLump.Read(data, wadLump.GetLength());
	Scanner sc(data, wadLump.GetLength());

	int token = TK_NoToken;
	bool skip = false;
	bool noReplace = false;
	while((token = sc.GetNextToken()) != TK_NoToken)
	{
		if(token == '[')
		{
			// match with language
			sc.MustGetToken(TK_Identifier);
			if(strcmp(language, sc.str.c_str()) != 0)
				skip = true;
			else
			{
				skip = false;
				noReplace = false;
			}

			if(sc.CheckToken(TK_Identifier))
			{
				if(sc.str.compare("default") == 0)
				{
					// if not the correct language, go in no replace mode.
					if(skip)
					{
						skip = false;
						noReplace = true;
					}
				}
				else
				{
					printf("Unexpected identifier '%s'\n", sc.str.c_str());
					exit(0);
				}
			}

			sc.MustGetToken(']');
		}
		else if(token == TK_Identifier)
		{
			string index = sc.str;
			sc.MustGetToken('=');
			sc.MustGetToken(TK_StringConst);
			if(!skip)
			{
				if(!noReplace || (noReplace && strings.find(index) == strings.end()))
					strings[index] = sc.str;
			}
			sc.MustGetToken(';');
		}
		else
		{
			printf("Unexpected token.\n");
			exit(0);
		}
	}
}

const char* Language::operator[] (const char* index) const
{
	map<string, string>::const_iterator it = strings.find(index);
	if(it != strings.end())
		return it->second.c_str();
	return index;
}
