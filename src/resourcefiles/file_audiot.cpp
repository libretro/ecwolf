#include <cstring>
#include <string>
using namespace std;

#include "wl_def.h"
#include "resourcefile.h"
#include "w_wad.h"
#include "lumpremap.h"

class FAudiot : public FUncompressedFile
{
	public:
		FAudiot(const char* filename, FileReader *file) : FUncompressedFile(filename, file), audiotFile(filename)
		{
			string path(filename);
			int lastSlash = static_cast<int> (path.find_last_of('\\')) > static_cast<int> (path.find_last_of('/')) ?
				path.find_last_of('\\') : path.find_last_of('/');
			extension = path.substr(lastSlash+8);
			path = path.substr(0, lastSlash+1);

			audiohedFile = path + "audiohed." + extension;
		}

		bool Open(bool quiet)
		{
			FileReader audiohedReader;
			if(!audiohedReader.Open(audiohedFile.c_str()))
				return false;
			audiohedReader.Seek(0, SEEK_END);
			NumLumps = audiohedReader.Tell()/4;
			audiohedReader.Seek(0, SEEK_SET);
			Lumps = new FUncompressedLump[NumLumps];
			// The vgahead has 24-bit ints.
			DWORD* positions = new DWORD[NumLumps];
			DWORD* sizes = new DWORD[NumLumps];
			audiohedReader.Read(positions, NumLumps*4);

			// Since the music is at the end and is easy to identify we'll look for the end of the music.
			// HACK: I'm using the fact that the IMFs have the string IMF near
			//       the end of the files to identify them.
			int musicStart = -1;
			for(int i = NumLumps-1;i > 0;i--)
			{
				if(i != 0)
					sizes[i-1] = positions[i] - positions[i-1];
				if(i != NumLumps-1 && musicStart == -1) // See if we can find the first music lump.
				{
					if(sizes[i] < 64)
					{
						musicStart = i+1;
						continue;
					}
					Reader->Seek(positions[i]+sizes[i]-64, SEEK_SET);
					char data[64];
					Reader->Read(data, 64);
					bool isMusic = false;
					for(unsigned int j = 0;j+3 < 64;j++)
					{
						if(strnicmp(&data[j], "IMF", 3) == 0)
						{
							isMusic = true;
							break;
						}
					}
					if(!isMusic)
					{
						musicStart = i+1;
						continue;
					}
				}
			}
			for(unsigned int i = 0;i < NumLumps;i++)
			{
				// Give the lump a temporary name.
				char lumpname[9];
				sprintf(lumpname, "AUD%05d", i);
				Lumps[i].Owner = this;
				Lumps[i].LumpNameSetup(lumpname);
				Lumps[i].Namespace = i >= musicStart ? ns_music : ns_sounds;
				Lumps[i].Position = positions[i];
				Lumps[i].LumpSize = sizes[i];
			}
			delete[] positions;
			delete[] sizes;
			if(!quiet) Printf(", %d lumps\n", NumLumps);

			LumpRemaper::AddFile(extension.c_str(), this, LumpRemaper::AUDIOT);
			return true;
		}

	private:
		string		extension;
		string		audiotFile;
		string		audiohedFile;
};

FResourceFile *CheckAudiot(const char *filename, FileReader *file, bool quiet)
{
	string fname(filename);
	int lastSlash = static_cast<int> (fname.find_last_of('\\')) > static_cast<int> (fname.find_last_of('/')) ?
		fname.find_last_of('\\') : fname.find_last_of('/');
	if(lastSlash != -1)
		fname = fname.substr(lastSlash+1);

	if(fname.length() > 7) // file must be audiot.something
	{
		char name[7];
		strncpy(name, fname.c_str(), 6);
		name[6] = 0;
		if(stricmp(name, "audiot") == 0)
		{
			FResourceFile *rf = new FAudiot(filename, file);
			if(rf->Open(quiet)) return rf;
			delete rf;
		}
	}
	return NULL;
}
