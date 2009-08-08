// Emacs style mode select   -*- C++ -*-
// =============================================================================
// ### ### ##   ## ###  #   ###  ##   #   #  ##   ## ### ##  ### ###  #  ###
// #    #  # # # # #  # #   #    # # # # # # # # # # #   # #  #   #  # # #  #
// ###  #  #  #  # ###  #   ##   # # # # # # #  #  # ##  # #  #   #  # # ###
//   #  #  #     # #    #   #    # # # # # # #     # #   # #  #   #  # # #  #
// ### ### #     # #    ### ###  ##   #   #  #     # ### ##  ###  #   #  #  #
//                                     --= http://bitowl.com/sde/ =--
// =============================================================================
// Copyright (C) 2008 "Blzut3" (admin@maniacsvault.net)
// Copyright (C) 2008 GhostlyDeath (ghostlydeath@gmail.com)
// The SDE Logo is a trademark of GhostlyDeath (ghostlydeath@gmail.com)
// =============================================================================
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
// =============================================================================
// Description:
// =============================================================================

#include <string>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <cstdio>
#include <cstdarg>

#include "scanner.hpp"
#include "config.hpp"

using namespace std;

////////////////////////////////////////////////////////////////////////////////

Scanner::Scanner(const char* data, UInt32 length)
: number(0), decimal(0), boolean(false), lastToken(0), error(false), length(length), line(0), lpos(0), pos(0), ret(NULL)
{
	this->data = new char[length];
	memcpy(this->data, data, length);
	CheckForWhitespace();
}

Scanner::~Scanner()
{
	if(ret != NULL)
		delete[] ret;
	delete[] data;
}

void Scanner::MustGetToken(char token)
{
	GetToken(pos, lpos, line, token, true);
}

bool Scanner::CheckToken(char token)
{
	if(error)
		return false;
	UInt32 nPos = pos;
	UInt32 nLpos = lpos;
	UInt32 nLine = line;
	GetToken(nPos, nLpos, nLine, token, false);
	if(!error)
	{
		pos = nPos;
		lpos = nLpos;
		line = nLine;
		return true;
	}
	else
		error = false;
	return false;
}

ETokenType Scanner::GetNextToken()
{
	if(pos >= length)
		return TK_NoToken;

	if(data[pos] >= '0' && data[pos] <= '9')
	{
		if(CheckToken(TK_Identifier))
			return TK_Identifier;
		else if(CheckToken(TK_FloatConst))
		{
			double integerPart = 0.0;
			if(modf(decimal, &integerPart) != 0.0)
				return TK_FloatConst;
			else
			{
				number = static_cast<UInt32> (integerPart);
				return TK_IntConst;
			}
		}
		else if(CheckToken(TK_IntConst))
			return TK_IntConst;
	}
	else if(data[pos] >= 'a' && data[pos] <= 'z')
	{
		if(CheckToken(TK_BoolConst))
			return TK_BoolConst;
		else if(CheckToken(TK_Void))
			return TK_Void;
		else if(CheckToken(TK_String))
			return TK_String;
		else if(CheckToken(TK_Int))
			return TK_Int;
		else if(CheckToken(TK_Bool))
			return TK_Bool;
		else if(CheckToken(TK_Identifier))
			return TK_Identifier;
	}
	else if((data[pos] >= 'A' && data[pos] <= 'Z') || data[pos] == '_')
	{
		if(CheckToken(TK_Identifier))
			return TK_Identifier;
	}
	else if(data[pos] == '"')
	{
		if(CheckToken(TK_StringConst))
			return TK_StringConst;
	}
	else
	{
		if(CheckToken(TK_AndAnd))
			return TK_AndAnd;
		else if(CheckToken(TK_OrOr))
			return TK_OrOr;
		else if(CheckToken(TK_EqEq))
			return TK_EqEq;
		else if(CheckToken(TK_NotEq))
			return TK_NotEq;
		else if(CheckToken(TK_GtrEq))
			return TK_GtrEq;
		else if(CheckToken(TK_LessEq))
			return TK_LessEq;
		else if(CheckToken(TK_ShiftLeft))
			return TK_ShiftLeft;
		else if(CheckToken(TK_ShiftRight))
			return TK_ShiftRight;
	}
	if(CheckToken(data[pos]))
		return static_cast<ETokenType> (data[pos-1]);
	return TK_NoToken;
}

