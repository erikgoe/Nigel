#include "stdafx.h"
#include "AST_Parser.h"

namespace nigel
{
	using TT = Token::Type;
	u32 AstBlock::nextID { 0 };

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
			for( auto n : ast->as<AstBlock>()->content )
			{
				printSubAST( n, tabCount + 1 );
			}
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
			out = tabs + "<TERM> op(" + ast->token->toString() + ") : " + ast->as<AstTerm>()->returnTypeString();
			log( out );
			printSubAST( ast->as<AstTerm>()->lVal, tabCount + 1 );
			printSubAST( ast->as<AstTerm>()->rVal, tabCount + 1 );
		}
		else if( ast->type == AstExpr::Type::unary )
		{//Print unary content
			out = tabs + "<UNARY> op(" + ast->token->toString() + ") : " + ast->as<AstUnary>()->returnTypeString();
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
			out = tabs + "<PAN> : " + ast->as<AstParenthesis>()->returnTypeString();
			log( out );
			printSubAST( ast->as<AstParenthesis>()->content, tabCount + 1 );
		}
		else if( ast->type == AstExpr::Type::booleanParenthesis )
		{//Print boolean parenthesis block content
			out = tabs + "<BOOLPAN>";
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
		else if( ast->type == AstExpr::Type::whileStat )
		{//Print while statement
			if( ast->as<AstWhile>()->isDoWhile )
			{
				out = tabs + "<DO_WHILE>";
				log( out );
				printSubAST( ast->as<AstWhile>()->block, tabCount + 1 );
				printSubAST( ast->as<AstWhile>()->condition, tabCount + 1 );
			}
			else
			{
				out = tabs + "<WHILE>";
				log( out );
				printSubAST( ast->as<AstWhile>()->condition, tabCount + 1 );
				printSubAST( ast->as<AstWhile>()->block, tabCount + 1 );
			}
		}
		else if( ast->type == AstExpr::Type::keywordCondition )
		{//Print keyword (true/false)
			if( ast->as<AstKeywordCondition>()->val ) out = tabs + "<TRUE>";
			else out = tabs + "<FALSE>";
			log( out );
		}
		else if( ast->type == AstExpr::Type::arithmenticCondition )
		{//Resolve a arithmetic operation into a boolean'
			out = tabs + "<ARTH_COND>";
			log( out );
			printSubAST( ast->as<AstArithmeticCondition>()->ret, tabCount + 1 );
		}
		else if( ast->type == AstExpr::Type::comparisonCondition )
		{//Comparison between two arithmetic expressions
			out = tabs + "<COMPARE> op(" + ast->token->toString() + ")";
			log( out );
			printSubAST( ast->as<AstComparisonCondition>()->lVal, tabCount + 1 );
			printSubAST( ast->as<AstComparisonCondition>()->rVal, tabCount + 1 );
		}
		else if( ast->type == AstExpr::Type::combinationCondition )
		{//Combination of two conditional expressions
			out = tabs + "<COMBINE> op(" + ast->token->toString() + ")";
			log( out );
			if( ast->as<AstCombinationCondition>()->lVal != nullptr )
				printSubAST( ast->as<AstCombinationCondition>()->lVal, tabCount + 1 );
			printSubAST( ast->as<AstCombinationCondition>()->rVal, tabCount + 1 );
		}
		else if( ast->type == AstExpr::Type::breakStat )
		{//Break statement
			out = tabs + "<BREAK>";
			log( out );
		}
		else if( ast->type == AstExpr::Type::functionDefinition )
		{//Function definition
			auto f = ast->as<AstFunction>();
			out = tabs + "<FUNC> " + f->symbol + " : "
				+ AstReturning::returnTypeString( f->retType );
			log( out );
			printSubAST( f->content, tabCount + 1 );
		}
		else if( ast->type == AstExpr::Type::functionCall )
		{//Function call
			auto f = ast->as<AstFunctionCall>();
			out = tabs + "<CALL> " + f->symbol + " : "
				+ AstReturning::returnTypeString( f->retType );
			log( out );
			for( auto p : f->parameters )
			{
				printSubAST( p, tabCount + 1 );
			}
		}
		else if( ast->type == AstExpr::Type::returnStat )
		{//Return statement
			auto r = ast->as<AstReturnStatement>();
			out = tabs + "<RET>";
			log( out );
			printSubAST( r->expr, tabCount + 1 );
		}
		else if( ast->type == AstExpr::Type::refer )
		{//Referencing
			auto r = ast->as<AstRefer>();
			out = tabs + "<REF>";
			log( out );
			printSubAST( r->var, tabCount + 1 );
		}
		else if( ast->type == AstExpr::Type::derefer )
		{//Dereferencing
			auto r = ast->as<AstDerefer>();
			out = tabs + "<DEREF>";
			log( out );
			printSubAST( r->expr, tabCount + 1 );
		}

		else
		{//Unknown ast
			log( tabs + "-!-UNKNOWN-!-" );
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

		opPriority[TT::function] = 13;
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

		if( token->type == TT::type_byte || token->type == TT::type_ptr )
		{//Allocation block
			std::shared_ptr<AstAllocation> newAst = std::make_shared<AstAllocation>();
			newAst->token = token;
			newAst->lVal = std::make_shared<AstVariable>();//Create new variable
			newAst->lVal->retType = BasicType::tByte;
			newAst->lVal->model = base->memModel;
			if( blockStack.top() != base->globalAst ) newAst->lVal->model = MemModel::stack;

			if( lValue != nullptr )
			{//Check if has lValue
				generateNotification( NT::err_unexpectedReturningBeforeByteKeyword, currToken );
				ignoreExpr();
				return nullptr;
			}

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
			if( lValue != nullptr )
			{//Check if has lValue
				generateNotification( NT::err_unexpectedReturningBeforeFastKeyword, currToken );
				ignoreExpr();
				return nullptr;
			}

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
			if( lValue != nullptr )
			{//Check if has lValue
				generateNotification( NT::err_unexpectedReturningBeforeNormKeyword, currToken );
				ignoreExpr();
				return nullptr;
			}

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
			if( lValue != nullptr )
			{//Check if has lValue
				generateNotification( NT::err_unexpectedReturningBeforeUnsignedKeyword, currToken );
				ignoreExpr();
				return nullptr;
			}

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
			std::shared_ptr<AstExpr> newAst;

			bool found = false;
			VariableBinding bind;
			size_t scopeOffset = 0;
			for( auto &v : blockStack.top()->variables ) if( v.first.first == identifier )
			{//Search in variables
				found = true;
				bind = v.first;
				scopeOffset = v.second;
				break;
			}
			if( !found )
			{//Search in functions
				if( functions.find( identifier ) != functions.end() )
				{//Found function
					auto ast = std::make_shared<AstFunctionCall>();
					ast->token = token;
					ast->symbol = identifier;

					if( next()->type != TT::tok_parenthesis_open )
					{
						generateNotification( NT::err_expectedOpeningParenthesis_atFunctionCall, token );
						ignoreExpr();
						return nullptr;
					}

					openParenthesisCount++;
					expectValue = true;

					bool loopParameterList = true;
					while( loopParameterList )
					{
						auto nextAst = resolveNextExpr();
						if( openParenthesisCount == 0 || nextAst == nullptr )
						{//closed parenthesis or comma
							ast->parameters.push_back( lValue->as<AstReturning>() );
							ast->symbol += "@" + lValue->as<AstReturning>()->returnTypeString();
							expectValue = true;
							lValue = nullptr;

							if( openParenthesisCount == 0 ) break;
							else if( nextAst == nullptr ) continue;//comma
						}
						expectValue = false;
						if( !nextAst->isTypeReturnable() )
						{
							generateNotification( NT::err_expectedReturningExpression_atFunctionCall, nextAst->token );
							ignoreExpr();
							return nullptr;
						}

						lValue = nextAst;

					}
					expectValue = false;

					if( functions[identifier].find( ast->symbol ) == functions[identifier].end() )
					{
						generateNotification( NT::err_notFoundMatchingFunctionDeclaration, token );
						ignoreExpr();
						return nullptr;
					}
					ast->retType = functions[identifier][ast->symbol]->retType;

					newAst = ast;
				}
				else
				{
					generateNotification( NT::err_undefinedIdentifier, token );
					newAst = std::make_shared<AstVariable>();
				}
			}
			else
			{//Found variable
				newAst = std::make_shared<AstVariable>( *bind.second );
				newAst->token = token;
				newAst->as<AstVariable>()->scopeOffset = scopeOffset;

				if( lValue != nullptr )
				{//Check if has lValue
					generateNotification( NT::err_unexpectedReturningBeforeIdentifier, currToken );
					ignoreExpr();
					return nullptr;
				}
			}

			if( !expectValue )
			{
				lValue = newAst;//New current value
			}
			return newAst;
		}
		else if( token->isOperator() )
		{//Expression is a term or a unary
			if( blockStack.size() <= 1 )
			{//Is in global scope!
				generateNotification( NT::err_operationsAreNotAllowedInGlobalScope, token );
				ignoreExpr();
				return nullptr;
			}
			else
			{//Not global
				if( ( expectValue || lValue == nullptr ) && ( token->type == TT::op_and || token->type == TT::op_mul ) )
				{//Refer/derefer
					if( token->type == TT::op_and )
					{//Refer
						std::shared_ptr<AstRefer> newAst = std::make_shared<AstRefer>();
						newAst->token = token;

						expectValue = true;
						std::shared_ptr<AstExpr> nextAst = resolveNextExpr();
						if( nextAst == nullptr || nextAst->type != AstExpr::Type::variable )
						{//no variable
							generateNotification( NT::err_expectedVariableForReference, currToken );
							expectValue = false;
							ignoreExpr();
						}

						newAst->var = nextAst->as<AstVariable>();
						newAst->retType = newAst->var->retType;
						lValue = newAst;
					}
					else if( token->type == TT::op_mul )
					{//Derefer
						std::shared_ptr<AstDerefer> newAst = std::make_shared<AstDerefer>();
						newAst->token = token;

						expectValue = true;
						std::shared_ptr<AstExpr> nextAst = resolveNextExpr();
						if( nextAst == nullptr || !nextAst->isTypeReturnable() )
						{//no variable
							generateNotification( NT::err_expectedVariableForReference, currToken );
							expectValue = false;
							ignoreExpr();
						}

						newAst->expr = nextAst->as<AstReturning>();
						newAst->retType = newAst->expr->retType;
						lValue = newAst;
					}

					expectValue = false;
					return lValue;
				}
				else if( token->type == TT::op_inc || token->type == TT::op_dec ||
					( ( expectValue || lValue == nullptr ) && ( token->type == TT::op_inv || token->type == TT::op_sub || token->type == TT::op_add || token->type == TT::op_not ) ) )
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
				else if( token->type == TT::op_eql || token->type == TT::op_not_eql ||
						 token->type == TT::op_less || token->type == TT::op_more || token->type == TT::op_less_eql || token->type == TT::op_more_eql )
				{//Comparison operation
					std::shared_ptr<AstComparisonCondition> newAst = std::make_shared<AstComparisonCondition>();
					newAst->token = token;

					if( lValue == nullptr )
					{//Check if has lValue
						generateNotification( NT::err_expectedIdentifierBeforeOperator, currToken );
						ignoreExpr();
						return nullptr;
					}
					newAst->op = token->type;

					auto prevLValue = lValue;
					bool prevExpectBool = expectBool;

					//Check rValue
					expectBool = false;
					expectValue = true;
					lValue = nullptr;
					std::shared_ptr<AstExpr> nextAst = resolveNextExpr();
					expectBool = prevExpectBool;//Reset

					if( nextAst == nullptr )
					{//no rValue
						generateNotification( NT::err_expectedIdentifierAfterOperator, currToken );
						expectValue = false;
						return nullptr;
					}
					if( !nextAst->isTypeReturnable() )
					{//rValue is not a arithmethic expression
						generateNotification( NT::err_expectedExprWithReturnValue_atOperation, currToken );
						ignoreExpr();
						return newAst;
					}
					newAst->rVal = nextAst->as<AstReturning>();
					newAst->lVal = splitMostRightExpr( prevLValue, newAst, opPriority[newAst->op] );

					expectValue = false;

					if( prevLValue != newAst->lVal ) lValue = prevLValue;
					else lValue = newAst;
					return newAst;
				}
				else if( token->type == TT::op_and_log || token->type == TT::op_or_log )
				{//Conditional expression
					std::shared_ptr<AstCombinationCondition> newAst = std::make_shared<AstCombinationCondition>();
					newAst->token = token;

					if( lValue == nullptr )
					{//Check if has lValue
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
					if( !nextAst->isTypeCondition() )
					{//rValue is not a conditional
						if( nextAst->isTypeReturnable() )
						{
							auto condition = std::make_shared<AstArithmeticCondition>();
							condition->ret = nextAst->as<AstReturning>();
							newAst->rVal = condition;
						}
						else
						{
							generateNotification( NT::err_expectedExprWithConditionalValue_atOperation, currToken );
							ignoreExpr();
							return newAst;
						}
					}
					else newAst->rVal = nextAst->as<AstCondition>();

					newAst->lVal = splitMostRightExpr( prevLValue, newAst, opPriority[newAst->op] );

					expectValue = false;

					if( prevLValue != newAst->lVal ) lValue = prevLValue;
					else lValue = newAst;
					return newAst;
				}
				else if( token->type == TT::op_not )
				{//not-operator
					std::shared_ptr<AstCombinationCondition> newAst = std::make_shared<AstCombinationCondition>();
					newAst->token = token;

					if( lValue != nullptr )
					{//Check if has lValue
						generateNotification( NT::err_unexpectedIdentifierBeforeNotOperator, currToken );
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
					if( !nextAst->isTypeCondition() )
					{//rValue is not a conditional
						if( nextAst->isTypeReturnable() )
						{
							auto condition = std::make_shared<AstArithmeticCondition>();
							condition->ret = nextAst->as<AstReturning>();
							newAst->rVal = condition;
						}
						else
						{
							generateNotification( NT::err_expectedExprWithConditionalValue_atOperation, currToken );
							ignoreExpr();
							return newAst;
						}
					}
					else newAst->rVal = nextAst->as<AstCondition>();

					expectValue = false;

					lValue = newAst;
					return newAst;
				}
				else
				{//Artithmetic term
					std::shared_ptr<AstTerm> newAst = std::make_shared<AstTerm>();
					newAst->token = token;

					if( lValue == nullptr )
					{//Check if has lValue
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
		}
		else if( token->type == TT::tok_parenthesis_open )
		{//Open a new parenthesis
			size_t myOpenParenthesisNr = ++openParenthesisCount;
			std::shared_ptr<AstExpr> newAst;

			if( lValue != nullptr )
			{//Has lValue
				generateNotification( NT::err_unexpectedReturningBeforeParenthesisBlock, currToken );
				ignoreExpr();
				return nullptr;
			}
			else
			{
				if( expectBool ) newAst = std::make_shared<AstBooleanParenthesis>();
				else newAst = std::make_shared<AstParenthesis>();
				newAst->token = token;

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

						if( !lValue->isTypeCondition() )
						{//Content is not a condition
							if( nextAst->isTypeReturnable() )
							{
								auto condition = std::make_shared<AstArithmeticCondition>();
								condition->ret = lValue->as<AstReturning>();
								newAstExpr->content = condition;
							}
							else
							{//Expression that can't be convertet into a condition
								generateNotification( NT::err_expectedExprWithBooleanValue_atParenthesis, currToken );
								ignoreExpr();
								return newAst;
							}
						}
						else newAstExpr->content = lValue->as<AstCondition>();
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
			newAst->token = token;

			bool mHead = blockHasHead;
			blockHasHead = false;

			if( lValue != nullptr )
			{//Check if has lValue
				generateNotification( NT::err_unexpectedReturningBeforeBlock, currToken );
				ignoreExpr();
				return nullptr;
			}

			newAst->variables.insert( newAst->variables.begin(), blockStack.top()->variables.begin(), blockStack.top()->variables.end() );
			for( auto &v : newAst->variables ) v.second++;//Increment scope offset.
			for( auto &v : functionParameters )
			{
				newAst->variables.push_back( std::pair<VariableBinding, size_t>( v, 0 ) );
				newAst->parameters.push_back( v );
			}
			functionParameters.clear();
			blockStack.push( newAst );

			while( myOpenBraceNr <= openBraceCount )//Iterate for this block
			{
				std::shared_ptr<AstExpr> expr = resolveNextExpr();
			}

			if( openParenthesisCount > 0 ) generateNotification( NT::err_aParenthesisWasNotClosed, token );
			lValue = nullptr;
			expectValue = false;

			blockStack.pop();
			if( !mHead ) //Add if it has no head.
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
			if( blockStack.size() <= 1 )
			{//Is in global scope!
				generateNotification( NT::err_operationsAreNotAllowedInGlobalScope, token );
				ignoreExpr();
				return nullptr;
			}
			else
			{//Not global
				std::shared_ptr<AstIf> newAst = std::make_shared<AstIf>();
				newAst->token = token;

				//Condition
				expectBool = true;
				std::shared_ptr<AstExpr> nextAst = resolveNextExpr();
				if( nextAst->type != AstExpr::Type::booleanParenthesis )
				{
					generateNotification( NT::err_expectedIdentifierAfterOperator, currToken );
					return nullptr;
				}
				newAst->condition = nextAst->as<AstBooleanParenthesis>();
				expectBool = false;
				lValue = nullptr;

				//If case block
				blockHasHead = true;
				nextAst = resolveNextExpr();
				if( nextAst->type != AstExpr::Type::block )
				{
					generateNotification( NT::err_expectBlockAfterIf, currToken );
					return nullptr;
				}
				newAst->ifCase = nextAst->as<AstBlock>();

				blockStack.top()->content.push_back( newAst );//Add to ast

				//Else case block
				blockHasHead = true;
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


				return newAst;
			}
		}
		else if( token->type == TT::cf_else )
		{//Else block. Just return a virtual ast.
			if( blockStack.size() <= 1 )
			{//Is in global scope!
				generateNotification( NT::err_operationsAreNotAllowedInGlobalScope, token );
				ignoreExpr();
				return nullptr;
			}
			else
			{//Not global
				auto newAst = std::make_shared<AstExpr>( AstExpr::Type::elseStat );
				newAst->token = token;
				return newAst;
			}
		}
		else if( token->type == TT::cf_while )
		{//Create a new while loop
			if( blockStack.size() <= 1 )
			{//Is in global scope!
				generateNotification( NT::err_operationsAreNotAllowedInGlobalScope, token );
				ignoreExpr();
				return nullptr;
			}
			else
			{//Not global
				std::shared_ptr<AstWhile> newAst = std::make_shared<AstWhile>();
				newAst->token = token;
				newAst->isDoWhile = previousDo;
				previousDo = false;

				if( newAst->isDoWhile )
				{//Is do-while
					//Block
					std::shared_ptr<AstBlock> block = blockStack.top()->content.back()->as<AstBlock>();
					blockStack.top()->content.pop_back();
					newAst->block = block;

					//Condition
					expectBool = true;
					std::shared_ptr<AstExpr> nextAst = resolveNextExpr();
					if( nextAst->type != AstExpr::Type::booleanParenthesis )
					{
						generateNotification( NT::err_expectedIdentifierAfterOperator, currToken );
						return nullptr;
					}
					newAst->condition = nextAst->as<AstBooleanParenthesis>();
					expectBool = false;
					lValue = newAst;//To force semicolon.
				}
				else
				{//Is normal while
					//Condition
					expectBool = true;
					std::shared_ptr<AstExpr> nextAst = resolveNextExpr();
					if( nextAst->type != AstExpr::Type::booleanParenthesis )
					{
						generateNotification( NT::err_expectedIdentifierAfterOperator, currToken );
						return nullptr;
					}
					newAst->condition = nextAst->as<AstBooleanParenthesis>();
					expectBool = false;
					lValue = nullptr;

					//Block
					blockHasHead = true;
					nextAst = resolveNextExpr();
					if( nextAst->type != AstExpr::Type::block )
					{
						generateNotification( NT::err_expectBlockAfterWhile, currToken );
						return nullptr;
					}
					newAst->block = nextAst->as<AstBlock>();
					blockStack.top()->content.push_back( newAst );//Add to ast
				}

				return newAst;
			}
		}
		else if( token->type == TT::cf_do )
		{//Create a new do while loop
			if( blockStack.size() <= 1 )
			{//Is in global scope!
				generateNotification( NT::err_operationsAreNotAllowedInGlobalScope, token );
				ignoreExpr();
				return nullptr;
			}
			else
			{//Not global
				std::shared_ptr<AstWhile> newAst = std::make_shared<AstWhile>();
				newAst->token = token;
				previousDo = true;
				return newAst;
			}
		}
		else if( token->type == TT::cf_break )
		{//Else block. Just return a virtual ast.
			if( blockStack.size() <= 1 )
			{//Is in global scope!
				generateNotification( NT::err_operationsAreNotAllowedInGlobalScope, token );
				ignoreExpr();
				return nullptr;
			}
			else
			{//Not global
				auto newAst = std::make_shared<AstExpr>( AstExpr::Type::breakStat );
				newAst->token = token;
				blockStack.top()->content.push_back( newAst );//Add to ast
				return newAst;
			}
		}
		else if( token->type == TT::function )
		{//Function deklation/definition
			if( blockStack.size() > 1 )
			{//Is in local scope!
				generateNotification( NT::err_functionDefinitionsAreNotAllowedInLocalScope, token );
				ignoreExpr();
				return nullptr;
			}
			else
			{//Global scope
				std::shared_ptr<AstFunction> newAst = std::make_shared<AstFunction>();
				newAst->token = token;


				if( lValue != nullptr )
				{//Check if has lValue
					generateNotification( NT::err_unexpectedReturningBeforeFunctionDeklaration, currToken );
					ignoreExpr();
					return nullptr;
				}

				auto retType = next();
				if( retType->type == TT::type_byte || retType->type == TT::type_ptr ) newAst->retType = BasicType::tByte;
				else if( retType->type == TT::type_int ) newAst->retType = BasicType::tInt;
				else
				{
					generateNotification( NT::err_unknownTypeAtFunction, retType );
					ignoreExpr();
					return nullptr;
				}

				auto funcNameToken = next();
				auto funcName = funcNameToken->as<Token_Identifier>()->identifier;
				if( funcNameToken->type != TT::identifier )
				{
					generateNotification( NT::err_expectedIdentifier_atFunction, funcNameToken );
					ignoreExpr();
					return nullptr;
				}
				newAst->symbol += funcName;

				//Loop all parameter variables
				if( next()->type != TT::tok_parenthesis_open )
				{
					generateNotification( NT::err_expectedOpeningParenthesis_atFunction, funcNameToken );
					ignoreExpr();
					return nullptr;
				}
				bool loopParameterList = true;
				while( loopParameterList )
				{
					auto typeToken = next();
					BasicType tmpType;
					if( typeToken->type == TT::type_byte ) tmpType = BasicType::tByte;
					else if( typeToken->type == TT::type_int ) tmpType = BasicType::tInt;
					else if( typeToken->type == TT::tok_parenthesis_close )
					{
						loopParameterList = false;
						break;
					}
					else
					{
						generateNotification( NT::err_unknownTypeAtFunctionParameter, typeToken );
						ignoreExpr();
						return nullptr;
					}
					auto tmpName = next();
					if( tmpName->type != TT::identifier )
					{
						generateNotification( NT::err_expectedIdentifier_atFunction, tmpName );
						ignoreExpr();
						return nullptr;
					}

					auto var = std::make_shared<AstVariable>();
					var->model = MemModel::param;
					var->name = tmpName->as<Token_Identifier>()->identifier;
					var->retType = tmpType;
					var->token = tmpName;
					newAst->parameters.push_back( std::pair<String, std::shared_ptr<AstVariable>>( var->name, var ) );

					newAst->symbol += "@" + var->returnTypeString();

					auto last = next();
					if( last->type == TT::tok_comma ) loopParameterList = true;
					else if( last->type == TT::tok_parenthesis_close ) loopParameterList = false;
					else
					{
						generateNotification( NT::err_unknownTokenAfterFunctionParameter, last );
						ignoreExpr();
						return nullptr;
					}
				}

				if( functions[funcName].find( newAst->symbol ) != functions[funcName].end() )
				{
					generateNotification( NT::err_functionIdentifierAlreadyAssigned, funcNameToken );
					ignoreExpr();
					return nullptr;
				}
				functions[funcName][newAst->symbol] = newAst;


				functionParameters = newAst->parameters;
				blockHasHead = true;

				//Resolve block
				auto nextAst = resolveNextExpr();
				if( nextAst->type != AstExpr::Type::block )
				{
					generateNotification( NT::err_expectedBlockAfterFunctionHead, nextAst->token );
				}
				newAst->content = nextAst->as<AstBlock>();


				blockStack.top()->content.push_back( newAst );//Add to ast

				return newAst;
			}
		}
		else if( token->type == TT::cf_return )
		{
			if( blockStack.size() <= 1 )
			{//Is in global scope!
				generateNotification( NT::err_operationsAreNotAllowedInGlobalScope, token );
				ignoreExpr();
				return nullptr;
			}
			else
			{//Not global
				std::shared_ptr<AstReturnStatement> newAst = std::make_shared<AstReturnStatement>();
				newAst->token = token;

				std::shared_ptr<AstExpr> nextAst = resolveNextExpr();
				if( !nextAst->isTypeReturnable() )
				{
					generateNotification( NT::err_expectedReturningExpression_AtReturn, currToken );
					return nullptr;
				}
				newAst->expr = nextAst->as<AstReturning>();

				lValue = newAst;

				return newAst;
			}
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
		else if( token->type == TT::tok_comma )
		{//Enumeration dividing;
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
	std::shared_ptr<AstReturning> AST_Parser::splitMostRightExpr( std::shared_ptr<AstExpr> currLVal, std::shared_ptr<AstTerm> cExpr, int priority )
	{
		//Check if lValue is a term
		std::shared_ptr<AstExpr> clTerm = nullptr;//Should be a term or a return statement
		while( true )
		{//Split all possible (unatomic) expressions
			if( ( currLVal->type == AstExpr::Type::term && opPriority[currLVal->as<AstTerm>()->op] < priority ) ||
				( priority == opPriority[TT::op_set] && opPriority[currLVal->as<AstTerm>()->op] == priority ) )
			{
				clTerm = currLVal;
				currLVal = currLVal->as<AstTerm>()->rVal;
			}
			else if( currLVal->type == AstExpr::Type::returnStat )
			{
				clTerm = currLVal;
				currLVal = currLVal->as<AstReturnStatement>()->expr;
			}
			else break;
		}

		//Test for lTerm
		if( clTerm != nullptr )
		{//Min. 1 term was splitted up
			if( clTerm->type == AstExpr::Type::term )
				clTerm->as<AstTerm>()->rVal = cExpr;
			else if( clTerm->type == AstExpr::Type::returnStat )
				clTerm->as<AstReturnStatement>()->expr = cExpr;
			return currLVal->as<AstReturning>();
		}
		else
		{//LValue is a atomic astExpr
			lValue = cExpr;//Set as lValue
			return currLVal->as<AstReturning>();
		}
	}
	std::shared_ptr<AstReturning> AST_Parser::splitMostRightExpr( std::shared_ptr<AstExpr> currLVal, std::shared_ptr<AstUnary> cExpr, int priority )
	{
		//Check if lValue is a term
		std::shared_ptr<AstTerm> clTerm = nullptr;
		while( ( currLVal->type == AstExpr::Type::term && opPriority[currLVal->as<AstTerm>()->op] < priority ) ||
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
	std::shared_ptr<AstCondition> AST_Parser::splitMostRightExpr( std::shared_ptr<AstExpr> currLVal, std::shared_ptr<AstCombinationCondition> cExpr, int priority )
	{
		//Check if lValue is a term
		std::shared_ptr<AstCombinationCondition> clTerm = nullptr;
		while( currLVal->type == AstExpr::Type::combinationCondition && opPriority[currLVal->as<AstCombinationCondition>()->op] < priority )
		{//Check if lTerm has to be splitted up
			clTerm = currLVal->as<AstCombinationCondition>();
			currLVal = currLVal->as<AstCombinationCondition>()->rVal;
		}

		//Test for lTerm
		if( clTerm != nullptr )
		{//Min. 1 term was splitted up
			clTerm->rVal = cExpr;
			return currLVal->as<AstCondition>();
		}
		else
		{//LValue is a atomic astExpr
			lValue = cExpr;//Set as lValue
			return currLVal->as<AstCondition>();
		}
	}
	std::shared_ptr<AstReturning> AST_Parser::splitMostRightExpr( std::shared_ptr<AstExpr> currLVal, std::shared_ptr<AstComparisonCondition> cExpr, int priority )
	{
		//Check if lValue is a term
		std::shared_ptr<AstExpr> clTerm = nullptr;
		while( true )
		{//Split all possible (unatomic) expressions
			if( currLVal->type == AstExpr::Type::combinationCondition && opPriority[currLVal->as<AstCombinationCondition>()->op] < priority )
			{
				clTerm = currLVal->as<AstCombinationCondition>();
				currLVal = currLVal->as<AstCombinationCondition>()->rVal;
			}
			else if( currLVal->type == AstExpr::Type::comparisonCondition && opPriority[currLVal->as<AstComparisonCondition>()->op] < priority )
			{
				clTerm = currLVal->as<AstComparisonCondition>();
				currLVal = currLVal->as<AstComparisonCondition>()->rVal;
			}
			else if( ( currLVal->type == AstExpr::Type::term && opPriority[currLVal->as<AstTerm>()->op] < priority ) ||
				( priority == opPriority[TT::op_set] && opPriority[currLVal->as<AstTerm>()->op] == priority ) )
			{
				clTerm = currLVal->as<AstTerm>();
				currLVal = currLVal->as<AstTerm>()->rVal;
			}
			else if( currLVal->type == AstExpr::Type::arithmenticCondition )
			{
				currLVal = currLVal->as<AstArithmeticCondition>()->ret;
			}
			else break;
		}

		//Test for lTerm
		if( clTerm != nullptr )
		{//Min. 1 term was splitted up
			if( clTerm->type == AstExpr::Type::combinationCondition ) clTerm->as<AstCombinationCondition>()->rVal = cExpr;
			else generateNotification( NT::err_comparisonConditionCannotBeThisRValue, clTerm->token );
			return currLVal->as<AstReturning>();
		}
		else
		{//LValue is a atomic astExpr
			lValue = cExpr;//Set as lValue
			return currLVal->as<AstReturning>();
		}
	}
	std::shared_ptr<AstReturning> AST_Parser::splitMostRightExpr( std::shared_ptr<AstExpr> currLVal, std::shared_ptr<AstFunctionCall> cExpr, int priority )
	{
		//Check if lValue is a term
		std::shared_ptr<AstTerm> clTerm = nullptr;
		while( ( currLVal->type == AstExpr::Type::term && opPriority[currLVal->as<AstTerm>()->op] < priority ) ||
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
