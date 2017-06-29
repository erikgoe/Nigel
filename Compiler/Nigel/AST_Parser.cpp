#include "stdafx.h"
#include "AST_Parser.h"

namespace nigel
{
	using TT = Token::Type;

	void AST_Parser::generateNotification( NT error, std::shared_ptr<Token> token )
	{
		notificationList.push_back( std::make_shared<CompileNotification>( error, token, base->srcFile ) );
	}

	std::shared_ptr<Token> AST_Parser::next()
	{
		if( currItr == base->lexerStruct.end() ) {
			generateNotification( NT::err_reachedEOF_unfinishedExpression, currToken );
			return nullptr;
		}
		lastToken = currToken;
		currToken = *(currItr++);
		return currToken;
	}

	void AST_Parser::ignoreExpr()
	{
		while( next()->type != TT::tok_semicolon );
		lValue = nullptr;
		while( !exprStack.empty() ) exprStack.pop();//todo check
	}

	void AST_Parser::printAST( CodeBase & base )
	{
		log( "AST:" );

		printSubAST( base.globalAst, 0 );

		log( "AST end" );
	}
	void AST_Parser::printSubAST( std::shared_ptr<AstExpr> ast, size_t tabCount )
	{
		String tabs = "", out = "";
		for( size_t i = 0 ; i < tabCount ; i++ ) tabs += ' ';

		if( ast->type == AstExpr::Type::block )
		{//Print block content
			out = tabs + "<BLOCK>";
			for( auto v : ast->as<AstBlock>()->variables )
			{
				out += "\n " + tabs + "<VAR_D> " + v.first;
			}
			log( out );
		}
		else if( ast->type == AstExpr::Type::variable )
		{//Print variable content
			out = tabs + "<VAR> " + ast->as<AstVariable>()->name + " : " + ast->as<AstVariable>()->returnTypeString();
			log( out );
		}
		else if( ast->type == AstExpr::Type::term )
		{//Print term content
			out = tabs + "<TERM> op(" + to_string( static_cast<u64>(ast->as<AstTerm>()->op )) + ") : " + ast->as<AstTerm>()->returnTypeString();
			log( out );
			printSubAST( ast->as<AstTerm>()->lVal, tabCount + 1 );
			printSubAST( ast->as<AstTerm>()->rVal, tabCount + 1 );
		}
		else if( ast->type == AstExpr::Type::allocation )
		{//Print allocation content
			out = tabs + "<ALLOC>";
			log( out );
			printSubAST( ast->as<AstAllocation>()->lVal, tabCount + 1 );
			printSubAST( ast->as<AstAllocation>()->rVal, tabCount + 1 );
		}
		else if( ast->type == AstExpr::Type::literal )
		{//Print literal content
			out = tabs + "<LIT> " + ast->as<AstLiteral>()->token->toString() + " : " + ast->as<AstLiteral>()->returnTypeString();
			log( out );
		}


		if( ast->type == AstExpr::Type::block )
		{
			for( auto n : ast->as<AstBlock>()->content )
			{
				printSubAST( n, tabCount + 1 );
			}
		}
	}

	AST_Parser::AST_Parser()
	{
		//Add the priority to operators.
		opPriority[TT::op_set] = 1;
		opPriority[TT::op_add_set] = 1;
		opPriority[TT::op_sub_set] = 1;
		opPriority[TT::op_mul_set] = 1;
		opPriority[TT::op_div_set] = 1;
		opPriority[TT::op_mod_set] = 1;
		opPriority[TT::op_shift_left_set] = 1;
		opPriority[TT::op_shift_right_set] = 1;
		opPriority[TT::op_and_set] = 1;
		opPriority[TT::op_or_set] = 1;
		opPriority[TT::op_xor_set] = 1;
		opPriority[TT::op_or_log] = 2;
		opPriority[TT::op_and_log] = 3;
		opPriority[TT::op_or] = 4;
		opPriority[TT::op_xor] = 5;
		opPriority[TT::op_and] = 6;
		opPriority[TT::op_eql] = 7;
		opPriority[TT::op_not_eql] = 7;
		opPriority[TT::op_less] = 8;
		opPriority[TT::op_more] = 8;
		opPriority[TT::op_less_eql] = 8;
		opPriority[TT::op_more_eql] = 8;
		opPriority[TT::op_shift_left] = 9;
		opPriority[TT::op_shift_right] = 9;
		opPriority[TT::op_add] = 10;
		opPriority[TT::op_sub] = 10;
		opPriority[TT::op_mul] = 11;
		opPriority[TT::op_div] = 11;
		opPriority[TT::op_mod] = 11;
		opPriority[TT::op_not] = 12;
		opPriority[TT::op_inv] = 12;
		opPriority[TT::op_inc] = 12;
		opPriority[TT::op_dec] = 12;
	}

