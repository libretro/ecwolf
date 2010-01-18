#ifndef __THINGDEF_H__
#define __THINGDEF_H__

#include <string>
#include <map>

#include "scanner.hpp"

class ClassDef;

#define DECLARE_NATIVE_CLASS(name, parent) \
	friend class ClassDef; \
	protected: \
		A##name(const ClassDef *classType) : A##parent(classType) {} \
	public: \
		static const ClassDef *__StaticClass;
#define IMPLEMENT_CLASS(name, parent) \
	const ClassDef *A##name::__StaticClass = ClassDef::DeclareNativeClass<A##name>(#name, A##parent::__StaticClass);
#define NATIVE_CLASS(name) A##name::__StaticClass;

class AActor
{
	friend class ClassDef;

	public:
		struct Frame
		{
			public:
				char	frame[4];
				int		duration;
				void	(*action);
				void	(*thinker);
		};

		unsigned int flags;

		static const ClassDef *__StaticClass;
	protected:
		AActor(const ClassDef *type);

		const ClassDef	*classType;
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
		ClassDef();
		~ClassDef();

		bool					IsDecendantOf(const ClassDef *parent) const;

		/**
		 * Use with IMPLEMENT_CLASS to add a natively defined class.
		 */
		template<class T>
		static const ClassDef	*DeclareNativeClass(const char* className, const ClassDef *parent);

		/**
		 * Prints the implemented classes in a tree.  This is not designed to 
		 * be fast since it's debug information more than anything.
		 */
		static void				DumpClasses();

		static const ClassDef	*FindClass(const std::string &className);
		static void				LoadActors();
		static void				UnloadActors();

	protected:
		static void	ParseActor(Scanner &sc);
		static void	ParseDecorateLump(int lumpNum);

		static std::map<std::string, ClassDef *>	classTable;

		std::string		name;
		const ClassDef	*parent;

		AActor			*defaultInstance;
};

#endif /* __THINGDEF_H__ */
