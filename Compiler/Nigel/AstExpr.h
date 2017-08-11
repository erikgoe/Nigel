#ifndef NIGEL_AST_EXPR_H
#define NIGEL_AST_EXPR_H

#include "Token.h"

namespace nigel
{
		//Base class for AST-expressions
	class AstExpr : public std::enable_shared_from_this<AstExpr>
	{
	public:
		const enum class Type
		{
			block,
			variable,
			term,
			unary,
			allocation,
			literal,
			parenthesis,

			booleanParenthesis,
			ifStat,
			elseStat,
			whileStat,
			keywordCondition,
			arithmenticCondition,
			comparisonCondition,
			combinationCondition,

			breakStat,

			functionDefinition,
			functionCall,
			returnStat,

			refer,
			derefer,

			count
		} type;

		std::shared_ptr<Token> token;//Main ast-token

		bool isTypeReturnable() { return type == Type::variable || type == Type::term || type == Type::unary || type == Type::literal || type == Type::parenthesis || type == Type::functionCall || type == Type::refer || type == Type::derefer; }
		bool isTypeCondition() { return type == Type::booleanParenthesis || type == Type::keywordCondition || type == Type::arithmenticCondition || type == Type::comparisonCondition || type == Type::combinationCondition; }

		AstExpr( Type type ) : type(type) { }
		virtual ~AstExpr() {}

		template <typename T>
		std::shared_ptr<T> as()
		{
			return std::static_pointer_cast< T >( shared_from_this() );
		}
	};

	//Class deklarations
	class AstVariable;
	class AstFunction;

	using VariableBinding = std::pair<String, std::shared_ptr<AstVariable>>;

		//Bock of linear executable expressions
	class AstBlock : public AstExpr
	{
		static u32 nextID;//Enables creation of new variables.

	public:
		u32 id;//Unique id for this block.
		std::list<std::shared_ptr<AstExpr>> content;
		std::list<std::pair<VariableBinding, size_t>> variables;//All available variables. Bindings mapped to their relative scope offset.
		std::list<VariableBinding> newVariables;//Variables which are declated in this block
		std::list<VariableBinding> parameters;//Variables which come into the block as parameters

		AstBlock() : AstExpr( AstExpr::Type::block )
		{
			id = nextID++;
		}
	};

		//Basic types of nigel
	enum BasicType
	{
		unknown,
		tByte,
		tInt,
		tUbyte,
		tUint,

		count
	};
		//Defines where a variable will be saved.
	enum class MemModel
	{
		fast,//Globals in intern ram
		large,//Globals in extern ram
		stack,//Locals on stack
		heap,//Dynamically allocated
		param,//Parameter similar to stack

		count
	};

		//Base class for a expression with a return value
	class AstReturning : public AstExpr
	{
	public:
		BasicType retType;

		static String returnTypeString( BasicType type )
		{
			if( type == tByte ) return "byte";
			else if( type == tInt ) return "int";
			else if( type == tUbyte ) return "unsigned byte";
			else if( type == tUint ) return "unsigned int";
			else return "-!-UNKNOWN-!-";
		}

		String returnTypeString()
		{
			return returnTypeString( retType );
		}

		AstReturning( AstExpr::Type type ) : AstExpr( type ) {}
		virtual ~AstReturning() {}
	};

		//A variable
	class AstVariable : public AstReturning
	{
	public:
		MemModel model = MemModel::large;
		String name;
		size_t scopeOffset = 0;//Offset of outer scope. Will be used in MemModel::stack.
		u8 predefinedAddress = 0;//Used for SFRs

		String modelString()
		{
			if( model == MemModel::fast ) return "fast";
			else if( model == MemModel::large ) return "large";
			else if( model == MemModel::stack ) return "stack";
			else if( model == MemModel::heap ) return "heap";
			else if( model == MemModel::param ) return "param";
			else return "-!-UNKNOWN-!-";
		}

		AstVariable() : AstReturning( AstExpr::Type::variable ) {}
		AstVariable( MemModel model, String name, u8 address, BasicType type ) : AstReturning( AstExpr::Type::variable )
		{
			this->model = model;
			this->name = name;
			predefinedAddress = address;
			retType = type;
		}
	};

		//Term expression
	class AstTerm : public AstReturning
	{
	public:
		std::shared_ptr<AstReturning> lVal;
		std::shared_ptr<AstReturning> rVal;
		Token::Type op;

		AstTerm() : AstReturning( AstExpr::Type::term ) {}
	};

