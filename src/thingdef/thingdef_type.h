/*
** Copyright (c) 2010, Braden "Blzut3" Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * The names of its contributors may be used to endorse or promote
**       products derived from this software without specific prior written
**       permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT,
** INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __TYPE_H__
#define __TYPE_H__

#include "wl_def.h"
#include "name.h"
#include "tarray.h"
#include "zstring.h"

class Function;
class FunctionPrototype;
class TypeRef;
class TypeHierarchy;

class Type
{
	public:
		enum TypeStatus
		{
			FORWARD,
			STRUCTURE
		};

		const Function	*LookupFunction(const FunctionPrototype &function) const;
		const FName		&GetName() const { return name; }
		unsigned int	GetSize() const { return 1; }
		bool			IsForwardDeclared() const { return status == FORWARD; }
		bool			IsKindOf(const Type *other) const;

	protected:
		friend class TypeHierarchy;
		typedef TMap<FName, Function> FunctionMap;

		Type(const FName &name, const Type *parent);

		const Type	*parent;

		TypeStatus	status;
		FName		name;

		FunctionMap	functions;
};

class TypeRef
{
	public:
		TypeRef(const Type *type=NULL) : type(type) {}

		const Type	*GetType() const { return type; }
		bool		operator==(const TypeRef &other) const { return GetType() == other.GetType(); }
		bool		operator!=(const TypeRef &other) const { return GetType() != other.GetType(); }
	protected:
		const Type	*type;
};

class FunctionPrototype
{
	public:
		FunctionPrototype(FString name, unsigned int argc, const TypeRef* argt) : name(name), argc(argc), argt(argt) {}

		unsigned int	GetArgCount() const { return argc; }
		const TypeRef	&GetArgType(unsigned int i) const { return argt[i]; }
		const FName		&GetName() const { return name; }
		bool			operator==(const FunctionPrototype &other) const;
	protected:
		FName			name;
		unsigned int	argc;
		const TypeRef*	argt;
};

class Function
{
	public:
		Function(const TypeRef &returnType, const FunctionPrototype &function);

		bool			CheckPrototype(const FunctionPrototype &function) const;
		const TypeRef	&GetReturnType() const { return returnType; }

	protected:
		TypeRef				returnType;
		FunctionPrototype	prototype;
};

class TypeHierarchy
{
	public:
		static TypeHierarchy staticTypes;

		enum PrimitiveTypes
		{
			VOID,
			STRING,
			BOOL,
			INT,
			FLOAT,

			NUM_TYPES
		};

		TypeHierarchy();

		Type		*CreateType(const FName &name, const Type *parent);
		const Type	*GetType(PrimitiveTypes type) const;
		const Type	*GetType(const FName &name) const;

	protected:
		typedef TMap<FName, Type> TypeMap;

		TypeMap	types;
};

class Symbol
{
	public:
		enum Scope
		{
			LOCAL,		// Script
			MAP,		// Map
			WORLD,		// Hub
			GLOBAL,		// Game
			MAPARRAY,
			WORLDARRAY,
			GLOBALARRAY
		};

		Symbol(unsigned int local, Scope scope, const FName &name, const TypeRef &type);
		Symbol(unsigned int local, const FName &name, const Function *func);

		unsigned int	GetLocal() const { return local; }
		const Type		*GetType() const { return isFunction ? func->GetReturnType().GetType() : type.GetType(); }
		bool			IsArray() const { return !IsFunction() && scope >= MAPARRAY; }
		bool			IsFunction() const { return isFunction; }
	protected:
		FName			name;

		bool			isFunction;
		Scope			scope;
		unsigned int	local;
		unsigned int	index;
		TypeRef			type;
		const Function	*func;
};

#endif /* __TYPE_H__ */
