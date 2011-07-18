/*
** thingdef.h
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

#ifndef __THINGDEF_H__
#define __THINGDEF_H__
#include "actor.h"
#include "scanner.h"
#include "tarray.h"
#include "wl_def.h"
#include "zstring.h"

class ClassDef;
class Frame;
class Symbol;

class ActionInfo
{
	public:
		ActionInfo(ActionPtr func, const FName &name);

		ActionPtr func;
		const FName name;
};

typedef TArray<ActionInfo *> ActionTable;

#define ACTION_FUNCTION(func) \
	void __AF_##func(AActor *self); \
	static const ActionInfo __AI_##func(__AF_##func, #func); \
	void __AF_##func(AActor *self)

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
		NextType	nextType;
		FString		nextArg;
		ActionPtr	functions[2];
};

struct FlagDef
{
	public:
		const unsigned int	value;
		const char* const	name;
		const int			varOffset;
};

union PropertyParam
{
	char	*s;
	double	f;
	int64_t	i;
};
typedef void (*PropHandler)(AActor *defaults, const unsigned int PARAM_COUNT, PropertyParam* params);
#define HANDLE_PROPERTY(property) void __Handler_##property(AActor *defaults, const unsigned int PARAM_COUNT, PropertyParam* params)
struct PropDef
{
	public:
		const ClassDef* const	className;
		const char* const		name;
		const char* const		params;
		PropHandler				handler;
};
#define NUM_PROPERTIES 9

class ClassDef
{
	public:
		ClassDef();
		~ClassDef();

		AActor					*CreateInstance() const;
		bool					IsDecendantOf(const ClassDef *parent) const;

		/**
		 * Use with IMPLEMENT_CLASS to add a natively defined class.
		 */
		template<class T>
		static const ClassDef	*DeclareNativeClass(const char* className, const ClassDef *parent)
		{
			ClassDef **definitionLookup = ClassTable().CheckKey(className);
			ClassDef *definition = NULL;
			if(definitionLookup == NULL)
			{
				definition = new ClassDef();
				ClassTable()[className] = definition;
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

		/**
		 * Prints the implemented classes in a tree.  This is not designed to 
		 * be fast since it's debug information more than anything.
		 */
		static void				DumpClasses();

		static const ClassDef	*FindClass(unsigned int ednum);
		static const ClassDef	*FindClass(const FName &className);
		const ActionPtr			FindFunction(const FName &function) const;
		const Frame				*FindState(const FName &stateName) const;
		Symbol					*FindSymbol(const FName &symbol) const { return NULL; }
		static void				LoadActors();
		static void				UnloadActors();

	protected:
		static void	ParseActor(Scanner &sc);
		static void	ParseDecorateLump(int lumpNum);
		static bool	SetFlag(ClassDef *newClass, const char* flagName, bool set);
		static bool SetProperty(ClassDef *newClass, const char* propName, Scanner &sc);

		void		InstallStates(const TArray<StateDefinition> &stateDefs);

		// We need to do this for proper initialization order.
		static TMap<FName, ClassDef *>	&ClassTable();
		static TMap<int, ClassDef *>	classNumTable;

		FName			name;
		const ClassDef	*parent;

		TMap<FName, Frame *>	stateList;
		TArray<Frame *>			frameList;

		ActionTable		actions;

		AActor			*defaultInstance;
};

#endif /* __THINGDEF_H__ */
