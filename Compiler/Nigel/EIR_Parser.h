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

	private:
		CodeBase *base;

		//Writes the EIR into the console.
		void printEIR( CodeBase &base );

		//Generate operation.
		void generateSet( OperationCombination comb, std::shared_ptr<EIR_Operator> lOp, std::shared_ptr<EIR_Operator> rOp, std::shared_ptr<Token> lValtoken );

		//Generate operation. \p op_val: hexcode for operation with a variable into acc, \p op_val: hexcode for operation with a constant into acc, \p op_val: hexcode for operation with a register into acc.
		void generateOperation( OperationCombination comb, HexOp op_val, HexOp op_const, HexOp op_r0, std::shared_ptr<EIR_Operator> lOp, std::shared_ptr<EIR_Operator> rOp, std::shared_ptr<Token> token );

		//Generates a command
		static std::shared_ptr<EIR_Command> generateCmd( HexOp operation, std::shared_ptr<EIR_Operator> lOp = nullptr, std::shared_ptr<EIR_Operator> rOp = nullptr );
	public:

		EIR_Parser();

		ExecutionResult onExecute( CodeBase &base ) override;

		//Parses a single ast
		void parseAst( std::shared_ptr<AstExpr> ast, std::map<String, std::shared_ptr<EIR_Variable>> varList );

	};
}

#endif // !NIGEL_EIR_PARSER_H
