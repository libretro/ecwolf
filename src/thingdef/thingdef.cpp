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

#include "actor.h"
#include "id_ca.h"
#include "m_random.h"
#include "r_sprites.h"
#include "scanner.h"
#include "w_wad.h"
#include "wl_def.h"
#include "thingdef/thingdef.h"
#include "thingdef/thingdef_type.h"
#include "thingdef/thingdef_expression.h"
#include "thinker.h"

// Code pointer stuff
void InitFunctionTable(ActionTable *table);
void ReleaseFunctionTable();
ActionInfo *LookupFunction(const FName &func, const ActionTable *table);

////////////////////////////////////////////////////////////////////////////////

#define DEFINE_FLAG(prefix, flag, type, variable) { prefix##_##flag, #flag, typeoffsetof(type,variable) }
const struct FlagDef
{
	public:
		const unsigned int	value;
		const char* const	name;
		const int			varOffset;
} flags[] =
{
	DEFINE_FLAG(FL, AMBUSH, AActor, flags),
	DEFINE_FLAG(FL, ATTACKMODE, AActor, flags),
	DEFINE_FLAG(FL, BONUS, AActor, flags),
	DEFINE_FLAG(FL, BRIGHT, AActor, flags),
	DEFINE_FLAG(FL, CANUSEWALLS, AActor, flags),
	DEFINE_FLAG(FL, COUNTKILL, AActor, flags),
	DEFINE_FLAG(FL, FIRSTATTACK, AActor, flags),
	DEFINE_FLAG(FL, ISMONSTER, AActor, flags),
	DEFINE_FLAG(FL, NEVERMARK, AActor, flags),
	DEFINE_FLAG(FL, NONMARK, AActor, flags),
	DEFINE_FLAG(FL, PICKUP, AActor, flags),
	DEFINE_FLAG(FL, SHOOTABLE, AActor, flags),
	DEFINE_FLAG(FL, SOLID, AActor, flags),
	DEFINE_FLAG(FL, VISABLE, AActor, flags)
};
extern const PropDef properties[];

////////////////////////////////////////////////////////////////////////////////

struct StateDefinition
{
	public:
		enum NextType
		{
			GOTO,
			LOOP,
			WAIT,
			STOP,

			NORMAL
		};

		FString		label;
		char		sprite[5];
		FString		frames;
		int			duration;
		bool			fullbright;
		NextType	nextType;
		FString		nextArg;
		Frame::ActionCall	functions[2];
};

static TArray<const SymbolInfo *> *symbolPool = NULL;

SymbolInfo::SymbolInfo(const ClassDef *cls, const FName &var, const int offset) :
	cls(cls), var(var), offset(offset)
{
	if(symbolPool == NULL)
		symbolPool = new TArray<const SymbolInfo *>();
	symbolPool->Push(this);
}

