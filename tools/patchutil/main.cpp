#include <cstring>
#include <cmath>
#include <list>
#include <iostream>
#include <fstream>

#include <zlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>

#include "patchdata.cpp"

using namespace std;

extern "C" int diff(int argc, char* argv[]);
extern "C" int patch(const unsigned char* data, unsigned int dataSize, const char* filename, const char* newfilename);

struct PatchData
{
	const unsigned char*	data;
	unsigned int			size;
};
#define NULLP {NULL, 0}

struct FileChecksum
{
	unsigned int	crc;
	int				size;
	const char*		filename;
	const char*		newFilename;
	PatchData		patchData;
	bool			optional;
};

struct OldDataSet
{
	int				numFiles;
	FileChecksum	fileChecksum[9];
	const char*		setName;
} dataSets[] =
{
	{
		9,
		{
			{4096914596u, 295394, "vgagraph.wl1", NULL, P(vgagraph10sw), false},
			{4079205602u, 56618, "maptemp.wl1", "gamemaps.wl1", P(maptemp10sw), false},
			{944411798u, 988, "audiohed.wl1", NULL, P(audiohed10sw), false},
			{3814678634u, 402, "maphead.wl1", NULL, P(maphead10sw), false},
			{1818995766u, 94379, "wolf3d.exe", NULL, P(wolf3d10sw), true},
			{932624354u, 1024, "vgadict.wl1", NULL, P(vgadict10sw), false},
			{4185907760u, 707072, "vswap.wl1", NULL, P(vswap10sw), false},
			{4132450727u, 130696, "audiot.wl1", NULL, P(audiot10sw), false},
			{323179087u, 447, "vgahead.wl1", NULL, P(vgahead10sw), false},
		},
		"Wolfenstein 3D Shareware v1.0"
	},
	{
		9,
		{
			{4239209784u, 1024, "vgadict.wl1", NULL, P(vgadict11sw), false},
			{2001789396u, 27448, "gamemaps.wl1", NULL, P(gamemaps11sw), false},
			{1935056717u, 132613, "audiot.wl1", NULL, P(audiot11sw), false},
			{3203508363u, 97605, "wolf3d.exe", NULL, P(wolf3d11sw), true},
			{3551669308u, 462, "vgahead.wl1", NULL, P(vgahead11sw), false},
			{542266284u, 402, "maphead.wl1", NULL, P(maphead11sw), false},
			{3146117708u, 742912, "vswap.wl1", NULL, P(vswap11sw), false},
			{2629083114u, 296826, "vgagraph.wl1", NULL, P(vgagraph11sw), false},
			{1376992708u, 1156, "audiohed.wl1", NULL, NULLP, false},
		},
		"Wolfenstein 3D Shareware v1.1"
	},
	{
		9,
		{
			{2006621000u, 1024, "vgadict.wl1", NULL, P(vgadict12sw), false},
			{391230369u, 27450, "gamemaps.wl1", NULL, P(gamemaps12sw), false},
			{2277491609u, 132613, "audiot.wl1", NULL, NULLP, false},
			{1036300494u, 97676, "wolf3d.exe", NULL, P(wolf3d12sw), true},
			{3668255810u, 462, "vgahead.wl1", NULL, P(vgahead12sw), false},
			{256482848u, 402, "maphead.wl1", NULL, P(maphead12sw), false},
			{1887524789u, 742912, "vswap.wl1", NULL, P(vswap12sw), false},
			{4155133539u, 296813, "vgagraph.wl1", NULL, P(vgagraph12sw), false},
			{1376992708u, 1156, "audiohed.wl1", NULL, NULLP, false},
		},
		"Wolfenstein 3D Shareware v1.2"
	},
	{
		9,
		{
			{898283639u, 1024, "vgadict.wl1", NULL, NULLP, false},
			{3428045633u, 27425, "gamemaps.wl1", NULL, NULLP, false},
			{2277491609u, 132613, "audiot.wl1", NULL, NULLP, false},
			{557669750u, 109959, "wolf3d.exe", NULL, NULLP, true},
			{2785712368u, 471, "vgahead.wl1", NULL, NULLP, false},
			{143619409u, 402, "maphead.wl1", NULL, NULLP, false},
			{2247076949u, 742912, "vswap.wl1", NULL, NULLP, false},
			{1953343984u, 326568, "vgagraph.wl1", NULL, NULLP, false},
			{1376992708u, 1156, "audiohed.wl1", NULL, NULLP, false},
		},
		"Wolfenstein 3D Shareware v1.4"
	}
};

