#ifndef NIGEL_EIR_PARSER_H
#define NIGEL_EIR_PARSER_H

#include "BuilderTask.h"
#include "CompileNotification.h"

namespace nigel
{
	using NT = CompileNotification::Type;

		//Parser to translate the AST into the Early Instruction Representation (EIR).
	class EIR_Parser : public BuilderExecutable
	{

		CodeBase *base;

		std::list<std::shared_ptr<CompileNotification>> notificationList;

		//Writes the EIR into the console.
		void printEIR( CodeBase &base );

		void generateNotification( NT error, std::shared_ptr<Token> token );

	public:
		enum class OperationCombination
		{
			vv,//variable - variable
			vc,//variable - constant
			vt,//variable - term
			cv,//constant - variable
			cc,//constant - constant
			ct,//constant - term
			tv,//term     - variable
			tc,//term     - constant
			tt,//term     - term

			count
		};

		EIR_Parser();

		ExecutionResult onExecute( CodeBase &base ) override;

		//Parses a single ast
		void parseAst( std::shared_ptr<AstExpr> ast, std::map<String, std::shared_ptr<EIR_Variable>> varList );

		//Generates a command
		static std::shared_ptr<EIR_Command> generateCmd( HexOp operation, std::shared_ptr<EIR_Operator> lOp = nullptr, std::shared_ptr<EIR_Operator> rOp = nullptr );
	};
}

#endif // !NIGEL_EIR_PARSER_H
