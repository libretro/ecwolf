#ifndef __THINGDEF_H__
#define __THINGDEF_H__

#include <string>

#include "scanner.hpp"

class ClassDef;

class Actor
{
	public:
		unsigned int flags;
};

struct FlagDef
{
	public:
		const unsigned int	value;
		const char* const	name;
		const int			varOffset;
};

class ClassDef
{
	public:
		struct Frame
		{
			public:
				char	frame[4];
				int		duration;
				void	(*action);
				void	(*thinker);
		};

		static void	LoadActors();
	protected:
		static void ParseActor(Scanner &sc);
		static void ParseDecorateLump(int lumpNum);

		std::string	name;
};

#endif /* __THINGDEF_H__ */
