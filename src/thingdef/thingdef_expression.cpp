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

#include "thingdef/thingdef.h"
#include "thingdef/thingdef_expression.h"
#include "scanner.h"
#include "thingdef/thingdef_type.h"

unsigned int ExpressionNode::nextJumpPoint = 0;

static const struct ExpressionOperator
{
	unsigned char		token;
	unsigned char		density;
	const char* const	instruction;
	unsigned char		operands;
	const char* const	function;
	bool				reqSymbol; // Operate only on variables.
} operators[] =
{
	// List should start with a NOP and end with a NOP.
	{ '\0', 0, "", 0, "", false },

//	{ TK_Increment,		2,	"",					1,	"operator--",	true },
//	{ TK_Decrement,		2,	"",					1,	"operator++",	true },
	{ '+',				3,	"",					1,	"operator+",	false },
	{ '-',				3,	"unaryminus",		1,	"operator-",	false },
	{ '!',				3,	"negatelogical",	1,	"operator!",	false },
	{ '~',				3,	"negatebinary",		1,	"operator-",	false },
#define BINARYOPSTART 5
	{ '*',				5,	"multiply",			2,	"operator*",	false },
	{ '/',				5,	"divide",			2,	"operator/",	false },
	{ '%',				5,	"modulus",			2,	"operator%",	false },
	{ '+',				6,	"add",				2,	"operator+",	false },
	{ '-',				6,	"subtract",			2,	"operator-",	false },
	{ TK_ShiftLeft,		7,	"lshift",			2,	"operator<<",	false },
	{ TK_ShiftRight,	7,	"rshift",			2,	"operator>>",	false },
	{ '<',				8,	"lt",				2,	"operator<",	false },
	{ TK_LessEq,		8,	"le",				2,	"operator<=",	false },
	{ '>',				8,	"gt",				2,	"operator>",	false },
	{ TK_GtrEq,			8,	"ge",				2,	"operator>=",	false },
	{ TK_EqEq,			9,	"eq",				2,	"operator==",	false },
	{ TK_NotEq,			9,	"ne",				2,	"operator!=",	false },
	{ '&',				10,	"andbitwise",		2,	"operator&",	false },
	{ '^',				11,	"eorbitwise",		2,	"operator^",	false },
	{ '|',				12,	"orbitwise",		2,	"operator|",	false },
	{ TK_AndAnd,		13,	"andlogical",		2,	"operator&&",	false },
	{ TK_OrOr,			14,	"orlogical",		2,	"operator||",	false },
//	{ '?',				15,	"ifnotgoto",		3,	"",				false }, // Special handling
//	{ '=',				16,	"",					2,	"operator=",	true },
//	{ TK_AddEq,			16,	"",					2,	"operator+=",	true },
//	{ TK_SubEq,			16,	"",					2,	"operator-=",	true },
//	{ TK_MulEq,			16,	"",					2,	"operator*=",	true },
//	{ TK_DivEq,			16,	"",					2,	"operator/=",	true },
//	{ TK_ModEq,			16,	"",					2,	"operator%=" ,	true },
//	{ TK_ShiftLeftEq,	16,	"",					2,	"operator<<=",	true },
//	{ TK_ShiftRightEq,	16,	"",					2,	"operator>>=",	true },
//	{ TK_AndEq,			16,	"",					2,	"operator&=",	true },
//	{ TK_OrEq,			16,	"",					2,	"operator|=",	true },
//	{ TK_XorEq,			16,	"",					2,	"operator^=",	true },

	{ '\0', 255, "", 0, "", false }
};

ExpressionNode::ExpressionNode(ExpressionNode *parent) : op(&operators[0]), parent(parent), type(CONSTANT), classType(NULL)
{
	term[0] = term[1] = term[2] = NULL;
}

ExpressionNode::~ExpressionNode()
{
	for(unsigned char i = 0;i < 3;i++)
	{
		if(term[i] != NULL)
			delete term[i];
	}
}

bool ExpressionNode::CheckAssignment() const
{
	// If both sides are primitives, the left is a symbol and we are assigning,
	// we can simply call the instruction.  We need to do this since there is
	// no easy way to handle this in assembly.  Oh well...
	return type == SYMBOL && op->token == '=' && term[0] == NULL &&
		symbol->GetType()->IsPrimitive() && term[1]->GetType()->IsPrimitive() &&
		symbol->GetType() == term[1]->GetType();
}

