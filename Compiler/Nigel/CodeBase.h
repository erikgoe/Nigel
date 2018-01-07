#ifndef NIGEL_CODE_BASE_H
#define NIGEL_CODE_BASE_H

#include "Token.h"
#include "AstExpr.h"
#include "IM_Command.h"

namespace nigel
{
		//Code base structure to enable execution in multiple building executions
	class CodeBase
	{
	public:
		//Instructions
		std::shared_ptr<fs::path> srcFile;//Input if only one file handled at once.
		std::shared_ptr<fs::path> destFile;//Output if only one file handled at once.

		//Flags
		MemModel memModel = MemModel::large;
		bool printLexer = false;
		bool printAST = false;
		bool printIMC = false;
		bool printAssembly = false;

		//Generated data
		class LineContent
		{
		public:
			std::shared_ptr<String> content;
			std::shared_ptr<fs::path> path;
			size_t line;
			LineContent() {}
			LineContent( std::shared_ptr<String> content, std::shared_ptr<fs::path> path, size_t line ) : content( content ), path( path ), line( line ) {}
		};
		std::vector<LineContent> fileCont;//Content from the pre-processed file, seperated in lines (content-lineNr-pairs)

		std::list<std::shared_ptr<Token>> lexerStruct;//List of tokens

		std::shared_ptr<AstBlock> globalAst;//Initial block of execution.

		std::list<std::shared_ptr<IM_Command>> imCommands;//List of commands represending the IMC.
		std::map<u32, std::shared_ptr<IM_Variable>> imValues; //Set of values id in the IMC, mapped to their id.

		std::vector<u8> hexBuffer;//Buffer with generated hex code
	};
}

#endif // !NIGEL_CODE_BASE_H
