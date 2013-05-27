/*
** gamemap_planes.cpp
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

#include "doomerrors.h"
#include "id_ca.h"
#include "g_mapinfo.h"
#include "gamemap.h"
#include "gamemap_common.h"
#include "lnspec.h"
#include "scanner.h"
#include "w_wad.h"
#include "wl_game.h"
#include "wl_shade.h"

static const char* const FeatureFlagNames[] = {
	"lightlevels",
	NULL
};

class Xlat : public TextMapParser
{
public:
	enum
	{
		TF_PATHING = 1,
		TF_HOLOWALL = 2
	};

	enum EFeatureFlags
	{
		FF_LIGHTLEVELS = 1
	};

	struct ThingXlat
	{
	public:
		static int SortCompare(const void *t1, const void *t2)
		{
			unsigned short old1 = static_cast<const ThingXlat *>(t1)->oldnum;
			unsigned short old2 = static_cast<const ThingXlat *>(t2)->oldnum;
			if(old1 < old2)
				return -1;
			else if(old1 > old2)
				return 1;
			return 0;
		}

		unsigned short	oldnum;
		bool			isTrigger;


		unsigned short	newnum;
		unsigned char	angles;
		uint32_t		flags;
		unsigned char	minskill;

		MapTrigger		templateTrigger;
	};

	struct ModZone
	{
	public:
		enum Type
		{
			AMBUSH,
			CHANGETRIGGER
		};

		Type			type;
		bool			fillZone;

		unsigned int	oldTrigger;
		MapTrigger		triggerTemplate;
	};

	Xlat() : lump(0)
	{
	}

	void ClearTables()
	{
		// Clear out old data (to be called before initial load)
		for(unsigned int i = 0;i < 256;++i)
		{
			flatTable[i][0].SetInvalid();
			flatTable[i][1].SetInvalid();
		}
		tileTriggers.Clear();
		thingTable.Clear();
	}

	void LoadXlat(int lump, const GameInfo::FStringStack *baseStack, bool included=false)
	{
		if(!included)
		{
			if(this->lump == lump)
				return;
			this->lump = lump;
			FeatureFlags = static_cast<EFeatureFlags>(0);

			ClearTables();
		}

		FMemLump data = Wads.ReadLump(lump);
		Scanner sc((const char*)data.GetMem(), data.GetSize());
		sc.SetScriptIdentifier(Wads.GetLumpFullName(lump));

		while(sc.TokensLeft())
		{
			sc.MustGetToken(TK_Identifier);

			if(sc->str.CompareNoCase("tiles") == 0)
				LoadTilesTable(sc);
			else if(sc->str.CompareNoCase("things") == 0)
				LoadThingTable(sc);
			else if(sc->str.CompareNoCase("flats") == 0)
				LoadFlatsTable(sc);
			else if(sc->str.CompareNoCase("include") == 0)
			{
				sc.MustGetToken(TK_StringConst);

				// "$base" is used to include the previous translator in the stack
				FString lumpName = sc->str;
				const GameInfo::FStringStack *baseStackNext = baseStack;
				if(lumpName.CompareNoCase("$base") == 0)
				{
					if(baseStack == NULL)
						sc.ScriptMessage(Scanner::ERROR, "$base is empty.");
					lumpName = baseStack->str;
					baseStackNext = baseStack->Next();
				}

				int includeLump = Wads.GetNumForFullName(lumpName);
				if(includeLump == -1)
					sc.ScriptMessage(Scanner::ERROR, "Could not open '%s'.", sc->str.GetChars());
				LoadXlat(includeLump, baseStackNext);
			}
			else if(sc->str.CompareNoCase("enable") == 0 || sc->str.CompareNoCase("disable") == 0)
			{
				bool enable = sc->str.CompareNoCase("enable") == 0;
				sc.MustGetToken(TK_Identifier);
				unsigned int i = 0;
				do
				{
					if(sc->str.CompareNoCase(FeatureFlagNames[i]) == 0)
					{
						if(enable)
							FeatureFlags = static_cast<EFeatureFlags>(FeatureFlags|(1<<i));
						else
							FeatureFlags = static_cast<EFeatureFlags>(FeatureFlags|(~(1<<i)));
						break;
					}
				}
				while(FeatureFlagNames[++i]);
				sc.MustGetToken(';');
			}
			else
				sc.ScriptMessage(Scanner::ERROR, "Unknown xlat property '%s'.", sc->str.GetChars());
		}
	}

	EFeatureFlags GetFeatureFlags() const { return FeatureFlags; }

	WORD GetTilePalette(TArray<MapTile> &tilePalette)
	{
		WORD max = 0;
		WORD min = 0xFFFF;

		TMap<WORD, MapTile>::Iterator iter(this->tilePalette);
		TMap<WORD, MapTile>::Pair *pair;
		while(iter.NextPair(pair))
		{
			if(pair->Key > max)
				max = pair->Key;
			if(pair->Key < min)
				min = pair->Key;
		}
		if(min > max)
			throw CRecoverableError("No tiles found for translation!");

		tilePalette.Resize(max-min+1);

		iter.Reset();
		while(iter.NextPair(pair))
		{
			tilePalette[pair->Key - min] = pair->Value;
		}
		return min;
	}
	void GetZonePalette(TArray<MapZone> &zonePalette)
	{
		TMap<WORD, MapZone>::Iterator iter(this->zonePalette);
		TMap<WORD, MapZone>::Pair *pair;
		while(iter.NextPair(pair))
		{
			pair->Value.index = zonePalette.Size();
			zonePalette.Push(pair->Value);
		}
	}
	bool GetModZone(unsigned short tile, ModZone &modZone)
	{
		ModZone *item = modZones.CheckKey(tile);
		if(item)
		{
			modZone = *item;
			return true;
		}
		return false;
	}
	bool IsValidTile(unsigned short tile)
	{
		MapTile *item = tilePalette.CheckKey(tile);
		if(item)
			return true;
		return false;
	}

	FTextureID TranslateFlat(unsigned int index, bool ceiling)
	{
		if(flatTable[index][ceiling].isValid())
			return flatTable[index][ceiling];
		return levelInfo->DefaultTexture[ceiling];
	}

	bool TranslateTileTrigger(unsigned short tile, MapTrigger &trigger)
	{
		MapTrigger *item = tileTriggers.CheckKey(tile);
		if(item)
		{
			trigger = *item;
			return true;
		}
		return false;
	}

	bool TranslateThing(MapThing &thing, MapTrigger &trigger, bool &isTrigger, uint32_t &flags, unsigned short oldnum) const
	{
		bool valid = false;
		unsigned int type = thingTable.Size()/2;
		unsigned int max = thingTable.Size()-1;
		unsigned int min = 0;
		do
		{
			if(thingTable[type].oldnum == oldnum ||
				(thingTable[type].angles && unsigned(oldnum - thingTable[type].oldnum) < thingTable[type].angles))
			{
				valid = true;
				break;
			}

			if(thingTable[type].oldnum > oldnum)
				max = type-1;
			else if(thingTable[type].oldnum < oldnum)
				min = type+1;

			type = (max+min)/2;
		}
		while(max >= min && max < thingTable.Size());

		if(valid)
		{
			isTrigger = thingTable[type].isTrigger;
			if(isTrigger)
			{
				trigger = thingTable[type].templateTrigger;
			}
			else
			{
				flags = thingTable[type].flags;

				thing.type = thingTable[type].newnum;

				// The player has a weird rotation pattern. It's 450-angle.
				bool playerRotation = false;
				if(thing.type == 1)
					playerRotation = true;

				if(thingTable[type].angles)
				{
					thing.angle = (oldnum - thingTable[type].oldnum)*(360/thingTable[type].angles);
					if(playerRotation)
						thing.angle = (360 + 360/thingTable[type].angles)-thing.angle;
				}
				else
					thing.angle = 0;

				thing.patrol = flags&Xlat::TF_PATHING;
				thing.skill[0] = thing.skill[1] = thingTable[type].minskill <= 1;
				thing.skill[2] = thingTable[type].minskill <= 2;
				thing.skill[3] = thingTable[type].minskill <= 3;
			}
		}

		return valid;
	}

	int TranslateZone(unsigned short tile)
	{
		MapZone *zone = zonePalette.CheckKey(tile);
		if(zone)
			return zone->index;
		return -1;
	}

protected:
	void LoadFlatsTable(Scanner &sc)
	{
		sc.MustGetToken('{');
		while(!sc.CheckToken('}'))
		{
			sc.MustGetToken(TK_Identifier);
			bool ceiling = sc->str.CompareNoCase("ceiling") == 0;
			if(!ceiling && sc->str.CompareNoCase("floor") != 0)
				sc.ScriptMessage(Scanner::ERROR, "Unknown flat section '%s'.", sc->str.GetChars());

			sc.MustGetToken('{');
			unsigned int index = 0;
			do
			{
				sc.MustGetToken(TK_StringConst);
				FTextureID texID = TexMan.GetTexture(sc->str, FTexture::TEX_Flat);
				if(sc.CheckToken('='))
				{
					sc.MustGetToken(TK_IntConst);
					index = sc->number;
					if(index > 255)
						index = 255;
				}
				flatTable[index++][ceiling] = texID;

				if(index == 256)
					break;
			}
			while(sc.CheckToken(','));
			sc.MustGetToken('}');
		}
	}

	void LoadTilesTable(Scanner &sc)
	{
		sc.MustGetToken('{');
		while(!sc.CheckToken('}'))
		{
			sc.MustGetToken(TK_Identifier);
			if(sc->str.CompareNoCase("trigger") == 0)
			{
				sc.MustGetToken(TK_IntConst);
				if(sc->number > 0xFFFF)
					sc.ScriptMessage(Scanner::ERROR, "Trigger number out of range.");

				MapTrigger &trigger = tileTriggers[sc->number];
				sc.MustGetToken('{');
				TextMapParser::ParseTrigger(sc, trigger);
			}
			else if(sc->str.CompareNoCase("tile") == 0)
			{
				sc.MustGetToken(TK_IntConst);
				if(sc->number > 0xFFFF)
					sc.ScriptMessage(Scanner::ERROR, "Tile number out of range.");

				MapTile &tile = tilePalette[sc->number];
				sc.MustGetToken('{');
				TextMapParser::ParseTile(sc, tile);
			}
			else if(sc->str.CompareNoCase("modzone") == 0)
			{
				sc.MustGetToken(TK_IntConst);
				if(sc->number > 0xFFFF)
					sc.ScriptMessage(Scanner::ERROR, "Modzone number out of range.");

				unsigned int zoneIndex = sc->number;
				ModZone &zone = modZones[zoneIndex];
				sc.MustGetToken(TK_Identifier);
				if(sc->str.CompareNoCase("fillzone") == 0)
				{
					zone.fillZone = true;
					sc.MustGetToken(TK_Identifier);
				}
				else
				{
					zonePalette[zoneIndex] = MapZone();
					zone.fillZone = false;
				}

				if(sc->str.CompareNoCase("ambush") == 0)
				{
					zone.type = ModZone::AMBUSH;
					sc.MustGetToken(';');
				}
				else if(sc->str.CompareNoCase("changetrigger") == 0)
				{
					sc.MustGetToken(TK_IntConst);
					zone.type = ModZone::CHANGETRIGGER;
					zone.oldTrigger = sc->number;

					sc.MustGetToken('{');
					TextMapParser::ParseTrigger(sc, zone.triggerTemplate);
				}
				else
					sc.ScriptMessage(Scanner::ERROR, "Unknown modzone type.");
			}
			else if(sc->str.CompareNoCase("zone") == 0)
			{
				sc.MustGetToken(TK_IntConst);

				MapZone &zone = zonePalette[sc->number];
				sc.MustGetToken('{');
				TextMapParser::ParseZone(sc, zone);
			}
		}
	}

	void LoadThingTable(Scanner &sc)
	{
		sc.MustGetToken('{');
		while(!sc.CheckToken('}'))
		{
			ThingXlat thing;

			// Property
			if(sc.CheckToken(TK_Identifier))
			{
				if(sc->str.CompareNoCase("trigger") == 0)
				{
					sc.MustGetToken(TK_IntConst);
					thing.isTrigger = true;
					thing.angles = 0;
					thing.oldnum = sc->number;

					sc.MustGetToken('{');
					TextMapParser::ParseTrigger(sc, thing.templateTrigger);
				}
				else
					sc.ScriptMessage(Scanner::ERROR, "Unknown thing table block '%s'.", sc->str.GetChars());
			}
			else
			{
				// Handle thing translation
				thing.isTrigger = false;
				sc.MustGetToken('{');
				sc.MustGetToken(TK_IntConst);
				thing.oldnum = sc->number;
				sc.MustGetToken(',');
				sc.MustGetToken(TK_IntConst);
				thing.newnum = sc->number;
				sc.MustGetToken(',');
				sc.MustGetToken(TK_IntConst);
				thing.angles = sc->number;
				sc.MustGetToken(',');
				if(sc.CheckToken(TK_IntConst))
					thing.flags = sc->number;
				else
				{
					thing.flags = 0;
					do
					{
						sc.MustGetToken(TK_Identifier);
						if(sc->str.CompareNoCase("PATHING") == 0)
							thing.flags |= TF_PATHING;
						else if(sc->str.CompareNoCase("HOLOWALL") == 0)
							thing.flags |= TF_HOLOWALL;
						else
							sc.ScriptMessage(Scanner::ERROR, "Unknown flag '%s'.", sc->str.GetChars());
					}
					while(sc.CheckToken('|'));
				}
				sc.MustGetToken(',');
				sc.MustGetToken(TK_IntConst);
				thing.minskill = sc->number;
				sc.MustGetToken('}');
			}

			thingTable.Push(thing);
		}

		qsort(&thingTable[0], thingTable.Size(), sizeof(thingTable[0]), ThingXlat::SortCompare);
	}

private:
	int lump;

	WORD pushwall;
	WORD patrolpoint;
	TArray<ThingXlat> thingTable;
	TMap<WORD, MapTile> tilePalette;
	TMap<WORD, MapTrigger> tileTriggers;
	TMap<WORD, ModZone> modZones;
	TMap<WORD, MapZone> zonePalette;
	FTextureID flatTable[256][2]; // Floor/ceiling textures
	EFeatureFlags FeatureFlags;
};

// Reads old format maps
void GameMap::ReadPlanesData()
{
	static Xlat xlat;
	static const unsigned short UNIT = 64;
	enum OldPlanes { Plane_Tiles, Plane_Object, Plane_Flats, NUM_USABLE_PLANES };

	if(levelInfo->Translator.IsEmpty())
		xlat.LoadXlat(Wads.GetNumForFullName(gameinfo.Translator.str), gameinfo.Translator.Next());
	else
		xlat.LoadXlat(Wads.GetNumForFullName(levelInfo->Translator), &gameinfo.Translator);

	Xlat::EFeatureFlags FeatureFlags = xlat.GetFeatureFlags();

	// Old format maps always have a tile size of 64
	header.tileSize = UNIT;

	FileReader *lump = lumps[0];

	// Read plane count
	lump->Seek(10, SEEK_SET);
	WORD numPlanes;
	lump->Read(&numPlanes, 2);
	numPlanes = LittleShort(numPlanes);

	lump->Seek(14, SEEK_SET);
	char name[17];
	lump->Read(name, 16);
	name[16] = 0;
	header.name = name;

	lump->Seek(30, SEEK_SET);
	WORD dimensions[2];
	lump->Read(dimensions, 4);
	dimensions[0] = LittleShort(dimensions[0]);
	dimensions[1] = LittleShort(dimensions[1]);
	DWORD size = dimensions[0]*dimensions[1];
	header.width = dimensions[0];
	header.height = dimensions[1];

	Plane &mapPlane = NewPlane();
	mapPlane.depth = UNIT;

	// We need to store the spots marked for ambush since it's stored in the
	// tiles plane instead of the objects plane.
	TArray<WORD> ambushSpots;
	TArray<MapTrigger> triggers;

	// Read and store the info plane so we can reference it
	WORD* infoplane = new WORD[size];
	if(numPlanes > 3)
	{
		lump->Seek(size*2*3, SEEK_CUR);
		lump->Read(infoplane, size*2);
		lump->Seek(34, SEEK_SET);
	}
	else
		memset(infoplane, 0, size*2);

	for(int plane = 0;plane < numPlanes && plane < NUM_USABLE_PLANES;++plane)
	{
		if(plane == 3) // Info plane is already read
			continue;

		WORD* oldplane = new WORD[size];
		lump->Read(oldplane, size*2);

		switch(plane)
		{
			default:
				break;

			case Plane_Tiles:
			{
				WORD tileStart = xlat.GetTilePalette(tilePalette);
				xlat.GetZonePalette(zonePalette);

				TArray<WORD> fillSpots;
				TMap<WORD, Xlat::ModZone> changeTriggerSpots;
				

				for(unsigned int i = 0;i < size;++i)
				{
					oldplane[i] = LittleShort(oldplane[i]);

					if(xlat.IsValidTile(oldplane[i]))
						mapPlane.map[i].SetTile(&tilePalette[oldplane[i]-tileStart]);
					else
						mapPlane.map[i].SetTile(NULL);

					Xlat::ModZone zone;
					if(xlat.GetModZone(oldplane[i], zone))
					{
						if(zone.fillZone)
							fillSpots.Push(i);

						switch(zone.type)
						{
							case Xlat::ModZone::AMBUSH:
								ambushSpots.Push(i);
								break;
							case Xlat::ModZone::CHANGETRIGGER:
								changeTriggerSpots[i] = zone;
								break;
						}
					}

					MapTrigger templateTrigger;
					if(xlat.TranslateTileTrigger(oldplane[i], templateTrigger))
					{
						templateTrigger.x = i%header.width;
						templateTrigger.y = i/header.width;
						templateTrigger.z = 0;

						triggers.Push(templateTrigger);
					}

					int zoneIndex;
					if((zoneIndex = xlat.TranslateZone(oldplane[i])) != -1)
						mapPlane.map[i].zone = &zonePalette[zoneIndex];
					else
						mapPlane.map[i].zone = NULL;
				}

				// Get a sound zone for modzones that aren't valid sound zones.
				for(unsigned int i = 0;i < fillSpots.Size();++i)
				{
					const int candidates[4] = {
						fillSpots[i] + 1,
						fillSpots[i] - (int)header.width,
						fillSpots[i] - 1,
						fillSpots[i] + (int)header.width
					};
					for(unsigned int j = 0;j < 4;++j)
					{
						// Ensure that all candidates are valid locations.
						// In addition for moving left/right check to see that
						// the new location is indeed in the same row.
						if((candidates[j] < 0 || (unsigned)candidates[j] > size) ||
							((j == Tile::East || j == Tile::West) &&
							(candidates[j]/header.width != fillSpots[i]/header.width)))
							continue;

						// First adjacent zone wins
						if(mapPlane.map[candidates[j]].zone != NULL)
						{
							mapPlane.map[fillSpots[i]].zone = mapPlane.map[candidates[j]].zone;
							break;
						}
					}
				}

				TMap<WORD, Xlat::ModZone>::Iterator iter(changeTriggerSpots);
				TMap<WORD, Xlat::ModZone>::Pair *pair;
				while(iter.NextPair(pair))
				{
					// Look for and switch exit triggers.
					// NOTE: We're modifying the adjacent tile so the directions here are reversed.
					//       That is to say the tile to the east modifies the west wall.
					const int candidates[4] = {
						pair->Key - 1,
						pair->Key + (int)header.width,
						pair->Key + 1,
						pair->Key - (int)header.width
					};
					for(unsigned int j = 0;j < 4;++j)
					{
						// Same as before, only this time we check if our
						// replacement trigger activates at the line between
						// this tile and the cadidate.
						if((candidates[j] < 0 || (unsigned)candidates[j] > size) ||
							((j == Trigger::East || j == Trigger::West) &&
							(candidates[j]/header.width != pair->Key/header.width)) ||
							!pair->Value.triggerTemplate.activate[j])
							continue;

						// Look for any triggers matching the candidate
						for(int k = triggers.Size()-1;k >= 0;--k)
						{
							if(triggers[k].action == pair->Value.oldTrigger &&
								triggers[k].x == (unsigned)candidates[j]%header.width &&
								triggers[k].y == (unsigned)candidates[j]/header.width)
							{
								// Disable
								triggers[k].activate[j] = false;

								Trigger &triggerTemplate = pair->Value.triggerTemplate;
								triggerTemplate.x = triggers[k].x;
								triggerTemplate.y = triggers[k].y;
								triggerTemplate.z = triggers[k].z;

								// Only enable the activation in the direction we're changing.
								triggerTemplate.activate[0] = triggerTemplate.activate[1] = triggerTemplate.activate[2] = triggerTemplate.activate[3] = false;
								triggerTemplate.activate[j] = true;

								triggers.Push(triggerTemplate);
							}
						}
					}
				}

				if(FeatureFlags & Xlat::FF_LIGHTLEVELS)
				{
					// Visibility is roughly exponential
					static const fixed visTable[16] = {
						0x8888, 0xDDDD, 2<<FRACBITS,
						3<<FRACBITS, 8<<FRACBITS, 15<<FRACBITS,
						29<<FRACBITS, 56<<FRACBITS, 108<<FRACBITS,
						// After this point we basically max out the depth fog any way
						200<<FRACBITS, 200<<FRACBITS, 200<<FRACBITS,
						200<<FRACBITS, 200<<FRACBITS, 200<<FRACBITS,
						200<<FRACBITS
					};

					gLevelVisibility = visTable[clamp(oldplane[3] - 0xFC, 0, 15)];
					gLevelLight = clamp(oldplane[2] - 0xD8, 0, 7)*8 + 130; // Seems to be approx accurate for every even number lighting (0, 2, 4, 6)
				}
				else
				{
					gLevelVisibility = VISIBILITY_DEFAULT;
					gLevelLight = LIGHTLEVEL_DEFAULT;
				}
				break;
			}

			case Plane_Object:
			{
				unsigned int ambushSpot = 0;
				ambushSpots.Push(0xFFFF); // Prevent uninitialized value errors. 

				for(unsigned int i = 0;i < size;++i)
				{
					oldplane[i] = LittleShort(oldplane[i]);

					if(oldplane[i] == 0)
					{
						// In case of malformed maps we need to always check this.
						if(ambushSpots[ambushSpot] == i)
							++ambushSpot;
						continue;
					}

					Thing thing;
					Trigger trigger;
					bool isTrigger;
					uint32_t flags = 0;

					if(!xlat.TranslateThing(thing, trigger, isTrigger, flags, oldplane[i]))
						printf("Unknown old type %d @ (%d,%d)\n", oldplane[i], i%header.width, i/header.width);
					else
					{
						if(isTrigger)
						{
							trigger.x = i%header.width;
							trigger.y = i/header.width;
							trigger.z = 0;

							triggers.Push(trigger);
						}
						else
						{
							if(flags & Xlat::TF_HOLOWALL)
							{
								MapSpot spot = &mapPlane.map[i];
								if(spot->tile)
								{
									spot->sideSolid[0] = spot->sideSolid[1] = spot->sideSolid[2] = spot->sideSolid[3] = false;
									if(flags & Xlat::TF_PATHING)
									{
										// If we created a holowall and we path into another wall it should also become non-solid.
										spot = spot->GetAdjacent(MapTile::Side(thing.angle/90));
										if(spot->tile)
											spot->sideSolid[0] = spot->sideSolid[1] = spot->sideSolid[2] = spot->sideSolid[3] = false;
									}
								}
							}

							thing.x = ((i%header.width)<<FRACBITS)+(FRACUNIT/2);
							thing.y = ((i/header.width)<<FRACBITS)+(FRACUNIT/2);
							thing.z = 0;
							thing.ambush = ambushSpots[ambushSpot] == i;
							things.Push(thing);
						}
					}

					if(ambushSpots[ambushSpot] == i)
						++ambushSpot;
				}
				break;
			}

			case Plane_Flats:
			{
				// Look for all unique floor/ceiling texture combinations.
				WORD type = 0;
				TMap<WORD, WORD> flatMap;
				for(unsigned int i = 0;i < size;++i)
				{
					oldplane[i] = LittleShort(oldplane[i]);

					if(!flatMap.CheckKey(oldplane[i]))
						flatMap[oldplane[i]] = type++;
				}

				// Build the palette.
				sectorPalette.Resize(type);
				TMap<WORD, WORD>::ConstIterator iter(flatMap);
				TMap<WORD, WORD>::ConstPair *pair;
				while(iter.NextPair(pair))
				{
					Sector &sect = sectorPalette[pair->Value];
					sect.texture[Sector::Floor] = xlat.TranslateFlat(pair->Key&0xFF, Sector::Floor);
					sect.texture[Sector::Ceiling] = xlat.TranslateFlat(pair->Key>>8, Sector::Ceiling);
				}

				// Now link the sector data to map points!
				for(unsigned int i = 0;i < size;++i)
					mapPlane.map[i].sector = &sectorPalette[flatMap[oldplane[i]]];
				break;
			}
		}
		delete[] oldplane;
	}

	SetupLinks();

	// Install triggers
	for(unsigned int i = 0;i < triggers.Size();++i)
	{
		Trigger &templateTrigger = triggers[i];

		// Check the info plane and if set move the activation point to a switch ot touch plate
		const WORD info = infoplane[templateTrigger.y*header.width + templateTrigger.x];
		if(info)
		{
			MapSpot target = GetSpot(templateTrigger.x, templateTrigger.y, 0);
			unsigned int tag = (templateTrigger.x<<8)|templateTrigger.y;
			SetSpotTag(target, tag);

			// Activated by touch plate or switch
			templateTrigger.arg[0] = tag;
			templateTrigger.x = (info>>8)&0xFF;
			templateTrigger.y = info&0xFF;

			MapSpot spot = GetSpot(templateTrigger.x, templateTrigger.y, 0);
			if(spot->tile) // Switch
			{
				templateTrigger.playerCross = false;
				templateTrigger.playerUse = true;
			}
			else // Touch plate
			{
				templateTrigger.playerCross = true;
				templateTrigger.playerUse = false;
			}
			templateTrigger.activate[0] = templateTrigger.activate[1] = templateTrigger.activate[2] = templateTrigger.activate[3] = true;
		}

		Trigger &trig = NewTrigger(templateTrigger.x, templateTrigger.y, templateTrigger.z);
		trig = templateTrigger;

		if(trig.isSecret)
			++gamestate.secrettotal;
	}
	delete[] infoplane;
}
