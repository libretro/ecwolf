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

#include "gamemap.h"
#include "lnspec.h"
#include "w_wad.h"
#include "wl_game.h"

// Reads old format maps
void GameMap::ReadPlanesData()
{
	static const unsigned short UNIT = 64;
	enum OldPlanes { Plane_Tiles, Plane_Object, Plane_Flats, NUM_USABLE_PLANES };

	valid = true;

	// Old format maps always have a tile size of 64
	header.tileSize = UNIT;

	FWadLump lump = Wads.OpenLumpNum(markerLump+1);

	// Read plane count
	lump.Seek(10, SEEK_SET);
	WORD numPlanes;
	lump.Read(&numPlanes, 2);
	numPlanes = LittleShort(numPlanes);

	lump.Seek(14, SEEK_SET);
	char name[16];
	lump.Read(name, 16);
	header.name = name;

	lump.Seek(30, SEEK_SET);
	WORD dimensions[2];
	lump.Read(dimensions, 4);
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

	for(int plane = 0;plane < numPlanes && plane < NUM_USABLE_PLANES;++plane)
	{
		WORD* oldplane = new WORD[size];
		lump.Read(oldplane, size*2);

		switch(plane)
		{
			default:
				break;

			case Plane_Tiles:
			{
				// Yay for magic numbers!
				static const WORD
					NUM_WALLS = 64,
					EXIT_TILE = 21,
					DOOR_START = 90,
					DOOR_END = DOOR_START + 6*2,
					AMBUSH_TILE = 106,
					ALT_EXIT = 107,
					ZONE_START = 108,
					ZONE_END = ZONE_START + 36;

				tilePalette.Resize(NUM_WALLS+(DOOR_END-DOOR_START));
				zonePalette.Resize(ZONE_END-ZONE_START);
				for(unsigned int i = 0;i < zonePalette.Size();++i)
					zonePalette[i].index = i;
				TArray<WORD> altExitSpots;

				// Import tiles
				for(unsigned int i = 0;i < NUM_WALLS;++i)
				{
					Tile &tile = tilePalette[i];
					tile.texture[Tile::North] = tile.texture[Tile::South] = TexMan.GetTile(i, false);
					tile.texture[Tile::East] = tile.texture[Tile::West] = TexMan.GetTile(i, true);
				}
				for(unsigned int i = 0;i < DOOR_END-DOOR_START;i++)
				{
					Tile &tile = tilePalette[i+NUM_WALLS];
					tile.offsetVertical = i%2 == 0;
					tile.offsetHorizontal = i%2 == 1;
					if(i%2 == 0)
					{
						tile.texture[Tile::North] = tile.texture[Tile::South] = TexMan.GetDoor(i/2, true, true);
						tile.texture[Tile::East] = tile.texture[Tile::West] = TexMan.GetDoor(i/2, true, false);
					}
					else
					{
						tile.texture[Tile::North] = tile.texture[Tile::South] = TexMan.GetDoor(i/2, false, false);
						tile.texture[Tile::East] = tile.texture[Tile::West] = TexMan.GetDoor(i/2, false, true);
					}
				}
				

				for(unsigned int i = 0;i < size;++i)
				{
					if(oldplane[i] < NUM_WALLS)
						mapPlane.map[i].tile = &tilePalette[oldplane[i]];
					else if(oldplane[i] >= DOOR_START && oldplane[i] < DOOR_END)
					{
						const bool vertical = oldplane[i]%2 == 0;
						const unsigned int doorType = (oldplane[i]-DOOR_START)/2;
						Trigger &trig = NewTrigger(i%UNIT, i/UNIT, 0);
						trig.action = Specials::Door_Open;
						trig.arg[0] = 1;
						trig.arg[1] = doorType >= 1 && doorType <= 4 ? doorType : 0;
						trig.playerUse = true;
						trig.monsterUse = true;
						trig.repeatable = true;
						if(vertical)
							trig.activate[Trigger::North] = trig.activate[Trigger::South] = false;
						else
							trig.activate[Trigger::East] = trig.activate[Trigger::West] = false;

						mapPlane.map[i].tile = &tilePalette[oldplane[i]-DOOR_START+NUM_WALLS];
					}
					else
						mapPlane.map[i].tile = NULL;

					// We'll be moving the ambush flag to the actor itself.
					if(oldplane[i] == AMBUSH_TILE)
						ambushSpots.Push(i);
					// For the alt exit we need to change the triggers after we
					// are done with this pass.
					else if(oldplane[i] == ALT_EXIT)
						altExitSpots.Push(i);
					else if(oldplane[i] == EXIT_TILE)
					{
						// Add exit trigger
						Trigger &trig = NewTrigger(i%UNIT, i/UNIT, 0);
						trig.action = Specials::Exit_Normal;
						trig.activate[Trigger::North] = trig.activate[Trigger::South] = false;
						trig.playerUse = true;
					}

					if(oldplane[i] >= ZONE_START && oldplane[i] < ZONE_END)
						mapPlane.map[i].zone = &zonePalette[oldplane[i]-ZONE_START];
					else
						mapPlane.map[i].zone = NULL;
				}

				// Get a sound zone for ambush spots if possible
				for(unsigned int i = 0;i < ambushSpots.Size();++i)
				{
					const int candidates[4] = {
						ambushSpots[i] + 1,
						ambushSpots[i] - UNIT,
						ambushSpots[i] - 1,
						ambushSpots[i] + UNIT
					};
					for(unsigned int j = 0;j < 4;++j)
					{
						// Ensure that all candidates are valid locations.
						// In addition for moving left/right check to see that
						// the new location is indeed in the same row.
						if((candidates[j] < 0 || candidates[j] > size) ||
							((j == Tile::East || j == Tile::West) &&
							(candidates[j]/UNIT != ambushSpots[i]/UNIT)))
							continue;

						// First adjacent zone wins
						if(mapPlane.map[candidates[j]].zone != NULL)
						{
							mapPlane.map[ambushSpots[i]].zone = mapPlane.map[candidates[j]].zone;
							break;
						}
					}
				}

				for(unsigned int i = 0;i < altExitSpots.Size();++i)
				{
					// Look for and switch exit triggers.
					const int candidates[4] = {
						altExitSpots[i] + 1,
						altExitSpots[i] - UNIT,
						altExitSpots[i] - 1,
						altExitSpots[i] + UNIT
					};
					for(unsigned int j = 0;j < 4;++j)
					{
						// Same as before
						if((candidates[j] < 0 || candidates[j] > size) ||
							((j == Trigger::East || j == Trigger::West) &&
							(candidates[j]/UNIT != altExitSpots[i]/UNIT)))
							continue;

						if(oldplane[candidates[j]] == EXIT_TILE)
						{
							// Look for any triggers matching the candidate
							for(unsigned int k = 0;k < triggers.Size();++k)
							{
								if((triggers[k].x == candidates[j]%UNIT) &&
									(triggers[k].y == candidates[j]/UNIT) &&
									(triggers[k].action == Specials::Exit_Normal))
									triggers[k].action = Specials::Exit_Secret;
							}
						}
					}
				}
				break;
			}

			case Plane_Object:
			{
				// Will need to consider externalizing some of these
				static const struct ThingXlat
				{
					unsigned short	oldnum;
					unsigned short	newnum;
					unsigned char	angles;
					bool			patrol;
					unsigned char	minskill;
				} xlat[] =
				{
					{19,	1,	4,	false,	0},
					{23,	33,	0,	false,	0},
					{24,	34,	0,	false,	0},
					{25,	35,	0,	false,	0},
					{26,	36,	0,	false,	0},
					{27,	37,	0,	false,	0},
					{28,	38,	0,	false,	0},
					{29,	39,	0,	false,	0},
					{30,	40,	0,	false,	0},
					{31,	41,	0,	false,	0},
					{32,	42,	0,	false,	0},
					{33,	43,	0,	false,	0},
					{34,	44,	0,	false,	0},
					{35,	45,	0,	false,	0},
					{36,	46,	0,	false,	0},
					{37,	47,	0,	false,	0},
					{38,	48,	0,	false,	0},
					{39,	49,	0,	false,	0},
					{40,	50,	0,	false,	0},
					{41,	51,	0,	false,	0},
					{42,	52,	0,	false,	0},
					{43,	53,	0,	false,	0},
					{44,	54,	0,	false,	0},
					{45,	55,	0,	false,	0},
					{46,	56,	0,	false,	0},
					{47,	57,	0,	false,	0},
					{48,	58,	0,	false,	0},
					{49,	59,	0,	false,	0},
					{50,	60,	0,	false,	0},
					{51,	61,	0,	false,	0},
					{52,	62,	0,	false,	0},
					{53,	63,	0,	false,	0},
					{54,	64,	0,	false,	0},
					{55,	65,	0,	false,	0},
					{56,	66,	0,	false,	0},
					{57,	67,	0,	false,	0},
					{58,	68,	0,	false,	0},
					{59,	69,	0,	false,	0},
					{60,	70,	0,	false,	0},
					{61,	71,	0,	false,	0},
					{62,	72,	0,	false,	0},
					{63,	73,	0,	false,	0},
					{64,	74,	0,	false,	0},
					{65,	75,	0,	false,	0},
					{66,	76,	0,	false,	0},
					{67,	77,	0,	false,	0},
					{68,	78,	0,	false,	0},
					{69,	79,	0,	false,	0},
					{70,	80,	0,	false,	0},
					{71,	81,	0,	false,	0},
					{72,	82,	0,	false,	0},
					{73,	91,	0,	false,	0},
					{74,	92,	0,	false,	0},
					{90,	10,	8,	false,	0}, // Patrol point
					{106,	28,	0,	false,	0},
					{107,	27,	0,	false,	0},
					{108,	11,	4,	false,	1},
					{112,	11,	4,	true,	1},
					{116,	12,	4,	false,	1},
					{120,	12,	4,	true,	1},
					{124,	81,	0,	false,	0},
					{125,	23,	0,	false,	0},
					{126,	13,	4,	false,	1},
					{130,	13,	4,	true,	1},
					{134,	14,	4,	false,	1},
					{138,	14,	4,	true,	1},
					{142,	24,	0,	false,	0},
					{143,	25,	0,	false,	0},
					{144,	11,	4,	false,	2},
					{148,	11,	4,	true,	2},
					{152,	12,	4,	false,	2},
					{156,	12,	4,	true,	2},
					{160,	18,	0,	false,	0},
					{161,	26,	0,	false,	0},
					{162,	13,	4,	false,	2},
					{166,	13,	4,	true,	2},
					{170,	14,	4,	false,	2},
					{174,	14,	4,	true,	2},
					{178,	19,	0,	false,	0},
					{179,	22,	0,	false,	0},
					{180,	11,	4,	false,	3},
					{184,	11,	4,	true,	3},
					{188,	12,	4,	false,	3},
					{192,	12,	4,	true,	3},
					{196,	17,	0,	false,	0},
					{197,	20,	0,	false,	0},
					{198,	13,	4,	false,	3},
					{202,	13,	4,	true,	3},
					{206,	14,	4,	false,	3},
					{210,	14,	4,	true,	3},
					{214,	16,	0,	false,	0},
					{215,	21,	0,	false,	0},
					{216,	15,	4,	false,	1},
					{220,	15,	4,	true,	1},
					{224,	29,	0,	false,	0},
					{225,	30,	0,	false,	0},
					{226,	31,	0,	false,	0},
					{227,	32,	0,	false,	0},
					{234,	15,	4,	false,	2},
					{238,	15,	4,	true,	2},
					{252,	15,	4,	false,	3},
					{256,	15,	4,	true,	3}
				};
				static const WORD PUSHWALL_TILE = 98,
					PATROLPOINT = 90;
				static const unsigned int NUM_XLAT_ENTRIES = sizeof(xlat)/sizeof(xlat[0]);

				unsigned int ambushSpot = 0;
				ambushSpots.Push(0xFFFF); // Prevent uninitialized value errors. 

				for(unsigned int i = 0;i < size;++i)
				{
					if(oldplane[i] == 0)
					{
						// In case of malformed maps we need to always check this.
						if(ambushSpots[ambushSpot] == i)
							++ambushSpot;
						continue;
					}

					if(oldplane[i] == PUSHWALL_TILE)
					{
						if(ambushSpots[ambushSpot] == i)
							++ambushSpot;

						Trigger &trig = NewTrigger(i%UNIT, i/UNIT, 0);
						trig.action = Specials::Pushwall_Move;
						trig.arg[0] = 1;
						trig.playerUse = true;
						trig.isSecret = true;
						++gamestate.secrettotal;
					}
					else
					{
						bool valid = false;
						unsigned int type = NUM_XLAT_ENTRIES/2;
						unsigned int max = NUM_XLAT_ENTRIES;
						unsigned int min = 0;
						do
						{
							if(xlat[type].oldnum == oldplane[i] ||
								(xlat[type].angles && unsigned(oldplane[i] - xlat[type].oldnum) < xlat[type].angles))
							{
								valid = true;
								break;
							}

							if(xlat[type].oldnum > oldplane[i])
								max = type-1;
							else if(xlat[type].oldnum < oldplane[i])
								min = type+1;

							type = (max+min)/2;
							
						}
						while(max >= min);
						if(!valid)
							printf("Unknown old type %d @ (%d,%d)\n", oldplane[i], i%UNIT, i/UNIT);

						Thing thing;
						thing.x = ((i%UNIT)<<FRACBITS)+(FRACUNIT/2);
						thing.y = ((i/UNIT)<<FRACBITS)+(FRACUNIT/2);
						thing.z = 0;
						thing.type = xlat[type].newnum;
						if(xlat[type].angles)
						{
							if(oldplane[i] >= PATROLPOINT && oldplane[i] < PATROLPOINT+8)
								thing.angle = ((oldplane[i] - xlat[type].oldnum)*45);// + (xlat[type].oldnum == PATROLPOINT ? 0 : 180);
							else
								thing.angle = (oldplane[i] - xlat[type].oldnum)*90;
						}
						thing.ambush = ambushSpots[ambushSpot] == i;
						if(thing.ambush)
							++ambushSpot;
						thing.patrol = xlat[type].patrol;
						thing.skill[0] = thing.skill[1] = xlat[type].minskill <= 1;
						thing.skill[2] = xlat[type].minskill <= 2;
						thing.skill[3] = xlat[type].minskill <= 3;
						things.Push(thing);
					}
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
					sect.texture[Sector::Floor] = TexMan.GetFlat(pair->Key&0xFF, Sector::Floor);
					sect.texture[Sector::Ceiling] = TexMan.GetFlat(pair->Key>>8, Sector::Ceiling);
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
}