int SymbolCompare(const void *s1, const void *s2)
{
	const Symbol * const sym1 = *((const Symbol **)s1);
	const Symbol * const sym2 = *((const Symbol **)s2);
	if(sym1->GetName() < sym2->GetName())
		return -1;
	else if(sym1->GetName() > sym2->GetName())
		return 1;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void ExprSin(AActor *self, ExpressionNode::Value &out, ExpressionNode* const *args, FRandom *rng)
{
	out = double(sintable[args[0]->Evaluate(self).GetInt()%360])/FRACUNIT;
}

void ExprCos(AActor *self, ExpressionNode::Value &out, ExpressionNode* const *args, FRandom *rng)
{
	out = double(sintable[(args[0]->Evaluate(self).GetInt()+90)%360])/FRACUNIT;
}

void ExprRandom(AActor *self, ExpressionNode::Value &out, ExpressionNode* const *args, FRandom *rng)
{
	int min = args[0]->Evaluate(self).GetInt();
	int max = args[1]->Evaluate(self).GetInt();
	if(min > max)
		out = int64_t(max+(*rng)(min-max));
	else
		out = int64_t(min+(*rng)(max-min));
}

void ExprFRandom(AActor *self, ExpressionNode::Value &out, ExpressionNode* const *args, FRandom *rng)
{
	static const unsigned int randomPrecision = 0x80000000;

	double min = args[0]->Evaluate(self).GetDouble();
	double max = args[1]->Evaluate(self).GetDouble();
	out = min+(double((*rng)(randomPrecision))/randomPrecision)*(max-min);
}

static const struct ExpressionFunction
{
	const char* const				name;
	int								ret;
	unsigned short					args;
	bool							takesRNG;
	FunctionSymbol::ExprFunction	function;
} functions[] =
{
	{ "cos",		TypeHierarchy::FLOAT,	1,	false,	ExprCos },
	{ "frandom",	TypeHierarchy::FLOAT,	2,	true,	ExprFRandom },
	{ "random",		TypeHierarchy::INT,		2,	true,	ExprRandom },
	{ "sin",		TypeHierarchy::FLOAT,	1,	false,	ExprSin },

	{ NULL, 0, false, NULL }
};

////////////////////////////////////////////////////////////////////////////////

TMap<int, ClassDef *> ClassDef::classNumTable;
SymbolTable ClassDef::globalSymbols;

ClassDef::ClassDef()
{
	defaultInstance = (AActor *) malloc(sizeof(AActor));
	defaultInstance = new (defaultInstance) AActor(this);
	defaultInstance->defaults = defaultInstance;
	defaultInstance->Init(true);
}

ClassDef::~ClassDef()
{
	for(unsigned int i = 0;i < frameList.Size();++i)
		delete frameList[i];
	defaultInstance->~AActor();
	free(defaultInstance);
	for(unsigned int i = 0;i < symbols.Size();++i)
		delete symbols[i];
}

TMap<FName, ClassDef *> &ClassDef::ClassTable()
{
	static TMap<FName, ClassDef *> classTable;
	return classTable;
}

AActor *ClassDef::CreateInstance() const
{
	if(!defaultInstance->SpawnState)
	{
		defaultInstance->DeathState = FindState("Death");
		defaultInstance->MeleeState = FindState("Melee");
		defaultInstance->MissileState = FindState("Missile");
		defaultInstance->PainState = FindState("Pain");
		defaultInstance->PathState = FindState("Path");
		defaultInstance->SpawnState = FindState("Spawn");
		defaultInstance->SeeState = FindState("See");
	}

	AActor *newactor = (AActor *) malloc(defaultInstance->__GetSize());
	memcpy(newactor, defaultInstance, defaultInstance->__GetSize());
	ConstructNative(this, newactor);
	newactor->classType = this;
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
	ClassDef **ret = ClassTable().CheckKey(className);
	if(ret == NULL)
		return NULL;
	return *ret;
}

const ActionInfo *ClassDef::FindFunction(const FName &function) const
{
	if(actions.Size() != 0)
	{
		ActionInfo *func = LookupFunction(function, &actions);
		if(func)
			return func;
	}
	if(parent)
		return parent->FindFunction(function);
	return NULL;
}

const Frame *ClassDef::FindState(const FName &stateName) const
{
	Frame *const *ret = stateList.CheckKey(stateName);
	if(ret == NULL)
		return (!parent ? NULL : parent->FindState(stateName));
	return *ret;
}

Symbol *ClassDef::FindSymbol(const FName &symbol) const
{
	unsigned int min = 0;
	unsigned int max = symbols.Size()-1;
	unsigned int mid = max/2;
	if(symbols.Size() > 0)
	{
		do
		{
			if(symbols[mid]->GetName() == symbol)
				return symbols[mid];

			if(symbols[mid]->GetName() > symbol)
				max = mid-1;
			else if(symbols[mid]->GetName() < symbol)
				min = mid+1;
			mid = (min+max)/2;
		}
		while(max >= min && max < symbols.Size());
	}

	if(parent)
		return parent->FindSymbol(symbol);
	else if(globalSymbols.Size() > 0)
	{
		// Search globals.
		min = 0;
		max = globalSymbols.Size()-1;
		mid = max/2;
		do
		{
			if(globalSymbols[mid]->GetName() == symbol)
				return globalSymbols[mid];

			if(globalSymbols[mid]->GetName() > symbol)
				max = mid-1;
			else if(globalSymbols[mid]->GetName() < symbol)
				min = mid+1;
			mid = (min+max)/2;
		}
		while(max >= min && max < globalSymbols.Size());
	}
	return NULL;
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
			thisFrame = new Frame();
			if(i == 0 && !thisStateDef.label.IsEmpty())
			{
				stateList[thisStateDef.label] = thisFrame;
				loopPoint = thisFrame;
			}
			memcpy(thisFrame->sprite, thisStateDef.sprite, 4);
			thisFrame->frame = thisStateDef.frames[i]-'A';
			thisFrame->duration = thisStateDef.duration;
			thisFrame->fullbright = thisStateDef.fullbright;
			thisFrame->action = thisStateDef.functions[0];
			thisFrame->thinker = thisStateDef.functions[1];
			thisFrame->next = NULL;
			thisFrame->spriteInf = 0;
			// Only free the action arguments if we are the last frame using them.
			thisFrame->freeActionArgs = i == thisStateDef.frames.Len()-1;
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

bool ClassDef::IsDescendantOf(const ClassDef *parent) const
{
	const ClassDef *currentParent = this->parent;
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

	InitFunctionTable(NULL);

	// Add function symbols
	const ExpressionFunction *func = functions;
	do
	{
		globalSymbols.Push(new FunctionSymbol(func->name, TypeHierarchy::staticTypes.GetType(TypeHierarchy::PrimitiveTypes(func->ret)), func->args, func->function, func->takesRNG));
	}
	while((++func)->name != NULL);
	qsort(&globalSymbols[0], globalSymbols.Size(), sizeof(globalSymbols[0]), SymbolCompare);

	int lastLump = 0;
	int lump = 0;
	while((lump = Wads.FindLump("DECORATE", &lastLump)) != -1)
	{
		ParseDecorateLump(lump);
	}

	ReleaseFunctionTable();
	delete symbolPool;
	DumpClasses();

	R_InitSprites();

	TMap<FName, ClassDef *>::Iterator iter(ClassTable());
	TMap<FName, ClassDef *>::Pair *pair;
	while(iter.NextPair(pair))
	{
		ClassDef * const cls = pair->Value;
		for(unsigned int i = 0;i < cls->frameList.Size();++i)
			cls->frameList[i]->spriteInf = R_GetSprite(cls->frameList[i]->sprite);
	}
}

void ClassDef::ParseActor(Scanner &sc)
{
	// Read the header
	sc.MustGetToken(TK_Identifier);
	ClassDef **classRef = ClassTable().CheckKey(sc->str);
	ClassDef *newClass;
	bool previouslyDefined = classRef != NULL;
	if(!previouslyDefined)
	{
		newClass = new ClassDef();
		ClassTable()[sc->str] = newClass;
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
	else
	{
		newClass->parent = NATIVE_CLASS(Actor);
		if(newClass->parent == newClass) // If no class was specified to inherit from, inherit from AActor, but not for AActor.
			newClass->parent = NULL;
	}
	if(sc.CheckToken(TK_IntConst))
	{
		classNumTable[sc->number] = newClass;
	}
	if(sc.CheckToken(TK_Identifier))
	{
		if(sc->str.CompareNoCase("native") == 0)
			native = true;
		else
			sc.ScriptMessage(Scanner::ERROR, "Unknown keyword '%s'.\n", sc->str.GetChars());
	}
	if(previouslyDefined && !native)
		sc.ScriptMessage(Scanner::ERROR, "Actor '%s' already defined.\n", newClass->name.GetChars());
	if(!native) // Initialize the default instance to the nearest native class.
	{
		newClass->ConstructNative = newClass->parent->ConstructNative;

		newClass->defaultInstance->~AActor();
		free(newClass->defaultInstance);
		newClass->defaultInstance = (AActor *) malloc(newClass->parent->defaultInstance->__GetSize());
		memcpy(newClass->defaultInstance, newClass->parent->defaultInstance, newClass->parent->defaultInstance->__GetSize());
		newClass->ConstructNative(newClass, newClass->defaultInstance);
		newClass->defaultInstance->defaults = newClass->defaultInstance;
		newClass->defaultInstance->Init(true);
	}
	else
	{
		// Copy the parents defaults for native classes
		if(newClass->parent)
			memcpy(newClass->defaultInstance, newClass->parent->defaultInstance, newClass->parent->defaultInstance->__GetSize());

		// Initialize the default instance
		newClass->ConstructNative(newClass, newClass->defaultInstance);
		newClass->defaultInstance->defaults = newClass->defaultInstance;
		newClass->defaultInstance->InitClean();
	}
	// Copy properties and flags.
	if(newClass->parent != NULL)
		*newClass->defaultInstance = *newClass->parent->defaultInstance;

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
				InitFunctionTable(&newClass->actions);

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
							sc.ScriptMessage(Scanner::ERROR, "Sprite name must be exactly 4 characters long.");

						R_LoadSprite(thisState.sprite);
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
						thisState.functions[0].pointer = thisState.functions[1].pointer = NULL;
						thisState.fullbright = false;

						do
						{
							if(sc.CheckToken('}'))
								goto FinishState;
							else
								sc.MustGetToken(TK_Identifier);

							if(sc->str.CompareNoCase("bright") == 0)
								thisState.fullbright = true;
							else
								break;
						}
						while(true);

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
										const ActionInfo *funcInf = newClass->FindFunction(sc->str);
										if(funcInf)
										{
											thisState.functions[func].pointer = *funcInf->func;

											CallArguments *&ca = thisState.functions[func].args;
											ca = new CallArguments();
											CallArguments::Value val;
											unsigned int argc = 0;
											if(sc.CheckToken('('))
											{
												if(funcInf->maxArgs == 0)
													sc.MustGetToken(')');
												else if(!(funcInf->minArgs == 0 && sc.CheckToken(')')))
												{
													do
													{
														val.isExpression = false;

														const Type *argType = funcInf->types[argc];
														if(argType == TypeHierarchy::staticTypes.GetType(TypeHierarchy::INT) ||
															argType == TypeHierarchy::staticTypes.GetType(TypeHierarchy::FLOAT))
														{
															val.isExpression = true;
															if(argType == TypeHierarchy::staticTypes.GetType(TypeHierarchy::INT))
																val.useType = CallArguments::Value::VAL_INTEGER;
															else
																val.useType = CallArguments::Value::VAL_DOUBLE;
															val.expr = ExpressionNode::ParseExpression(newClass, TypeHierarchy::staticTypes, sc);
														}
														else
														{
															sc.MustGetToken(TK_StringConst);
															val.useType = CallArguments::Value::VAL_STRING;
															val.str = sc->str;
														}
														ca->AddArgument(val);
														++argc;
													}
													while(sc.CheckToken(',') && argc <= funcInf->maxArgs);
													sc.MustGetToken(')');
												}
											}
											if(argc < funcInf->minArgs)
												sc.ScriptMessage(Scanner::ERROR, "Too few arguments.");
											else
											{
												// Push unused defaults.
												while(argc < funcInf->maxArgs)
													ca->AddArgument(funcInf->defaults[(argc++)-funcInf->minArgs]);
											}
										}
										else
											printf("Could not find function %s\n", sc->str.GetChars());
									}
								}

								if(!sc.CheckToken(TK_Identifier))
									break;
								else if(sc.CheckToken(':'))
								{
									needIdentifier = false;
									sc.Rewind();
									break;
								}
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
			else if(sc->str.CompareNoCase("action") == 0)
			{
				sc.MustGetToken(TK_Identifier);
				if(sc->str.CompareNoCase("native") != 0)
					sc.ScriptMessage(Scanner::ERROR, "Custom actions not supported.");
				sc.MustGetToken(TK_Identifier);
				ActionInfo *funcInf = LookupFunction(sc->str, NULL);
				newClass->actions.Push(funcInf);
				sc.MustGetToken('(');
				if(!sc.CheckToken(')'))
				{
					bool optRequired = false;
					do
					{
						sc.MustGetToken(TK_Identifier);
						const Type *type = TypeHierarchy::staticTypes.GetType(sc->str);
						if(type == NULL)
							sc.ScriptMessage(Scanner::ERROR, "Unknown type %s.\n", sc->str.GetChars());
						funcInf->types.Push(type);

						if(sc->str.CompareNoCase("class") == 0)
						{
							sc.MustGetToken('<');
							sc.MustGetToken(TK_Identifier);
							sc.MustGetToken('>');
						}
						sc.MustGetToken(TK_Identifier);
						if(optRequired || sc.CheckToken('='))
						{
							if(optRequired)
								sc.MustGetToken('=');
							else
								optRequired = true;

							CallArguments::Value defVal;
							defVal.isExpression = false;

							if(type == TypeHierarchy::staticTypes.GetType(TypeHierarchy::INT) ||
								type == TypeHierarchy::staticTypes.GetType(TypeHierarchy::FLOAT)
							)
							{
								ExpressionNode *node = ExpressionNode::ParseExpression(newClass, TypeHierarchy::staticTypes, sc);
								const ExpressionNode::Value &val = node->Evaluate(NULL);
								if(type == TypeHierarchy::staticTypes.GetType(TypeHierarchy::INT))
								{
									defVal.useType = CallArguments::Value::VAL_INTEGER;
									defVal.val.i = val.GetInt();
								}
								else
								{
									defVal.useType = CallArguments::Value::VAL_DOUBLE;
									defVal.val.d = val.GetDouble();
								}
								delete node;
							}
							else if(type == TypeHierarchy::staticTypes.GetType(TypeHierarchy::BOOL))
							{
								sc.MustGetToken(TK_BoolConst);
								defVal.useType = CallArguments::Value::VAL_INTEGER;
								defVal.val.i = sc->number;
							}
							else
							{
								sc.MustGetToken(TK_StringConst);
								defVal.useType = CallArguments::Value::VAL_STRING;
								defVal.str = sc->str;
							}
							funcInf->defaults.Push(defVal);
						}
						else
							++funcInf->minArgs;
						++funcInf->maxArgs;
					}
					while(sc.CheckToken(','));
					sc.MustGetToken(')');
				}
				sc.MustGetToken(';');
			}
			else if(sc->str.CompareNoCase("native") == 0)
			{
				sc.MustGetToken(TK_Identifier);
				const Type *type = TypeHierarchy::staticTypes.GetType(sc->str);
				if(type == NULL)
					sc.ScriptMessage(Scanner::ERROR, "Unknown type %s.\n", sc->str.GetChars());
				sc.MustGetToken(TK_Identifier);
				FName varName(sc->str);
				const SymbolInfo *symInf = NULL;
				for(unsigned int i = 0;i < symbolPool->Size();++i)
				{
					// I think the symbol pool will be small enough to do a
					// linear search on.
					if((*symbolPool)[i]->cls == newClass && (*symbolPool)[i]->var == varName)
					{
						symInf = (*symbolPool)[i];
						break;
					}
				}
				if(symInf == NULL)
					sc.ScriptMessage(Scanner::ERROR, "Could not identify symbol %s::%s.\n", newClass->name.GetChars(), varName.GetChars());
				sc.MustGetToken(';');

				newClass->symbols.Push(new VariableSymbol(varName, type, symInf->offset));
			}
			else
			{
				FString className("actor");
				FString propertyName = sc->str;
				if(sc.CheckToken('.'))
				{
					className = propertyName;
					sc.MustGetToken(TK_Identifier);
					propertyName = sc->str;
				}

				if(!SetProperty(newClass, className, propertyName, sc))
				{
					do
					{
						sc.GetNextToken();
					}
					while(sc.CheckToken(','));
					sc.ScriptMessage(Scanner::WARNING, "Unkown property '%s' for actor '%s'.\n", propertyName.GetChars(), newClass->name.GetChars());
				}
			}
		}
	}

	// Now sort the symbol table.
	qsort(&newClass->symbols[0], newClass->symbols.Size(), sizeof(newClass->symbols[0]), SymbolCompare);
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
		if(sc.CheckToken('#'))
		{
			sc.MustGetToken(TK_Identifier);
			if(sc->str.CompareNoCase("include") != 0)
				sc.ScriptMessage(Scanner::ERROR, "Expected 'include' got '%s' instead.", sc->str.GetChars());
			sc.MustGetToken(TK_StringConst);

			int lmp = Wads.GetNumForFullName(sc->str);
			if(lmp == -1)
				sc.ScriptMessage(Scanner::ERROR, "Could not find lump \"%s\".", sc->str.GetChars());
			ParseDecorateLump(lmp);
			continue;
		}

		sc.MustGetToken(TK_Identifier);
		if(sc->str.CompareNoCase("actor") == 0)
		{
			ParseActor(sc);
		}
		else if(sc->str.CompareNoCase("const") == 0)
		{
			sc.MustGetToken(TK_Identifier);
			const Type *type = TypeHierarchy::staticTypes.GetType(sc->str);
			if(type == NULL)
				sc.ScriptMessage(Scanner::ERROR, "Unknown type %s.\n", sc->str.GetChars());
			sc.MustGetToken(TK_Identifier);
			FName constName(sc->str);
			sc.MustGetToken('=');
			ExpressionNode *expr = ExpressionNode::ParseExpression(NATIVE_CLASS(Actor), TypeHierarchy::staticTypes, sc);
			ConstantSymbol *newSym = new ConstantSymbol(constName, type, expr->Evaluate(NULL));
			delete expr;
			sc.MustGetToken(';');

			// We must insert the constant into the table at the proper place
			// now since the next const may try to reference it.
			if(globalSymbols.Size() > 0)
			{
				unsigned int min = 0;
				unsigned int max = globalSymbols.Size()-1;
				unsigned int mid = max/2;
				if(max > 0)
				{
					do
					{
						if(globalSymbols[mid]->GetName() > constName)
							max = mid-1;
						else if(globalSymbols[mid]->GetName() < constName)
							min = mid+1;
						else
							break;
						mid = (min+max)/2;
					}
					while(max >= min && max < globalSymbols.Size());
				}
				globalSymbols.Insert(mid, newSym);
			}
			else
				globalSymbols.Push(newSym);
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

bool ClassDef::SetProperty(ClassDef *newClass, const char* className, const char* propName, Scanner &sc)
{
	static unsigned int NUM_PROPERTIES = 0;
	if(NUM_PROPERTIES == 0)
	{
		// Calculate NUM_PROPERTIES if needed.
		while(properties[NUM_PROPERTIES++].name != NULL)
			;
	}

	int min = 0;
	int max = NUM_PROPERTIES - 1;
	while(min <= max)
	{
		int mid = (min+max)/2;
		int ret = stricmp(properties[mid].name, propName);
		if(ret == 0)
		{
			if(newClass->IsDescendantOf(properties[mid].className) &&
				stricmp(properties[mid].prefix, className) != 0)
				sc.ScriptMessage(Scanner::ERROR, "Property %s.%s not available in this scope.\n", properties[mid].className->name.GetChars(), propName);

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
			if(*p != 0)
			{
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
								params[paramc].s = new char[sc->str.Len()+1];
								strcpy(params[paramc].s, sc->str);
								break;
							default:
							case 'I':
								if(sc.CheckToken('('))
								{
									params[paramc].isExpression = true;
									params[paramc].expr = ExpressionNode::ParseExpression(newClass, TypeHierarchy::staticTypes, sc, NULL);
									sc.MustGetToken(')');
									break;
								}
								else
									params[paramc].isExpression = false;

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
								params[paramc].s = new char[sc->str.Len()+1];
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
			}
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
	for(TMap<FName, ClassDef *>::Iterator iter(ClassTable());iter.NextPair(pair);)
	{
		delete pair->Value;
	}

	// Also clear globals
	for(unsigned int i = 0;i < globalSymbols.Size();++i)
		delete globalSymbols[i];
}

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
				for(TMap<FName, ClassDef *>::Iterator iter(ClassTable());iter.NextPair(pair);)
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

////////////////////////////////////////////////////////////////////////////////

CallArguments::~CallArguments()
{
	for(unsigned int i = 0;i < args.Size();++i)
	{
		if(args[i].isExpression)
			delete args[i].expr;
	}
}

void CallArguments::AddArgument(const CallArguments::Value &val)
{
	args.Push(val);
}

void CallArguments::Evaluate(AActor *self)
{
	for(unsigned int i = 0;i < args.Size();++i)
	{
		if(args[i].isExpression)
		{
			const ExpressionNode::Value &val = args[i].expr->Evaluate(self);
			if(args[i].useType == Value::VAL_INTEGER)
				args[i].val.i = val.GetInt();
			else
				args[i].val.d = val.GetDouble();
		}
	}
}
