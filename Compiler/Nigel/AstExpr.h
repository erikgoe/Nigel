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

			booleanParanthesis,
			ifStat,
			elseStat,
			whileStat,
			keywordCondition,
			arithmenticCondition,

			functionCall,
			returnStat,

			count
		} type;

		std::shared_ptr<Token> token;//Main ast-token

		bool isTypeReturnable() { return type == Type::variable || type == Type::term || type == Type::unary || type == Type::literal || type == Type::parenthesis || type == Type::functionCall; }
		bool isTypeCondition() { return type == Type::keywordCondition || type == Type::arithmenticCondition; }

		AstExpr( Type type ) : AstExpr::type(type) { }
		virtual ~AstExpr() {}

		template <typename T>
		std::shared_ptr<T> as()
		{
			return std::static_pointer_cast< T >( shared_from_this() );
		}
	};

	//Class deklaration
	class AstVariable;

	using VariableBinding = std::pair<String, std::shared_ptr<AstVariable>>;

		//Bock of linear executable expressions
	class AstBlock : public AstExpr
	{
	public:
		String name;//Block name
		std::list<std::shared_ptr<AstExpr>> content;
		std::list<std::pair<VariableBinding, size_t>> variables;//All available variables. Bindings mapped to their relative scope offset.
		std::list<VariableBinding> newVariables;//Variables which are declated in this block

		AstBlock() : AstExpr(AstExpr::Type::block) {}
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

		count
	};

		//Base class for a expression with a return value
	class AstReturning : public AstExpr
	{
	public:
		BasicType retType;

		String returnTypeString()
		{
			if( retType == tByte ) return "byte";
			else if( retType == tInt ) return "int";
			else if( retType == tUbyte ) return "unsigned byte";
			else if( retType == tUint ) return "unsigned int";
			else return "-!-UNKNOWN-!-";
		}

		AstReturning( AstExpr::Type type ) : AstExpr( type ) {}
		~AstReturning() {}
	};

		//A variable
	class AstVariable : public AstReturning
	{
	public:
		MemModel model = MemModel::large;
		String name;
		size_t scopeOffset = 0;//Offset of outer scope. Will be used in MemModel::stack.

		String modelString()
		{
			if( model == MemModel::fast ) return "fast";
			else if( model == MemModel::large ) return "large";
			else if( model == MemModel::stack ) return "stack";
			else if( model == MemModel::heap ) return "heap";
			else return "-!-UNKNOWN-!-";
		}

		AstVariable() : AstReturning( AstExpr::Type::variable ) {}
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
	};

		//A boolean block in parentheses
	class AstBooleanParenthesis : public AstCondition
	{
	public:
		std::shared_ptr<AstCondition> content = nullptr;

		AstBooleanParenthesis() : AstCondition( AstExpr::Type::booleanParanthesis ) {}
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

		//true/false-keyword
	class AstKeywordCondition : public AstCondition
	{
	public:
		bool val = true;

		AstKeywordCondition() : AstCondition( AstExpr::Type::keywordCondition ) {}
	};

		//Condition resolved from a returning value.
	class AstArithmeticCondition : public AstCondition
	{//todo
	public:
		std::shared_ptr<AstReturning> ret;

		AstArithmeticCondition() : AstCondition( AstExpr::Type::arithmenticCondition ) {}
	};
}

#endif // !NIGEL_AST_EXPR_H
