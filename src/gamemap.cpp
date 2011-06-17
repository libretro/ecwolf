/*
** gamemap.cpp
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
#include "tarray.h"
#include "w_wad.h"
#include "wl_def.h"
#include "lnspec.h"

GameMap::GameMap(const FString &map) : map(map), valid(false), zoneLinks(NULL)
{
	markerLump = Wads.GetNumForName(map);
	if(markerLump == -1)
		return;

	if(strcmp(Wads.GetLumpFullName(markerLump+1), "PLANES") == 0)
	{
		uwmf = false;
		numLumps = 1;
		ReadPlanesData();
	}
	else
	{
		// Expect UWMF formatted map.
		uwmf = true;
		if(strcmp(Wads.GetLumpFullName(markerLump+1), "TEXTMAP") != 0)
		{
			Quit("Invalid map format for %s!\n", map.GetChars());
			return;
		}
		for(int i = 2;i < Wads.GetNumLumps();i++)
		{
			if(strcmp(Wads.GetLumpFullName(markerLump+i), "ENDMAP") == 0)
			{
				valid = true;
				break;
			}
			numLumps++;
		}
		if(!valid)
		{
			Quit("ENDMAP not found for map %s!\n", map.GetChars());
			return;
		}
	}
}

GameMap::~GameMap()
{
	for(unsigned int i = 0;i < planes.Size();i++)
		delete[] planes[i].map;
	UnloadReject();
}

bool GameMap::ActivateTrigger(Trigger &trig, Trigger::Side direction, AActor *activator)
{
	MapSpot spot = GetSpot(trig.x, trig.y, trig.z);

	Specials::LineSpecialFunction func = Specials::LookupFunction(Specials::LineSpecials(trig.action));
	return func(spot, direction, activator);
}

bool GameMap::CheckLink(const Zone *zone1, const Zone *zone2, bool recurse)
{
	if(zone1 == NULL || zone2 == NULL)
		return false;

	// If we're doing a recursive check and the straight check passes use that
	bool straightCheck = *zoneLinks[zone1->index][zone2->index] > 0;
	if(!recurse || straightCheck)
		return straightCheck;

	// If doing a recursive check we need to make zone 1 the lower number that
	// way we can check the top half of the table.
	if(zone2->index < zone1->index)
	{
		const Zone *tmp = zone1;
		zone1 = zone2;
		zone2 = tmp;
	}

	for(unsigned int i = zone1->index+1;i < zonePalette.Size();++i)
	{
		if(*zoneLinks[zone1->index][i] > 0 &&
			(i == zone2->index || CheckLink(&zonePalette[i], zone2, true)))
				return true;
	}
	return false;
}

void GameMap::LinkZones(const Zone *zone1, const Zone *zone2, bool open)
{
	if(zone1 == zone2 || zone1 == NULL || zone2 == NULL)
		return;

	unsigned short &value = *zoneLinks[zone1->index][zone2->index];
	if(!open)
	{
		if(value > 0)
			--value;
	}
	else
		++value;
}

GameMap::Plane &GameMap::NewPlane()
{
	planes.Reserve(1);
	Plane &newPlane = planes[planes.Size()-1];
	newPlane.gm = this;
	newPlane.map = new Plane::Map[header.width*header.height];
	for(unsigned int i = 0;i < header.width*header.height;++i)
		newPlane.map[i].plane = &newPlane;
	return newPlane;
}

GameMap::Trigger &GameMap::NewTrigger(unsigned int x, unsigned int y, unsigned int z)
{
	MapSpot spot = GetSpot(x, y, z);
	Trigger newTrig;
	newTrig.x = x;
	newTrig.y = y;
	newTrig.z = z;
	spot->triggers.Push(newTrig);
	return spot->triggers[spot->triggers.Size()-1];
}

// Reads old format maps
void GameMap::ReadPlanesData()
{
	static const unsigned short UNIT = 64;
	enum OldPlanes { Plane_Tiles, Plane_Object, Plane_Flats, NUM_OLD_PLANES };

	valid = true;

	// Old format maps always have a tile size of 64
	header.tileSize = UNIT;

	FWadLump lump = Wads.OpenLumpNum(markerLump+1);
	lump.Seek(0, SEEK_SET);

	WORD dimensions[2];
	lump.Read(dimensions, 4);
	LittleShort(dimensions[0]);
	LittleShort(dimensions[1]);
	DWORD size = dimensions[0]*dimensions[1];
	header.width = dimensions[0];
	header.height = dimensions[1];

	char name[16];
	lump.Read(name, 16);
	header.name = name;

	Plane &mapPlane = NewPlane();
	mapPlane.depth = UNIT;

	// We need to store the spots marked for ambush since it's stored in the
	// tiles plane instead of the objects plane.
	TArray<WORD> ambushSpots;

	for(int plane = 0;plane < NUM_OLD_PLANES;++plane)
	{
		WORD oldplane[size];
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
						trig.playerUse = true;
						trig.monsterUse = true;
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
							(candidates[j]%UNIT != ambushSpots[i]%UNIT)))
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
							(candidates[j]%UNIT != altExitSpots[i]%UNIT)))
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
				static const WORD PUSHWALL_TILE = 98;

				for(unsigned int i = 0;i < size;++i)
				{
					if(oldplane[i] == PUSHWALL_TILE)
					{
						Trigger &trig = NewTrigger(i%UNIT, i/UNIT, 0);
						trig.action = Specials::Pushwall_Move;
						trig.arg[0] = 1;
						trig.playerUse = true;
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
					sect.texture[Sector::Floor] = TexMan.GetFlat(pair->Key&0xFF);
					sect.texture[Sector::Ceiling] = TexMan.GetFlat(pair->Key>>8);
				}

				// Now link the sector data to map points!
				for(unsigned int i = 0;i < size;++i)
					mapPlane.map[i].sector = &sectorPalette[flatMap[oldplane[i]]];
				break;
			}
		}
	}
	SetupReject();
}

void GameMap::SetupReject()
{
	// Might as well use the same pointer for each time x == y
	unsigned short *one = new unsigned short;
	*one = 1;

	// Set up the table
	zoneLinks = new unsigned short**[zonePalette.Size()];
	for(unsigned int i = 0;i < zonePalette.Size();++i)
	{
		zoneLinks[i] = new unsigned short*[zonePalette.Size()];
		zoneLinks[i][i] = one;
		for(unsigned int j = i+1;j < zonePalette.Size();++j)
		{
			zoneLinks[i][j] = new unsigned short;
			*zoneLinks[i][j] = (i == j);
		}
	}

	// Second half should just point to the first set.
	for(unsigned int i = 1;i < zonePalette.Size();++i)
	{
		for(unsigned int j = 0;j < i;++j)
		{
			zoneLinks[i][j] = zoneLinks[j][i];
		}
	}
}

void GameMap::UnloadReject()
{
	// Make sure there's stuff to unload.
	if(!zoneLinks)
		return;

	delete zoneLinks[0][0];
	for(unsigned int i = 0;i < zonePalette.Size();++i)
	{
		for(unsigned int j = i+1;j < zonePalette.Size();++j)
			delete zoneLinks[i][j];
		delete[] zoneLinks[i];
	}
	delete[] zoneLinks;
	zoneLinks = NULL;
}

////////////////////////////////////////////////////////////////////////////////

unsigned int GameMap::Plane::Map::GetX() const
{
	return static_cast<unsigned int>(this - plane->map)%plane->gm->GetHeader().width;
}

unsigned int GameMap::Plane::Map::GetY() const
{
	return static_cast<unsigned int>(this - plane->map)/plane->gm->GetHeader().width;
}

MapSpot GameMap::Plane::Map::GetAdjacent(MapTile::Side dir, bool opposite) const
{
	if(opposite) // Rotate the dir 180 degrees.
		dir = MapTile::Side((dir+2)%4);

	const int pos = static_cast<int>(this - plane->map);
	int x = GetX();
	int y = GetY();
	switch(dir)
	{
		case MapTile::South:
			++y;
			break;
		case MapTile::North:
			--y;
			break;
		case MapTile::West:
			--x;
			break;
		case MapTile::East:
			++x;
			break;
	}
	if(y < 0 || y >= plane->gm->GetHeader().height || x < 0 || x >= plane->gm->GetHeader().width)
		return NULL;
	return &plane->map[y*plane->gm->GetHeader().width+x];
}
