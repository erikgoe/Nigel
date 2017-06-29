#ifndef NIGEL_CODE_BASE_H
#define NIGEL_CODE_BASE_H

#include "Token.h"
#include "AstExpr.h"
#include "EIR_Command.h"

namespace nigel
{
		//Code base structure to enable execution in multiple building executions
	class CodeBase
	{
	public:
		//Instructions
		fs::path srcFile;//Input if only one file handled at once.
		fs::path destFile;//Output if only one file handled at once.


		//Generated data
		std::list<std::shared_ptr<Token>> lexerStruct;//List of tokens
		
		std::shared_ptr<AstBlock> globalAst;//Initial block of execution.

		std::list<std::shared_ptr<EIR_Command>> eirCommands;//List of commands represending the EIR.
		std::map<u32, std::shared_ptr<EIR_Variable>> eirValues;//Set of values id in the EIR, mapped to their id.

		std::vector<u8> hexBuffer;//Buffer with generated hex code
	};
}

#endif // !NIGEL_CODE_BASE_H
