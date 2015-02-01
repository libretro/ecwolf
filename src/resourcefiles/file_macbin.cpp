/*
** file_macbin.cpp
**
**---------------------------------------------------------------------------
** Copyright 2013 Braden Obrzut
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

#include "resourcefile.h"
#include "tmemory.h"
#include "w_wad.h"
#include "zstring.h"

/* Sound format:
 * 2: Format (0x0001)
 * 2: Number of data foramts (Ignore, should be 0x0001)
 * {
 *   2: Data format ID (0x0005 for sampled)
 *   4: Init option (Ensure 0x00000080 bit is set for mono, 0xC0 would be stereo)
 * }
 * 2: Number of sound commands
 * {
 *   2: Command
 *   2: Parm1
 *   4: Parm2
 * }
 * Sampled header
 * [
 *   4: Data offset (0 for immediate)
 *   4: Number of bytes in sample
 *   4: Sample rate
 *   4: Start loop point
 *   4: End loop point
 *   1: Encoding type
 *   1: Base frequency (0x3C)
 * ]
 * PCM data
 */

#pragma pack(push, 1)
struct FMacBinHeader
{
public:
	BYTE version;
	BYTE filenameLength;
	char filename[63];
	char fileType[4];
	char fileCreator[4];
	BYTE finderFlags;
	BYTE pad0;
	WORD verticalPosition;
	WORD horizontalPosition;
	WORD folderID;
	BYTE protectedFlag;
	BYTE pad1;
	DWORD dataForkLength;
	DWORD resourceForkLength;
	DWORD creationDate;
	DWORD modifiedDate;
	char pad2[27];
	WORD reserved;
};

struct FResHeader
{
public:
	DWORD resourceOffset;
	DWORD mapOffset;
	DWORD resourceLength;
	DWORD mapLength;
};

struct FMapHeader
{
public:
	// FResHeader copy;
	DWORD reservedHandle;
	WORD referenceNumber;
	WORD forkAttributes;
	WORD typeListOffset;
	WORD nameListOffset;
};

struct FResType
{
public:
	char type[4];
	WORD numResources;
	WORD offset;
};

struct FResReference
{
public:
	// We need to have this structure in two parts because...
	struct RefData
	{
		WORD resID;
		WORD nameOffset;
		BYTE attributes;
		BYTE hiDataOffset; // Yay a 24-bit type!
		WORD loDataOffset;
		DWORD reservedHandle;
	} ref;

	// Stuff we're going to calculate later
	DWORD dataOffset;
	FString name;
};
#pragma pack(pop)

#define MACWOLF
#include "filereaderlzss.h"
#undef MACWOLF

struct FMacResLump : FResourceLump
{
	enum CMode
	{
		MODE_Uncompressed,
		MODE_CSound,
		MODE_Compressed
	};
	CMode Compressed;
	int CompressedSize;
	int	Position;

	int GetFileOffset() { return Position; }
	FileReader *GetReader()
	{
		if(!Compressed)
		{
			Owner->Reader->Seek(Position, SEEK_SET);
			return Owner->Reader;
		}
		return NULL;
	}
	int FillCache()
	{
		if(!Compressed)
		{
			const char * buffer = Owner->Reader->GetBuffer();

			if (buffer != NULL)
			{
				// This is an in-memory file so the cache can point directly to the file's data.
				Cache = const_cast<char*>(buffer) + Position;
				RefCount = -1;
				return -1;
			}
		}

		Owner->Reader->Seek(Position, SEEK_SET);
		Cache = new char[LumpSize];

		switch(Compressed)
		{
			case MODE_Compressed:
			{
				FileReaderLZSS<0> lzss(*Owner->Reader);
				lzss.Read(Cache, LumpSize);
				break;
			}
			case MODE_CSound:
			{
				FileReaderLZSS<1> lzss(*Owner->Reader);
				lzss.Read(Cache, LumpSize);
				break;
			}
			case MODE_Uncompressed:
				Owner->Reader->Read(Cache, LumpSize);
				break;
		}

		RefCount = 1;
		return 1;
	}
};