#if 0
void ExpressionNode::DumpExpression(stringstream &out, std::string endLabel) const
{
	TypeRef argumentTypes[2];
	bool writeEndLabel = endLabel.empty();
	if(writeEndLabel)
		endLabel = GetJumpPoint();

	// [BL] Debug code to give a visual dump of the tree.
#if 0
	int level = 1;
	for(const ExpressionNode *tmp=this;tmp->parent != NULL;tmp = tmp->parent, level++);
	const Type *tmpType = GetType();
	Print(ML_NOTICE, "%s%s (%s)", string(level, '\t').c_str(), op->instruction, tmpType ? tmpType->GetName().c_str() : "");
#endif

	// Check if we have a value or term to push (left)
	if(term[0] == NULL)
	{
		if(type == CONSTANT)
			out << "pushnumber " << value << "\n";
		else if(type == SYMBOL)
		{
			// On an assignment operation we'll put the instruction after the
			// expression is evaluated.
			if(!CheckAssignment())
				out << symbol->PushSymbol() << "\n";
		}
		else
			out << "pushnumber " << identifier << "\n";
		argumentTypes[0] = TypeRef(GetType());
	}
	else
	{
		term[0]->DumpExpression(out, endLabel);
		argumentTypes[0] = TypeRef(term[0]->GetType());
	}

	// Short
	if(op->token == TK_AndAnd)
		out << "dup\nifnotgoto " << endLabel << "\n";
	else if(op->token == TK_OrOr)
		out << "dup\nifgoto " << endLabel << "\n";

	if(op->operands > 1)
	{
		if(op->operands == 3) // Ternary
		{
			string elsePoint = GetJumpPoint();
			string endPoint = writeEndLabel ? endLabel : GetJumpPoint();

			out << op->instruction << " " << elsePoint << "\n";
			if(term[1] != NULL)
				term[1]->DumpExpression(out, endLabel);
			out << "goto " << endPoint << "\n" << elsePoint << ":\n";
			if(term[2] != NULL)
				term[2]->DumpExpression(out, endLabel);
			out << endPoint << ":\n";
			return;
		}

		// right term
		if(term[1] != NULL)
		{
			term[1]->DumpExpression(out, endLabel);
			argumentTypes[1] = TypeRef(term[1]->GetType());
		}
	}

	// Print the instruction
	if(op->token != '\0')// && op->instruction[0] != '\0')
	{
		if(CheckAssignment())
		{
			// Check to see if we're doing a multiple assignment.
			if(parent != NULL && parent->CheckAssignment())
				out << "dup\n";
			out << symbol->AssignSymbol();
		}

		const Function *function = argumentTypes[0].GetType()->LookupFunction(FunctionPrototype(op->function, op->operands-1, argumentTypes+1));
		if(!function && argumentTypes[0].GetType()->IsPrimitive() && (op->operands <= 1 || argumentTypes[1].GetType()->IsPrimitive()))
			out << op->instruction << "\n";
		else // TODO: Stuff
			out << "callfunc\n";
	}

	if(writeEndLabel)
		out << endLabel << ":\n";
}
#endif

const Type *ExpressionNode::GetType() const
{
	bool isOp = op->token != '\0';
	const Function *function = NULL;

	TypeRef argumentTypes[2] = { TypeRef(classType), TypeRef(NULL) };
	if(isOp && op->operands > 1)
		argumentTypes[1] = TypeRef(term[1]->GetType());

	if(classType == NULL && term[0] != NULL)
	{
		argumentTypes[0] = TypeRef(term[0]->GetType());
	}
	if(isOp)
		function = argumentTypes[0].GetType()->LookupFunction(FunctionPrototype(op->function, op->operands-1, argumentTypes+1));
	return function ? function->GetReturnType().GetType() : argumentTypes[0].GetType();
}

FString ExpressionNode::GetJumpPoint()
{
	char pointName[14];
	sprintf(pointName, "Expr%09X", nextJumpPoint++);
	return pointName;
}

