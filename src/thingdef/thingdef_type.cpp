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

#include "thingdef/thingdef_type.h"

Type::Type(const FName &name, const Type *parent) : parent(parent), status(FORWARD), name(name)
{
}

const Function *Type::LookupFunction(const FunctionPrototype &function) const
{
#if 0
	string argumentProtocol;
	for(int i = 0;i < function.GetArgCount();++i)
	{
		argumentProtocol += function.GetArgType(i).GetType()->GetName();
		if(i != function.GetArgCount()-1)
			argumentProtocol += ", ";
	}
	Print(ML_NOTICE, "Looking up function %s::%s(%s)", name.c_str(), function.GetName().c_str(), argumentProtocol.c_str());
#endif

	// Lookup the function.
	// ECWolf Note: Unlike ACC++ we don't have to deal with overloading.
	const Function *ret = functions.CheckKey(function.GetName());
	if(ret != NULL)
		return ret;
	return NULL;
}

bool Type::IsKindOf(const Type *other) const
{
	if(other == this)
		return true;
	if(parent != NULL)
		return parent->IsKindOf(parent);
	return false;
}

void Type::MakePrimitive()
{
	status = PRIMITIVE;
}

////////////////////////////////////////////////////////////////////////////////

TypeHierarchy::TypeHierarchy()
{
	static const char* primitives[NUM_TYPES] = {"void", "str", "char", "int", "fixed"};

	for(unsigned int i = 0;i < NUM_TYPES;++i)
	{
		primitiveTypes[i] = CreateType(primitives[i], NULL);
		primitiveTypes[i]->MakePrimitive();
	}
}

Type *TypeHierarchy::CreateType(const FName &name, const Type *parent)
{
	// Check for an already existing type.
	Type *existing = types.CheckKey(name);
	if(existing)
	{
		if(existing->IsForwardDeclared())
			return existing;
		else
			return NULL;
	}
	return &types.Insert(name, Type(name, parent));
}

const Type *TypeHierarchy::GetType(const FName &name) const
{
	return types.CheckKey(name);
}

////////////////////////////////////////////////////////////////////////////////

bool FunctionPrototype::operator==(const FunctionPrototype &other) const
{
	if(GetArgCount() != other.GetArgCount())
		return false;

	for(unsigned int i = 0;i < GetArgCount();++i)
	{
		if(other.GetArgType(i).GetType()->IsKindOf(GetArgType(i).GetType()))
			return false;
	}
	return GetName() == other.GetName();
}

////////////////////////////////////////////////////////////////////////////////

Function::Function(const TypeRef &returnType, const FunctionPrototype &function) :
	returnType(returnType), prototype(function)
{
}

bool Function::CheckPrototype(const FunctionPrototype &function) const
{
	return prototype == function;
}

////////////////////////////////////////////////////////////////////////////////

Symbol::Symbol(unsigned int local, Scope scope, const FName &name, const TypeRef &type) :
	name(name), isFunction(false), scope(scope), local(local), index(0),
	type(type), func(NULL)
{
}

Symbol::Symbol(unsigned int local, const FName &name, const Function *func) :
	name(name), isFunction(true), scope(GLOBAL), local(local), index(0),
	type(NULL), func(func)
{
}