		//Unary expression. Will be handled as term.
	class AstUnary : public AstReturning
	{
	public:
		enum Side
		{
			left,
			right
		};
		Side side;
		std::shared_ptr<AstReturning> val;
		Token::Type op;

		AstUnary() : AstReturning( AstExpr::Type::unary ) {}
	};

		//Allocation of a variable
	class AstAllocation : public AstReturning
	{
	public:
		std::shared_ptr<AstVariable> lVal;

		AstAllocation() : AstReturning( AstExpr::Type::allocation ) {}
	};

		//Any literal
	class AstLiteral : public AstReturning
	{
	public:
		std::shared_ptr<Token> token = nullptr;

		AstLiteral() : AstReturning( AstExpr::Type::literal ) {}
	};

		//A arithmetic block in parentheses
	class AstParenthesis : public AstReturning
	{
	public:
		std::shared_ptr<AstReturning> content = nullptr;

		AstParenthesis() : AstReturning( AstExpr::Type::parenthesis ) {}
	};


		//A base class for boolean expressions which can be resolved to true or false.
	class AstCondition : public AstExpr
	{
	public:
		AstCondition( AstExpr::Type type ) : AstExpr( type ) {}
		virtual ~AstCondition() {}
	};

		//A boolean block in parentheses
	class AstBooleanParenthesis : public AstCondition
	{
	public:
		std::shared_ptr<AstCondition> content = nullptr;

		AstBooleanParenthesis() : AstCondition( AstExpr::Type::booleanParenthesis ) {}
	};

		//A construct to constrol the flow of the program
	class AstIf : public AstExpr
	{
	public:
		std::shared_ptr<AstBooleanParenthesis> condition;
		std::shared_ptr<AstBlock> ifCase;
		std::shared_ptr<AstBlock> elseCase;

		AstIf() : AstExpr( AstExpr::Type::ifStat ) {}
	};

		//A construct to loop a chain of expressioms
	class AstWhile : public AstExpr
	{
	public:
		std::shared_ptr<AstBooleanParenthesis> condition;
		std::shared_ptr<AstBlock> block;
		bool isDoWhile = false;

		AstWhile() : AstExpr( AstExpr::Type::whileStat ) {}
	};

		//true/false-keyword
	class AstKeywordCondition : public AstCondition
	{
	public:
		bool val = true;

		AstKeywordCondition() : AstCondition( AstExpr::Type::keywordCondition ) {}
	};

		//Condition resolved from a arithmetic expression.
	class AstArithmeticCondition : public AstCondition
	{
	public:
		std::shared_ptr<AstReturning> ret;

		AstArithmeticCondition() : AstCondition( AstExpr::Type::arithmenticCondition ) {}
	};

		//Boolean comparison
	class AstComparisonCondition : public AstCondition
	{
	public:
		std::shared_ptr<AstReturning> lVal;
		std::shared_ptr<AstReturning> rVal;
		Token::Type op;

		AstComparisonCondition() : AstCondition( AstExpr::Type::comparisonCondition ) {}
	};

		//Boolean combination
	class AstCombinationCondition : public AstCondition
	{
	public:
		std::shared_ptr<AstCondition> lVal;
		std::shared_ptr<AstCondition> rVal;
		Token::Type op;

		AstCombinationCondition() : AstCondition( AstExpr::Type::combinationCondition ) {}
	};


		//A function definition
	class AstFunction : public AstExpr
	{
	public:
		String symbol;
		std::list<VariableBinding> parameters;//All available variables.
		BasicType retType;
		std::shared_ptr<AstBlock> content;

		AstFunction() : AstExpr( AstExpr::Type::functionDefinition ) {}
	};

		//A function call
	class AstFunctionCall : public AstReturning
	{
	public:
		String symbol;
		std::list<std::shared_ptr<AstReturning>> parameters;

		AstFunctionCall() : AstReturning( AstExpr::Type::functionCall ) {}
	};

		//Return statement
	class AstReturnStatement : public AstExpr
	{
	public:
		std::shared_ptr<AstReturning> expr;

		AstReturnStatement() : AstExpr( AstExpr::Type::returnStat ) {}
	};

		//Get the address of a variable
	class AstRefer : public AstReturning
	{
	public:
		std::shared_ptr<AstVariable> var;

		AstRefer() : AstReturning( AstExpr::Type::refer ) {}
	};

		//Get the address of a variable
	class AstDerefer : public AstReturning
	{
	public:
		std::shared_ptr<AstReturning> expr;

		AstDerefer() : AstReturning( AstExpr::Type::derefer ) {}
	};

}

#endif // !NIGEL_AST_EXPR_H
