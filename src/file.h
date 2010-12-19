#ifndef __FILE_H__
#define __FILE_H__

#include "tarray.h"
#include "zstring.h"

class File
{
	public:
		File(const FString &filename);
		~File() {}

		bool					exists() const { return existing; }
		const TArray<FString>	&getFileList() const { return files; }
		bool					isDirectory() const { return directory; }
		bool					isFile() const { return !directory; }

	protected:
		FString	filename;

		TArray<FString>	files;
		bool			directory;
		bool			existing;
};

#endif /* __FILE_H__ */
