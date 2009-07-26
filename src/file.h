#ifndef __FILE_H__
#define __FILE_H__

#include <string>
#include <deque>

class File
{
	public:
		File(std::string filename);
		~File() {}

		bool							exists() const { return existing; }
		const std::deque<std::string>	&getFileList() const { return files; }
		bool							isDirectory() const { return directory; }
		bool							isFile() const { return !directory; }

	protected:
		std::string	filename;

		std::deque<std::string>	files;
		bool					directory;
		bool					existing;
};

#endif /* __FILE_H__ */
