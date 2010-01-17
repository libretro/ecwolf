#include <cstring>
#include <string>
#include <vector>
using namespace std;

#include "wl_def.h"
#include "resourcefile.h"
#include "w_wad.h"
#include "lumpremap.h"

// Some sounds in the VSwap file are multiparty so we need mean to concationate
// them.
struct FVSwapSound : public FResourceLump
{
	protected:
		struct Chunk
		{
			public:
				int	offset;
				int	length;
		};

		Chunk *chunks;
		unsigned short numChunks;

	public:
		FVSwapSound(int maxNumChunks) : FResourceLump(), numChunks(0)
		{
			if(maxNumChunks < 0)
				maxNumChunks = 0;
			chunks = new Chunk[maxNumChunks];
			LumpSize = 0;
		}
		~FVSwapSound()
		{
			delete[] chunks;
		}

		int AddChunk(int offset, int length)
		{
			LumpSize += length;

			chunks[numChunks].offset = offset;
			chunks[numChunks].length = length;
			numChunks++;
		}

		int FillCache()
		{
			Cache = new char[LumpSize];
			unsigned int pos = 0;
			for(unsigned int i = 0;i < numChunks;i++)
			{
				Owner->Reader->Seek(chunks[i].offset, SEEK_SET);
				Owner->Reader->Read(Cache+pos, chunks[i].length);
				pos += chunks[i].length;
			}
			return 1;
		}
};

class FVSwap : public FResourceFile
{
	public:
		FVSwap(const char* filename, FileReader *file) : FResourceFile(filename, file), spriteStart(0), soundStart(0), Lumps(NULL), SoundLumps(NULL), vswapFile(filename)
		{
			int lastSlash = static_cast<int> (vswapFile.find_last_of('\\')) > static_cast<int> (vswapFile.find_last_of('/')) ?
				vswapFile.find_last_of('\\') : vswapFile.find_last_of('/');
			extension = vswapFile.substr(lastSlash+7);
		}
		~FVSwap()
		{
			if(Lumps != NULL)
				delete[] Lumps;
			if(SoundLumps != NULL)
			{
				for(unsigned int i = 0;i < NumLumps-soundStart;i++)
					delete SoundLumps[i];
				delete[] SoundLumps;
			}
		}

		bool Open(bool quiet)
		{
			FileReader vswapReader;
			if(!vswapReader.Open(vswapFile.c_str()))
				return false;

			char header[6];
			vswapReader.Read(header, 6);
			int numChunks = READINT16(&header[0]);

			spriteStart = READINT16(&header[2]);
			soundStart = READINT16(&header[4]);

			Lumps = new FUncompressedLump[soundStart];


			char* data = new char[6*numChunks];
			vswapReader.Read(data, 6*numChunks);

			for(unsigned int i = 0;i < soundStart;i++)
			{
				char lumpname[9];
				sprintf(lumpname, "VSP%05d", i);

				Lumps[i].Owner = this;
				Lumps[i].LumpNameSetup(lumpname);
				Lumps[i].Namespace = i >= soundStart ? ns_sounds : (i >= spriteStart ? ns_sprites : ns_newtextures);
				Lumps[i].Position = READINT32(&data[i*4]);
				Lumps[i].LumpSize = READINT16(&data[i*2 + 4*numChunks]);
			}

			// Now for sounds we need to get the last Chunk and read the sound information.
			int soundMapOffset = READINT32(&data[(numChunks-1)*4]);
			int soundMapSize = READINT16(&data[(numChunks-1)*2 + 4*numChunks]);
			int numDigi = soundMapSize/4;
			byte* soundMap = new byte[soundMapSize];
			SoundLumps = new FVSwapSound*[numDigi];
			vswapReader.Seek(soundMapOffset, SEEK_SET);
			vswapReader.Read(soundMap, soundMapSize);
			for(unsigned int i = 0;i < numDigi;i++)
			{
				int start = READINT16(&soundMap[i*4]);
				int end = READINT16(&soundMap[i*4 + 4]);

				if(start + soundStart > numChunks - 1)
				{ // Read past end of chunks.
					numDigi = i;
					break;
				}

				char lumpname[9];
				sprintf(lumpname, "VSP%05d", i+soundStart);
				SoundLumps[i] = new FVSwapSound(end-start);
				SoundLumps[i]->Owner = this;
				SoundLumps[i]->LumpNameSetup(lumpname);
				SoundLumps[i]->Namespace = ns_sounds;
				for(unsigned int j = start;j < end && end + soundStart < numChunks;j++)
					SoundLumps[i]->AddChunk(READINT32(&data[(soundStart+j)*4]), READINT16(&data[(soundStart+j)*2 + numChunks*4]));
			}

			// Number of lumps is not the number of chunks, but the number of
			// chunks up to sounds + how many sounds are formed from the chunks.
			NumLumps = soundStart + numDigi;

			delete[] data;
			if(!quiet) Printf(", %d lumps\n", NumLumps);

			LumpRemaper::AddFile(extension.c_str(), this, LumpRemaper::VSWAP);
			return true;
		}

		FResourceLump *GetLump(int no)
		{
			if(no < soundStart)
				return &Lumps[no];
			return SoundLumps[no-soundStart];
		}

	private:
		unsigned short spriteStart;
		unsigned short soundStart;

		FUncompressedLump* Lumps;
		FVSwapSound* *SoundLumps;

		string	extension;
		string	vswapFile;
};

FResourceFile *CheckVSwap(const char *filename, FileReader *file, bool quiet)
{
	string fname(filename);
	int lastSlash = static_cast<int> (fname.find_last_of('\\')) > static_cast<int> (fname.find_last_of('/')) ?
		fname.find_last_of('\\') : fname.find_last_of('/');
	if(lastSlash != -1)
		fname = fname.substr(lastSlash+1);

	if(fname.length() > 6) // file must be vswap.something
	{
		char name[6];
		strncpy(name, fname.c_str(), 5);
		name[5] = 0;
		if(stricmp(name, "vswap") == 0)
		{
			FResourceFile *rf = new FVSwap(filename, file);
			if(rf->Open(quiet)) return rf;
			delete rf;
		}
	}
	return NULL;
}
