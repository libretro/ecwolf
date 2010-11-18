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

#ifndef __SCANNER_H__
#define __SCANNER_H__

#include <string>

enum
{
	TK_Identifier,	// Ex: SomeIdentifier
	TK_StringConst,	// Ex: "Some String"
	TK_IntConst,	// Ex: 27
	TK_FloatConst,	// Ex: 1.5
	TK_BoolConst,	// Ex: true
	TK_AndAnd,		// &&
	TK_OrOr,		// ||
	TK_EqEq,		// ==
	TK_NotEq,		// !=
	TK_GtrEq,		// >=
	TK_LessEq,		// <=
	TK_ShiftLeft,	// <<
	TK_ShiftRight,	// >>

	TK_NumSpecialTokens,

	TK_NoToken = -1
};

class Scanner
{
	public:
		Scanner(const char* data, int length=-1);
		~Scanner();

		void		CheckForWhitespace();
		bool		CheckToken(char token);
		void		ExpandState();
		int			GetLine() const { return tokenLine; }
		int			GetLinePos() const { return tokenLinePosition; }
		bool		GetNextString();
		bool		GetNextToken(bool expandState=true);
		void		MustGetToken(char token);
		void		ScriptError(const char* error, ...) const;
		bool		TokensLeft() const;

		static const std::string	&Escape(std::string &str);
		static const std::string	Escape(const char *str);
		static const std::string	&Unescape(std::string &str);

		std::string		str;
		unsigned int	number;
		double			decimal;
		bool			boolean;
		char			token;

	protected:
		void	IncrementLine();

	private:
		struct ParserState
		{
			std::string		str;
			unsigned int	number;
			double			decimal;
			bool			boolean;
			char			token;
			unsigned int	tokenLine;
			unsigned int	tokenLinePosition;

			unsigned int	rewindScanPos;
			unsigned int	rewindLine;
			unsigned int	rewindLineStart;
		}				nextState;

		char*			data;
		unsigned int	length;

		unsigned int	line;
		unsigned int	lineStart;
		unsigned int	tokenLine;
		unsigned int	tokenLinePosition;
		unsigned int	scanPos;

		bool			needNext; // If checkToken returns false this will be false.
};

#endif /* __SCANNER_H__ */
