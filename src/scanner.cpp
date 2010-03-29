//------------------------------------------------------------------------------
// scanner.h
//------------------------------------------------------------------------------
//
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301, USA.
//
//------------------------------------------------------------------------------
// Copyright (C) 2010 "Blzut3" <admin@maniacsvault.net>
//------------------------------------------------------------------------------

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <string>

#include "scanner.h"

using namespace std;

static const char* const TokenNames[TK_NumSpecialTokens] =
{
	"Identifier",
	"String Constant",
	"Integer Constant",
	"Float Constant",
	"Boolean Constant",
	"Logical And",
	"Logical Or",
	"Equals",
	"Not Equals",
	"Greater Than or Equals"
	"Less Than or Equals",
	"Left Shift",
	"Right Shift"
};

Scanner::Scanner(const char* data, int length) : line(1), lineStart(0), tokenLine(1), tokenLinePosition(0), scanPos(0), needNext(true)
{
	if(length == -1)
		length = strlen(data);
	this->length = length;
	this->data = new char[length];
	memcpy(this->data, data, length);

	CheckForWhitespace();
}

Scanner::~Scanner()
{
	delete[] data;
}

void Scanner::CheckForWhitespace()
{
	int comment = 0; // 1 = till next new line, 2 = till end block
	while(scanPos < length)
	{
		char cur = data[scanPos];
		char next = scanPos+1 < length ? data[scanPos+1] : 0;
		if(comment == 2)
		{
			if(cur != '*' || next != '/')
				scanPos++;
			else
			{
				comment = 0;
				scanPos += 2;
			}
			continue;
		}

		if(cur == ' ' || cur == '\t' || cur == 0)
			scanPos++;
		else if(cur == '\n' || cur == '\r')
		{
			scanPos++;
			if(comment == 1)
				comment = 0;

			// Do a quick check for Windows style new line
			if(cur == '\r' && next == '\n')
				scanPos++;
			IncrementLine();
		}
		else if(cur == '/' && comment == 0)
		{
			switch(next)
			{
				case '/':
					comment = 1;
					break;
				case '*':
					comment = 2;
					break;
				default:
					return;
			}
			scanPos += 2;
		}
		else
		{
			if(comment == 0)
				return;
			else
				scanPos++;
		}
	}
}

bool Scanner::CheckToken(char token)
{
	if(needNext)
	{
		if(!GetNextToken(false))
			return false;
	}

	// An int can also be a float.
	if(nextState.token == token || (nextState.token == TK_IntConst && token == TK_FloatConst))
	{
		needNext = true;
		ExpandState();
		return true;
	}
	needNext = false;
	return false;
}

void Scanner::ExpandState()
{
	str = nextState.str;
	number = nextState.number;
	decimal = nextState.decimal;
	boolean = nextState.boolean;
	token = nextState.token;
	tokenLine = nextState.tokenLine;
	tokenLinePosition = nextState.tokenLinePosition;
}

bool Scanner::GetNextString()
{
	if(scanPos >= length)
		return false;

	int start = scanPos;
	int end = scanPos;
	if(data[scanPos] == '"') // String Constant
	{
		end = ++start; // Remove starting quote
		scanPos++;
		while(scanPos < length)
		{
			char cur = data[scanPos];
			if(cur == '"')
				end = scanPos;
			else if(cur == '\\')
			{
				scanPos += 2;
				continue;
			}
			scanPos++;
			if(start != end)
				break;
		}
	}
	else // Unquoted string
	{
		while(scanPos < length)
		{
			char cur = data[scanPos];
			switch(cur)
			{
				default:
					break;
				case ' ':
				case '\t':
				case '\n':
				case '\r':
					end = scanPos;
					break;
			}
			if(start != end)
				break;
			scanPos++;
		}
	}
	if(end-start > 0)
	{
		string thisString(data+start, end-start);
		CheckForWhitespace();
		return true;
	}
	CheckForWhitespace();
	return false;
}