ExpressionNode *ExpressionNode::ParseExpression(const ClassDef *cls, TypeHierarchy &types, Scanner &sc, ExpressionNode *root, unsigned char opLevel)
{
	// We can't back out of our level in this recursion
	unsigned char initialLevel = opLevel;
	if(root == NULL)
		root = new ExpressionNode();

	ExpressionNode *thisNode = root;
	// We're going to basically alternate term/operator in this loop.
	bool awaitingTerm = true;
	do
	{
		if(awaitingTerm)
		{
			// Check for unary operators
			bool foundUnary = false;
			// We can check the density here since the only lower operators are
			// increment/decrement which I would think should always come before a variable so...
			for(unsigned char i = 1;operators[i].operands == 1 && operators[i].density <= opLevel;i++)
			{
				if(sc.CheckToken(operators[i].token))
				{
					foundUnary = true;
					thisNode->term[0] = new ExpressionNode(thisNode);
					thisNode->term[0]->op = &operators[i];
					ParseExpression(cls, types, sc, thisNode->term[0], operators[i].density);
					awaitingTerm = false;
					break;
				}
			}
			if(foundUnary)
				continue;

			// Remember parenthesis are just normal terms.  So recurse.
			if(sc.CheckToken('('))
			{
				thisNode->term[0] = new ExpressionNode(thisNode);
				ParseExpression(cls, types, sc, thisNode->term[0]);
				sc.MustGetToken(')');
			}
			else if(sc.CheckToken(TK_IntConst))
			{
				thisNode->classType = types.GetType(TypeHierarchy::INT);
				thisNode->value = sc->number;
			}
			else if(sc.CheckToken(TK_FloatConst))
			{
				thisNode->classType = types.GetType(TypeHierarchy::FIXED);
				thisNode->value = static_cast<int>(sc->decimal*65536);
			}
			else if(sc.CheckToken(TK_StringConst))
			{
				thisNode->type = STRING;
				thisNode->classType = types.GetType(TypeHierarchy::STRING);
				thisNode->str = sc->str;
			}
			else if(sc.CheckToken(TK_Identifier))
			{
				Symbol *symbol = cls->FindSymbol(sc->str);
				if(symbol == NULL)
					sc.ScriptMessage(Scanner::ERROR, "Undefined symbol `%s`.", sc->str.GetChars());
				thisNode->type = SYMBOL;
				thisNode->classType = symbol->GetType();
				thisNode->symbol = symbol;

				if(sc.CheckToken('['))
				{
					if(symbol->IsArray())
					{
						// TODO: Implement array subscripts
						ExpressionNode *node = new ExpressionNode(NULL);
						ParseExpression(cls, types, sc, node);
						delete node;
						sc.MustGetToken(']');
					}
					else
						sc.ScriptMessage(Scanner::ERROR, "Array subscript operator not yet supported.");
				}
			}
			else
				sc.ScriptMessage(Scanner::ERROR, "Expected expression term.");
			awaitingTerm = false;
		}
		else
		{
			// ternary operator support.
			if(thisNode->parent != NULL && thisNode->parent->op->operands == 3 && thisNode->parent->term[2] == NULL)
			{
				sc.MustGetToken(':');
				thisNode = thisNode->parent->term[2] = new ExpressionNode(thisNode);
				awaitingTerm = true;
			}
			else
			{
				// Go through the operators list until we either hit the end or an
				// operator that has too high of a priority (what I'm calling density)
				for(unsigned char i = BINARYOPSTART;operators[i].token != '\0';i++)
				{
					if(sc.CheckToken(operators[i].token))
					{
						if(operators[i].reqSymbol && thisNode->type != SYMBOL)
							sc.ScriptMessage(Scanner::ERROR, "Operation only valid on variables.");

						if(operators[i].density <= opLevel)
						{
							opLevel = operators[i].density;
							thisNode->op = &operators[i];
							thisNode = thisNode->term[1] = new ExpressionNode(thisNode);
							awaitingTerm = true;
						}
						else
						{
							// We need to relink our tree to fit the new node.
							ExpressionNode *newRoot = new ExpressionNode(root->parent);
							if(root->parent != NULL)
							{
								// Figure out if we were on the left or right side.
								root->parent->term[root->parent->term[1] == root ? 1 : 0] = newRoot;
							}
							root->parent = newRoot;
							newRoot->term[0] = root;
							newRoot->term[1] = new ExpressionNode(newRoot);
							newRoot->op = &operators[i];
							ParseExpression(cls, types, sc, newRoot->term[1], operators[i].density);
							root = newRoot;
						}
						break;
					}
				}
			}

			// Are we done with the recursion?
			if(!awaitingTerm)
				break;
		}
	}
	while(true);

	return root;
}
