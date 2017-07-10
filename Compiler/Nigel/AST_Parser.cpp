#include "stdafx.h"
#include "AST_Parser.h"

namespace nigel
{
	using TT = Token::Type;

	std::shared_ptr<Token> AST_Parser::next()
	{
		if( currItr == base->lexerStruct.end() )
		{
			generateNotification( NT::err_reachedEOF_unfinishedExpression, currToken );
			return nullptr;
		}
		lastToken = currToken;
		currToken = *( currItr++ );
		return currToken;
	}

	void AST_Parser::ignoreExpr()
	{
		while( next()->type != TT::tok_semicolon );
		lValue = nullptr;
		expectValue = false;
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
			for( auto v : ast->as<AstBlock>()->newVariables )
			{
				out += "\n " + tabs + "<VAR_D> " + v.first;
			}
			log( out );
		}
		else if( ast->type == AstExpr::Type::variable )
		{//Print variable content
			auto v = ast->as<AstVariable>();
			out = tabs + "<VAR> " + v->name + " : "
				+ v->modelString() + " "
				+ v->returnTypeString() +
				( v->model == MemModel::stack ? ", offset = " + to_string( v->scopeOffset ) : "" );
			log( out );
		}
		else if( ast->type == AstExpr::Type::term )
		{//Print term content
			out = tabs + "<TERM> op(" + ast->as<AstTerm>()->token->toString() + ") : " + ast->as<AstTerm>()->returnTypeString();
			log( out );
			printSubAST( ast->as<AstTerm>()->lVal, tabCount + 1 );
			printSubAST( ast->as<AstTerm>()->rVal, tabCount + 1 );
		}
		else if( ast->type == AstExpr::Type::unary )
		{//Print unary content
			out = tabs + "<UNARY> op(" + ast->as<AstUnary>()->token->toString() + ") : " + ast->as<AstUnary>()->returnTypeString();
			log( out );
			printSubAST( ast->as<AstUnary>()->val, tabCount + 1 );
		}
		else if( ast->type == AstExpr::Type::allocation )
		{//Print allocation content
			out = tabs + "<ALLOC>";
			log( out );
			printSubAST( ast->as<AstAllocation>()->lVal, tabCount + 1 );
		}
		else if( ast->type == AstExpr::Type::literal )
		{//Print literal content
			out = tabs + "<LIT> " + ast->as<AstLiteral>()->token->toString() + " : " + ast->as<AstLiteral>()->returnTypeString();
			log( out );
		}
		else if( ast->type == AstExpr::Type::parenthesis )
		{//Print parenthesis block content
			out = tabs + "<PAR> : " + ast->as<AstParenthesis>()->returnTypeString();
			log( out );
			printSubAST( ast->as<AstParenthesis>()->content, tabCount + 1 );
		}
		else if( ast->type == AstExpr::Type::booleanParanthesis )
		{//Print boolean parenthesis block content
			out = tabs + "<BOOLPAR>";
			log( out );
			printSubAST( ast->as<AstBooleanParenthesis>()->content, tabCount + 1 );
		}
		else if( ast->type == AstExpr::Type::ifStat )
		{//Print if statement
			out = tabs + "<IF>";
			log( out );
			printSubAST( ast->as<AstIf>()->condition, tabCount + 1 );
			printSubAST( ast->as<AstIf>()->ifCase, tabCount + 1 );
			if( ast->as<AstIf>()->elseCase != nullptr )
			{
				out = tabs + "<ELSE>";
				log( out );
				printSubAST( ast->as<AstIf>()->elseCase, tabCount + 1 );
			}
		}
		else if( ast->type == AstExpr::Type::keywordCondition )
		{//Print keyword (true/false)
			if( ast->as<AstKeywordCondition>()->val ) out = tabs + "<TRUE>";
			else out = tabs + "<FALSE>";
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
		opPriority[TT::op_set_get] = 1;
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
		blockStack.push( base.globalAst );

		finishedParsing = false;

		while( !finishedParsing )//Iterate the lexer
		{
			std::shared_ptr<AstExpr> expr = resolveNextExpr();
		}

		bool hasError = false;
		for( auto &n : notificationList ) if( n->type > NT::begin_err && n->type < NT::begin_warning )
		{
			hasError = true;
			break;
		}

		if( base.printAST ) printAST( base );

		if( hasError ) return ExecutionResult::astParsingFailed;
		else return ExecutionResult::success;
	}
	std::shared_ptr<AstExpr> AST_Parser::resolveNextExpr()
	{
		std::shared_ptr<Token> token = next();

		if( token->type == TT::type_byte )
		{//Allocation block
			std::shared_ptr<AstAllocation> newAst = std::make_shared<AstAllocation>();
			newAst->token = token;
			newAst->lVal = std::make_shared<AstVariable>();//Create new variable
			newAst->lVal->retType = BasicType::tByte;
			newAst->lVal->model = base->memModel;
			if( blockStack.top() != base->globalAst ) newAst->lVal->model = MemModel::stack;

			//Get name of variable
			std::shared_ptr<Token> valName = next();
			if( valName->type != TT::identifier )
			{
				generateNotification( NT::err_expectedIdentifier_atAllocation, valName );
				ignoreExpr();
				return nullptr;
			}
			else newAst->lVal->name = valName->as<Token_Identifier>()->identifier;
			newAst->lVal->token = valName;

			//Check if variable already exists
			bool found = false;
			for( auto &v : blockStack.top()->newVariables ) if( v.first == newAst->lVal->name )
			{
				found = true;
				break;
			}
			//Save variable in current block
			if( found ) generateNotification( NT::err_variableAlreadyDefined, valName );
			else
			{
				blockStack.top()->variables.push_front( std::pair<VariableBinding, size_t>( VariableBinding(newAst->lVal->name, newAst->lVal), 0 ) );
				blockStack.top()->newVariables.push_front( VariableBinding( newAst->lVal->name, newAst->lVal ) );
			}


			lValue = newAst->lVal;

			return newAst;
		}
		if( token->type == TT::fast_attr )
		{//Pre-allocation block
			auto nextExpr = resolveNextExpr();
			if( nextExpr->type != AstExpr::Type::allocation )
			{
				generateNotification( NT::err_noAllocationAfterVariableAttribute, nextExpr->token );
				ignoreExpr();
				return nullptr;
			}
			nextExpr->as<AstAllocation>()->lVal->model = MemModel::fast;
			return nextExpr;
		}
		if( token->type == TT::norm_attr )
		{//Pre-allocation block
			auto nextExpr = resolveNextExpr();
			if( nextExpr->type != AstExpr::Type::allocation )
			{
				generateNotification( NT::err_noAllocationAfterVariableAttribute, nextExpr->token );
				ignoreExpr();
				return nullptr;
			}
			nextExpr->as<AstAllocation>()->lVal->model = MemModel::large;
			return nextExpr;
		}
		if( token->type == TT::unsigned_attr )
		{//Pre-allocation block
			auto nextExpr = resolveNextExpr();
			if( nextExpr->type != AstExpr::Type::allocation )
			{
				generateNotification( NT::err_noAllocationAfterVariableAttribute, nextExpr->token );
				ignoreExpr();
				return nullptr;
			}
			auto allocationAst = nextExpr->as<AstAllocation>();
			if( allocationAst->retType == BasicType::tByte )
			{
				allocationAst->retType = BasicType::tUbyte;
				allocationAst->lVal->retType = BasicType::tUbyte;
			}
			else if( allocationAst->retType == BasicType::tInt )
			{
				allocationAst->retType = BasicType::tUint;
				allocationAst->lVal->retType = BasicType::tUint;
			}
			return nextExpr;
		}
		else if( token->type == TT::literalN )
		{//Number literal at expression
			std::shared_ptr<AstLiteral> newAst = std::make_shared<AstLiteral>();
			newAst->token = token;
			newAst->retType = BasicType::tByte;
			newAst->as<AstExpr>()->token = token;

			if( lValue != nullptr )
			{//Check if has lValue
				generateNotification( NT::err_unexpectedReturningBeforeLiteral, currToken );
				ignoreExpr();
				return nullptr;
			}
			if( !expectValue )
			{
				lValue = newAst;//New current value
			}
			return newAst;
		}
		else if( token->type == TT::literalTrue )
		{//true literal
			std::shared_ptr<AstKeywordCondition> newAst = std::make_shared<AstKeywordCondition>();
			newAst->token = token;
			newAst->val = true;

			if( lValue != nullptr )
			{//Check if has lValue
				generateNotification( NT::err_unexpectedReturningBeforeLiteral, currToken );
				ignoreExpr();
				return nullptr;
			}
			if( !expectValue )
			{
				lValue = newAst;//New current value
			}
			return newAst;
		}
		else if( token->type == TT::literalFalse )
		{//false literal
			std::shared_ptr<AstKeywordCondition> newAst = std::make_shared<AstKeywordCondition>();
			newAst->token = token;
			newAst->val = false;

			if( lValue != nullptr )
			{//Check if has lValue
				generateNotification( NT::err_unexpectedReturningBeforeLiteral, currToken );
				ignoreExpr();
				return nullptr;
			}
			if( !expectValue )
			{
				lValue = newAst;//New current value
			}
			return newAst;
		}
		else if( token->type == TT::identifier )
		{//Identifier at expression
			String identifier = token->as<Token_Identifier>()->identifier;
			std::shared_ptr<AstVariable> newAst;

			bool found = false;
			VariableBinding bind;
			size_t scopeOffset = 0;
			for( auto &v : blockStack.top()->variables ) if( v.first.first == identifier )
			{
				found = true;
				bind = v.first;
				scopeOffset = v.second;
				break;
			}
			if( !found )
			{
				generateNotification( NT::err_undefinedIdentifier, token );
				newAst = std::make_shared<AstVariable>();
			}
			else newAst = std::make_shared<AstVariable>( *bind.second );
			newAst->token = token;
			newAst->scopeOffset = scopeOffset;

			if( lValue != nullptr )
			{//Check if has lValue
				generateNotification( NT::err_unexpectedReturningBeforeIdentifier, currToken );
				ignoreExpr();
				return nullptr;
			}

			if( !expectValue )
			{
				lValue = newAst;//New current value
			}
			return newAst;
		}
		else if( token->isOperator() )
		{//Expression is a term or a unary
			if( token->type == TT::op_inc || token->type == TT::op_dec ||
				( expectValue && ( token->type == TT::op_inv || token->type == TT::op_sub || token->type == TT::op_add || token->type == TT::op_not ) ) )
			{//Unary
				std::shared_ptr<AstUnary> newAst = std::make_shared<AstUnary>();
				newAst->token = token;
				newAst->op = token->type;

				if( expectValue )
				{//L operator
					newAst->side = AstUnary::Side::left;

					//Check value
					expectValue = true;
					std::shared_ptr<AstExpr> nextAst = resolveNextExpr();
					if( nextAst == nullptr )
					{//no value
						generateNotification( NT::err_expectedIdentifierAfterOperator, currToken );
						expectValue = false;
						return nullptr;
					}
					if( !nextAst->isTypeReturnable() )
					{//lValue is not a returnable
						generateNotification( NT::err_expectedExprWithReturnValue_atOperation, currToken );
						ignoreExpr();
						return newAst;
					}
					newAst->val = nextAst->as<AstReturning>();
				}
				else
				{//R operator
					newAst->side = AstUnary::Side::right;

					newAst->val = splitMostRightExpr( lValue, newAst, opPriority[token->type] );
				}

				newAst->retType = newAst->val->retType;
				expectValue = false;
				lValue = newAst;
				return newAst;
			}
			else
			{//Term
				std::shared_ptr<AstTerm> newAst = std::make_shared<AstTerm>();
				newAst->token = token;

				if( lValue == nullptr )
				{//Check if hast lValue
					generateNotification( NT::err_expectedIdentifierBeforeOperator, currToken );
					ignoreExpr();
					return nullptr;
				}
				newAst->op = token->type;

				auto prevLValue = lValue;

				//Check rValue
				expectValue = true;
				lValue = nullptr;
				std::shared_ptr<AstExpr> nextAst = resolveNextExpr();
				if( nextAst == nullptr )
				{//no rValue
					generateNotification( NT::err_expectedIdentifierAfterOperator, currToken );
					expectValue = false;
					return nullptr;
				}
				if( !nextAst->isTypeReturnable() )
				{//rValue is not a returnable
					generateNotification( NT::err_expectedExprWithReturnValue_atOperation, currToken );
					ignoreExpr();
					return newAst;
				}
				newAst->rVal = nextAst->as<AstReturning>();

				if( prevLValue->type == AstExpr::Type::term && newAst->op == TT::op_set ) newAst->op = TT::op_set_get;
				newAst->lVal = splitMostRightExpr( prevLValue, newAst, opPriority[newAst->op] );

				if( newAst->rVal->retType != newAst->lVal->retType )
					generateNotification( NT::err_unmatchingTypeFound_atTerm, token );
				else newAst->retType = newAst->lVal->retType;

				expectValue = false;

				if( prevLValue != newAst->lVal ) lValue = prevLValue;
				else lValue = newAst;
				return newAst;
			}
		}
		else if( token->type == TT::tok_parenthesis_open )
		{//Open a new parenthesis
			size_t myOpenParenthesisNr = ++openParenthesisCount;
			std::shared_ptr<AstExpr> newAst;
			if( expectBool ) newAst = std::make_shared<AstBooleanParenthesis>();
			else newAst = std::make_shared<AstParenthesis>();
			newAst->token = token;

			if( lValue != nullptr )
			{//Check if has lValue
				generateNotification( NT::err_unexpectedReturningBeforeParenthesisBlock, currToken );
				ignoreExpr();
				return nullptr;
			}

			//Check content
			expectValue = false;
			std::shared_ptr<AstExpr> nextAst = resolveNextExpr();
			while( myOpenParenthesisNr <= openParenthesisCount )
			{
				if( nextAst == nullptr )
				{//no content
					generateNotification( NT::err_expectedIdentifierInParenthesis, currToken );
					expectValue = false;
					return nullptr;
				}

				if( expectBool )
				{//Boolean expression
					std::shared_ptr<AstBooleanParenthesis> newAstExpr = newAst->as<AstBooleanParenthesis>();

					if( !nextAst->isTypeCondition() )
					{//Content is not a returnable
						generateNotification( NT::err_expectedExprWithBooleanValue_atParenthesis, currToken );
						ignoreExpr();
						return newAst;
					}
					
					newAstExpr->content = nextAst->as<AstCondition>();
				}
				else
				{//Non boolean expression
					std::shared_ptr<AstParenthesis> newAstExpr = newAst->as<AstParenthesis>();

					if( !nextAst->isTypeReturnable() )
					{//Content is not a returnable
						generateNotification( NT::err_expectedExprWithReturnValue_atParenthesis, currToken );
						ignoreExpr();
						return newAst;
					}

					if( newAstExpr->content != nullptr &&
						newAstExpr->content->type == AstExpr::Type::term && nextAst->type == AstExpr::Type::term &&
						newAstExpr->content->as<AstTerm>()->rVal != nextAst && nextAst->as<AstTerm>()->lVal != newAstExpr->content )
					{//Error with two following but seperate operations
						generateNotification( NT::err_expectedTermAfterReturnableInParenthesis, nextAst->token );
					}
					else if( newAstExpr->content == nullptr ||
							 newAstExpr->content->type != AstExpr::Type::term || nextAst != newAstExpr->content->as<AstTerm>()->rVal )
					{
						newAstExpr->content = nextAst->as<AstReturning>();
					}

					newAstExpr->retType = newAstExpr->content->retType;
				}

				nextAst = resolveNextExpr();
			}
			lValue = newAst;
			return newAst;
		}
		else if( token->type == TT::tok_parenthesis_close )
		{//Close a open parenthesis
			if( openParenthesisCount <= 0 ) generateNotification( NT::err_unexpectedCloseOfParenthesis, token );
			else openParenthesisCount--;

			return nullptr;
		}
		else if( token->type == TT::tok_brace_open )
		{//Open a new block
			size_t myOpenBraceNr = ++openBraceCount;
			std::shared_ptr<AstBlock> newAst = std::make_shared<AstBlock>();

			if( lValue != nullptr )
			{//Check if has lValue
				generateNotification( NT::err_unexpectedReturningBeforeBlock, currToken );
				ignoreExpr();
				return nullptr;
			}

			newAst->variables.insert( newAst->variables.begin(), blockStack.top()->variables.begin(), blockStack.top()->variables.end() );
			for( auto &v : newAst->variables ) v.second++;//Increment scope offset.
			blockStack.push( newAst );

			while( myOpenBraceNr <= openBraceCount )//Iterate for this block
			{
				std::shared_ptr<AstExpr> expr = resolveNextExpr();
			}

			if( openParenthesisCount > 0 ) generateNotification( NT::err_aParenthesisWasNotClosed, token );
			lValue = nullptr;
			expectValue = false;

			blockStack.pop();
			blockStack.top()->content.push_back( newAst );

			return newAst;
		}
		else if( token->type == TT::tok_brace_close )
		{//Close a open block
			if( openBraceCount <= 0 ) generateNotification( NT::err_unexpectedCloseOfBlock, token );
			else openBraceCount--;

			return nullptr;
		}
		else if( token->type == TT::cf_if )
		{//Open a new if condition expression
			std::shared_ptr<AstIf> newAst = std::make_shared<AstIf>();

			//Condition
			expectBool = true;
			std::shared_ptr<AstExpr> nextAst = resolveNextExpr();
			if( nextAst->type != AstExpr::Type::booleanParanthesis )
			{
				generateNotification( NT::err_expectedIdentifierAfterOperator, currToken );
				return nullptr;
			}
			newAst->condition = nextAst->as<AstBooleanParenthesis>();
			expectBool = false;
			lValue = nullptr;

			//If case block
			nextAst = resolveNextExpr();
			if( nextAst->type != AstExpr::Type::block )
			{
				generateNotification( NT::err_expectBlockAfterIf, currToken );
				return nullptr;
			}
			newAst->ifCase = nextAst->as<AstBlock>();

			//Else case block
			nextAst = resolveNextExpr();
			if( nextAst != nullptr && nextAst->type == AstExpr::Type::elseStat )
			{
				nextAst = resolveNextExpr();
				if( nextAst->type != AstExpr::Type::block )
				{
					generateNotification( NT::err_expectBlockAfterElse, currToken );
					return nullptr;
				}
				newAst->elseCase = nextAst->as<AstBlock>();
			}

			blockStack.top()->content.push_back( newAst );//Add to ast

			return newAst;
		}
		else if( token->type == TT::cf_else )
		{//Else block. Just return a virtual ast.
			return std::make_shared<AstExpr>( AstExpr::Type::elseStat );
		}
		else if( token->type == TT::tok_semicolon )
		{//End of expression
			if( openParenthesisCount > 0 ) generateNotification( NT::err_aParenthesisWasNotClosed, token );
			if( lValue != nullptr )
				blockStack.top()->content.push_back( lValue );//Add to ast
			lValue = nullptr;
			expectValue = false;
			return nullptr;//Finish expression
		}
		else if( token->type == TT::eof )
		{//End of file
			if( openBraceCount > 0 ) generateNotification( NT::err_aBlockWasNotClosed, token );
			finishedParsing = true;//Finish file processing
			if( lValue != nullptr ) generateNotification( NT::err_reachedEOF_unfinishedExpression, currToken );
			return nullptr;
		}
		else if( token->type == TT::comment )
		{//Just ignore comments
			return nullptr;
		}
		else
		{
			generateNotification( NT::err_unexpectedToken, token );
			return nullptr;
		}
	}
	std::shared_ptr<AstReturning> AST_Parser::splitMostRightExpr( std::shared_ptr<AstExpr> currLVal, std::shared_ptr<AstReturning> cExpr, int priority )
	{
		//Check if lValue is a term
		std::shared_ptr<AstTerm> clTerm = nullptr;
		while( currLVal->type == AstExpr::Type::term && opPriority[currLVal->as<AstTerm>()->op] < priority || 
			   ( priority == opPriority[TT::op_set] && opPriority[currLVal->as<AstTerm>()->op] == priority ) )
		{//Check if lTerm has to be splitted up
			clTerm = currLVal->as<AstTerm>();
			currLVal = currLVal->as<AstTerm>()->rVal;
		}
		
		//Test for lTerm
		if( clTerm != nullptr )
		{//Min. 1 term was splitted up
			clTerm->rVal = cExpr;
			return currLVal->as<AstReturning>();
		}
		else
		{//LValue is a atomic astExpr
			lValue = cExpr;//Set as lValue
			return currLVal->as<AstReturning>();
		}
	}
}
