#ifndef __THINGDEF_H__
#define __THINGDEF_H__

#include <string>
#include <map>

#include "scanner.h"
#include "wl_def.h"

class ClassDef;

#define DECLARE_NATIVE_CLASS(name, parent) \
	friend class ClassDef; \
	protected: \
		A##name(const ClassDef *classType) : A##parent(classType) {} \
		virtual AActor *__NewNativeInstance(const ClassDef *classType) { return new A##name(classType); } \
	public: \
		static const ClassDef *__StaticClass;
#define IMPLEMENT_CLASS(name, parent) \
	const ClassDef *A##name::__StaticClass = ClassDef::DeclareNativeClass<A##name>(#name, A##parent::__StaticClass);
#define NATIVE_CLASS(name) A##name::__StaticClass;

typedef uint32_t flagstype_t;

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

		// Basic properties from objtype
		flagstype_t flags;

		int32_t	distance; // if negative, wait for that door to open
		dirtype	dir;

		fixed	x, y;
		word	tilex, tiley;
		byte	areanumber;

		short	angle;
		short	health;
		int32_t	speed;

		static const ClassDef *__StaticClass;
	protected:
		AActor(const ClassDef *type);
		virtual AActor *__NewNativeInstance(const ClassDef *classType) { return new AActor(classType); }

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
		static bool	SetFlag(ClassDef *newClass, const char* flagName, bool set);

		static std::map<std::string, ClassDef *>	classTable;

		std::string		name;
		const ClassDef	*parent;

		AActor			*defaultInstance;
};

#endif /* __THINGDEF_H__ */
