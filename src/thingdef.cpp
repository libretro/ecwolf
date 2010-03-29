#include <string>

#include "scanner.h"
#include "w_wad.h"
#include "wl_def.h"
#include "thingdef.h"

using namespace std;

#define DEFINE_FLAG(prefix, flag, type, variable) { prefix##_##flag, #flag, (int)(size_t)&((type*)1)->variable - 1 }
const FlagDef flags[] =
{
	DEFINE_FLAG(FL, AMBUSH, AActor, flags),
	DEFINE_FLAG(FL, ATTACKMODE, AActor, flags),
	DEFINE_FLAG(FL, BONUS, AActor, flags),
	DEFINE_FLAG(FL, FIRSTATTACK, AActor, flags),
	DEFINE_FLAG(FL, FULLBRIGHT, AActor, flags),
	DEFINE_FLAG(FL, SHOOTABLE, AActor, flags),
	DEFINE_FLAG(FL, NEVERMARK, AActor, flags),
	DEFINE_FLAG(FL, NONMARK, AActor, flags),
	DEFINE_FLAG(FL, VISABLE, AActor, flags),
};

map<string, ClassDef *> ClassDef::classTable;

ClassDef::ClassDef()
{
	defaultInstance = new AActor(this);
}

ClassDef::~ClassDef()
{
	delete defaultInstance;
}

const ClassDef *ClassDef::FindClass(const std::string &className)
{
	map<string, ClassDef *>::const_iterator iter = classTable.find(className);
	if(iter == classTable.end())
		return NULL;
	return iter->second;
}

bool ClassDef::IsDecendantOf(const ClassDef *parent) const
{
	const ClassDef *currentParent = parent;
	while(currentParent != NULL)
	{
		if(currentParent == parent)
			return true;
		currentParent = currentParent->parent;
	}
	return false;
}

void ClassDef::LoadActors()
{
	printf("ClassDef: Loading actor definitions.\n");
	atexit(&ClassDef::UnloadActors);

	int lastLump = 0;
	int lump = 0;
	while((lump = Wads.FindLump("DECORATE", &lastLump)) != -1)
	{
		ParseDecorateLump(lump);
	}
	DumpClasses();
}

void ClassDef::ParseActor(Scanner &sc)
{
	// Read the header
	sc.MustGetToken(TK_Identifier);
	ClassDef *newClass = classTable[sc.str];
	bool previouslyDefined = newClass != NULL;
	if(!previouslyDefined)
	{
		newClass = new ClassDef();
		classTable[sc.str] = newClass;
	}
	bool native = false;
	newClass->name = sc.str;
	if(sc.CheckToken(':'))
	{
		sc.MustGetToken(TK_Identifier);
		newClass->parent = FindClass(sc.str);
		if(newClass->parent == NULL)
			sc.ScriptError("Could not find parent actor '%s'\n", sc.str.c_str());
	}
	else if(!previouslyDefined) // If no class was specified to inherit from, inherit from AActor, but not for AActor.
		newClass->parent = NATIVE_CLASS(Actor)
	if(sc.CheckToken(TK_Identifier))
	{
		if(stricmp(sc.str.c_str(), "native") == 0)
			native = true;
		else
			sc.ScriptError("Unknown keyword '%s'.\n", sc.str.c_str());
	}
	if(previouslyDefined && !native)
		sc.ScriptError("Actor '%s' already defined.\n", newClass->name.c_str());
	if(!native) // Initialize the default instance to the nearest native class.
	{
		delete newClass->defaultInstance;
		newClass->defaultInstance = newClass->parent->defaultInstance->__NewNativeInstance(newClass);
	}

	sc.MustGetToken('{');
	while(!sc.CheckToken('}'))
	{
		if(sc.CheckToken('+') || sc.CheckToken('-'))
		{
			bool set = sc.token == '+';
			string flagName;
			sc.MustGetToken(TK_Identifier);
			flagName = sc.str;
			if(sc.CheckToken('.'))
			{
				sc.MustGetToken(TK_Identifier);
				flagName += string(".") + sc.str;
			}
			if(!SetFlag(newClass, flagName.c_str(), set))
				printf("Warning: Unkown flag '%s' for actor '%s'.\n", flagName.c_str(), newClass->name.c_str());
		}
		else
		{
			sc.MustGetToken(TK_Identifier);
			if(stricmp(sc.str.c_str(), "states") == 0)
			{
				sc.MustGetToken('{');
				sc.MustGetToken(TK_Identifier); // We should already have grabbed the identifier in all other cases.
				while(sc.token != '}')
				{
					if(sc.CheckToken(':'))
					{
						// New state
						if(sc.CheckToken('}'))
							break;
						sc.MustGetToken(TK_Identifier);
					}
					else
					{
						if(sc.str.length() != 4)
							sc.ScriptError("Frame must be exactly 4 characters long.");
						if(stricmp(sc.str.c_str(), "goto") == 0)
						{
							sc.MustGetToken(TK_Identifier);
							if(sc.CheckToken('}'))
								break;
							sc.MustGetToken(TK_Identifier);
							continue;
						}
						else if(stricmp(sc.str.c_str(), "wait") == 0)
						{
							if(sc.CheckToken('}'))
								break;
							sc.MustGetToken(TK_Identifier);
							continue;
						}
						else if(stricmp(sc.str.c_str(), "loop") == 0)
						{
							if(sc.CheckToken('}'))
								break;
							sc.MustGetToken(TK_Identifier);
							continue;
						}
						else if(stricmp(sc.str.c_str(), "stop") == 0)
						{
							if(sc.CheckToken('}'))
								break;
							sc.MustGetToken(TK_Identifier);
							continue;
						}
						sc.MustGetToken(TK_Identifier);
						sc.CheckToken('-');
						sc.MustGetToken(TK_IntConst);
						if(sc.CheckToken('}'))
							break;
						for(int func = 0;func < 3;func++)
						{
							sc.MustGetToken(TK_Identifier);
							if(sc.str.length() == 4 || func == 2) // Assuming we won't have any 4 character length function names.
								break; // Second condition is just a dummy so we satisfy our need for an indentifer before the next loop.
							if(sc.CheckToken('}'))
								break;
							if(sc.CheckToken('('))
								sc.MustGetToken(')');
							if(sc.CheckToken('}'))
								break;
						}
					}
				}
			}
			else
			{
				string propertyName = sc.str;
				if(sc.CheckToken('.'))
				{
					sc.MustGetToken(TK_Identifier);
					propertyName = string(".") + sc.str;
				}
				do
				{
					sc.GetNextToken();
				}
				while(sc.CheckToken(','));
				printf("Warning: Unkown property '%s' for actor '%s'.\n", propertyName.c_str(), newClass->name.c_str());
			}
		}
	}
}

