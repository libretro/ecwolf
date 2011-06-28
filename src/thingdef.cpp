/*
** thingdef.cpp
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include "r_sprites.h"
#include "scanner.h"
#include "w_wad.h"
#include "wl_def.h"
#include "thingdef.h"
#include "thinker.h"

// Code pointer stuff
void InitFunctionTable();
void ReleaseFunctionTable();
ActionPtr FindFunction(const FName &func);

#define DEFINE_FLAG(prefix, flag, type, variable) { prefix##_##flag, #flag, typeoffsetof(type,variable) }
const FlagDef flags[] =
{
	DEFINE_FLAG(FL, AMBUSH, AActor, flags),
	DEFINE_FLAG(FL, ATTACKMODE, AActor, flags),
	DEFINE_FLAG(FL, BONUS, AActor, flags),
	DEFINE_FLAG(FL, FIRSTATTACK, AActor, flags),
	DEFINE_FLAG(FL, FULLBRIGHT, AActor, flags),
	DEFINE_FLAG(FL, ISMONSTER, AActor, flags),
	DEFINE_FLAG(FL, NEVERMARK, AActor, flags),
	DEFINE_FLAG(FL, NONMARK, AActor, flags),
	DEFINE_FLAG(FL, SHOOTABLE, AActor, flags),
	DEFINE_FLAG(FL, VISABLE, AActor, flags)
};
extern const PropDef properties[NUM_PROPERTIES];

TMap<FName, ClassDef *> ClassDef::classTable;
TMap<int, ClassDef *> ClassDef::classNumTable;

ClassDef::ClassDef()
{
	defaultInstance = new AActor(this);
	defaultInstance->defaults = defaultInstance;
	defaultInstance->Init(true);
}

ClassDef::~ClassDef()
{
	for(unsigned int i = 0;i < frameList.Size();i++)
		delete frameList[i];
	delete defaultInstance;
}

AActor *ClassDef::CreateInstance() const
{
	AActor *newactor = defaultInstance->__NewNativeInstance(this);
	*newactor = *defaultInstance;
	newactor->Init();
	return newactor;
}

const ClassDef *ClassDef::FindClass(unsigned int ednum)
{
	ClassDef **ret = classNumTable.CheckKey(ednum);
	if(ret == NULL)
		return NULL;
	return *ret;
}

const ClassDef *ClassDef::FindClass(const FName &className)
{
	ClassDef **ret = classTable.CheckKey(className);
	if(ret == NULL)
		return NULL;
	return *ret;
}

const Frame *ClassDef::FindState(const FName &stateName) const
{
	Frame *const *ret = stateList.CheckKey(stateName);
	if(ret == NULL)
		return (!parent ? NULL : parent->FindState(stateName));
	return *ret;
}

struct Goto
{
	public:
		Frame	*frame;
		FString	label;
};
void ClassDef::InstallStates(const TArray<StateDefinition> &stateDefs)
{
	// We need to resolve gotos after we install the states.
	TArray<Goto> gotos;

	FString thisLabel;
	Frame *prevFrame = NULL;
	Frame *loopPoint = NULL;
	Frame *thisFrame = NULL;
	for(unsigned int iter = 0;iter < stateDefs.Size();iter++)
	{
		const StateDefinition &thisStateDef = stateDefs[iter];

		// Special case, `Label: stop`, remove state.  Hmm... I wonder if ZDoom handles fall throughs on this.
		if(!thisStateDef.label.IsEmpty() && thisStateDef.sprite[0] == 0 && thisStateDef.nextType == StateDefinition::STOP)
		{
			stateList.Remove(thisStateDef.label);
			continue;
		}

		for(int i = 0;i < thisStateDef.frames.Len();i++)
		{
			R_LoadSprite(thisStateDef.sprite);

			thisFrame = new Frame();
			if(i == 0 && !thisStateDef.label.IsEmpty())
			{
				stateList[thisStateDef.label] = thisFrame;
				loopPoint = thisFrame;
			}
			memcpy(thisFrame->sprite, thisStateDef.sprite, 4);
			thisFrame->frame = thisStateDef.frames[i];
			thisFrame->duration = thisStateDef.duration;
			thisFrame->action = thisStateDef.functions[0];
			thisFrame->thinker = thisStateDef.functions[1];
			thisFrame->next = NULL;
			if(i == thisStateDef.frames.Len()-1) // Handle nextType
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
					gotos.Push(thisGoto);
				}
			}
			if(prevFrame != NULL)
				prevFrame->next = thisFrame;

			if(thisStateDef.nextType == StateDefinition::NORMAL || i != thisStateDef.frames.Len()-1)
				prevFrame = thisFrame;
			else
				prevFrame = NULL;
			//printf("Adding frame: %s %c %d\n", thisStateDef.sprite, thisFrame->frame, thisFrame->duration);
			frameList.Push(thisFrame);
		}
	}

	// Resolve Gotos
	for(unsigned int iter = 0;iter < gotos.Size();++iter)
	{
		const Frame *result = FindState(gotos[iter].label);
		gotos[iter].frame->next = result;
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

	InitFunctionTable();

	int lastLump = 0;
	int lump = 0;
	while((lump = Wads.FindLump("DECORATE", &lastLump)) != -1)
	{
		ParseDecorateLump(lump);
	}

	ReleaseFunctionTable();
	DumpClasses();

	R_InitSprites();
}

void ClassDef::ParseActor(Scanner &sc)
{
	// Read the header
	sc.MustGetToken(TK_Identifier);
	ClassDef **classRef = classTable.CheckKey(sc->str);
	ClassDef *newClass;
	bool previouslyDefined = classRef != NULL;
	if(!previouslyDefined)
	{
		newClass = new ClassDef();
		classTable[sc->str] = newClass;
	}
	else
		newClass = *classRef;
	bool native = false;
	newClass->name = sc->str;
	if(sc.CheckToken(':'))
	{
		sc.MustGetToken(TK_Identifier);
		newClass->parent = FindClass(sc->str);
		if(newClass->parent == NULL)
			sc.ScriptMessage(Scanner::ERROR, "Could not find parent actor '%s'\n", sc->str.GetChars());
	}
	else if(!previouslyDefined) // If no class was specified to inherit from, inherit from AActor, but not for AActor.
		newClass->parent = NATIVE_CLASS(Actor);
	if(sc.CheckToken(TK_Identifier))
	{
		if(sc->str.CompareNoCase("native") == 0)
			native = true;
		else
			sc.ScriptMessage(Scanner::ERROR, "Unknown keyword '%s'.\n", sc->str.GetChars());
	}
	else if(sc.CheckToken(TK_IntConst))
	{
		classNumTable[sc->number] = newClass;
	}
	if(previouslyDefined && !native)
		sc.ScriptMessage(Scanner::ERROR, "Actor '%s' already defined.\n", newClass->name.GetChars());
	if(!native) // Initialize the default instance to the nearest native class.
	{
		delete newClass->defaultInstance;
		newClass->defaultInstance = newClass->parent->defaultInstance->__NewNativeInstance(newClass);
		newClass->defaultInstance->defaults = newClass->defaultInstance;
		newClass->defaultInstance->Init(true);
	}

	sc.MustGetToken('{');
	while(!sc.CheckToken('}'))
	{
		if(sc.CheckToken('+') || sc.CheckToken('-'))
		{
			bool set = sc->token == '+';
			FString flagName;
			sc.MustGetToken(TK_Identifier);
			flagName = sc->str;
			if(sc.CheckToken('.'))
			{
				sc.MustGetToken(TK_Identifier);
				flagName += FString(".") + sc->str;
			}
			if(!SetFlag(newClass, flagName, set))
				printf("Warning: Unknown flag '%s' for actor '%s'.\n", flagName.GetChars(), newClass->name.GetChars());
		}
		else
		{
			sc.MustGetToken(TK_Identifier);
			if(sc->str.CompareNoCase("states") == 0)
			{
				TArray<StateDefinition> stateDefs;

				sc.MustGetToken('{');
				//sc.MustGetToken(TK_Identifier); // We should already have grabbed the identifier in all other cases.
				bool needIdentifier = true;
				bool infiniteLoopProtection = false;
				while(sc->token != '}' && !sc.CheckToken('}'))
				{
					StateDefinition thisState;
					thisState.sprite[0] = thisState.sprite[4] = 0;
					thisState.duration = 0;
					thisState.nextType = StateDefinition::NORMAL;

					if(needIdentifier)
						sc.MustGetToken(TK_Identifier);
					else
						needIdentifier = true;
					FString stateString = sc->str;
					if(sc.CheckToken(':'))
					{
						infiniteLoopProtection = false;
						thisState.label = stateString;
						// New state
						if(sc.CheckToken('}'))
							sc.ScriptMessage(Scanner::ERROR, "State defined with no frames.");
						sc.MustGetToken(TK_Identifier);
					}

					bool invalidSprite = (sc->str.Len() != 4);
					strncpy(thisState.sprite, sc->str, 4);

					if(sc.CheckToken(TK_Identifier) || sc.CheckToken(TK_StringConst))
					{
						infiniteLoopProtection = false;
						if(invalidSprite) // We now know this is a frame so check sprite length
							sc.ScriptMessage(Scanner::ERROR, "Frame must be exactly 4 characters long.");

						thisState.frames = sc->str;
						if(sc.CheckToken('-'))
						{
							sc.MustGetToken(TK_FloatConst);
							thisState.duration = -1;
						}
						else
						{
							if(sc.CheckToken(TK_FloatConst))
								thisState.duration = static_cast<int> (sc->decimal*2);
							else if(stricmp(thisState.sprite, "goto") == 0)
							{
								thisState.nextType = StateDefinition::GOTO;
								thisState.nextArg = thisState.frames;
								thisState.frames = FString();
							}
							else
								sc.ScriptMessage(Scanner::ERROR, "Expected frame duration.");
						}
						thisState.functions[0] = thisState.functions[1] = NULL;

						if(sc.CheckToken('}'))
							goto FinishState;
						else
							sc.MustGetToken(TK_Identifier);

						if(thisState.nextType == StateDefinition::NORMAL)
						{
							for(int func = 0;func <= 2;func++)
							{
								if(sc->str.Len() == 4 || func == 2)
								{
									if(sc->str.CompareNoCase("goto") == 0)
									{
										sc.MustGetToken(TK_Identifier);
										thisState.nextType = StateDefinition::GOTO;
										thisState.nextArg = sc->str;
									}
									else if(sc->str.CompareNoCase("wait") == 0)
									{
										thisState.nextType = StateDefinition::WAIT;
									}
									else if(sc->str.CompareNoCase("loop") == 0)
									{
										thisState.nextType = StateDefinition::LOOP;
									}
									else if(sc->str.CompareNoCase("stop") == 0)
									{
										thisState.nextType = StateDefinition::STOP;
									}
									else
										needIdentifier = false;
									break;
								}
								else
								{
									if(sc->str.CompareNoCase("NOP") != 0)
									{
										ActionPtr &ptr = thisState.functions[func];
										ActionPtr func = FindFunction(sc->str);
										if(func)
										{
											ptr = *func;
										}
										else
											printf("Could not find function %s\n", sc->str.GetChars());

										if(sc.CheckToken('('))
										{
											while(!sc.CheckToken(')') && sc.TokensLeft())
												sc.GetNextToken();
										}
									}
								}

								if(!sc.CheckToken(TK_Identifier))
									break;
							}
						}
					}
					else
					{
						thisState.sprite[0] = 0;
						needIdentifier = false;
						if(infiniteLoopProtection)
							sc.ScriptMessage(Scanner::ERROR, "Malformed script.");
						infiniteLoopProtection = true;
					}
				FinishState:
					stateDefs.Push(thisState);
				}

				newClass->InstallStates(stateDefs);
			}
			else
			{
				FString propertyName = sc->str;
				if(sc.CheckToken('.'))
				{
					sc.MustGetToken(TK_Identifier);
					propertyName = FString(".") + sc->str;
				}
				if(!SetProperty(newClass, propertyName, sc))
				{
					do
					{
						sc.GetNextToken();
					}
					while(sc.CheckToken(','));
					printf("Warning: Unkown property '%s' for actor '%s'.\n", propertyName.GetChars(), newClass->name.GetChars());
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
	sc.SetScriptIdentifier(Wads.GetLumpFullName(lumpNum));

	while(sc.TokensLeft())
	{
		sc.MustGetToken(TK_Identifier);
		if(sc->str.CompareNoCase("actor") == 0)
		{
			ParseActor(sc);
		}
		else
			sc.ScriptMessage(Scanner::ERROR, "Unknown thing section '%s'.", sc->str.GetChars());
	}
	delete[] data;
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
							params[paramc].s = new char[sc->str.Len()];
							strcpy(params[paramc].s, sc->str);
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
							params[paramc].i = (negate ? -1 : 1) * static_cast<int64_t> (sc->decimal);
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
							params[paramc].f = (negate ? -1 : 1) * sc->number;
							break;
						case 'S':
							if(!optional)
								sc.MustGetToken(TK_StringConst);
							else if(!sc.CheckToken(TK_StringConst))
							{
								done = true;
								break;
							}
							params[paramc].s = new char[sc->str.Len()];
							strcpy(params[paramc].s, sc->str);
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
				sc.ScriptMessage(Scanner::ERROR, "Not enough parameters.");

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
	TMap<FName, ClassDef *>::Pair *pair;
	for(TMap<FName, ClassDef *>::Iterator iter(classTable);iter.NextPair(pair);)
	{
		delete pair->Value;
	}
}

template<class T>
const ClassDef *ClassDef::DeclareNativeClass(const char* className, const ClassDef *parent)
{
	ClassDef **definitionLookup = classTable.CheckKey(className);
	ClassDef *definition = NULL;
	if(definitionLookup == NULL)
	{
		definition = new ClassDef();
		classTable[className] = definition;
	}
	else
		definition = *definitionLookup;
	definition->name = className;
	definition->parent = parent;
	delete definition->defaultInstance;
	definition->defaultInstance = new T(definition);
	definition->defaultInstance->defaults = definition->defaultInstance;
	definition->defaultInstance->Init(true);
	return definition;
}

////////////////////////////////////////////////////////////////////////////////

// We can't make AActor a thinker since we create non-thinking objects.
class AActorProxy : public Thinker
{
	DECLARE_THINKER(AActorProxy)
	public:
		AActorProxy(AActor *parent) : parent(parent)
		{
		}

		~AActorProxy()
		{
			delete parent;
		}

		void Tick()
		{
			parent->Tick();
		}
	private:
		AActor * const parent;
};
IMPLEMENT_THINKER(AActorProxy)

LinkedList<AActor *> AActor::actors;
const ClassDef *AActor::__StaticClass = ClassDef::DeclareNativeClass<AActor>("Actor", NULL);

AActor::AActor(const ClassDef *type) : classType(type), flags(0), dir(nodir), soundZone(NULL)
{
}

AActor::~AActor()
{
	if(actorRef)
		actors.Remove(actorRef);
}

void AActor::Destroy()
{
	assert(thinker != NULL);
	thinker->Destroy();
}

void AActor::Die()
{
	const Frame *death = FindState("Death");
	if(death)
		SetState(death);
	else
		Destroy();
}

void AActor::EnterZone(const MapZone *zone)
{
	if(zone)
		soundZone = zone;
}

const Frame *AActor::FindState(const FName &name) const
{
	return classType->FindState(name);
}

void AActor::Init(bool nothink)
{
	if(!nothink)
	{
		actorRef = actors.Push(this);
		thinker = new AActorProxy(this);

		SetState(FindState("Spawn"), true);
	}
	else
	{
		actorRef = NULL;
		thinker = NULL;
		state = NULL;
	}
}

void AActor::SetState(const Frame *state, bool notic)
{
	this->state = state;
	ticcount = state->duration;
	if(!notic && state->action)
		state->action(this);
}

void AActor::Tick()
{
	if(state == NULL)
	{
		Destroy();
		return;
	}

	if(ticcount > 0)
		--ticcount;

	while(ticcount == 0)
	{
		if(!state->next)
		{
			Destroy();
			return;
		}
		SetState(state->next);
	}

	if(state->thinker)
		state->thinker(this);
}

AActor *AActor::Spawn(const ClassDef *type, fixed x, fixed y, fixed z)
{
	if(type == NULL)
	{
		printf("Tried to spawn classless actor.\n");
		return NULL;
	}

	AActor *actor = type->CreateInstance();
	actor->x = x;
	actor->y = y;
	actor->tilex = x>>FRACBITS;
	actor->tiley = y>>FRACBITS;

	MapSpot spot = map->GetSpot(actor->tilex, actor->tiley, 0);
	actor->EnterZone(spot->zone);
	return actor;
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
				TMap<FName, ClassDef *>::Pair *pair;
				for(TMap<FName, ClassDef *>::Iterator iter(classTable);iter.NextPair(pair);)
				{
					if(pair->Value->parent == classType)
					{
						*nextChild = new ClassTree(pair->Value);
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
				for(int i = spacing;i > 0;--i)
					printf("  ");
				printf("%s\n", thisClass->name.GetChars());
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
