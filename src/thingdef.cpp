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
	DEFINE_FLAG(FL, NEVERMARK, AActor, flags),
	DEFINE_FLAG(FL, NONMARK, AActor, flags),
	DEFINE_FLAG(FL, SHOOTABLE, AActor, flags),
	DEFINE_FLAG(FL, VISABLE, AActor, flags),
};
extern const PropDef properties[NUM_PROPERTIES];

map<string, ClassDef *> ClassDef::classTable;

ClassDef::ClassDef() : defaultInstance(NULL)
{
	defaultInstance = new AActor(this);
}

ClassDef::~ClassDef()
{
	for(deque<Frame *>::iterator iter = frameList.begin();iter != frameList.end();iter++)
		delete *iter;
	delete defaultInstance;
}

AActor *ClassDef::CreateInstance() const
{
	return defaultInstance->__NewNativeInstance(this);
}

const ClassDef *ClassDef::FindClass(const std::string &className)
{
	map<string, ClassDef *>::const_iterator iter = classTable.find(className);
	if(iter == classTable.end())
		return NULL;
	return iter->second;
}

Frame *ClassDef::FindState(const char* stateName) const
{
	Frame *ret = NULL;
	map<string, Frame *>::const_iterator iter = stateList.find(stateName);
	if(iter != stateList.end())
		ret = iter->second;
	return ret;
}