void ClassDef::ParseDecorateLump(int lumpNum)
{
	FWadLump lump = Wads.OpenLumpNum(lumpNum);
	char* data = new char[Wads.LumpLength(lumpNum)];
	lump.Read(data, Wads.LumpLength(lumpNum));
	Scanner sc(data, Wads.LumpLength(lumpNum));

	while(sc.TokensLeft())
	{
		sc.MustGetToken(TK_Identifier);
		if(stricmp(sc.str.c_str(), "actor") == 0)
		{
			ParseActor(sc);
		}
		else
			sc.ScriptError("Unknown thing section '%s'.", sc.str.c_str());
	}
}

bool ClassDef::SetFlag(ClassDef *newClass, const char* flagName, bool set)
{
	int min = 0;
	int max = sizeof(flags)/sizeof(FlagDef) - 1;
	while(min <= max)
	{
		int mid = (min+max)/2;
		int ret = stricmp(flags[mid].name, flagName);
		if(ret == 0)
		{
			if(set)
				*reinterpret_cast<flagstype_t *>(newClass->defaultInstance + flags[mid].varOffset) |= flags[mid].value;
			else
				*reinterpret_cast<flagstype_t *>(newClass->defaultInstance + flags[mid].varOffset) &= ~flags[mid].value;
			return true;
		}
		else if(ret > 0)
			min = mid+1;
		else
			max = mid-1;
	}
	return false;
}

void ClassDef::UnloadActors()
{
	for(map<string, ClassDef *>::iterator iter = classTable.begin();iter != classTable.end();iter++)
	{
		delete iter->second;
	}
}

template<class T>
const ClassDef *ClassDef::DeclareNativeClass(const char* className, const ClassDef *parent)
{
	ClassDef *definition = classTable[className];
	if(definition == NULL)
	{
		definition = new ClassDef();
		classTable[className] = definition;
	}
	definition->name = className;
	definition->parent = parent;
	delete definition->defaultInstance;
	definition->defaultInstance = new T(definition);
	return definition;
}

////////////////////////////////////////////////////////////////////////////////

const ClassDef *AActor::__StaticClass = ClassDef::DeclareNativeClass<AActor>("Actor", NULL);

AActor::AActor(const ClassDef *type) : classType(type)
{
}


class AWeapon : public AActor
{
	DECLARE_NATIVE_CLASS(Weapon, Actor)
};
IMPLEMENT_CLASS(Weapon, Actor)

////////////////////////////////////////////////////////////////////////////////

void ClassDef::DumpClasses()
{
	struct ClassTree
	{
		public:
			ClassTree(const ClassDef *classType) : child(NULL), next(NULL), thisClass(classType)
			{
				ClassTree **nextChild = &child;
				for(map<string, ClassDef *>::iterator iter = classTable.begin();iter != classTable.end();iter++)
				{
					if(iter->second->parent == classType)
					{
						*nextChild = new ClassTree(iter->second);
						nextChild = &(*nextChild)->next;
					}
				}
			}

			~ClassTree()
			{
				if(child != NULL)
					delete child;
				if(next != NULL)
					delete next;
			}

			void Dump(int spacing)
			{
				string myName = string(spacing*2, ' ') + thisClass->name;
				printf("%s\n", myName.c_str());
				if(child != NULL)
					child->Dump(spacing+1);
				if(next != NULL)
					next->Dump(spacing);
			}

			ClassTree		*child;
			ClassTree		*next;
			const ClassDef	*thisClass;
	};

	ClassTree root(FindClass("Actor"));
	root.Dump(0);
}
