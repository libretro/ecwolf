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
extern "C" int patch(const unsigned char* data, unsigned int dataSize, const char* filename);

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
	PatchData		patchData;
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
			{4239209784, 1024, "vgadict.wl1", P(vgadict11sw)},
			{2001789396, 27448, "gamemaps.wl1", P(gamemaps11sw)},
			{1935056717, 132613, "audiot.wl1", P(audiot11sw)},
			{3203508363, 97605, "wolf3d.exe", P(wolf3d11sw)},
			{3551669308, 462, "vgahead.wl1", P(vgahead11sw)},
			{542266284, 402, "maphead.wl1", P(maphead11sw)},
			{3146117708, 742912, "vswap.wl1", P(vswap11sw)},
			{2629083114, 296826, "vgagraph.wl1", P(vgagraph11sw)},
			{1376992708, 1156, "audiohed.wl1", NULLP},
		},
		"Wolfenstein 3D Shareware v1.1"
	},
	{
		9,
		{
			{2006621000, 1024, "vgadict.wl1", P(vgadict12sw)},
			{391230369, 27450, "gamemaps.wl1", P(gamemaps12sw)},
			{2277491609, 132613, "audiot.wl1", NULLP},
			{1036300494, 97676, "wolf3d.exe", P(wolf3d12sw)},
			{3668255810, 462, "vgahead.wl1", P(vgahead12sw)},
			{256482848, 402, "maphead.wl1", P(maphead12sw)},
			{1887524789, 742912, "vswap.wl1", P(vswap12sw)},
			{4155133539, 296813, "vgagraph.wl1", P(vgagraph12sw)},
			{1376992708, 1156, "audiohed.wl1", NULLP},
		},
		"Wolfenstein 3D Shareware v1.2"
	},
	{
		9,
		{
			{898283639, 1024, "vgadict.wl1", NULLP},
			{3428045633, 27425, "gamemaps.wl1", NULLP},
			{2277491609, 132613, "audiot.wl1", NULLP},
			{557669750, 109959, "wolf3d.exe", NULLP},
			{2785712368, 471, "vgahead.wl1", NULLP},
			{143619409, 402, "maphead.wl1", NULLP},
			{2247076949, 742912, "vswap.wl1", NULLP},
			{1953343984, 326568, "vgagraph.wl1", NULLP},
			{1376992708, 1156, "audiohed.wl1", NULLP},
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
		struct stat *fileInfo = new struct stat;
		for(int f = 0;f < dataSets[i].numFiles;f++)
		{
			if(stat(dataSets[i].fileChecksum[f].filename, fileInfo) != 0 || fileInfo->st_size != dataSets[i].fileChecksum[f].size)
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
			if(dataSets[i].fileChecksum[f].patchData.data == NULL)
				continue;
			cout << "\t->" << dataSets[i].fileChecksum[f].filename << "\n";
			patch(dataSets[i].fileChecksum[f].patchData.data, dataSets[i].fileChecksum[f].patchData.size, dataSets[i].fileChecksum[f].filename);
		}
	}
	return 0;
}
