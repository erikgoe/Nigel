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
	std::shared_ptr<Token> Lexer::makeToken( std::shared_ptr<Token> token )
	{
		token->lineNo = currLineNo;
		token->columnNo = ( currLineNo == previousLineNo ? previousColumnNo : 1 );
		token->line = currLine;
		token->path = currPath;
		return token;
	}
	void Lexer::adoptToken( String &str, CodeBase &base )
	{
		if( !str.empty() && !isWhitespace( str.front() ) )
		{
			std::shared_ptr<Token> token;

			if( isNumber( str.front() ) || ( str.size() > 1 && ( str.front() == '-' || str.front() == '+' ) && isNumber( str[1] ) ) )
			{
				token = makeToken( std::make_shared<Token_NumberL>( stoi( str, 0, 0 ) ) );
			}
			else if( isOperator( str.front() ) )
			{
				token = makeToken( std::make_shared<Token_Operator>( str ) );
			}
			else if( isDividingToken( str.front() ) )
			{
				token = makeToken( std::make_shared<Token_DividingToken>( str.front() ) );
			}
			else
			{
				token = makeToken( std::make_shared<Token_Identifier>( str ) );
			}

			base.lexerStruct.push_back( token );
			previousLineNo = currLineNo;
			previousColumnNo = currColumnNo;
		}
		str.clear();
	}
	void Lexer::splitToken( String &identifier, size_t index, std::list<std::shared_ptr<Token>>::iterator &token, std::list<std::shared_ptr<Token>> &lexerStruct )
	{
		std::shared_ptr<Token> tmp1 = std::make_shared<Token_Operator>( identifier.substr( 0, index ) );
		std::shared_ptr<Token> tmp2 = std::make_shared<Token_Operator>( identifier.substr( index ) );

		tmp1->columnNo = ( *token )->columnNo;
		tmp1->lineNo = ( *token )->lineNo;
		tmp1->line = ( *token )->line;
		tmp1->path = ( *token )->path;
		tmp2->columnNo = ( *token )->columnNo + index;
		tmp2->lineNo = ( *token )->lineNo;
		tmp2->line = ( *token )->line;
		tmp2->path = ( *token )->path;

		identifier = tmp1->as<Token_Operator>()->operatorToken;
		lexerStruct.insert( token, tmp1 );
		token->swap( tmp2 );
		token--;
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
		size_t fileLine = 0;
		//size_t fileColumn = 0;

		u8 c, previousC = 0;
		String tmpStr;
		bool finish = false;
		currLineNo = 1;
		currColumnNo = 0;

		//Special values
		bool isString = false;//If is string literal
		bool isLineComment = false;//If is comment from // untile lineend
		bool isMultiComment = false;//If is multiline-comment with /* */
		int multiCommentCount = 0;//Enables nested comments

		while( !finish )
		{//Iterate whole file content
			while( fileLine < base.fileCont.size() && currColumnNo >= base.fileCont[fileLine].content->size() )
			{//Newline
				if( isLineComment )
				{
					base.lexerStruct.push_back( makeToken( std::make_shared<Token_Comment>( tmpStr ) ) );
					tmpStr.clear();
					isLineComment = false;
				}
				currColumnNo = 0;
				fileLine++;
				if( fileLine < base.fileCont.size() )
				{
					currLineNo = base.fileCont[fileLine].line;
					currLine = base.fileCont[fileLine].content;
					currPath = base.fileCont[fileLine].path;
				}
			}
			if( fileLine >= base.fileCont.size() )
			{
				finish = true;
				c = 0;
			}
			else c = base.fileCont[fileLine].content->at( currColumnNo++ );


			if( isString && ( c != '"' || previousC == '\\'  ) )
			{//Capture string literal
				if( previousC == '\\' )
				{
					if( c == '\\' ) tmpStr += '\\';//Backslash
					else if( c == 'n' ) tmpStr += '\n';//Newline
					else if( c == 'r' ) tmpStr += '\r';//Carriage return
					else if( c == 't' ) tmpStr += '\t';//Tabulator
					else if( c == 'v' ) tmpStr += '\v';//Vertical tabulator
					else if( c == 'b' ) tmpStr += '\b';//Backspace
					else if( c == 'f' ) tmpStr += '\f';//Form feed
					else if( c == 'a' ) tmpStr += '\a';//Alert
					else if( c == '?' ) tmpStr += '\?';//Question mark
					else if( c == '\'' ) tmpStr += '\'';//Single quote
					else if( c == '"' ) tmpStr += '\"';//Double quote
					else if( c == '0' ) tmpStr += '\0';//Null as value, not as character.
				}
				else if( c != '\\' ) tmpStr += c;
			}
			else if( isLineComment )
			{//Capture comment
				tmpStr += c;
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

						base.lexerStruct.push_back( makeToken( std::make_shared<Token_Comment>( tmpStr ) ) );
						tmpStr.clear();
						isMultiComment = false;
					}
				}
				if( previousC == '/' && c == '*' ) multiCommentCount++;
			}


			else if( isWhitespace( c ) )
			{//Ignore and split token
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
			else if( isNumber( c ) || ( c == 'x' && tmpStr == "0" ) )
			{
				if( tmpStr.empty() || isNumber( previousC ) || isIdentifier( tmpStr.front() ) || previousC == '0' ||
					( ( tmpStr == "-" || tmpStr == "+" ) && ( base.lexerStruct.empty() || base.lexerStruct.back()->type == Token::Type::operatorToken || base.lexerStruct.back()->type == Token::Type::dividingToken ) )
					) tmpStr += c;
				else if( !tmpStr.empty() )
				{//Finish previous token
					adoptToken( tmpStr, base );
					tmpStr += c;
				}
			}
			else if( isDividingToken( c ) && !isDividingToken( previousC ) )
			{
				if( c == '"' )
				{
					if( isString )
					{//Admit string
						base.lexerStruct.push_back( makeToken( std::make_shared<Token_StringL>( tmpStr ) ) );
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

			if( previousC != '\\' ) previousC = c;
			else previousC = 0;
		}
		base.lexerStruct.push_back( makeToken( std::make_shared<Token>( Token::Type::eof ) ) );

		postLexer( base );

		if( base.printLexer ) printLexerStructure( base );

		return ExecutionResult::success;
	}
	ExecutionResult Lexer::postLexer( CodeBase &base )
	{
		std::shared_ptr<Token> tmp, tmp2;
		std::list<std::list<std::shared_ptr<Token>>::iterator> toRemove;

		for( auto token = base.lexerStruct.begin() ; token != base.lexerStruct.end() ; token++ )
		{
			if( ( *token )->type == Token::Type::identifier )
			{//Specify some tokens in more detail
				String identifier = ( *token )->as<Token_Identifier>()->identifier;

				if( identifier == "_EOF_" )
				{
					tmp = std::make_shared<Token>( Token::Type::eof );
					token->swap( tmp );
				}
				else if( identifier == "fn" )
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
					tmp = std::make_shared<Token>( Token::Type::unsigned_attr );
					token->swap( tmp );
				}
				else if( identifier == "fast" )
				{
					tmp = std::make_shared<Token>( Token::Type::fast_attr );
					token->swap( tmp );
				}
				else if( identifier == "norm" )
				{
					tmp = std::make_shared<Token>( Token::Type::norm_attr );
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

				if( identifier.size() > 1 )
				{//May have to be splitted
					if( identifier.substr( 0, 3 ) == "<<=" || identifier.substr( 0, 3 ) == ">>=" )
					{
						if( identifier.size() > 3 )
						{//Split
							splitToken( identifier, 3, token, base.lexerStruct );
						}
					}
					else if( identifier.substr( 0, 2 ) == "&&" || identifier.substr( 0, 2 ) == "||" || identifier.substr( 0, 2 ) == "<<" || identifier.substr( 0, 2 ) == ">>" || identifier.substr( 0, 2 ) == ">=" || identifier.substr( 0, 2 ) == "<=" || identifier.substr( 0, 2 ) == "++" || identifier.substr( 0, 2 ) == "--" || identifier.substr( 0, 2 ) == "==" || identifier.substr( 0, 2 ) == "!=" || identifier.substr( 0, 2 ) == "+=" || identifier.substr( 0, 2 ) == "-=" || identifier.substr( 0, 2 ) == "*=" || identifier.substr( 0, 2 ) == "/=" || identifier.substr( 0, 2 ) == "%=" || identifier.substr( 0, 2 ) == "&=" || identifier.substr( 0, 2 ) == "|=" || identifier.substr( 0, 2 ) == "^=" )
					{
						if( identifier.size() > 2 )
						{//Split
							splitToken( identifier, 2, token, base.lexerStruct );
						}
					}

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
					else if( identifier == "++" )
					{
						tmp = std::make_shared<Token>( Token::Type::op_inc );
						token->swap( tmp );
					}
					else if( identifier == "--" )
					{
						tmp = std::make_shared<Token>( Token::Type::op_dec );
						token->swap( tmp );
					}
					else
					{//Split
						splitToken( identifier, 1, token, base.lexerStruct );
					}
				}
				if( identifier.size() == 1 )
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
				( *token )->line = tmp->line;
				( *token )->path = tmp->path;
			}
			tmp = nullptr;

			if( ( *token )->type == Token::Type::empty )
			{
				toRemove.push_back( token );
			}
		}

		//Remove empty token, if any
		for( auto &remove : toRemove )
		{
			base.lexerStruct.erase( remove );
		}

		return ExecutionResult::success;
	}
}