void Scanner::CheckForWhitespace(UInt32 *nPos, UInt32 *nLpos)
{
	UInt32 & uPos = (nPos ? (*nPos) : pos);
	UInt32 & uLpos = (nLpos ? (*nLpos) : lpos);
	while(data[uPos] == ' ' || data[uPos] == '\0' || data[uPos] == '	' || data[uPos] == '\n' || data[uPos] == '\r' ||
		(data[uPos] == '/' && uPos+1 < length && (data[uPos+1] == '/' || data[uPos+1] == '*')))
	{
		//comment
		if(data[uPos] == '/' && uPos+1 < length && (data[uPos+1] == '/' || data[uPos+1] == '*'))
		{
			uPos += 2;
			if(uPos == length)
				return;
			if(data[uPos-1] == '*') // multiline
			{
				while(true)
				{
					// Since we may be skipping end of lines lets check for them.
					if(data[uPos] == '\r' || data[uPos] == '\n')
					{
						if(data[uPos] == '\r' && uPos+1 < length && data[uPos+1] == '\n')
							uPos++;
						line++;
						uLpos = 0;
					}

					uPos++;
					if(data[uPos-1] == '*' && uPos < length && data[uPos] == '/')
						break;
					if(uPos == length)
						return;
				}
			}
			else // single line
			{
				// Go until the end of line is reached.
				while(data[uPos] != '\r' && data[uPos] != '\n')
				{
					uPos++;
					if(uPos == length)
						return;
				}
			}
		}

		if(data[uPos] == '\r' || data[uPos] == '\n')
		{
			if(data[uPos] == '\r' && uPos+1 < length && data[uPos+1] == '\n')
				uPos++;
			line++;
			uLpos = 0;
		}

		uPos++;
		uLpos++;
		if(uPos >= length)
			return;
	}
}

//For exmaple GENERIC_TOKEN(void) would do all the needed errors and such for looking for void
#define GENERIC_GETTOKEN(token) \
{ \
	char* ident = GetNext(pos, lpos, TK_Identifier, report); \
	if(ident != NULL) \
	{ \
		if(strcasecmp(ident, token) == 0) \
		{ \
			str = ident; \
		} \
		else \
		{ \
			error = true; \
			if(report) \
				{printf("Line %d:%d: Expected \"%s\", but got \"%s\" instead.\n", line, lpos, token, ident);exit(0);} \
		} \
	} \
	break; \
}
//For searching for symbols like == or <<
#define GENERIC_DOUBLETOKEN(token) \
{ \
	const char* tkn = token; \
	if(pos >= length-1) \
	{ \
		error = true; \
		if(report) \
			{printf("Line %d:%d: Expected \"%s\".\n", line, lpos, token);exit(0);} \
	} \
	if(data[pos] == tkn[0] && data[pos+1] == tkn[1]) \
	{ \
		str = token; \
		pos += 2; \
		lpos += 2; \
	} \
	else \
	{ \
		error = true; \
		if(report) \
			{printf("Line %d:%d: Expected \"%s\", but got \"%c%c\" instead.\n", line, lpos, token, data[pos], data[pos+1]);exit(0);} \
	} \
	break; \
}
void Scanner::GetToken(UInt32 &pos, UInt32 &lpos, UInt32 &line, char token, bool report)
{
	if(error)
		return;
	if(pos >= length)
	{
		if(report)
			{printf("Unexpected end of file.\n");exit(0);}
		return;
	}
	switch(token)
	{
		case TK_Identifier:
		{
			char* ident = GetNext(pos, lpos, TK_Identifier, report);
			if(ident != NULL)
			{
				str = ident;
			}
			break;
		}
		case TK_StringConst:
		{
			char* stringConst = GetNext(pos, lpos, TK_StringConst, report);
			if(stringConst != NULL)
			{
				str = stringConst;
			}
			break;
		}
		case TK_IntConst:
		{
			const char* integer = GetNext(pos, lpos, TK_IntConst, report);
			if(integer != NULL)
				number = atoi(integer);
			break;
		}
		case TK_FloatConst:
		{
			const char* integer = GetNext(pos, lpos, TK_FloatConst, report);
			if(integer != NULL)
				decimal = atof(integer);
			break;
		}
		case TK_BoolConst:
		{
			const char* ident = GetNext(pos, lpos, TK_Identifier, report);
			if(ident != NULL)
			{
				if(strcasecmp(ident, "true") == 0)
					boolean = true;
				else if(strcasecmp(ident, "false") == 0)
					boolean = false;
				else
				{
					error = true;
					if(report)
						{printf("Line %d:%d: Expected true/false, but got \"%s\" instead.\n", line, lpos, ident);exit(0);}
					break;
				}
			}
			break;
		}
		case TK_Void:
			GENERIC_GETTOKEN("void");
		case TK_String:
			GENERIC_GETTOKEN("str");
		case TK_Int:
			GENERIC_GETTOKEN("int");
		case TK_Float:
			GENERIC_GETTOKEN("float");
		case TK_Bool:
			GENERIC_GETTOKEN("bool");
		case TK_AndAnd:
			GENERIC_DOUBLETOKEN("&&");
		case TK_OrOr:
			GENERIC_DOUBLETOKEN("||");
		case TK_EqEq:
			GENERIC_DOUBLETOKEN("==");
		case TK_NotEq:
			GENERIC_DOUBLETOKEN("!=");
		case TK_GtrEq:
			GENERIC_DOUBLETOKEN(">=");
		case TK_LessEq:
			GENERIC_DOUBLETOKEN("<=");
		case TK_ShiftLeft:
			GENERIC_DOUBLETOKEN("<<");
		case TK_ShiftRight:
			GENERIC_DOUBLETOKEN(">>");
		default:
			if(data[pos] != token)
			{
				error = true;
				if(report)
					{printf("Line %d:%d: Expected '%c', but got '%c' instead.\n", line, lpos, token, data[pos]);exit(0);}
			}
			pos++;
			lpos++;
			break;
	}
	if(!error)
		lastToken = token;
	else
		lastToken = 0;

	CheckForWhitespace(&pos, &lpos);
}

