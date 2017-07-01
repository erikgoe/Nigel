#ifndef NIGEL_TOKEN_H
#define NIGEL_TOKEN_H

#include "stdafx.h"

namespace nigel
{
	//Defines a single token
	class Token : public std::enable_shared_from_this<Token>
	{
	public:
		enum class Type
		{
			eof,//End of file
			ppEnd,//end of preprocessor directive

			identifier,//e. g. variable name
			function,
			type_byte,
			type_int,
			unsigned_type,
			cf_if,
			cf_else,
			cf_while,
			cf_for,
			cf_do,
			return_fn,
			dy_new,
			dy_delete,

			literalN,
			literalS,

			operatorToken,//Any operator
			op_add,// +
			op_sub,// -
			op_mul,// *
			op_div,// /
			op_mod,// %
			op_add_set,// +=
			op_sub_set,// -=
			op_mul_set,// *=
			op_div_set,// /=
			op_mod_set,// %=
			op_set,// =
			op_eql,// ==
			op_not_eql,// !=
			op_less,// <
			op_more,// >
			op_less_eql,// <=
			op_more_eql,// >=
			op_shift_left,// <<
			op_shift_right,// >>
			op_shift_left_set,// <<=
			op_shift_right_set,// >>=
			op_not,// !
			op_and,// &
			op_and_set,// &=
			op_and_log,// &&
			op_or,// |
			op_or_set,// |=
			op_or_log,// ||
			op_xor,// ^
			op_xor_set,// ^=
			op_inv,// ~
			op_inc,// ++
			op_dec,// --

			dividingToken,//another divider like e. g. $
			tok_dollar,// $
			tok_hash,// #
			tok_parenthesis_open,// (
			tok_parenthesis_close,// )
			tok_bracket_open,// [
			tok_bracket_close,// ]
			tok_brace_open,// {
			tok_brace_close,// }
			tok_comma,// ,
			tok_period,// .
			tok_colon,// :
			tok_semicolon,// ;
			tok_questionmark,// ?
			tok_at,// @
			tok_backslash,// '\' 
			tok_grave,// `

			comment,

			count
		} type;

		size_t lineNo = 0;//Line in the code file
		size_t columnNo = 0;//Column in the code file
		std::shared_ptr<String> line;//Line of code (as text)
		std::shared_ptr<fs::path> path;//Path of the file


		Token( Type type )
		{
			this->type = type;
		}
		virtual ~Token() {}

		virtual String toString( bool extended = false ) const;

		template <typename T>
		std::shared_ptr<const T> as() const
		{
			return std::static_pointer_cast< const T >( shared_from_this() );
		}

			//Returns true if this token is a operator token
		bool isOperator() const
		{
			return type == Type::op_add || type == Type::op_sub || type == Type::op_mul || type == Type::op_div || type == Type::op_mod || type == Type::op_add_set || type == Type::op_sub_set || type == Type::op_mul_set || type == Type::op_div_set || type == Type::op_mod_set || type == Type::op_set || type == Type::op_eql || type == Type::op_not_eql ||
				type == Type::op_less || type == Type::op_more || type == Type::op_less_eql || type == Type::op_more_eql || type == Type::op_shift_left || type == Type::op_shift_right || type == Type::op_shift_left_set || type == Type::op_shift_right_set || type == Type::op_not || type == Type::op_and || type == Type::op_and_set || type == Type::op_and_log || type == Type::op_or || type == Type::op_or_set || type == Type::op_or_log || type == Type::op_xor || type == Type::op_xor_set || type == Type::op_inv;
		}
	};
	class Token_NumberL : public Token
	{
	public:
		int number;

		Token_NumberL( int number ) : Token( Type::literalN )
		{
			this->number = number;
		}

		String toString( bool extended ) const override;
	};
	class Token_StringL : public Token
	{
	public:
		String string;

		Token_StringL( String string ) : Token( Type::literalS )
		{
			this->string = string;
		}

		String toString( bool extended ) const override;
	};
	class Token_Identifier : public Token
	{
	public:
		String identifier;

		Token_Identifier( String identifier ) : Token( Type::identifier )
		{
			this->identifier = identifier;
		}

		String toString( bool extended ) const override;
	};
	//Objects of this class will only exist temporarily and are later translated into simple tokens.
	class Token_Operator : public Token
	{
	public:
		String operatorToken;

		Token_Operator( String operatorToken ) : Token( Type::operatorToken )
		{
			this->operatorToken = operatorToken;
		}
	};
	//Objects of this class will only exist temporarily and are later translated into simple tokens.
	class Token_DividingToken : public Token
	{
	public:
		u8 token;

		Token_DividingToken( char token ) : Token( Type::dividingToken )
		{
			this->token = token;
		}
	};
	class Token_Comment : public Token
	{
	public:
		String comment;

		Token_Comment( String comment ) : Token( Type::comment )
		{
			this->comment = comment;
		}

		String toString( bool extended ) const override;
	};

}

#endif // !NIGEL_TOKEN_H
