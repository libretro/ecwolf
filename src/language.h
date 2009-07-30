#ifndef __LANGUAGE_H__
#define __LANGUAGE_H__

#include <string>
#include <map>
#include <list>

class Language
{
	public:
		void		SetupStrings(const char* language=NULL);

		const char*	operator[] (const char* index) const;

	protected:
		void		ReadLump(int lump, const char* language);

	private:
		std::map<std::string, std::string>	strings;
};

extern Language language;

#endif /* __LANGUAGE_H__ */