enum EListType
{
	LIST_Sounds,
	LIST_Walls,
	LIST_Maps,
	LIST_Songs,

	NUM_LISTS,
	LIST_None
};

static const struct BRGRConstant
{
	WORD resNum;
	const char* name;
	EListType list;
	bool compressed;
} BRGRConstants[] =
{
	{ 128, "IDLOGO", LIST_None, true },
	{ 129, "MACPLAY", LIST_None, true },
	{ 130, "MPLAYPAL", LIST_None, false },
	{ 131, "IDPAL", LIST_None, false },
	{ 132, "BLACKPAL", LIST_None, false },
	{ 133, "TITLEPIC", LIST_None, true },
	{ 134, "TITLEPAL", LIST_None, false },
	{ 135, "SOUNDLST", LIST_Sounds, false },
	{ 136, "DARKMAP", LIST_None, false },
	{ 137, "WALLLIST", LIST_Walls, false },
	{ 138, "BJFACE", LIST_None, false },
	{ 139, "INTERPIC", LIST_None, true },
	{ 140, "INTERPAL", LIST_None, false },
	{ 141, "INTERBJ", LIST_None, true },
	{ 142, "FACE320", LIST_None, true },
	{ 143, "FACE512", LIST_None, true },
	{ 144, "FACE640", LIST_None, true },
	{ 145, "PLAYPAL", LIST_None, false },
	{ 146, "MAPLIST", LIST_Maps, false },
	{ 147, "SONGLIST", LIST_Songs, false },
	{ 148, "GETPSYCH", LIST_None, true },
	{ 149, "YUMMYPIC", LIST_None, true },
	{ 150, "YUMMYPAL", LIST_None, false },
	{ 151, "FINETAN", LIST_None, false },
	{ 152, "FINESIN", LIST_None, false },
	{ 153, "SCALEATZ", LIST_None, false },
	{ 154, "VIEWANGX", LIST_None, false },
	{ 155, "XVIEWANG", LIST_None, false }
};

class FMacBin : public FResourceFile
{
	private:
		FMacResLump *Lumps;

	public:
		FMacBin(const char* filename, FileReader *file) : FResourceFile(filename, file), Lumps(NULL)
		{
		}
		~FMacBin()
		{
			delete[] Lumps;
		}

		FResourceLump *GetLump(int index) { return &Lumps[index]; }

