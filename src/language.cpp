#include "language.h"
#include "w_wad.h"
#include "scanner.h"

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
	while(sc.GetNextToken())
	{
		token = sc->token;
		if(token == '[')
		{
			// match with language
			sc.MustGetToken(TK_Identifier);
			if(sc->str.Compare(language) != 0)
				skip = true;
			else
			{
				skip = false;
				noReplace = false;
			}

			if(sc.CheckToken(TK_Identifier))
			{
				if(sc->str.Compare("default") == 0)
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
					printf("Unexpected identifier '%s'\n", sc->str.GetChars());
					exit(0);
				}
			}

			sc.MustGetToken(']');
		}
		else if(token == TK_Identifier)
		{
			FName index = sc->str;
			sc.MustGetToken('=');
			sc.MustGetToken(TK_StringConst);
			if(!skip)
			{
				if(!noReplace || (noReplace && strings.CheckKey(index) == NULL))
					strings[index] = sc->str;
			}
			sc.MustGetToken(';');
		}
		else
		{
			sc.ScriptMessage(Scanner::ERROR, "Unexpected token.\n");
			exit(0);
		}
	}
}

const char* Language::operator[] (const char* index) const
{
	const FString *it = strings.CheckKey(index);
	if(it != NULL)
		return it->GetChars();
	return index;
}
