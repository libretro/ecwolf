#ifdef WINDOWS
#include <windows.h>
#else
#include <sys/stat.h>
#include <dirent.h>
#endif

#include <cstdio>
#include "file.h"

using namespace std;

File::File(string filename) : filename(filename), existing(false), directory(false)
{
#ifdef WINDOWS
	/* Windows, why must you be such a pain?
	 * Why must you require me to add a '*' to my filename?
	 * Why must you design your API around the do...while loop?
	 * Why???
	 */
	DWORD fAttributes = GetFileAttributes(filename.c_str());
	if(fAttributes != INVALID_FILE_ATTRIBUTES)
	{
		existing = true;
		if(fAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			directory = true;
			WIN32_FIND_DATA fdata;
			HANDLE hnd = INVALID_HANDLE_VALUE;
			hnd = FindFirstFile((filename + "\\*").c_str(), &fdata);
			if(hnd != INVALID_HANDLE_VALUE)
			{
				do
				{
					files.push_back(fdata.cFileName);
				}
				while(FindNextFile(hnd, &fdata) != 0);
			}
		}
	}
#else
	struct stat statRet;
	if(stat(filename.c_str(), &statRet) == 0)
		existing = true;

	if(existing && (statRet.st_mode & S_IFDIR))
	{
		directory = true;

		// Populate a base list.
		DIR *direct = opendir(filename.c_str());
		if(direct != NULL)
		{
			dirent *file = NULL;
			while((file = readdir(direct)) != NULL)
				files.push_back(file->d_name);
		}
	}
#endif
}