struct Goto
{
	public:
		Frame	*frame;
		string	label;
};
void ClassDef::InstallStates(deque<StateDefinition> &stateDefs)
{
	// First populate the state index with that of the parent.
	if(parent != NULL)
	{
		for(map<string, Frame *>::const_iterator iter = parent->stateList.begin();iter != parent->stateList.end();iter++)
			stateList[iter->first] = iter->second;
	}

	// We need to resolve gotos after we install the states.
	deque<Goto> gotos;

	string thisLabel;
	Frame *prevFrame = NULL;
	Frame *loopPoint = NULL;
	Frame *thisFrame = NULL;
	for(deque<StateDefinition>::const_iterator iter = stateDefs.begin();iter != stateDefs.end();iter++)
	{
		const StateDefinition &thisStateDef = *iter;

		// Special case, `Label: stop`, remove state.  Hmm... I wonder if ZDoom handles fall throughs on this.
		if(!thisStateDef.label.empty() && thisStateDef.sprite[0] == 0 && thisStateDef.nextType == StateDefinition::STOP)
		{
			stateList.erase(stateList.find(thisStateDef.label));
			continue;
		}

		for(int i = 0;i < thisStateDef.frames.length();i++)
		{
			thisFrame = new Frame();
			if(i == 0 && !thisStateDef.label.empty())
			{
				stateList[thisStateDef.label] = thisFrame;
				loopPoint = thisFrame;
			}
			memcpy(thisFrame->sprite, thisStateDef.sprite, 4);
			thisFrame->frame = thisStateDef.frames[i];
			thisFrame->duration = thisStateDef.duration;
			thisFrame->action = NULL;
			thisFrame->thinker = NULL;
			thisFrame->next = NULL;
			if(i == thisStateDef.frames.length()-1) // Handle nextType
			{
				if(thisStateDef.nextType == StateDefinition::WAIT)
					thisFrame->next = thisFrame;
				else if(thisStateDef.nextType == StateDefinition::LOOP)
					thisFrame->next = loopPoint;
				// Add to goto list
				else if(thisStateDef.nextType == StateDefinition::GOTO)
				{
					Goto thisGoto;
					thisGoto.frame = thisFrame;
					thisGoto.label = thisStateDef.nextArg;
					gotos.push_back(thisGoto);
				}
			}
			if(prevFrame != NULL)
				prevFrame->next = thisFrame;

			if(!thisStateDef.nextType == StateDefinition::NORMAL || i != thisStateDef.frames.length()-1)
				prevFrame = thisFrame;
			else
				prevFrame = NULL;
			//printf("Adding frame: %s %c %d\n", thisStateDef.sprite, thisFrame->frame, thisFrame->duration);
			frameList.push_back(thisFrame);
		}
	}

	// Resolve Gotos
	for(deque<Goto>::const_iterator iter = gotos.begin();iter != gotos.end();iter++)
	{
		Frame *result = FindState((*iter).label.c_str());
		(*iter).frame->next = result;
	}
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
		newClass->parent = NATIVE_CLASS(Actor);
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
				deque<StateDefinition> stateDefs;

				sc.MustGetToken('{');
				//sc.MustGetToken(TK_Identifier); // We should already have grabbed the identifier in all other cases.
				bool needIdentifier = true;
				bool infiniteLoopProtection = false;
				while(sc.token != '}' && !sc.CheckToken('}'))
				{
					StateDefinition thisState;
					thisState.sprite[0] = thisState.sprite[4] = 0;
					thisState.duration = 0;
					thisState.nextType = StateDefinition::NORMAL;

					if(needIdentifier)
						sc.MustGetToken(TK_Identifier);
					else
						needIdentifier = true;
					string stateString = sc.str;
					if(sc.CheckToken(':'))
					{
						infiniteLoopProtection = false;
						thisState.label = stateString;
						// New state
						if(sc.CheckToken('}'))
							sc.ScriptError("State defined with no frames.");
						sc.MustGetToken(TK_Identifier);
					}

					bool invalidSprite = (sc.str.length() != 4);
					strncpy(thisState.sprite, sc.str.c_str(), 4);

					if(sc.CheckToken(TK_Identifier) || sc.CheckToken(TK_StringConst))
					{
						infiniteLoopProtection = false;
						if(invalidSprite) // We now know this is a frame so check sprite length
							sc.ScriptError("Frame must be exactly 4 characters long.");

						thisState.frames = sc.str;
						if(sc.CheckToken('-'))
						{
							sc.MustGetToken(TK_FloatConst);
							thisState.duration = -1;
						}
						else
						{
							if(sc.CheckToken(TK_FloatConst))
								thisState.duration = static_cast<int> (sc.decimal*2);
							else if(stricmp(thisState.sprite, "goto") == 0)
							{
								thisState.nextType = StateDefinition::GOTO;
								thisState.nextArg = thisState.frames;
								thisState.frames.clear();
							}
							else
								sc.ScriptError("Expected frame duration.");
						}

						if(sc.CheckToken('}'))
							goto FinishState;
						else
							sc.MustGetToken(TK_Identifier);

						if(thisState.nextType == StateDefinition::NORMAL)
						{
							for(int func = 0;func <= 2;func++)
							{
								if(sc.str.length() == 4 || func == 2)
								{
									if(stricmp(sc.str.c_str(), "goto") == 0)
									{
										sc.MustGetToken(TK_Identifier);
										thisState.nextType = StateDefinition::GOTO;
										thisState.nextArg = sc.str;
									}
									else if(stricmp(sc.str.c_str(), "wait") == 0)
									{
										thisState.nextType = StateDefinition::WAIT;
									}
									else if(stricmp(sc.str.c_str(), "loop") == 0)
									{
										thisState.nextType = StateDefinition::LOOP;
									}
									else if(stricmp(sc.str.c_str(), "stop") == 0)
									{
										thisState.nextType = StateDefinition::STOP;
									}
									else
										needIdentifier = false;
									break;
								}
								else
								{
									if(sc.CheckToken('('))
									{
										while(!sc.CheckToken(')') && sc.TokensLeft())
											sc.GetNextToken();
									}
								}

								if(!sc.CheckToken(TK_Identifier))
									break;
							}
						}
					}
					else
					{
						needIdentifier = false;
						if(infiniteLoopProtection)
							sc.ScriptError("Malformed script.");
						infiniteLoopProtection = true;
					}
				FinishState:
					stateDefs.push_back(thisState);
				}

				newClass->InstallStates(stateDefs);
			}
			else
			{
				string propertyName = sc.str;
				if(sc.CheckToken('.'))
				{
					sc.MustGetToken(TK_Identifier);
					propertyName = string(".") + sc.str;
				}
				if(!SetProperty(newClass, propertyName.c_str(), sc))
				{
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
				*reinterpret_cast<flagstype_t *>((int8_t*)newClass->defaultInstance + flags[mid].varOffset) |= flags[mid].value;
			else
				*reinterpret_cast<flagstype_t *>((int8_t*)newClass->defaultInstance + flags[mid].varOffset) &= ~flags[mid].value;
			return true;
		}
		else if(ret > 0)
			max = mid-1;
		else
			min = mid+1;
	}
	return false;
}

