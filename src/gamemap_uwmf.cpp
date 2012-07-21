/*
** gamemap_uwmf.cpp
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

#include "gamemap.h"
#include "gamemap_common.h"
#include "lnspec.h"
#include "scanner.h"
#include "w_wad.h"
#include "wl_game.h"

#define CheckKey(x) if(key.CompareNoCase(x) == 0)

#define StartParseBlock \
	while(!sc.CheckToken('}')) { \
		sc.MustGetToken(TK_Identifier); \
		FString key(sc->str); \
		if(sc.CheckToken('=')) {
#define EndParseBlock \
			else \
				sc.GetNextToken(); \
			sc.MustGetToken(';'); \
		} else \
			sc.ScriptMessage(Scanner::ERROR, "Invalid syntax.\n"); \
	}

void TextMapParser::ParseTile(Scanner &sc, MapTile &tile)
{
	StartParseBlock

	CheckKey("texturenorth")
	{
		sc.MustGetToken(TK_StringConst);
		tile.texture[MapTile::North] = TexMan.CheckForTexture(sc->str, FTexture::TEX_Wall);
	}
	else CheckKey("texturesouth")
	{
		sc.MustGetToken(TK_StringConst);
		tile.texture[MapTile::South] = TexMan.CheckForTexture(sc->str, FTexture::TEX_Wall);
	}
	else CheckKey("texturewest")
	{
		sc.MustGetToken(TK_StringConst);
		tile.texture[MapTile::West] = TexMan.CheckForTexture(sc->str, FTexture::TEX_Wall);
	}
	else CheckKey("textureeast")
	{
		sc.MustGetToken(TK_StringConst);
		tile.texture[MapTile::East] = TexMan.CheckForTexture(sc->str, FTexture::TEX_Wall);
	}
	else CheckKey("offsetvertical")
	{
		sc.MustGetToken(TK_BoolConst);
		tile.offsetVertical = sc->boolean;
	}
	else CheckKey("offsethorizontal")
	{
		sc.MustGetToken(TK_BoolConst);
		tile.offsetHorizontal = sc->boolean;
	}

	EndParseBlock
}

void TextMapParser::ParseTrigger(Scanner &sc, MapTrigger &trigger)
{
	StartParseBlock

	CheckKey("x")
	{
		sc.MustGetToken(TK_IntConst);
		trigger.x = sc->number;
	}
	else CheckKey("y")
	{
		sc.MustGetToken(TK_IntConst);
		trigger.y = sc->number;
	}
	else CheckKey("z")
	{
		sc.MustGetToken(TK_IntConst);
		trigger.z = sc->number;
	}
	else CheckKey("activatenorth")
	{
		sc.MustGetToken(TK_BoolConst);
		trigger.activate[MapTrigger::North] = sc->boolean;
	}
	else CheckKey("activatesouth")
	{
		sc.MustGetToken(TK_BoolConst);
		trigger.activate[MapTrigger::South] = sc->boolean;
	}
	else CheckKey("activateeast")
	{
		sc.MustGetToken(TK_BoolConst);
		trigger.activate[MapTrigger::East] = sc->boolean;
	}
	else CheckKey("activatewest")
	{
		sc.MustGetToken(TK_BoolConst);
		trigger.activate[MapTrigger::West] = sc->boolean;
	}
	else CheckKey("action")
	{
		sc.MustGetToken(TK_IntConst);
		trigger.action = sc->number;
	}
	else CheckKey("arg0")
	{
		sc.MustGetToken(TK_IntConst);
		trigger.arg[0] = sc->number;
	}
	else CheckKey("arg1")
	{
		sc.MustGetToken(TK_IntConst);
		trigger.arg[1] = sc->number;
	}
	else CheckKey("arg2")
	{
		sc.MustGetToken(TK_IntConst);
		trigger.arg[2] = sc->number;
	}
	else CheckKey("arg3")
	{
		sc.MustGetToken(TK_IntConst);
		trigger.arg[3] = sc->number;
	}
	else CheckKey("arg4")
	{
		sc.MustGetToken(TK_IntConst);
		trigger.arg[4] = sc->number;
	}
	else CheckKey("playeruse")
	{
		sc.MustGetToken(TK_BoolConst);
		trigger.playerUse = sc->boolean;
	}
	else CheckKey("playercross")
	{
		sc.MustGetToken(TK_BoolConst);
		trigger.playerCross = sc->boolean;
	}
	else CheckKey("monsteruse")
	{
		sc.MustGetToken(TK_BoolConst);
		trigger.monsterUse = sc->boolean;
	}
	else CheckKey("repeatable")
	{
		sc.MustGetToken(TK_BoolConst);
		trigger.repeatable = sc->boolean;
	}
	else CheckKey("secret")
	{
		sc.MustGetToken(TK_BoolConst);
		trigger.isSecret = sc->boolean;
	}

	EndParseBlock
}

void TextMapParser::ParseZone(Scanner &sc, MapZone &zone)
{
	StartParseBlock
	if(false);
	EndParseBlock
}

class UWMFParser : public TextMapParser
{
	public:
		UWMFParser(GameMap *gm, Scanner &sc) : gm(gm), sc(sc)
		{
		}
		~UWMFParser()
		{
			for(unsigned int i = 0;i < data.Size();++i)
				delete[] data[i];
		}

		void Parse()
		{
			bool canChangeHeader = true;
			gm->header.width = 64;
			gm->header.height = 64;
			gm->header.tileSize = 64;

			while(sc.TokensLeft())
			{
				sc.MustGetToken(TK_Identifier);
				FString key(sc->str);
				if(sc.CheckToken('='))
				{
					CheckKey("namespace")
					{
						sc.MustGetToken(TK_StringConst);
						if(sc->str.Compare("Wolf3D") != 0)
							sc.ScriptMessage(Scanner::WARNING, "Wolf3D is the only supported namespace.\n");
					}
					else CheckKey("tilesize")
					{
						sc.MustGetToken(TK_IntConst);
						gm->header.tileSize = sc->number;
					}
					else CheckKey("name")
					{
						sc.MustGetToken(TK_StringConst);
						gm->header.name = sc->str;
					}
					else CheckKey("width")
					{
						if(!canChangeHeader)
							sc.ScriptMessage(Scanner::ERROR, "Changing dimensions after dependent data.\n");
						sc.MustGetToken(TK_IntConst);
						gm->header.width = sc->number;
					}
					else CheckKey("height")
					{
						if(!canChangeHeader)
							sc.ScriptMessage(Scanner::ERROR, "Changing dimensions after dependent data.\n");
						sc.MustGetToken(TK_IntConst);
						gm->header.height = sc->number;
					}
					else
						sc.GetNextToken();
					sc.MustGetToken(';');
				}
				else if(sc.CheckToken('{'))
				{
					CheckKey("planemap")
					{
						canChangeHeader = false;
						ParsePlaneMap();
					}
					else CheckKey("tile")
						ParseTile();
					else CheckKey("sector")
						ParseSector();
					else CheckKey("zone")
						ParseZone();
					else CheckKey("plane")
						ParsePlane();
					else CheckKey("thing")
						ParseThing();
					else CheckKey("trigger")
						ParseTrigger();
					else
					{
						while(!sc.CheckToken('}'))
							sc.GetNextToken();
					}
				}
				else
					sc.ScriptMessage(Scanner::ERROR, "Unable to parse TEXTMAP, invalid syntax.\n");
			}

			InstallPlanes();
		}

	protected:
		void InstallPlanes()
		{
			// Transfer data into actual structure with pointers.
			const unsigned int size = gm->GetHeader().width*gm->GetHeader().height;

			for(unsigned int i = 0;i < data.Size();++i)
			{
				MapPlane &plane = gm->planes[i];
				PMData* pdata = data[i];
				for(unsigned int j = 0;j < size;++j)
				{
					plane.map[j].SetTile(pdata[j].tile < 0 ? NULL : &gm->tilePalette[pdata[j].tile]);
					plane.map[j].sector = pdata[j].sector < 0 ? NULL : &gm->sectorPalette[pdata[j].sector];
					plane.map[j].zone = pdata[j].zone < 0 ? NULL : &gm->zonePalette[pdata[j].zone];
				}
			}
		}

		void ParsePlaneMap()
		{
			const unsigned int size = gm->GetHeader().width*gm->GetHeader().height;

			PMData* pdata = new PMData[size];
			data.Push(pdata);
			unsigned int i = 0;
			// Different syntax
			while(!sc.CheckToken('}'))
			{
				bool negative;

				sc.MustGetToken('{');
				negative = sc.CheckToken('-');
				sc.MustGetToken(TK_IntConst);
				pdata[i].tile = negative ? -sc->number : sc->number;
				sc.MustGetToken(',');
				negative = sc.CheckToken('-');
				sc.MustGetToken(TK_IntConst);
				pdata[i].sector = negative ? -sc->number : sc->number;
				sc.MustGetToken(',');
				negative = sc.CheckToken('-');
				sc.MustGetToken(TK_IntConst);
				pdata[i].zone = negative ? -sc->number : sc->number;
				sc.MustGetToken('}');
				if(++i != size)
					sc.MustGetToken(',');
				else
				{
					sc.MustGetToken('}');
					break;
				}
			}
			if(i != size)
				sc.ScriptMessage(Scanner::ERROR, "Not enough data in planemap.\n");
		}

		void ParsePlane()
		{
			MapPlane &plane = gm->NewPlane();
			StartParseBlock

			CheckKey("depth")
			{
				sc.MustGetToken(TK_IntConst);
				plane.depth = sc->number;
			}

			EndParseBlock
		}

		void ParseSector()
		{
			MapSector sector;
			StartParseBlock

			CheckKey("texturefloor")
			{
				sc.MustGetToken(TK_StringConst);
				sector.texture[MapSector::Floor] = TexMan.GetTexture(sc->str, FTexture::TEX_Flat);
			}
			else CheckKey("textureceiling")
			{
				sc.MustGetToken(TK_StringConst);
				sector.texture[MapSector::Ceiling] = TexMan.GetTexture(sc->str, FTexture::TEX_Flat);
			}

			EndParseBlock
			gm->sectorPalette.Push(sector);
		}

		void ParseThing()
		{
			MapThing thing;
			StartParseBlock

			CheckKey("x")
			{
				sc.MustGetToken(TK_FloatConst);
				thing.x = sc->decimal*FRACUNIT;
			}
			else CheckKey("y")
			{
				sc.MustGetToken(TK_FloatConst);
				thing.y = sc->decimal*FRACUNIT;
			}
			else CheckKey("z")
			{
				sc.MustGetToken(TK_FloatConst);
				thing.z = sc->decimal*FRACUNIT;
			}
			else CheckKey("angle")
			{
				sc.MustGetToken(TK_IntConst);
				thing.angle = sc->number;
			}
			else CheckKey("type")
			{
				sc.MustGetToken(TK_IntConst);
				thing.type = sc->number;
			}
			else CheckKey("ambush")
			{
				sc.MustGetToken(TK_BoolConst);
				thing.ambush = sc->boolean;
			}
			else CheckKey("patrol")
			{
				sc.MustGetToken(TK_BoolConst);
				thing.patrol = sc->boolean;
			}
			else CheckKey("skill1")
			{
				sc.MustGetToken(TK_BoolConst);
				thing.skill[0] = sc->boolean;
			}
			else CheckKey("skill2")
			{
				sc.MustGetToken(TK_BoolConst);
				thing.skill[1] = sc->boolean;
			}
			else CheckKey("skill3")
			{
				sc.MustGetToken(TK_BoolConst);
				thing.skill[2] = sc->boolean;
			}
			else CheckKey("skill4")
			{
				sc.MustGetToken(TK_BoolConst);
				thing.skill[3] = sc->boolean;
			}

			EndParseBlock
			gm->things.Push(thing);
		}

		void ParseTile()
		{
			MapTile tile;
			TextMapParser::ParseTile(sc, tile);

			gm->tilePalette.Push(tile);
		}

		void ParseTrigger()
		{
			// Since we can't know the x,y,z yet we need to create a temp and
			// copy it later.
			MapTrigger trigger;
			TextMapParser::ParseTrigger(sc, trigger);

			MapTrigger &trig = gm->NewTrigger(trigger.x, trigger.y, trigger.z);
			trig = trigger;

			if(trig.isSecret)
				++gamestate.secrettotal;
		}

		void ParseZone()
		{
			MapZone zone;
			zone.index = gm->zonePalette.Size();

			TextMapParser::ParseZone(sc, zone);
			gm->zonePalette.Push(zone);
		}

	private:
		struct PMData
		{
			int tile;
			int sector;
			int zone;
		};

		GameMap * const gm;
		Scanner &sc;
		TArray<PMData*> data;
};

void GameMap::ReadUWMFData()
{
	long size = lumps[0]->GetLength();
	char *data = new char[size];
	lumps[0]->Read(data, size);
	Scanner sc(data, size);
	delete[] data;

	// Read TEXTMAP
	UWMFParser parser(this, sc);
	parser.Parse();

	SetupLinks();
}