bool Scanner::GetNextToken(bool expandState)
{
	if(!needNext)
	{
		needNext = true;
		return true;
	}

	nextState.tokenLine = line;
	nextState.tokenLinePosition = scanPos - lineStart;
	if(scanPos >= length)
		return false;

	int start = scanPos;
	int end = scanPos;
	nextState.token = TK_NoToken;
	int integerBase = 10;
	bool floatHasDecimal = false;
	bool floatHasExponent = false;
	bool stringFinished = false; // Strings are the only things that can have 0 length tokens.

	char cur = data[scanPos++];
	// Determine by first character
	if(cur == '_' || (cur >= 'A' && cur <= 'Z') || (cur >= 'a' && cur <= 'z'))
		nextState.token = TK_Identifier;
	else if(cur >= '0' && cur <= '9')
	{
		if(cur == '0')
			integerBase = 8;
		nextState.token = TK_IntConst;
	}
	else if(cur == '.')
	{
		floatHasDecimal = true;
		nextState.token = TK_FloatConst;
	}
	else if(cur == '"')
	{
		end = ++start; // Move the start up one character so we don't have to trim it later.
		nextState.token = TK_StringConst;
	}
	else
	{
		end = scanPos;
		nextState.token = cur;

		// Now check for operator tokens
		if(scanPos < length)
		{
			char next = data[scanPos];
			if(cur == '&' && next == '&')
				nextState.token = TK_AndAnd;
			else if(cur == '|' && next == '|')
				nextState.token = TK_OrOr;
			else if(cur == '<' && next == '<')
				nextState.token = TK_ShiftLeft;
			else if(cur == '>' && next == '>')
				nextState.token = TK_ShiftRight;
			else if(next == '=')
			{
				switch(cur)
				{
					case '=':
						nextState.token = TK_EqEq;
						break;
					case '!':
						nextState.token = TK_NotEq;
						break;
					case '>':
						nextState.token = TK_GtrEq;
						break;
					case '<':
						nextState.token = TK_LessEq;
						break;
					default:
						break;
				}
			}

			if(nextState.token != cur)
			{
				scanPos++;
				end = scanPos;
			}
		}
	}

	if(start == end)
	{
		while(scanPos < length)
		{
			cur = data[scanPos];
			switch(nextState.token)
			{
				default:
					break;
				case TK_Identifier:
					if(cur != '_' && (cur < 'A' || cur > 'Z') && (cur < 'a' || cur > 'z') && (cur < '0' || cur > '9'))
						end = scanPos;
					break;
				case TK_IntConst:
					if(cur == '.' || (scanPos-1 != start && cur == 'e'))
						nextState.token = TK_FloatConst;
					else if((cur == 'x' || cur == 'X') && scanPos-1 == start)
					{
						integerBase = 16;
						break;
					}
					else
					{
						switch(integerBase)
						{
							default:
								if(cur < '0' || cur > '9')
									end = scanPos;
								break;
							case 8:
								if(cur < '0' || cur > '7')
									end = scanPos;
								break;
							case 16:
								if((cur < '0' || cur > '9') && (cur < 'A' || cur > 'F') && (cur < 'a' || cur > 'f'))
									end = scanPos;
								break;
						}
						break;
					}
				case TK_FloatConst:
					if(cur < '0' || cur > '9')
					{
						if(!floatHasDecimal && cur == '.')
						{
							floatHasDecimal = true;
							break;
						}
						else if(!floatHasExponent && cur == 'e')
						{
							floatHasDecimal = true;
							floatHasExponent = true;
							if(scanPos+1 < length)
							{
								char next = data[scanPos+1];
								if((next < '0' || next > '9') && next != '+' && next != '-')
									end = scanPos;
								else
									scanPos++;
							}
							break;
						}
						end = scanPos;
					}
					break;
				case TK_StringConst:
					if(cur == '"')
					{
						stringFinished = true;
						end = scanPos;
						scanPos++;
					}
					else if(cur == '\\')
						scanPos++; // Will add two since the loop automatically adds one
					break;
			}
			if(start == end && !stringFinished)
				scanPos++;
			else
				break;
		}
	}

	if(end-start > 0 || stringFinished)
	{
		nextState.str = string(data+start, end-start);
		if(nextState.token == TK_FloatConst)
		{
			nextState.decimal = atof(nextState.str.c_str());
			nextState.number = static_cast<int> (nextState.decimal);
			nextState.boolean = (nextState.number != 0);
		}
		else if(nextState.token == TK_IntConst)
		{
			nextState.number = strtol(nextState.str.c_str(), NULL, integerBase);
			nextState.decimal = nextState.number;
			nextState.boolean = (nextState.number != 0);
		}
		else if(nextState.token == TK_Identifier)
		{
			// Check for a boolean constant.
			if(nextState.str.compare("true") == 0)
			{
				nextState.token = TK_BoolConst;
				nextState.boolean = true;
			}
			else if(nextState.str.compare("false") == 0)
			{
				nextState.token = TK_BoolConst;
				nextState.boolean = false;
			}
		}
		else if(nextState.token == TK_StringConst)
		{
			Unescape(nextState.str);
		}
		if(expandState)
			ExpandState();
		CheckForWhitespace();
		return true;
	}
	CheckForWhitespace();
	return false;
}