		bool Open(bool quiet)
		{
			// Kill data types we definitely don't care about.
			static const char* const IgnoreTypes[] = {
				"ALRT", "BNDL", "cicn", "DITL", "DLOG",
				"FREF", "hfdr", "icl4", "icl8", "ICN#",
				"ics#", "ics4", "ics8", "MBAR", "MDRV",
				"mstr", "MENU", "PICT", "STR#", "TEXT",
				"vers", "WDEF", "hmnu", "SMOD", "CODE",
				"XREF", "DATA", "SIZE", "cfrg",
				NULL
			};

			FMacBinHeader header;
			FResHeader resHeader;
			FMapHeader mapHeader;
			FResType* resTypes = NULL;
			FResReference* refs = NULL;
			DWORD resourceForkOffset;
			DWORD resTypeListOffset;
			WORD numTypes;

			unsigned int BRGRref = 0xFFFFFFFF;
			FMacResLump *lists[NUM_LISTS] = { NULL };

			// Read MacBin version 0 header
			Reader->Read(&header, sizeof(FMacBinHeader));
			header.dataForkLength = BigLong(header.dataForkLength);
			header.resourceForkLength = BigLong(header.resourceForkLength);
			resourceForkOffset = sizeof(FMacBinHeader) + ((header.dataForkLength+127)&(~0x7F));
			if(header.version != 0)
				return false;

			// We only care about the resource fork so seek to it and read the header
			Reader->Seek(resourceForkOffset, SEEK_SET);
			Reader->Read(&resHeader, sizeof(FResHeader));
			resHeader.resourceOffset = BigLong(resHeader.resourceOffset);
			resHeader.mapOffset = BigLong(resHeader.mapOffset);
			resHeader.resourceLength = BigLong(resHeader.resourceLength);
			resHeader.mapLength = BigLong(resHeader.mapLength);

			// Next we need to read the resource map header
			Reader->Seek(resourceForkOffset + resHeader.mapOffset + sizeof(FResHeader), SEEK_SET);
			Reader->Read(&mapHeader, sizeof(FMapHeader));
			mapHeader.typeListOffset = BigShort(mapHeader.typeListOffset);
			mapHeader.nameListOffset = BigShort(mapHeader.nameListOffset);
			resTypeListOffset = resourceForkOffset + resHeader.mapOffset + mapHeader.typeListOffset;

			// Get the types and the number of chunks within each
			unsigned int numTotalResources = 0;
			Reader->Seek(resTypeListOffset, SEEK_SET);
			*Reader >> numTypes;
			numTypes = BigShort(numTypes)+1;
			resTypes = new FResType[numTypes];
			for(unsigned int i = 0;i < numTypes;++i)
			{
				Reader->Read(&resTypes[i], sizeof(FResType));
				resTypes[i].numResources = BigShort(resTypes[i].numResources)+1;
				resTypes[i].offset = BigShort(resTypes[i].offset);

				// Discard types we want to ignore.
				bool doIgnore = false;
				for(const char* const * ignType = IgnoreTypes;*ignType != NULL;++ignType)
				{
					if(strncmp(*ignType, resTypes[i].type, 4) == 0)
					{
						--i;
						--numTypes;
						doIgnore = true;
						break;
					}
				}
				if(doIgnore)
					continue;

				// We need to do some manipulation of these resources so take
				// note of where it is.
				if(strncmp(resTypes[i].type, "BRGR", 4) == 0)
					BRGRref = i;

				numTotalResources += resTypes[i].numResources;
			}

			// Read in the refs
			refs = new FResReference[numTotalResources];
			{
				FResReference *refPtr = refs;
				for(unsigned int i = 0;i < numTypes;++i)
				{
					Reader->Seek(resTypes[i].offset + resTypeListOffset, SEEK_SET);
					for(unsigned int j = 0;j < resTypes[i].numResources;++j)
					{
						Reader->Read(refPtr, sizeof(FResReference::RefData));
						refPtr->ref.resID = BigShort(refPtr->ref.resID);
						refPtr->ref.nameOffset = BigShort(refPtr->ref.nameOffset);
						refPtr->dataOffset = (refPtr->ref.hiDataOffset<<16)|BigShort(refPtr->ref.loDataOffset);

						++refPtr;
					}
				}
			}

			// TODO: Do we need to bother with this?
			// Get the names
			{
				DWORD nameListOffset = resourceForkOffset + resHeader.mapOffset + mapHeader.nameListOffset;
				FResReference *refPtr = refs;
				char resNamesBuffer[256];
				for(unsigned int i = 0;i < numTotalResources;++i, ++refPtr)
				{
					if(refPtr->ref.nameOffset == 0xFFFF)
						continue;

					Reader->Seek(nameListOffset + refPtr->ref.nameOffset, SEEK_SET);
					BYTE length;
					*Reader >> length;
					Reader->Read(resNamesBuffer, length);
					resNamesBuffer[length] = 0;

					refPtr->name = resNamesBuffer;
				}
			}

			// Now we can finally load the lumps, assigning temporary names
			NumLumps = numTotalResources;
			Lumps = new FMacResLump[NumLumps];
			{
				FMacResLump *lump = Lumps;
				FResReference *refPtr = refs;
				for(unsigned int i = 0;i < numTypes;++i)
				{
					char type[5];
					memcpy(type, resTypes[i].type, 4);
					type[4] = 0;

					const bool csnd = strncmp(type, "csnd", 4) == 0;
					const bool brgr = BRGRref == i;

					for(unsigned int j = 0;j < resTypes[i].numResources;++j, ++refPtr, ++lump)
					{
						char name[9];
						sprintf(name, "%s%04d", type, j);

						lump->Compressed = FMacResLump::MODE_Uncompressed;

						// Find special BRGR lumps
						if(brgr)
						{
							if(const BRGRConstant *c = BinarySearch(&BRGRConstants[0], countof(BRGRConstants), &BRGRConstant::resNum, refPtr->ref.resID))
							{
								strcpy(name, c->name);
								if(c->list != LIST_None)
									lists[c->list] = lump;
								if(c->compressed)
									lump->Compressed = FMacResLump::MODE_Compressed;
							}
						}

						DWORD length;
						lump->Position = resourceForkOffset + refPtr->dataOffset + resHeader.resourceOffset + 4;
						lump->Namespace = ns_global;
						lump->Owner = this;
						lump->LumpNameSetup(name);

						Reader->Seek(lump->Position - 4, SEEK_SET);
						*Reader >> length;
						lump->LumpSize = BigLong(length);

						if(csnd)
							lump->Compressed = FMacResLump::MODE_CSound;
						if(lump->Compressed != FMacResLump::MODE_Uncompressed)
						{
							*Reader >> length;
							lump->CompressedSize = lump->LumpSize-4;
							lump->Position += 4;
							lump->LumpSize = BigLong(length);
						}
						else if(refPtr->ref.resID >= 428) // Sprites
						{
							lump->Compressed = FMacResLump::MODE_Compressed;
							WORD csize;
							*Reader >> csize;
							lump->CompressedSize = lump->LumpSize-2;
							lump->Position += 2;
							lump->LumpSize = LittleShort(csize);
						}
					}
				}
			}

			// Scan for the wall textures and mark them as compressed with a
			// fixed size.
			if(lists[LIST_Walls] && lists[LIST_Walls]->LumpSize >= 4)
			{
				FMacResLump *lump = Lumps;
				FResReference *refPtr = refs;
				for(unsigned int i = 0;i < BRGRref;++i)
				{
					lump += resTypes[i].numResources;
					refPtr += resTypes[i].numResources;
				}

				const unsigned int numWalls = lists[LIST_Walls]->LumpSize/2 - 1;
				Reader->Seek(lists[LIST_Walls]->Position+2, SEEK_SET);
				TUniquePtr<WORD[]> walls(new WORD[numWalls]);
				Reader->Read(walls.Get(), numWalls*2);
				for(unsigned int i = 0;i < numWalls;++i)
					walls[i] = BigShort(walls[i]);

				for(unsigned int j = 0;j < resTypes[BRGRref].numResources;++j, ++refPtr, ++lump)
				{
					for(unsigned int i = 0;i < numWalls;++i)
					{
						if((walls[i]&0x3FF) == refPtr->ref.resID)
						{
							lump->Compressed = FMacResLump::MODE_Compressed;
							lump->CompressedSize = lump->LumpSize;
							lump->LumpSize = 0x4000;
							break;
						}
					}
				}
			}

			delete[] refs;
			delete[] resTypes;
			if(!quiet) Printf(", %d lumps\n", NumLumps);

			return true;
		}
};

FResourceFile *CheckMacBin(const char *filename, FileReader *file, bool quiet)
{
	if(file->GetLength() > 128)
	{
		char type[8];
		unsigned int sizes[2];
		file->Seek(65, SEEK_SET);
		file->Read(type, 8);
		file->Seek(83, SEEK_SET);
		file->Read(&sizes, 8);
		file->Seek(0, SEEK_SET);

		if((strncmp(type, "APPLWOLF", 8) == 0 || strncmp(type, "MAPSWOLF", 8) == 0) &&
			BigLong(sizes[0]) + BigLong(sizes[1]) < static_cast<unsigned>(file->GetLength()))
		{
			FResourceFile *rf = new FMacBin(filename, file);
			if(rf->Open(quiet)) return rf;
			rf->Reader = NULL; // to avoid destruction of reader
			delete rf;
		}
	}
	return NULL;
}