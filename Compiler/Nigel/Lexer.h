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

		size_t currLineNo = 1;
		size_t currColumnNo = 0;
		size_t previousLineNo = 0;//Line in the code file
		size_t previousColumnNo = 0;//Column in the code file
		std::shared_ptr<String> currLine;
		std::shared_ptr<fs::path> currPath;

		std::shared_ptr<Token> makeToken( std::shared_ptr<Token> token );

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
