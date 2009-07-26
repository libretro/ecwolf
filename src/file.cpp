#include <sys/stat.h>
#include <dirent.h>

#include "file.h"

using namespace std;

File::File(string filename) : filename(filename), existing(false), directory(false)
{
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
}
