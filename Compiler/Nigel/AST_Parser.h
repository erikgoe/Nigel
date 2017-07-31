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
		//Static stuff
		std::map<Token::Type, int> opPriority;//Priority of a operator. Higher nr is a higher priority.

		//Runtime stuff
		std::list<std::shared_ptr<Token>>::iterator currItr;//Next listitem for current file
		bool finishedParsing = false;
		std::stack<std::shared_ptr<AstBlock>> blockStack;//Stack of blocks
		std::shared_ptr<AstExpr> lValue;//Current lValue
		bool expectValue = false;//If a variable is expected. E. g. after operator
		bool expectBool = false;//If a boolean paranthesis is expected.
		bool blockHasHead = false;//If the following block has a head. E. g. a if block.
		bool previousDo = false;//If the previous was the do keyword.
		std::list<VariableBinding> functionParameters;//List of parameters (to extend the local variables)
		std::map<String, std::map<String, std::shared_ptr<AstFunction>>> functions;//All available functions. Symbols mapped to thier deklarations.
		size_t openParenthesisCount = 0;
		size_t openBraceCount = 0;

		CodeBase *base;
		std::shared_ptr<Token> currToken;
		std::shared_ptr<Token> lastToken;



		std::shared_ptr<Token> next();
			//Skips all tokens until the semicolon
		void ignoreExpr();

			//Writes the AST into the console.
		void printAST( CodeBase &base );
		void printSubAST( std::shared_ptr<AstExpr> ast, size_t tabCount );

	public:
		AST_Parser();

		ExecutionResult onExecute( CodeBase &base ) override;

	private:
		std::shared_ptr<AstExpr> resolveNextExpr();

		//todo try to template the following methods
		//Splits to the most right atomic expr with a same or higher priority from the lValue
		std::shared_ptr<AstReturning> splitMostRightExpr( std::shared_ptr<AstExpr> clVal, std::shared_ptr<AstTerm> cExpr, int priority );
		//Splits to the most right atomic expr with a same or higher priority from the lValue
		std::shared_ptr<AstReturning> splitMostRightExpr( std::shared_ptr<AstExpr> clVal, std::shared_ptr<AstUnary> cExpr, int priority );
		//Splits to the most right atomic expr with a same or higher priority from the lValue
		std::shared_ptr<AstCondition> splitMostRightExpr( std::shared_ptr<AstExpr> clVal, std::shared_ptr<AstCombinationCondition> cExpr, int priority );
		//Splits to the most right atomic expr with a same or higher priority from the lValue
		std::shared_ptr<AstReturning> splitMostRightExpr( std::shared_ptr<AstExpr> clVal, std::shared_ptr<AstComparisonCondition> cExpr, int priority );
		//Splits to the most right atomic expr with a same or higher priority from the lValue
		std::shared_ptr<AstReturning> splitMostRightExpr( std::shared_ptr<AstExpr> clVal, std::shared_ptr<AstFunctionCall> cExpr, int priority );

	};
}

#endif // !NIGEL_AST_PARSER_H