bool ClassDef::SetProperty(ClassDef *newClass, const char* propName, Scanner &sc)
{
	int min = 0;
	int max = NUM_PROPERTIES - 1;
	while(min <= max)
	{
		int mid = (min+max)/2;
		int ret = stricmp(properties[mid].name, propName);
		if(ret == 0)
		{
			PropertyParam* params = new PropertyParam[strlen(properties[mid].params)];
			// Key:
			//   K - Keyword (Identifier)
			//   I - Integer
			//   F - Float
			//   S - String
			bool optional = false;
			bool done = false;
			const char* p = properties[mid].params;
			unsigned int paramc = 0;
			do
			{
				if(*p != 0)
				{
					while(*p == '_') // Optional
					{
						optional = true;
						p++;
					}

					bool negate = false;
					params[paramc].i = 0; // Try to default to 0

					switch(*p)
					{
						case 'K':
							if(!optional)
								sc.MustGetToken(TK_Identifier);
							else if(!sc.CheckToken(TK_Identifier))
							{
								done = true;
								break;
							}
							params[paramc].s = new char[sc.str.length()];
							strcpy(params[paramc].s, sc.str.c_str());
							break;
						default:
						case 'I':
							if(sc.CheckToken('-'))
								negate = true;

							if(!optional) // Float also includes integers
								sc.MustGetToken(TK_FloatConst);
							else if(!sc.CheckToken(TK_FloatConst))
							{
								done = true;
								break;
							}
							params[paramc].i = (negate ? -1 : 1) * static_cast<int64_t> (sc.decimal);
							break;
						case 'F':
							if(sc.CheckToken('-'))
								negate = true;

							if(!optional)
								sc.MustGetToken(TK_FloatConst);
							else if(!sc.CheckToken(TK_FloatConst))
							{
								done = true;
								break;
							}
							params[paramc].f = (negate ? -1 : 1) * sc.number;
							break;
						case 'S':
							if(!optional)
								sc.MustGetToken(TK_StringConst);
							else if(!sc.CheckToken(TK_StringConst))
							{
								done = true;
								break;
							}
							params[paramc].s = new char[sc.str.length()];
							strcpy(params[paramc].s, sc.str.c_str());
							break;
					}
					paramc++;
					p++;
				}
				else
					sc.GetNextToken();
			}
			while(sc.CheckToken(','));
			if(!optional && *p != 0 && *p != '_')
				sc.ScriptError("Not enough parameters.");

			properties[mid].handler(newClass->defaultInstance, paramc, params);

			// Clean up
			p = properties[mid].params;
			unsigned int i = 0;
			do
			{
				if(*p == 'S' || *p == 'K')
					delete[] params[i].s;
				i++;
			}
			while(*(++p) != 0 && i < paramc);
			delete[] params;
			return true;
		}
		else if(ret > 0)
			max = mid-1;
		else
			min = mid+1;
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

AActor::AActor(const ClassDef *type) : classType(type), flags(0)
{
}

AActor::~AActor()
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
