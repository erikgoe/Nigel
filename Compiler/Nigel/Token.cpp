#include "stdafx.h"
#include "Token.h"

namespace nigel
{
	String Token::toString( bool extended ) const
	{
		if( type == Token::Type::eof ) return "_!EOF!_";
		else if( type == Token::Type::ppEnd ) return "PP-END";
		else if( type == Token::Type::function ) return "fn";
		else if( type == Token::Type::type_byte ) return "byte";
		else if( type == Token::Type::type_int ) return "int";
		else if( type == Token::Type::unsigned_attr ) return "unsigned";
		else if( type == Token::Type::fast_attr ) return "fast";
		else if( type == Token::Type::norm_attr ) return "norm";
		else if( type == Token::Type::type_ptr ) return "ptr";
		else if( type == Token::Type::cf_if ) return "if";
		else if( type == Token::Type::cf_else ) return "else";
		else if( type == Token::Type::cf_while ) return "while";
		else if( type == Token::Type::cf_for ) return "for";
		else if( type == Token::Type::cf_do ) return "do";
		else if( type == Token::Type::cf_return ) return "return";
		else if( type == Token::Type::cf_break ) return "break";
		else if( type == Token::Type::dy_new ) return "new";
		else if( type == Token::Type::dy_delete ) return "delete";
		else if( type == Token::Type::literalTrue ) return "true";
		else if( type == Token::Type::literalFalse ) return "false";
		else if( type == Token::Type::operatorToken ) return as<Token_Operator>()->operatorToken + ( extended ? "\t\tas unknown operator" : "" );
		else if( type == Token::Type::op_add ) return "+";
		else if( type == Token::Type::op_sub ) return "-";
		else if( type == Token::Type::op_mul ) return "*";
		else if( type == Token::Type::op_div ) return "/";
		else if( type == Token::Type::op_mod ) return "%";
		else if( type == Token::Type::op_add_set ) return "+=";
		else if( type == Token::Type::op_sub_set ) return "-=";
		else if( type == Token::Type::op_mul_set ) return "*=";
		else if( type == Token::Type::op_div_set ) return "/=";
		else if( type == Token::Type::op_mod_set ) return "%=";
		else if( type == Token::Type::op_set ) return "=";
		else if( type == Token::Type::op_eql ) return "==";
		else if( type == Token::Type::op_not_eql ) return "!=";
		else if( type == Token::Type::op_more ) return ">";
		else if( type == Token::Type::op_less ) return "<";
		else if( type == Token::Type::op_more_eql ) return ">=";
		else if( type == Token::Type::op_less_eql ) return "<=";
		else if( type == Token::Type::op_shift_left ) return "<<";
		else if( type == Token::Type::op_shift_right ) return ">>";
		else if( type == Token::Type::op_shift_left_set ) return "<<=";
		else if( type == Token::Type::op_shift_right_set ) return ">>=";
		else if( type == Token::Type::op_not ) return "!";
		else if( type == Token::Type::op_and ) return "&";
		else if( type == Token::Type::op_and_set ) return "&=";
		else if( type == Token::Type::op_and_log ) return "&&";
		else if( type == Token::Type::op_or ) return "|";
		else if( type == Token::Type::op_or_set ) return "|=";
		else if( type == Token::Type::op_or_log ) return "||";
		else if( type == Token::Type::op_xor ) return "^";
		else if( type == Token::Type::op_xor_set ) return "^=";
		else if( type == Token::Type::op_inv ) return "~";
		else if( type == Token::Type::op_inc ) return "++";
		else if( type == Token::Type::op_dec ) return "--";
		else if( type == Token::Type::dividingToken ) return String( 1, as<Token_DividingToken>()->token ) + ( extended ? "\t\tas unknown divider token" : "" );
		else if( type == Token::Type::tok_dollar ) return "$";
		else if( type == Token::Type::tok_hash ) return "#";
		else if( type == Token::Type::tok_parenthesis_open ) return "(";
		else if( type == Token::Type::tok_parenthesis_close ) return ")";
		else if( type == Token::Type::tok_bracket_open ) return "[";
		else if( type == Token::Type::tok_bracket_close ) return "]";
		else if( type == Token::Type::tok_brace_open ) return "{";
		else if( type == Token::Type::tok_brace_close ) return "}";
		else if( type == Token::Type::tok_comma ) return ",";
		else if( type == Token::Type::tok_period ) return ".";
		else if( type == Token::Type::tok_colon ) return ":";
		else if( type == Token::Type::tok_semicolon ) return ";";
		else if( type == Token::Type::tok_questionmark ) return "?";
		else if( type == Token::Type::tok_at ) return "@";
		else if( type == Token::Type::tok_backslash ) return "\\";
		else if( type == Token::Type::tok_grave ) return "`";
		else return "-!-UNKNOWN-!-";
	}
	String Token_NumberL::toString( bool extended ) const
	{
		return to_string( number ) + ( extended ? "\t\tas number literal" : "" );
	}
	String Token_StringL::toString( bool extended ) const
	{
		return string + ( extended ? "\t\tas string literal" : "" );
	}
	String Token_Identifier::toString( bool extended ) const
	{
		return identifier + ( extended ? "\t\tas identifier" : "" );
	}
	String Token_Comment::toString( bool extended ) const
	{
		return comment + ( extended ? "\t\tas comment" : "" );
	}
}
