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
		enum class OperationType
		{
			v,//variable
			c,//constant
			t,//term

			count
		};

	private:
		CodeBase *base;
		std::stack<u32> breakableIDs;//Stack of breakable blocks.
		std::stack<u32> funcIDs;//Stack of functions.
		String currSymbol;//Symbol of the current function

		//Converts a binary operation to an unary operation.
		OperationType binaryToUnaryOperationType( OperationCombination comb );

		//Set the rValue to term. Useful for second operations e. g. '+='
		OperationCombination setRValTerm( OperationCombination comb );

		//Returns the size of a specific type
		u8 sizeOfType( BasicType type );

		//Writes the EIR into the console.
		void printEIR( CodeBase &base );

		//Writes the EIR as assembly into the console.
		void printAssembly( CodeBase &base );
		//Extracts a string from a operator
		String operatorToString( std::shared_ptr<EIR_Operator> op );

	public:

		EIR_Parser();

		ExecutionResult onExecute( CodeBase &base ) override;

		//Parses a single ast
		void parseAst( std::shared_ptr<AstExpr> ast, std::map<String, std::shared_ptr<EIR_Variable>> varList );

	private:

		//Parses a single condition ast
		void parseCondition( std::shared_ptr<AstExpr> ast, std::map<String, std::shared_ptr<EIR_Variable>> varList );

		//Generate a move operation.
		void generateSet( OperationCombination comb, std::shared_ptr<EIR_Operator> lOp, std::shared_ptr<EIR_Operator> rOp, std::shared_ptr<Token> lValToken, bool copyInAcc );

		//Generate a move operation after another operation. This will minimize the needed code.
		void generateSetAfterOp( OperationCombination comb, std::shared_ptr<EIR_Operator> lOp, std::shared_ptr<Token> lValToken );

		//Generate a arithmetic operation. \p op_val: hexcode for operation with a variable into acc, \p op_const: hexcode for operation with a constant into acc, \p op_atr0: hexcode for operation with a stack value (revered by r0) into acc.
		void generateOperation( OperationCombination comb, HexOp op_val, HexOp op_const, HexOp op_atr0, std::shared_ptr<EIR_Operator> lOp, std::shared_ptr<EIR_Operator> rOp, std::shared_ptr<Token> token );

		//Generate a move operation
		void EIR_Parser::generateMoveAB( OperationCombination comb, std::shared_ptr<EIR_Operator> lOp, std::shared_ptr<EIR_Operator> rOp, std::shared_ptr<Token> token );

		//Generate a move operation
		void EIR_Parser::generateMoveA( OperationType ot, std::shared_ptr<EIR_Operator> op );

		//Generate a unary left operation. \p op_acc: hexcode for operation in the acc. \p iterations defines the count of iterations on a operation.
		void generateUnaryLOperation( OperationType comb, HexOp op_acc, std::shared_ptr<EIR_Operator> op, std::shared_ptr<Token> token, bool changeValue = false, u16 iterations = 1 );

		//Generate a unary right operation. \p op_acc: hexcode for operation in the acc.
		void generateUnaryROperation( OperationType comb, HexOp op_acc, std::shared_ptr<EIR_Operator> op, std::shared_ptr<Token> token );

		//Resolve a SP-relative address and save it to r0
		void generateLoadStackR0( std::shared_ptr<EIR_Operator> op );

		//Resolve a SP-relative address and save it to r1
		void generateLoadStackR1( std::shared_ptr<EIR_Operator> op );


		//Generates a command
		static std::shared_ptr<EIR_Command> generateCmd( HexOp operation, std::shared_ptr<EIR_Operator> lOp = nullptr, std::shared_ptr<EIR_Operator> rOp = nullptr );

		//Generates and adds a command to the current commands list
		void addCmd( HexOp operation, std::shared_ptr<EIR_Operator> lOp = nullptr, std::shared_ptr<EIR_Operator> rOp = nullptr );
	};
}

#endif // !NIGEL_EIR_PARSER_H
