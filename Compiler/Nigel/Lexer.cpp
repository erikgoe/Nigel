#include "stdafx.h"
#include "Lexer.h"

namespace nigel
{
	bool Lexer::isWhitespace( char c )
	{
		return c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == 0;//0 = eof
	}
	bool Lexer::isOperator( char c )
	{
		return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
			c == '<' || c == '>' || c == '=' ||
			c == '&' || c == '|' || c == '!' || c == '^' || c == '~';
	}
	bool Lexer::isNumber( char c )
	{
		return c >= 48 && c <= 57;
	}
	bool Lexer::isDividingToken( char c )
	{
		return c == '$' || c == '#' || c == '"' || c == '\'' ||
			c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' ||
			c == ',' || c == '.' || c == ':' || c == ';' ||
			c == '?' || c == '@' || c == '\\' || c == '`';
	}
	bool Lexer::isIdentifier( char c )
	{
		return !isWhitespace( c ) && !isOperator( c ) && !isNumber( c ) && !isDividingToken( c );
	}
	void Lexer::adoptToken( String &str, CodeBase &base )
	{
		if( !str.empty() && !isWhitespace( str.front() ) )
		{
			std::shared_ptr<Token> token;
			if( isOperator( str.front() ) )
			{
				token = std::make_shared<Token_Operator>( str );
			}
			else if( isNumber( str.front() ) )
			{
				token = std::make_shared<Token_NumberL>( stoi( str ) );
			}
			else if( isDividingToken( str.front() ) )
			{
				token = std::make_shared<Token_DividingToken>( str.front() );
			}
			else
			{
				token = std::make_shared<Token_Identifier>( str );
			}

			token->lineNo = currLineNo;
			token->columnNo = ( currLineNo == previousLineNo ? previousColumnNo : 1 );
			base.lexerStruct.push_back( token );
			previousLineNo = currLineNo;
			previousColumnNo = currColumnNo;
		}
		str.clear();
	}
	void Lexer::printLexerStructure( CodeBase & base )
	{
		log( "Lexer structure (" + to_string( base.lexerStruct.size() ) + " items): " );
		for( auto l : base.lexerStruct )
		{
			log( l->toString( true ) );
		}
		log( "Lexer end" );
	}
	Lexer::Lexer()
	{
	}

