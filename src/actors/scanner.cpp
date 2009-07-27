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
#include <cstdio>

#include "actors/scanner.hpp"

using namespace std;

////////////////////////////////////////////////////////////////////////////////

Scanner::Scanner(char* data, UInt32 length)
: str(NULL), number(0), decimal(0), boolean(false), lastToken(0), error(false), data(data), length(length), line(0), lpos(0), pos(0)
{
}

Scanner::~Scanner()
{
	if(str != NULL) {
		delete [] str;
		str = NULL;
	}
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
	GetToken(nPos, nLpos, nLine, token);
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

//For exmaple GENERIC_TOKEN(void) would do all the needed errors and such for looking for void
#define GENERIC_GETTOKEN(token) \
{ \
	char* ident = GetNext(pos, lpos, TK_Identifier, report); \
	if(ident != NULL) \
	{ \
		if(strcasecmp(ident, token) == 0) \
		{ \
			if(str != NULL) \
			{ \
				delete [] str; \
				str = NULL; \
			} \
			str = ident; \
		} \
		else \
		{ \
			error = true; \
			if(report) \
				printf("Line %d:%d: Expected \"%s\", but got \"%s\" instead.\n", line, lpos, token, ident); \
		} \
	} \
	break; \
}
//For searching for symbols like == or <<
#define GENERIC_DOUBLETOKEN(token) \
{ \
	const char* tkn = token; \
	if(pos == length-1) \
	{ \
		error = true; \
		if(report) \
			printf("Line %d:%d: Expected \"%s\".\n", line, lpos, token); \
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
			printf("Line %d:%d: Expected \"%s\", but got \"%c%c\" instead.\n", line, lpos, token, data[pos], data[pos+1]); \
	} \
	break; \
}
void Scanner::GetToken(UInt32 &pos, UInt32 &lpos, UInt32 &line, char token, bool report)
{
	if(error)
		return;
	if(pos == length)
	{
		if(report)
			printf("Unexpected end of file.\n");
		return;
	}
	switch(token)
	{
		case TK_Identifier:
		{
			char* ident = GetNext(pos, lpos, TK_Identifier, report);
			if(ident != NULL)
			{
				if(str != NULL)
				{
					delete [] str;
					str = NULL;
				}
				str = ident;
			}
			break;
		}
		case TK_StringConst:
		{
			char* stringConst = GetNext(pos, lpos, TK_StringConst, report);
			if(stringConst != NULL)
			{
				if(str != NULL)
				{
					delete [] str;
					str = NULL;
				}
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
						printf("Line %d:%d: Expected true/false, but got \"%s\" instead.\n", line, lpos, ident);
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
					printf("Line %d:%d: Expected '%c', but got '%c' instead.\n", line, lpos, token, data[pos]);
			}
			pos++;
			lpos++;
			break;
	}
	if(!error)
		lastToken = token;
	else
		lastToken = 0;
	while(data[pos] == ' ' || data[pos] == '\0' || data[pos] == '	' || data[pos] == '\n' || data[pos] == '\r')
	{
		if(data[pos] == '\r' || data[pos] == '\n')
		{
			if(data[pos] == '\r' && pos+1 < length && data[pos+1] == '\n')
				pos++;
			line++;
			lpos = 0;
		}
		lpos++;
		pos++;
		if(pos == length)
			return;
	}
}

//Find the next identifier by looping until we find an invalid character.
char* Scanner::GetNext(UInt32 &pos, UInt32 &lpos, char type, bool report)
{
	if(pos == length)
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
				printf("Line %d:%d: Expected an identifier, but got '%c' instead.\n", line, lpos, data[pos]);
			else if(type == TK_StringConst)
				printf("Line %d:%d: Expected a string constant, but got '%c' instead.\n", line, lpos, data[pos]);
			else if(type == TK_IntConst)
				printf("Line %d:%d: Expected an integer, but got '%c' instead.\n", line, lpos, data[pos]);
			else
				printf("Line %d:%d: Expected a decimal value, but got '%c' instead.\n", line, lpos, data[pos]);
		}
		return NULL;
	}
	string result(data, pos, end - pos);
	//strip \\ and \" and cut out the opening quote
	if(type == TK_StringConst)
	{
		result = result.substr(1, result.length()-1);
		while(result.find("\\\\") != result.npos)
			result.replace(result.find("\\\\"), 2, 1, '\\');
		while(result.find("\\\"") != result.npos)
			result.replace(result.find("\\\""), 2, 1, '"');
	}
	result.append(1, '\0');
	lpos += end - pos;
	pos = end;
	if(type == TK_StringConst)
	{
		pos++;
		lpos++;
	}

	char * ret = new char[result.length() + 1];
	for(UInt32 ret_pos = 0; ret_pos < result.length(); ret_pos ++)
		ret[ret_pos] = result[ret_pos];
	ret[result.length()] = '\0';
	return ret;
}
