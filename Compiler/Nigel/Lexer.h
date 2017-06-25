#ifndef NIGEL_LEXER_H
#define NIGEL_LEXER_H

#include "BuilderTask.h"

namespace nigel
{
		//Creates the LexerStructure from Sourcecode
	class Lexer : public BuilderExecutable
	{
		bool isWhitespace(char c);
		bool isOperator(char c);
		bool isNumber(char c);
		bool isDividingToken(char c);
		bool isIdentifier( char c );

		//Determine the token type and write it to the codebase.
		void adoptToken( String &str, CodeBase &base );

		//Writes the lexer structure into the console.
		void printLexerStructure( CodeBase &base );
	public:
		Lexer();

		ExecutionResult onExecute( CodeBase &base ) override;

		ExecutionResult postLexer( CodeBase &base );
	};
}

#endif // !NIGEL_LEXER_H