	ExecutionResult Lexer::onExecute( CodeBase &base )
	{
		size_t fileIndex = 0;
		u8 c, previousC = 0;
		String tmpStr;
		bool finish = false;
		currLineNo = 1;
		currColumnNo = 0;

		//Special values
		bool isNewline = false;//To enable lastWasNewline
		bool lastWasNewline = true;//Will be set true, if previous char was a newline. Default is true, because the first line will be handled as newline.
		bool isPP = false;//If is preprocessor directive
		bool isString = false;//If is string literal
		bool isLineComment = false;//If is comment from // untile lineend
		bool isMultiComment = false;//If is multiline-comment with /* */
		int multiCommentCount = 0;//Enables nested comments

		while( !finish )
		{//Iterate whole file content
			if( fileIndex >= base.fileCont.size() )
			{
				finish = true;
				c = 0;
			}
			else c = base.fileCont[fileIndex++];


			if( previousC == '\r' || c == '\n' )
			{//A newline
				currColumnNo = 0;
				currLineNo++;
			}
			if( previousC != '\r' || c != '\n' ) currColumnNo++;

			if( isString && c != '"' )
			{//Capture string literal
				tmpStr += c;
			}
			else if( isLineComment )
			{//Capture comment
				tmpStr += c;
				if( previousC == '\r' || c == '\n' )
				{//Comment finished
					if( previousC == '\r' ) tmpStr.pop_back();//Remove carriage return
					tmpStr.pop_back();//Remove newline

					base.lexerStruct.push_back( std::make_shared<Token_Comment>( tmpStr ) );
					tmpStr.clear();
					isLineComment = false;
				}
			}
			else if( isMultiComment )
			{//Capture comment
				tmpStr += c;
				if( previousC == '*' && c == '/' )
				{//Comment finished
					multiCommentCount--;
					if( multiCommentCount == 0 )
					{
						tmpStr.pop_back();//Remove *
						tmpStr.pop_back();//Remove /

						base.lexerStruct.push_back( std::make_shared<Token_Comment>( tmpStr ) );
						tmpStr.clear();
						isMultiComment = false;
					}
				}
				if( previousC == '/' && c == '*' ) multiCommentCount++;
			}


			else if( isWhitespace( c ) )
			{//Ignore and split token
				if( previousC == '\r' || c == '\n' )
				{ //Add eol for preprocessor
					if( isPP )
					{
						base.lexerStruct.push_back( std::make_shared<Token>( Token::Type::ppEnd ) );
						isPP = false;
					}
					isNewline = true;
				}
				if( !tmpStr.empty() )
				{//Finish previous token
					adoptToken( tmpStr, base );
				}
				previousColumnNo++;
			}
			else if( isOperator( c ) )
			{
				if( c == '/' && tmpStr == "/" )
				{//Found comment with //
					isLineComment = true;
					tmpStr.clear();
				}
				else if( c == '*' && tmpStr == "/" )
				{//Found comment with /*
					isMultiComment = true;
					multiCommentCount++;
					tmpStr.clear();
				}

				else if( isOperator( previousC ) || tmpStr.empty() ) tmpStr += c;
				else if( !tmpStr.empty() )
				{//Finish previous token
					adoptToken( tmpStr, base );
					tmpStr += c;
				}
			}
			else if( isNumber( c ) )
			{
				if( tmpStr.empty() || isNumber( previousC ) || isIdentifier( tmpStr.front() ) ) tmpStr += c;
				else if( !tmpStr.empty() )
				{//Finish previous token
					adoptToken( tmpStr, base );
					tmpStr += c;
				}
			}
			else if( isDividingToken( c ) && !isDividingToken( previousC ) )
			{
				if( c == '#' && lastWasNewline ) isPP = true;//Line is PP
				if( c == '"' )
				{
					if( isString )
					{//Admit string
						base.lexerStruct.push_back( std::make_shared<Token_StringL>( tmpStr ) );
						tmpStr.clear();
						isString = false;
					}
					else if( tmpStr.empty() ) isString = true;
				}
				else
				{
					if( isDividingToken( previousC ) || tmpStr.empty() ) tmpStr += c;
					else if( !tmpStr.empty() )
					{//Finish previous token
						adoptToken( tmpStr, base );
						tmpStr += c;
					}
				}
			}
			else
			{
				if( previousC == 0 || isIdentifier( previousC ) )
					tmpStr += c;
				else
				{//Finish previous token
					adoptToken( tmpStr, base );
					tmpStr += c;
				}
			}

			previousC = c;

			//Reset newline stuff
			if( isNewline )
			{
				lastWasNewline = true;
				isNewline = false;
			}
			else lastWasNewline = false;
		}
		base.lexerStruct.push_back( std::make_shared<Token>( Token::Type::eof ) );

		postLexer( base );

		printLexerStructure( base );

		return ExecutionResult::success;
	}
	ExecutionResult Lexer::postLexer( CodeBase &base )
	{
		std::shared_ptr<Token> tmp;
		for( auto token = base.lexerStruct.begin() ; token != base.lexerStruct.end() ; token++ )
		{
			if( ( *token )->type == Token::Type::identifier )
			{//Specify some tokens in more detail
				String identifier = ( *token )->as<Token_Identifier>()->identifier;

				if( identifier == "fn" )
				{
					tmp = std::make_shared<Token>( Token::Type::function );
					token->swap( tmp );
				}
				else if( identifier == "byte" )
				{
					tmp = std::make_shared<Token>( Token::Type::type_byte );
					token->swap( tmp );
				}
				else if( identifier == "int" )
				{
					tmp = std::make_shared<Token>( Token::Type::type_int );
					token->swap( tmp );
				}
				else if( identifier == "unsigned" )
				{
					tmp = std::make_shared<Token>( Token::Type::unsigned_type );
					token->swap( tmp );
				}
				else if( identifier == "if" )
				{
					tmp = std::make_shared<Token>( Token::Type::cf_if );
					token->swap( tmp );
				}
				else if( identifier == "else" )
				{
					tmp = std::make_shared<Token>( Token::Type::cf_else );
					token->swap( tmp );
				}
				else if( identifier == "while" )
				{
					tmp = std::make_shared<Token>( Token::Type::cf_while );
					token->swap( tmp );
				}
				else if( identifier == "for" )
				{
					tmp = std::make_shared<Token>( Token::Type::cf_for );
					token->swap( tmp );
				}
				else if( identifier == "do" )
				{
					tmp = std::make_shared<Token>( Token::Type::cf_do );
					token->swap( tmp );
				}
				else if( identifier == "return" )
				{
					tmp = std::make_shared<Token>( Token::Type::return_fn );
					token->swap( tmp );
				}
				else if( identifier == "new" )
				{
					tmp = std::make_shared<Token>( Token::Type::dy_new );
					token->swap( tmp );
				}
				else if( identifier == "delete" )
				{
					tmp = std::make_shared<Token>( Token::Type::dy_delete );
					token->swap( tmp );
				}
			}
			else if( ( *token )->type == Token::Type::operatorToken )
			{
				String identifier = ( *token )->as<Token_Operator>()->operatorToken;

				if( identifier.size() >= 2 )
				{
					if( ( identifier[1] != '=' ||
						( identifier[0] != '+' && identifier[0] != '-' && identifier[0] != '*' && identifier[0] != '/' && identifier[0] != '%' && identifier[0] != '=' && identifier[0] != '!' && identifier[0] != '<' && identifier[0] != '>' && identifier[0] != '&' && identifier[0] != '|' && identifier[0] != '^' ) ) &&
						identifier.substr(0, 2) != "&&" && identifier.substr( 0, 2 ) != "||" && identifier.substr( 0, 2 ) != "<<" && identifier.substr( 0, 2 ) != ">>" && identifier.substr( 0, 3 ) != "<<=" && identifier.substr( 0, 3 ) != ">>=" )
					{//Split operator
						tmp = std::make_shared<Token_Operator>( String(1, identifier[0]) );
						token->swap( tmp );
						tmp = std::make_shared<Token_Operator>( identifier.substr( 1 ) );
						base.lexerStruct.insert( token, tmp );
					}//todo: catch case of e. g. "&&-"
					else
					{//Define dual (or more) char operators
						if( identifier == "+=" )
						{
							tmp = std::make_shared<Token>( Token::Type::op_add_set );
							token->swap( tmp );
						}
						else if( identifier == "-=" )
						{
							tmp = std::make_shared<Token>( Token::Type::op_sub_set );
							token->swap( tmp );
						}
						else if( identifier == "*=" )
						{
							tmp = std::make_shared<Token>( Token::Type::op_mul_set );
							token->swap( tmp );
						}
						else if( identifier == "/=" )
						{
							tmp = std::make_shared<Token>( Token::Type::op_div_set );
							token->swap( tmp );
						}
						else if( identifier == "%=" )
						{
							tmp = std::make_shared<Token>( Token::Type::op_mod_set );
							token->swap( tmp );
						}
						else if( identifier == "==" )
						{
							tmp = std::make_shared<Token>( Token::Type::op_eql );
							token->swap( tmp );
						}
						else if( identifier == "!=" )
						{
							tmp = std::make_shared<Token>( Token::Type::op_not_eql );
							token->swap( tmp );
						}
						else if( identifier == "<=" )
						{
							tmp = std::make_shared<Token>( Token::Type::op_less_eql );
							token->swap( tmp );
						}
						else if( identifier == ">=" )
						{
							tmp = std::make_shared<Token>( Token::Type::op_more_eql );
							token->swap( tmp );
						}
						else if( identifier == "&=" )
						{
							tmp = std::make_shared<Token>( Token::Type::op_and_set );
							token->swap( tmp );
						}
						else if( identifier == "|=" )
						{
							tmp = std::make_shared<Token>( Token::Type::op_or_set );
							token->swap( tmp );
						}
						else if( identifier == "^=" )
						{
							tmp = std::make_shared<Token>( Token::Type::op_xor_set );
							token->swap( tmp );
						}
						else if( identifier == "&&" )
						{
							tmp = std::make_shared<Token>( Token::Type::op_and_log );
							token->swap( tmp );
						}
						else if( identifier == "||" )
						{
							tmp = std::make_shared<Token>( Token::Type::op_or_log );
							token->swap( tmp );
						}
						else if( identifier == "<<" )
						{
							tmp = std::make_shared<Token>( Token::Type::op_shift_left );
							token->swap( tmp );
						}
						else if( identifier == ">>" )
						{
							tmp = std::make_shared<Token>( Token::Type::op_shift_right );
							token->swap( tmp );
						}
						else if( identifier == "<<=" )
						{
							tmp = std::make_shared<Token>( Token::Type::op_shift_left_set );
							token->swap( tmp );
						}
						else if( identifier == ">>=" )
						{
							tmp = std::make_shared<Token>( Token::Type::op_shift_right_set );
							token->swap( tmp );
						}
					}
				}
				else
				{//Single char operators
					String identifier = ( *token )->as<Token_Operator>()->operatorToken;

					if( identifier == "+" )
					{
						tmp = std::make_shared<Token>( Token::Type::op_add );
						token->swap( tmp );
					}
					else if( identifier == "-" )
					{
						tmp = std::make_shared<Token>( Token::Type::op_sub );
						token->swap( tmp );
					}
					else if( identifier == "*" )
					{
						tmp = std::make_shared<Token>( Token::Type::op_mul );
						token->swap( tmp );
					}
					else if( identifier == "/" )
					{
						tmp = std::make_shared<Token>( Token::Type::op_div );
						token->swap( tmp );
					}
					else if( identifier == "%" )
					{
						tmp = std::make_shared<Token>( Token::Type::op_mod );
						token->swap( tmp );
					}
					else if( identifier == "=" )
					{
						tmp = std::make_shared<Token>( Token::Type::op_set );
						token->swap( tmp );
					}
					else if( identifier == "<" )
					{
						tmp = std::make_shared<Token>( Token::Type::op_less );
						token->swap( tmp );
					}
					else if( identifier == ">" )
					{
						tmp = std::make_shared<Token>( Token::Type::op_more );
						token->swap( tmp );
					}
					else if( identifier == "!" )
					{
						tmp = std::make_shared<Token>( Token::Type::op_not );
						token->swap( tmp );
					}
					else if( identifier == "&" )
					{
						tmp = std::make_shared<Token>( Token::Type::op_and );
						token->swap( tmp );
					}
					else if( identifier == "|" )
					{
						tmp = std::make_shared<Token>( Token::Type::op_or );
						token->swap( tmp );
					}
					else if( identifier == "^" )
					{
						tmp = std::make_shared<Token>( Token::Type::op_xor );
						token->swap( tmp );
					}
					else if( identifier == "~" )
					{
						tmp = std::make_shared<Token>( Token::Type::op_inv );
						token->swap( tmp );
					}
				}
			}
			else if( ( *token )->type == Token::Type::dividingToken )
			{
				u8 t = ( *token )->as<Token_DividingToken>()->token;

				if( t == '$' )
				{
					tmp = std::make_shared<Token>( Token::Type::tok_dollar );
					token->swap( tmp );
				}
				else if( t == '#' )
				{
					tmp = std::make_shared<Token>( Token::Type::tok_hash );
					token->swap( tmp );
				}
				else if( t == '(' )
				{
					tmp = std::make_shared<Token>( Token::Type::tok_parenthesis_open );
					token->swap( tmp );
				}
				else if( t == ')' )
				{
					tmp = std::make_shared<Token>( Token::Type::tok_parenthesis_close );
					token->swap( tmp );
				}
				else if( t == '[' )
				{
					tmp = std::make_shared<Token>( Token::Type::tok_bracket_open );
					token->swap( tmp );
				}
				else if( t == ']' )
				{
					tmp = std::make_shared<Token>( Token::Type::tok_bracket_close );
					token->swap( tmp );
				}
				else if( t == '{' )
				{
					tmp = std::make_shared<Token>( Token::Type::tok_brace_open );
					token->swap( tmp );
				}
				else if( t == '}' )
				{
					tmp = std::make_shared<Token>( Token::Type::tok_brace_close );
					token->swap( tmp );
				}
				else if( t == ',' )
				{
					tmp = std::make_shared<Token>( Token::Type::tok_comma );
					token->swap( tmp );
				}
				else if( t == '.' )
				{
					tmp = std::make_shared<Token>( Token::Type::tok_period );
					token->swap( tmp );
				}
				else if( t == ':' )
				{
					tmp = std::make_shared<Token>( Token::Type::tok_colon );
					token->swap( tmp );
				}
				else if( t == ';' )
				{
					tmp = std::make_shared<Token>( Token::Type::tok_semicolon );
					token->swap( tmp );
				}
				else if( t == '?' )
				{
					tmp = std::make_shared<Token>( Token::Type::tok_questionmark );
					token->swap( tmp );
				}
				else if( t == '@' )
				{
					tmp = std::make_shared<Token>( Token::Type::tok_at );
					token->swap( tmp );
				}
				else if( t == '\\' )
				{
					tmp = std::make_shared<Token>( Token::Type::tok_backslash );
					token->swap( tmp );
				}
				else if( t == '`' )
				{
					tmp = std::make_shared<Token>( Token::Type::tok_grave );
					token->swap( tmp );
				}
			}
			if( tmp != nullptr )
			{//Move also metadata. Use inverted order because of swap
				( *token )->lineNo = tmp->lineNo;
				( *token )->columnNo = tmp->columnNo;
			}
			tmp = nullptr;
		}

		return ExecutionResult::success;
	}
}
