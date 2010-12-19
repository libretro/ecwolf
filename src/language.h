#ifndef __LANGUAGE_H__
#define __LANGUAGE_H__

#include "tarray.h"

class FName;
class FString;

class Language
{
	public:
		void		SetupStrings(const char* language=NULL);

		const char*	operator[] (const char* index) const;

	protected:
		void		ReadLump(int lump, const char* language);

	private:
		TMap<FName, FString>	strings;
};

extern Language language;

#endif /* __LANGUAGE_H__ */
