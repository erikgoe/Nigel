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
			functionCall,
			returnStat,
			ifStat,
			whileStat,

			count
		} type;

		std::shared_ptr<Token> token;//Main ast-token

		bool isTypeReturnable() { return type == Type::variable || type == Type::term || type == Type::unary || type == Type::literal || type == Type::functionCall; }

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

		//Bock of linear executable expressions
	class AstBlock : public AstExpr
	{
	public:
		String name;//Block name
		std::list<std::shared_ptr<AstExpr>> content;
		std::map<String, std::shared_ptr<AstVariable>> variables;

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
		fast,
		large,

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



}

#endif // !NIGEL_AST_EXPR_H