//Find the next identifier by looping until we find an invalid character.
char* Scanner::GetNext(UInt32 &pos, UInt32 &lpos, char type, bool report)
{
	if(pos >= length)
		return NULL;
	UInt32 end = pos;
	bool special = false;
	bool special2 = false;
	for(UInt32 i = pos;i < length;i++)
	{
		if(type == TK_Identifier)
		{
			if(!((data[i] >= 'a' && data[i] <= 'z') || (data[i] >= 'A' && data[i] <= 'Z') ||
				data[i] == '_' || (i != pos && data[i] >= '0' && data[i] <= '9')))
			{
				break;
			}
		}
		else if(type == TK_StringConst)
		{
			if(!special) // Did we start the string?
			{
				if(data[i] != '"')
					break;
				special = true;
			}
			else
			{
				if(!special2 && data[i] == '"')
					break;
				if(data[i] == '\\')
					special2 = !special2;
				else
					special2 = false;
			}
		}
		else if(type == TK_IntConst)
		{
			if(!(data[i] >= '0' && data[i] <= '9'))
				break;
		}
		else
		{
			if(data[i] == '.')
			{
				if(special)
					break;
				special = true;
			}
			else if(!(data[i] >= '0' && data[i] <= '9'))
				break;
		}
		end = i+1;
	}
	if(end == pos)
	{
		error = true;
		if(report)
		{
			if(type == TK_Identifier)
				{printf("Line %d:%d: Expected an identifier, but got '%c' instead.\n", line, lpos, data[pos]);exit(0);}
			else if(type == TK_StringConst)
				{printf("Line %d:%d: Expected a string constant, but got '%c' instead.\n", line, lpos, data[pos]);exit(0);}
			else if(type == TK_IntConst)
				{printf("Line %d:%d: Expected an integer, but got '%c' instead.\n", line, lpos, data[pos]);exit(0);}
			else
				{printf("Line %d:%d: Expected a decimal value, but got '%c' instead.\n", line, lpos, data[pos]);exit(0);}
		}
		return NULL;
	}
	string result(data, pos, end - pos);
	//strip \\ and \" and cut out the opening quote
	if(type == TK_StringConst)
	{
		result = result.substr(1, result.length()-1);
		Config::Unescape(result);
	}
	result.append(1, '\0');
	lpos += end - pos;
	pos = end;
	if(type == TK_StringConst)
	{
		pos++;
		lpos++;
	}
	if(ret != NULL)
		delete[] ret;
	ret = new char[result.length() + 1];
	for(UInt32 ret_pos = 0; ret_pos < result.length(); ret_pos ++)
		ret[ret_pos] = result[ret_pos];
	ret[result.length()] = '\0';
	return ret;
}

void Scanner::ScriptError(const char* message, ...)
{
	printf("%d:%d:", line, lpos);

	va_list list;
	va_start(list, message);
	printf(message, list);
	va_end(list);

	exit(0);
}

