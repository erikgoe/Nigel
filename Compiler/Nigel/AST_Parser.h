#ifndef NIGEL_AST_PARSER_H
#define NIGEL_AST_PARSER_H

#include "BuilderTask.h"
#include "CompileNotification.h"

namespace nigel
{
	using NT = CompileNotification::Type;

		//Parser to generate a Abstract Syntax Tree (AST) from the lexer
	class AST_Parser : public BuilderExecutable
	{
		//Runtime stuff
		std::list<std::shared_ptr<Token>>::iterator currItr;//Next listitem for current file
		std::stack<std::shared_ptr<AstBlock>> blockStack;//Stack of blocks
		bool finishedParsing = false;

		CodeBase *base;
		std::shared_ptr<Token> currToken;
		std::shared_ptr<Token> lastToken;

		std::list<std::shared_ptr<CompileNotification>> notificationList;

		void generateNotification( NT error, Token token );
		std::shared_ptr<Token> next();
			//Skips all tokens until the semicolon
		void ignoreExpr();

			//Writes the AST into the console.
		void printAST( CodeBase &base );
		void printSubAST( std::shared_ptr<AstExpr> ast, size_t tabCount );

	public:
		AST_Parser();

		ExecutionResult onExecute( CodeBase &base ) override;

		std::shared_ptr<AstExpr> resolveNextExpr();

	};
}

#endif // !NIGEL_AST_PARSER_H