void Scanner::IncrementLine()
{
	line++;
	lineStart = scanPos;
}

void Scanner::MustGetToken(char token)
{
	if(!CheckToken(token))
	{
		ExpandState();
		if(token < TK_NumSpecialTokens && this->token < TK_NumSpecialTokens)
			ScriptError("Expected '%s' but got '%s' instead.", TokenNames[token], TokenNames[this->token]);
		else if(token < TK_NumSpecialTokens && this->token >= TK_NumSpecialTokens)
			ScriptError("Expected '%s' but got '%c' instead.", TokenNames[token], this->token);
		else if(token >= TK_NumSpecialTokens && this->token < TK_NumSpecialTokens)
			ScriptError("Expected '%c' but got '%s' instead.", token, TokenNames[this->token]);
		else
			ScriptError("Expected '%c' but got '%c' instead.", token, this->token);
	}
}

void Scanner::ScriptError(const char* error, ...) const
{
	printf("%s\n", data+scanPos);
	char* newMessage = new char[strlen(error) + 20];
	sprintf(newMessage, "%d:%d:%s", GetLine(), GetLinePos(), error);
	va_list list;
	va_start(list, error);
	vfprintf(stderr, newMessage, list);
	va_end(list);
	exit(0);
}

bool Scanner::TokensLeft() const
{
	return scanPos < length;
}

// NOTE: Be sure that '\\' is the first thing in the array otherwise it will re-escape.
static char escapeCharacters[] = {'\\', '"', 'n', 0};
static char resultCharacters[] = {'\\', '"', '\n', 0};
const string &Scanner::Escape(string &str)
{
	for(unsigned int i = 0;escapeCharacters[i] != 0;i++)
	{
		// += 2 because we'll be inserting 1 character.
		for(size_t p = 0;p < str.length() && (p = str.find_first_of(escapeCharacters[i], p)) != string::npos;p += 2)
		{
			str.insert(p, 1, '\\');
		}
	}
	return str;
}
const string Scanner::Escape(const char *str)
{
	string tmp(str);
	Escape(tmp);
	return tmp;
}
const string &Scanner::Unescape(string &str)
{
	for(unsigned int i = 0;escapeCharacters[i] != 0;i++)
	{
		string sequence = string("\\").append(1, escapeCharacters[i]);
		for(size_t p = 0;p < str.length() && (p = str.find(sequence, p)) != string::npos;p++)
		{
			str.replace(str.find_first_of(sequence, p), 2, 1, resultCharacters[i]);
		}
	}
	return str;
}