	ExecutionResult AST_Parser::onExecute( CodeBase &base )
	{
		//Set current iterator
		currItr = base.lexerStruct.begin();
		this->base = &base;

		base.globalAst = std::make_shared<AstBlock>();
		base.globalAst->name = "";
		blockStack.push(base.globalAst);

		finishedParsing = false;

		while( !finishedParsing )//Iterate the lexer
		{
			std::shared_ptr<AstExpr> expr = resolveNextExpr();
			//if( expr != nullptr ) base.globalAst->content.push_back( expr );
		}

		{//Print notifications
			size_t errorCount = 0, warningCount = 0, notificationCount = 0;

			for( auto &n : notificationList )
			{
				if( n->type > NT::begin_err && n->type < NT::begin_warning )
				{//error
					log( "Error: " + to_string( n->getCode() ) + " at token '" + n->token->toString() + "' in line " + to_string( n->token->lineNo ) + " column " + to_string( n->token->columnNo ) + ".", LogLevel::Error );
					errorCount++;
				}
				else if( n->type > NT::begin_warning && n->type < NT::begin_improvements )
				{//warning
					log( "Warning: " + to_string( n->getCode() ) + " at token '" + n->token->toString() + "' in line " + to_string( n->token->lineNo ) + " column " + to_string( n->token->columnNo ) + ".", LogLevel::Warning );
					warningCount++;
				}
				else if( n->type > NT::begin_improvements && n->type < NT::count )
				{//notification
					log( "Notification: " + to_string( n->getCode() ) + " at token '" + n->token->toString() + "' in line " + to_string( n->token->lineNo ) + " column " + to_string( n->token->columnNo ) + ".", LogLevel::Information );
					notificationCount++;
				}
			}
			if( errorCount > 0 )
			{
				log( "FAILED " + to_string( errorCount ) + (errorCount>1?" ERRORS": " ERROR")+" OCCURRED! " + to_string( warningCount ) + " warnings occurred. " + to_string( notificationCount ) + " improvements available." );
				_getch();
				return ExecutionResult::astParsingFailed;
			}
			else if( warningCount > 0 || notificationCount > 0 )
			{
				log( "Finshed with " + to_string( errorCount ) + " errors, " + to_string( warningCount ) + ( warningCount==1 ? " warning" : " warnings" ) + " and " + to_string( notificationCount ) + ( warningCount == 1 ? " improvement" : " improvements" ) + "." );
			}
		}

		printAST( base );
		return ExecutionResult::success;
	}
	std::shared_ptr<AstExpr> AST_Parser::resolveNextExpr()
	{
		std::shared_ptr<Token> token = next();

		if( token->type == TT::type_byte )
		{//Allocation block
			std::shared_ptr<AstAllocation> newAst = std::make_shared<AstAllocation>();
			newAst->lVal = std::make_shared<AstVariable>();//Create new variable
			newAst->lVal->retType = BasicType::tByte;

			//Get name of variable
			std::shared_ptr<Token> valName = next();
			if( valName->type != TT::identifier )
			{
				generateNotification( NT::err_expectedIdentifier_atAllocation, valName );
				ignoreExpr();
				return nullptr;
			}
			else newAst->lVal->name = valName->as<Token_Identifier>()->identifier;

			//Save variable in current block
			if( blockStack.top()->variables.find( newAst->lVal->name ) != blockStack.top()->variables.end() )
			{
				generateNotification( NT::err_variableAlreadyDefined, valName );
			}
			else blockStack.top()->variables[newAst->lVal->name] = newAst->lVal;

			//Skip equal sign but print error if none found
			std::shared_ptr<Token> eqlSign = next();
			if( eqlSign->type != TT::op_set )
			{
				generateNotification( NT::err_expectedEqlSign_atAllocation, eqlSign );
				ignoreExpr();
				return nullptr;
			}

			//Resolve rValue as retuning expression
			newAst->rVal = resolveNextExpr();
			if( !newAst->rVal->isTypeReturnable() )
			{
				generateNotification( NT::err_expectedExprWithReturnValue_atAllocation, currToken );
				ignoreExpr();
				return nullptr;
			}

			base->globalAst->content.push_back( newAst );//Add to ast
			return newAst;
		}
		else if( token->type == TT::literalN )
		{//Number literal at expression
			std::shared_ptr<AstLiteral> newAst = std::make_shared<AstLiteral>();
			newAst->retType = BasicType::tByte;
			newAst->token = token;


			//Test for parent-expr
			std::shared_ptr<AstExpr> parentAst = resolveNextExpr();
			if( parentAst != nullptr )
			{
				parentAst->as<AstTerm>()->lVal = newAst;
				return parentAst;
			}
			else return newAst;
		}
		else if( token->type == TT::identifier )
		{//Identifier at expression
			String identifier = token->as<Token_Identifier>()->identifier;
			std::shared_ptr<AstVariable> newAst;
			if( blockStack.top()->variables.find( identifier ) == blockStack.top()->variables.end() ) {
				generateNotification( NT::err_undefinedIdentifier, token );
				newAst = std::make_shared<AstVariable>();
			}
			else newAst = blockStack.top()->variables[identifier];

			if( lastToken->type == TT::identifier )
			{
				generateNotification( NT::err_unexpectedIdentifierAfterIdentifier, currToken );
				ignoreExpr();
			}
			
			if( lValue == nullptr )
			{
				lValue = newAst;//New current value
			}
			return newAst;
		}
		else if( token->isOperator() )
		{//Expression is a term
			std::shared_ptr<AstTerm> newAst = std::make_shared<AstTerm>();

			if( lValue == nullptr )
			{//Check if hast lValue
				generateNotification( NT::err_expectedIdentifierBeforeOperator, currToken );
				ignoreExpr();
				return nullptr;
			}
			auto clVal = lValue;//Current lValue
			newAst->op = token->type;

			//Check rValue
			std::shared_ptr<AstExpr> nextAst = resolveNextExpr();
			if( nextAst == nullptr )
			{//no rValue
				generateNotification( NT::err_expectedIdentifierAfterOperator, currToken );
				return nullptr;
			}
			if( !nextAst->isTypeReturnable() )
			{//lValue is not a returnable
				generateNotification( NT::err_expectedExprWithReturnValue_atOperation, currToken );
				ignoreExpr();
				return newAst;
			}
			newAst->rVal = nextAst->as<AstReturning>();


			//Check if lValue is a term
			std::shared_ptr<AstTerm> clTerm = nullptr;
			while( clVal->type == AstExpr::Type::term && opPriority[clVal->as<AstTerm>()->op] < opPriority[newAst->op] )
			{//Check if lTerm has to be splitted up
				clTerm = clVal->as<AstTerm>();
				clVal = clVal->as<AstTerm>()->rVal;
			}
			//Test for lTerm
			if( clTerm != nullptr )
			{//Min. 1 term was splitted up
				newAst->lVal = clVal->as<AstReturning>();
				clTerm->rVal = newAst;
			}
			else
			{//LValue is a atomic astExpr
				newAst->lVal = clVal->as<AstReturning>();
				lValue = newAst;//Set as lValue
			}


			if( newAst->rVal->retType != newAst->lVal->retType )
			{
				generateNotification( NT::err_unmatchingTypeFound_atTerm, token );
			}
			else newAst->retType = newAst->lVal->retType;


			return newAst;
		}
		else if( token->type == TT::tok_semicolon)
		{//End of expression
			if( lValue != nullptr )
				base->globalAst->content.push_back( lValue );//Add to ast
			lValue = nullptr;
			while( !exprStack.empty() ) exprStack.pop();//todo check
			return nullptr;//Finish expression
		}
		else if( token->type == TT::eof )
		{//End of file
			finishedParsing = true;//Finish file processing
			return nullptr;
		}
		else if( token->type == TT::comment )
		{//Just ignore comments
			return nullptr;
		}
		else
		{
			generateNotification(NT::err_unexpectedToken, token);
			return nullptr;
		}
	}
}