unsigned int calcCrc32(const char* file, unsigned int size)
{
	fstream in(file, ios_base::in|ios_base::binary);
	char* fileData = new char[size];
	in.read(fileData, size);
	in.close();
	unsigned int crc = crc32(0, Z_NULL, 0);
	crc = crc32(crc, (Bytef*)fileData, size);
	in.close();
	delete[] fileData;
	return crc;
}

int main(int argc, char* argv[])
{
#ifdef LEARN
	if(argc == 3 && strcmp(argv[1], "learn") == 0)
	{
		DIR *oldDir = opendir("old");
		if(oldDir == NULL)
		{
			cout << "Place original files in old/ and new files in new/\n";
			return 0;
		}

		fstream out("patch.txt", ios_base::out|ios_base::binary);
		list<string> fileList;
		out << "{\n";
		dirent *file;
		int fileCount = 0;
		while(file = readdir(oldDir))
		{
			if(file->d_name[0] == '.')
				continue;

			fileCount++;
		}
		closedir(oldDir);
		out << "\t" << fileCount << ",\n\t{\n";
		oldDir = opendir("old");
		while(file = readdir(oldDir))
		{
			if(file->d_name[0] == '.')
				continue;

			string thisFile = string("old/") + file->d_name;
			fileList.push_back(file->d_name);
			struct stat *fileInfo = new struct stat;
			stat(thisFile.c_str(), fileInfo);
			unsigned int crc = calcCrc32(thisFile.c_str(), fileInfo->st_size);
			out << "\t\t{" << crc << ", " << (int) fileInfo->st_size << ", \"" << file->d_name << "\"},\n";
			delete fileInfo;
		}
		out << "\t},\n\t-1,\n\t\"" << argv[2] << "\"\n}\n";

		closedir(oldDir);

		const char* dargs[4] = {"patchutil", "", "", "tmp"};
		for(list<string>::const_iterator iter = fileList.begin();iter != fileList.end();iter++)
		{
			out << "//" << *iter << "\n";
			string oldFile = string("old/") + *iter;
			string newFile = string("new/") + *iter;
			dargs[1] = oldFile.c_str();
			dargs[2] = newFile.c_str();
			diff(4, (char**)dargs);

			fstream in("tmp", ios_base::in|ios_base::binary);
			char data[4096];
			short position = 0;
			while(!in.eof())
			{
				in.read(data, 4096);
				for(int i = 0;i < in.gcount();i++)
				{
					short initPosition = position;
					position += 2 + (data[i] == 0 ? 0 : static_cast<int>(log10((unsigned char)(data[i]))));
					if(position > 80)
					{
						out << "\n";
						position -= initPosition;
					}
					out << (unsigned int)(unsigned char)data[i] << ",";
				}
			}
			in.close();
			out << "\n";
		}
		out.close();
		return 0;
	}
#endif

	for(int i = 0;i < countof(dataSets);i++)
	{
		bool identified = true;
		bool noOptional = false;
		struct stat *fileInfo = new struct stat;
		for(int f = 0;f < dataSets[i].numFiles;f++)
		{
			if(stat(dataSets[i].fileChecksum[f].filename, fileInfo) != 0)
			{
				if(dataSets[i].fileChecksum[f].optional)
				{
					noOptional = true;
					continue;
				}
				identified = false;
				break;
			}
			if(fileInfo->st_size != dataSets[i].fileChecksum[f].size)
			{
				identified = false;
				break;
			}
			unsigned int crc = calcCrc32(dataSets[i].fileChecksum[f].filename, fileInfo->st_size);
			if(crc != dataSets[i].fileChecksum[f].crc)
			{
				identified = false;
				break;
			}
		}
		delete fileInfo;

		if(!identified)
			continue;

		cout << "Patching: " << dataSets[i].setName << "\n";
		for(int f = 0;f < dataSets[i].numFiles;f++)
		{
			if(dataSets[i].fileChecksum[f].patchData.data == NULL || (noOptional && dataSets[i].fileChecksum[f].optional))
				continue;
			cout << "\t->" << dataSets[i].fileChecksum[f].filename << "\n";
			patch(dataSets[i].fileChecksum[f].patchData.data, dataSets[i].fileChecksum[f].patchData.size, dataSets[i].fileChecksum[f].filename, dataSets[i].fileChecksum[f].newFilename ? dataSets[i].fileChecksum[f].newFilename : dataSets[i].fileChecksum[f].filename);
		}
	}
	return 0;
}
