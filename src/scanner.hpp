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

#ifndef __SCANNER_HPP__
#define __SCANNER_HPP__

#include <string>

typedef unsigned int UInt32;
typedef int Int32;

enum ETokenType
{
	TK_Identifier,	// Ex: SomeIdentifier
	TK_StringConst,	// Ex: "Some String"
	TK_IntConst,	// Ex: 27
	TK_FloatConst,	// Ex: 1.5
	TK_BoolConst,	// Ex: true
	TK_Void,		// void
	TK_String,		// str
	TK_Int,			// int
	TK_Float,		// float
	TK_Bool,		// bool
	TK_AndAnd,		// &&
	TK_OrOr,		// ||
	TK_EqEq,		// ==
	TK_NotEq,		// !=
	TK_GtrEq,		// >=
	TK_LessEq,		// <=
	TK_ShiftLeft,	// <<
	TK_ShiftRight,	// >>

	TK_UnquotedString, // For compatibility.

	TK_NoToken = -1,
};

/**
 * Scanner reads scripts by checking individual tokens.
 * @author Blzut3
 */
class Scanner
{
	public:
		Scanner(const char* data, UInt32 length);
		~Scanner();

		/**
		 * This function will scan the next token and return if it is of the 
		 * specified type.  If it returns true, then the position will be moved 
		 * to after the token, otherwise it will be reset.
		 */
		bool		CheckToken(char token);
		/**
		 * Gets whatever token is next.
		 */
		ETokenType	GetNextToken();
		/**
		 * Requires that the next token be of the specified type.  Errors will 
		 * be printed if that is not the case.
		 */
		void		MustGetToken(char token);
		/**
		 * Returns true if there is still more to read.
		 */
		bool		TokensLeft() { return (error != 0) || (pos < length); }
		void		ScriptError(const char* message, ...);

		std::string	str;
		UInt32		number;
		double		decimal;
		bool		boolean;
		char		lastToken;
	protected:
		/**
		 * Moves the position ahead any whitespace that there might be from the 
		 * current position.
		 */
		void		CheckForWhitespace(UInt32 *nPos = NULL, UInt32 *nLpos = NULL);
		void		GetToken(UInt32 &pos, UInt32 &lpos, UInt32 &line, char token, bool report=false);
		char*		GetNext(UInt32 &pos, UInt32 &lpos, char type, bool report=false);

		bool		error;
		char*		data;
		UInt32		length;
		UInt32		line;
		UInt32		lpos;
		UInt32		pos;

	private:
		char*		ret; // tmp variable.
};

#endif /* __SCANNER_HPP__ */
