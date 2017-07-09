#include "stdafx.h"
#include "EIR_Parser.h"

namespace nigel
{
	using OC = EIR_Parser::OperationCombination;
	using OT = EIR_Parser::OperationType;
	u32 EIR_Variable::nextID { 0 };

	OT EIR_Parser::binaryToUnaryOperationType( OC comb )
	{
		if( comb == OC::vv || comb == OC::vc || comb == OC::vt ) return OT::v;
		else if( comb == OC::cv || comb == OC::cc || comb == OC::ct ) return OT::c;
		else if( comb == OC::tv || comb == OC::tc || comb == OC::tt ) return OT::t;
		else return OT::count;//error
	}

	OC EIR_Parser::setRValTerm( OC comb )
	{
		if( comb == OC::vv || comb == OC::vc ) return OC::vt;
		else if( comb == OC::cv || comb == OC::cc ) return OC::ct;
		else if( comb == OC::tv || comb == OC::tc ) return OC::tt;
		else return comb;
	}

	u8 EIR_Parser::sizeOfType( BasicType type )
	{
		return type == BasicType::tByte ? 8 :
			type == BasicType::tInt ? 16 :
			type == BasicType::tUbyte ? 8 :
			type == BasicType::tUint ? 16 : 0;
	}

	void EIR_Parser::printEIR( CodeBase & base )
	{
		log( "EIR:\n#VARDEF" );

		for( auto v : base.eirValues )
		{//Print all variables
			log( int_to_hex( v.second->id ) + "-" + int_to_hex( v.second->address ) + "-" + int_to_hex( v.second->size ) );
		}

		log( "#CODE" );
		for( auto c : base.eirCommands )
		{//Print all commands
			String out = int_to_hex( static_cast< u8 >( c->operation ) );

			if( c->op1 != nullptr )
			{//Has operator
				out += ", ";
				if( c->op1->type == EIR_Operator::Type::variable )
				{
					std::shared_ptr<EIR_Variable> var = c->op1->as<EIR_Variable>();
					out += "V" + int_to_hex( var->id ) + "-" + int_to_hex( var->address ) + "-" + int_to_hex( var->size );
				}
				else if( c->op1->type == EIR_Operator::Type::constant )
				{
					std::shared_ptr<EIR_Constant> var = c->op1->as<EIR_Constant>();
					out += "C" + int_to_hex( var->data );
				}
				else if( c->op1->type == EIR_Operator::Type::sfr )
				{
					std::shared_ptr<EIR_SFR> var = c->op1->as<EIR_SFR>();
					out += "S" + int_to_hex( var->address );
				}

				if( c->op2 != nullptr )
				{//Has two operators
					out += ", ";
					if( c->op2->type == EIR_Operator::Type::variable )
					{
						std::shared_ptr<EIR_Variable> var = c->op2->as<EIR_Variable>();
						out += "V" + int_to_hex( var->id ) + "-" + int_to_hex( var->address ) + "-" + int_to_hex( var->size );
					}
					else if( c->op2->type == EIR_Operator::Type::constant )
					{
						std::shared_ptr<EIR_Constant> var = c->op2->as<EIR_Constant>();
						out += "C" + int_to_hex( var->data );
					}
					else if( c->op2->type == EIR_Operator::Type::sfr )
					{
						std::shared_ptr<EIR_SFR> var = c->op2->as<EIR_SFR>();
						out += "S" + int_to_hex( var->address );
					}
				}
			}
			log( out );
		}

		log( "EIR end" );
	}


	EIR_Parser::EIR_Parser()
	{
	}
	ExecutionResult EIR_Parser::onExecute( CodeBase &base )
	{
		this->base = &base;
		std::map<String, std::shared_ptr<EIR_Variable>> varList;

		for( auto &v : base.globalAst->variables )
		{//Add all variables of the global block.
			varList[v.first] = EIR_Variable::getNew( sizeOfType( v.second->retType ) );
			varList[v.first]->model = v.second->model;
			base.eirValues[varList[v.first]->id] = varList[v.first];
		}
		for( auto &a : base.globalAst->content )
		{//Iterate the global ast.
			parseAst( a, varList );
		}

		bool hasError = false;
		for( auto &n : notificationList ) if( n->type > NT::begin_err && n->type < NT::begin_warning )
		{
			hasError = true;
			break;
		}


		if( base.printEIR ) printEIR( base );

		if( hasError ) return ExecutionResult::eirParsingFailed;
		else return ExecutionResult::success;
	}
	void EIR_Parser::parseAst( std::shared_ptr<AstExpr> ast, std::map<String, std::shared_ptr<EIR_Variable>> varList )
	{
		if( ast->type == AstExpr::Type::block )
		{
			std::shared_ptr<AstBlock> a = ast->as<AstBlock>();
			std::map<String, std::shared_ptr<EIR_Variable>> localVarList;

			localVarList.insert( varList.begin(), varList.end() );
			u8 stackInc = 0;
			for( auto &v : a->newVariables )
			{
				localVarList[v.first] = EIR_Variable::getNew( sizeOfType( v.second->retType ) );
				localVarList[v.first]->model = MemModel::stack;
				localVarList[v.first]->address = stackInc + 1;
				stackInc += sizeOfType( v.second->retType ) / 8;
			}
			//Subtract final stack increase.
			for( auto &v : a->newVariables ) localVarList[v.first]->address -= stackInc;

			//Increment SP
			addCmd( HexOp::mov_a_adr, EIR_SFR::getSFR( EIR_SFR::SFR::SP ) );
			addCmd( HexOp::add_a_const, EIR_Constant::fromConstant( stackInc ) );
			addCmd( HexOp::mov_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::SP ) );

			for( auto &a : a->content )
			{//Iterate the ast.
				parseAst( a, localVarList );
			}

			//Decrement SP
			addCmd( HexOp::mov_a_adr, EIR_SFR::getSFR( EIR_SFR::SFR::SP ) );
			addCmd( HexOp::add_a_const, EIR_Constant::fromConstant( ~stackInc + 1 ) );
			addCmd( HexOp::mov_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::SP ) );
		}
		else if( ast->type == AstExpr::Type::allocation )
		{//Allocate a variable
			std::shared_ptr<AstAllocation> a = ast->as<AstAllocation>();
			std::shared_ptr<EIR_Command> newCmd = std::make_shared<EIR_Command>();

			/*if( a->rVal->type == AstExpr::Type::literal )
			{//Use the literal
				newCmd->operation = HexOp::mov_adr_const;
				newCmd->op1 = varList[a->lVal->name];
				newCmd->op2 = EIR_Constant::fromAstLiteral( a->rVal->as<AstLiteral>() );
				base->eirCommands.push_back( newCmd );
			}
			else
			{
				//parseAst( a->rVal );//Result to acc
			}*/
		}
		else if( ast->type == AstExpr::Type::term )
		{//Do some operation
			std::shared_ptr<AstTerm> a = ast->as<AstTerm>();

			OC comb;//Operator type combination
			std::shared_ptr<EIR_Operator> lOp;
			std::shared_ptr<EIR_Operator> rOp;

			//Memorize rVal
			if( a->rVal->type == AstExpr::Type::variable ) rOp = varList[a->rVal->as<AstVariable>()->name];
			else if( a->rVal->type == AstExpr::Type::literal ) rOp = EIR_Constant::fromAstLiteral( a->rVal->as<AstLiteral>() );
			else if( a->rVal->type == AstExpr::Type::term ||
					 a->rVal->type == AstExpr::Type::unary ||
					 a->rVal->type == AstExpr::Type::parenthesis )
			{
				parseAst( a->rVal, varList );
				if( a->lVal->type == AstExpr::Type::term ||
					a->lVal->type == AstExpr::Type::unary ||
					a->lVal->type == AstExpr::Type::parenthesis )//push to stack if OC::tt
					addCmd( HexOp::push_adr, EIR_SFR::getSFR( EIR_SFR::SFR::A ) );
				else addCmd( HexOp::mov_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
			}
			//Check lValue and combination
			if( a->lVal->type == AstExpr::Type::variable )
			{//lValue is a variable
				lOp = varList[a->lVal->as<AstVariable>()->name];
				if( a->rVal->type == AstExpr::Type::variable ) comb = OC::vv;
				else if( a->rVal->type == AstExpr::Type::literal ) comb = OC::vc;
				else if( a->rVal->type == AstExpr::Type::term ||
						 a->rVal->type == AstExpr::Type::unary ||
						 a->rVal->type == AstExpr::Type::parenthesis ) comb = OC::vt;
			}
			else if( a->lVal->type == AstExpr::Type::literal )
			{//lValue is a constant
				lOp = EIR_Constant::fromAstLiteral( a->lVal->as<AstLiteral>() );
				if( a->rVal->type == AstExpr::Type::variable ) comb = OC::cv;
				else if( a->rVal->type == AstExpr::Type::literal ) comb = OC::cc;
				else if( a->rVal->type == AstExpr::Type::term ||
						 a->rVal->type == AstExpr::Type::unary ||
						 a->rVal->type == AstExpr::Type::parenthesis ) comb = OC::ct;
			}
			else if( a->lVal->type == AstExpr::Type::term ||
					 a->lVal->type == AstExpr::Type::unary ||
					 a->lVal->type == AstExpr::Type::parenthesis )
			{//lValue is a term
				parseAst( a->lVal, varList );
				if( a->rVal->type == AstExpr::Type::variable ) comb = OC::tv;
				else if( a->rVal->type == AstExpr::Type::literal ) comb = OC::tc;
				else if( a->rVal->type == AstExpr::Type::term ||
						 a->rVal->type == AstExpr::Type::unary ||
						 a->rVal->type == AstExpr::Type::parenthesis )
				{//pop from stack to B
					comb = OC::tt;
					addCmd( HexOp::pop_adr, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				}
			}


			//Handle operation
			if( a->op == Token::Type::op_set )
			{// =
				generateSet( comb, lOp, rOp, a->lVal->token, false );
			}
			if( a->op == Token::Type::op_set_get )
			{// = (The operation is itselve a returning, so it has to copy the value into the acc)
				generateSet( comb, lOp, rOp, a->lVal->token, true );
			}
			else if( a->op == Token::Type::op_add )
			{// +
				generateOperation( comb, HexOp::add_a_adr, HexOp::add_a_const, HexOp::add_a_atr0, lOp, rOp, a->token );
			}
			else if( a->op == Token::Type::op_sub )
			{// -
				addCmd( HexOp::clr_c );
				generateOperation( comb, HexOp::sub_a_adr, HexOp::sub_a_const, HexOp::sub_a_atr0, lOp, rOp, a->token );
			}
			else if( a->op == Token::Type::op_mul )
			{// *
				generateMoveAB( comb, lOp, rOp, a->token );
				addCmd( HexOp::mul_a_b );
			}
			else if( a->op == Token::Type::op_div )
			{// /
				generateMoveAB( comb, lOp, rOp, a->token );
				addCmd( HexOp::div_a_b );
			}
			else if( a->op == Token::Type::op_mod )
			{// %
				generateMoveAB( comb, lOp, rOp, a->token );
				addCmd( HexOp::div_a_b );
				addCmd( HexOp::xch_a_adr, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
			}
			else if( a->op == Token::Type::op_and )
			{// &
				generateOperation( comb, HexOp::and_a_adr, HexOp::and_a_const, HexOp::and_a_atr0, lOp, rOp, a->token );
			}
			else if( a->op == Token::Type::op_or )
			{// |
				generateOperation( comb, HexOp::or_a_adr, HexOp::or_a_const, HexOp::or_a_atr0, lOp, rOp, a->token );
			}
			else if( a->op == Token::Type::op_xor )
			{// ^
				generateOperation( comb, HexOp::xor_a_adr, HexOp::xor_a_const, HexOp::xor_a_atr0, lOp, rOp, a->token );
			}
			else if( a->op == Token::Type::op_shift_left )
			{// <<
				if( a->rVal->type != AstExpr::Type::literal || rOp->type != EIR_Operator::Type::constant )
				{
					generateNotification( NT::err_onlyConstantsAreAllowedForBitShifts, a->rVal->token );
					return;
				}
				else if( rOp->as<EIR_Constant>()->data >= 0x80 )
				{
					generateNotification( NT::err_onlyPositiveConstantsAreAllowedForBitShifts, a->rVal->token );
					return;
				}
				generateUnaryLOperation( binaryToUnaryOperationType( comb ), HexOp::rl_a, lOp, a->token, false, rOp->as<EIR_Constant>()->data );
			}
			else if( a->op == Token::Type::op_shift_right )
			{// >>
				if( a->rVal->type != AstExpr::Type::literal || rOp->type != EIR_Operator::Type::constant )
				{
					generateNotification( NT::err_onlyConstantsAreAllowedForBitShifts, a->rVal->token );
					return;
				}
				else if( rOp->as<EIR_Constant>()->data >= 0x80 )
				{
					generateNotification( NT::err_onlyPositiveConstantsAreAllowedForBitShifts, a->rVal->token );
					return;
				}
				generateUnaryLOperation( binaryToUnaryOperationType( comb ), HexOp::rr_a, lOp, a->token, false, rOp->as<EIR_Constant>()->data );
			}


			else if( a->op == Token::Type::op_add_set )
			{// +=
				generateOperation( comb, HexOp::add_a_adr, HexOp::add_a_const, HexOp::add_a_atr0, lOp, rOp, a->token );
				generateSetAfterOp( setRValTerm( comb ), lOp, a->lVal->token );
			}
			else if( a->op == Token::Type::op_sub_set )
			{// -=
				addCmd( HexOp::clr_c );
				generateOperation( comb, HexOp::sub_a_adr, HexOp::sub_a_const, HexOp::sub_a_atr0, lOp, rOp, a->token );
				generateSetAfterOp( setRValTerm( comb ), lOp, a->lVal->token );
			}
			else if( a->op == Token::Type::op_mul_set )
			{// *=
				generateMoveAB( comb, lOp, rOp, a->token );
				addCmd( HexOp::mul_a_b );
				generateSetAfterOp( setRValTerm( comb ), lOp, a->lVal->token );
			}
			else if( a->op == Token::Type::op_div_set )
			{// /=
				generateMoveAB( comb, lOp, rOp, a->token );
				addCmd( HexOp::div_a_b );
				generateSetAfterOp( setRValTerm( comb ), lOp, a->lVal->token );
			}
			else if( a->op == Token::Type::op_mod_set )
			{// %=
				generateMoveAB( comb, lOp, rOp, a->token );
				addCmd( HexOp::div_a_b );
				addCmd( HexOp::xch_a_adr, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				generateSetAfterOp( setRValTerm( comb ), lOp, a->lVal->token );
			}
			else if( a->op == Token::Type::op_and_set )
			{// &=
				generateOperation( comb, HexOp::and_a_adr, HexOp::and_a_const, HexOp::and_a_atr0, lOp, rOp, a->token );
				generateSetAfterOp( setRValTerm( comb ), lOp, a->lVal->token );
			}
			else if( a->op == Token::Type::op_or_set )
			{// |=
				generateOperation( comb, HexOp::or_a_adr, HexOp::or_a_const, HexOp::or_a_atr0, lOp, rOp, a->token );
				generateSetAfterOp( setRValTerm( comb ), lOp, a->lVal->token );
			}
			else if( a->op == Token::Type::op_xor_set )
			{// ^=
				generateOperation( comb, HexOp::xor_a_adr, HexOp::xor_a_const, HexOp::xor_a_atr0, lOp, rOp, a->token );
				generateSetAfterOp( setRValTerm( comb ), lOp, a->lVal->token );
			}
			else if( a->op == Token::Type::op_shift_left_set )
			{// <<=
				if( a->rVal->type != AstExpr::Type::literal || rOp->type != EIR_Operator::Type::constant )
				{
					generateNotification( NT::err_onlyConstantsAreAllowedForBitShifts, a->rVal->token );
					return;
				}
				else if( rOp->as<EIR_Constant>()->data >= 0x80 )
				{
					generateNotification( NT::err_onlyPositiveConstantsAreAllowedForBitShifts, a->rVal->token );
					return;
				}
				generateUnaryLOperation( binaryToUnaryOperationType( comb ), HexOp::rl_a, lOp, a->token, false, rOp->as<EIR_Constant>()->data );
				generateSetAfterOp( setRValTerm( comb ), lOp, a->lVal->token );
			}
			else if( a->op == Token::Type::op_shift_right_set )
			{// >>=
				if( a->rVal->type != AstExpr::Type::literal || rOp->type != EIR_Operator::Type::constant )
				{
					generateNotification( NT::err_onlyConstantsAreAllowedForBitShifts, a->rVal->token );
					return;
				}
				else if( rOp->as<EIR_Constant>()->data >= 0x80 )
				{
					generateNotification( NT::err_onlyPositiveConstantsAreAllowedForBitShifts, a->rVal->token );
					return;
				}
				generateUnaryLOperation( binaryToUnaryOperationType( comb ), HexOp::rr_a, lOp, a->token, false, rOp->as<EIR_Constant>()->data );
				generateSetAfterOp( setRValTerm( comb ), lOp, a->lVal->token );
			}


			else
			{
				//todo error: unknown binary operator
			}

		}
		else if( ast->type == AstExpr::Type::unary )
		{//Do some operation
			std::shared_ptr<AstUnary> a = ast->as<AstUnary>();
			OperationType ot;//Operator type combination
			std::shared_ptr<EIR_Operator> op;

			//Memorize val
			if( a->val->type == AstExpr::Type::variable )
			{
				ot = OT::v;
				op = varList[a->val->as<AstVariable>()->name];
			}
			else if( a->val->type == AstExpr::Type::literal )
			{
				ot = OT::c;
				op = EIR_Constant::fromAstLiteral( a->val->as<AstLiteral>() );
			}
			else if( a->val->type == AstExpr::Type::term )
			{
				ot = OT::t;
				parseAst( a->val, varList );
			}


			if( a->side == AstUnary::Side::left )
			{//L op
				if( a->token->type == Token::Type::op_inc )
				{// ++
					generateUnaryLOperation( ot, HexOp::inc_a, op, a->token, true );
				}
				else if( a->token->type == Token::Type::op_dec )
				{// --
					generateUnaryLOperation( ot, HexOp::dec_a, op, a->token, true );
				}
				else if( a->token->type == Token::Type::op_sub )
				{// -
					OC comb = OC::cv;
					if( ot == OT::v ) comb = OC::cv;
					else if( ot == OT::c ) comb = OC::cc;
					else if( ot == OT::t ) comb = OC::ct;

					generateOperation( comb, HexOp::sub_a_adr, HexOp::sub_a_const, HexOp::sub_a_atr0, std::make_shared<EIR_Constant>( 0 ), op, a->token );
				}
				else if( a->token->type == Token::Type::op_add )
				{// + //Ignore this operation and do nothing
					generateMoveA( ot, op );
				}
				else if( a->token->type == Token::Type::op_inv )
				{// ~
					generateUnaryLOperation( ot, HexOp::cpl_a, op, a->token, false );
				}
				else
				{
					//todo error: unknown unary operator
				}
			}
			else
			{//R op
				if( a->token->type == Token::Type::op_inc )
				{// ++
					generateUnaryROperation( ot, HexOp::inc_a, op, a->token );
				}
				else if( a->token->type == Token::Type::op_dec )
				{// --
					generateUnaryROperation( ot, HexOp::dec_a, op, a->token );
				}
			}
		}
		else if( ast->type == AstExpr::Type::parenthesis )
		{//Submit content
			std::shared_ptr<AstParenthesis> a = ast->as<AstParenthesis>();
			OperationType ot;//Operator type combination
			std::shared_ptr<EIR_Operator> op;

			//Memorize val
			if( a->content->type == AstExpr::Type::variable )
			{
				ot = OT::v;
				op = varList[a->content->as<AstVariable>()->name];
			}
			else if( a->content->type == AstExpr::Type::literal )
			{
				ot = OT::c;
				op = EIR_Constant::fromAstLiteral( a->content->as<AstLiteral>() );
			}
			else if( a->content->type == AstExpr::Type::term )
			{
				ot = OT::t;
				parseAst( a->content, varList );
			}

			generateMoveA( ot, op );
		}
	}

	void EIR_Parser::generateSet( OperationCombination comb, std::shared_ptr<EIR_Operator> lOp, std::shared_ptr<EIR_Operator> rOp, std::shared_ptr<Token> lValToken, bool copyInAcc )
	{
		if( comb == OC::vv )
		{
			if( lOp->as<EIR_Variable>()->model == MemModel::fast &&
				rOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				if( copyInAcc )
				{
					addCmd( HexOp::mov_a_adr, rOp );
					addCmd( HexOp::mov_adr_a, lOp );
				}
				else addCmd( HexOp::mov_adr_adr, rOp, lOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large &&
					 rOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				addCmd( HexOp::mov_a_adr, rOp );
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_dptr_a );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::stack &&
					 rOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				generateLoadStackR0( lOp );
				if( copyInAcc )
				{
					addCmd( HexOp::mov_a_adr, rOp );
					addCmd( HexOp::mov_atr0_a );
				}
				else addCmd( HexOp::mov_atr0_adr, rOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::fast &&
					 rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::mov_dptr_const, rOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( HexOp::mov_adr_a, lOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large &&
					 rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::mov_dptr_const, rOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_dptr_a );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::stack &&
					 rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				generateLoadStackR0( lOp );
				addCmd( HexOp::mov_dptr_const, rOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( HexOp::mov_atr0_a );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::fast &&
					 rOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( rOp );
				if( copyInAcc )
				{
					addCmd( HexOp::mov_a_atr0 );
					addCmd( HexOp::mov_adr_a, lOp );
				}
				else addCmd( HexOp::mov_adr_atr0, rOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large &&
					 rOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( rOp );
				addCmd( HexOp::mov_a_atr0 );
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_dptr_a );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::stack &&
					 rOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( rOp );
				generateLoadStackR1( lOp );
				addCmd( HexOp::mov_a_atr0 );
				addCmd( HexOp::mov_atr1_a );
			}
		}
		else if( comb == OC::vc )
		{
			if( lOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				if( copyInAcc )
				{
					addCmd( HexOp::mov_a_const, rOp );
					addCmd( HexOp::mov_adr_a, lOp );
				}
				else addCmd( HexOp::mov_adr_const, lOp, rOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::mov_a_const, rOp );
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_dptr_a );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( lOp );
				if( copyInAcc )
				{
					addCmd( HexOp::mov_a_const, rOp );
					addCmd( HexOp::mov_atr0_a );
				}
				else addCmd( HexOp::mov_atr0_const, rOp );
			}
		}
		else if( comb == OC::vt )
		{
			if( lOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				if( copyInAcc )
				{
					addCmd( HexOp::mov_a_adr, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
					addCmd( HexOp::mov_adr_a, lOp );
				}
				else addCmd( HexOp::mov_adr_adr, EIR_SFR::getSFR( EIR_SFR::SFR::B ), lOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::mov_a_adr, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_dptr_a );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( lOp );
				if( copyInAcc )
				{
					addCmd( HexOp::mov_a_adr, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
					addCmd( HexOp::mov_atr0_a );
				}
				else addCmd( HexOp::mov_atr0_adr, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
			}
		}
		else generateNotification( NT::err_cannotSetAConstantLiteral, lValToken );
	}

	void EIR_Parser::generateSetAfterOp( OperationCombination comb, std::shared_ptr<EIR_Operator> lOp, std::shared_ptr<Token> lValToken )
	{
		if( comb == OC::vv || comb == OC::vc || comb == OC::vt )
		{//Is the same, because after a operation everything will be located in the acc.
			if( lOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				addCmd( HexOp::mov_adr_a, lOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_dptr_a );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( lOp );
				addCmd( HexOp::mov_atr0_a );
			}
		}
		else generateNotification( NT::err_cannotSetAConstantLiteral, lValToken );
	}

	void EIR_Parser::generateOperation( OperationCombination comb, HexOp op_val, HexOp op_const, HexOp op_atr0, std::shared_ptr<EIR_Operator> lOp, std::shared_ptr<EIR_Operator> rOp, std::shared_ptr<Token> token )
	{
		if( comb == OC::vv )
		{
			if( lOp->as<EIR_Variable>()->model == MemModel::fast &&
				rOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				addCmd( HexOp::mov_a_adr, lOp );
				addCmd( op_val, rOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large &&
					 rOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( op_val, rOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::stack &&
				rOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				generateLoadStackR0( lOp );
				addCmd( HexOp::mov_a_atr0 );
				addCmd( op_val, rOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::fast &&
					 rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::mov_dptr_const, rOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( HexOp::mov_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_a_adr, lOp );
				addCmd( op_val, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large &&
					 rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::mov_dptr_const, rOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( HexOp::mov_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( op_val, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::stack &&
					 rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				generateLoadStackR0( lOp );
				addCmd( HexOp::mov_dptr_const, rOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( HexOp::mov_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_a_atr0 );
				addCmd( op_val, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::fast &&
					 rOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( rOp );
				addCmd( HexOp::mov_a_adr, lOp );
				addCmd( op_atr0 );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large &&
					 rOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( rOp );
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( op_atr0 );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::stack &&
					 rOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( rOp );
				generateLoadStackR1( lOp );
				addCmd( HexOp::mov_a_atr1 );
				addCmd( op_atr0 );
			}
		}
		else if( comb == OC::vc )
		{
			if( lOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				addCmd( HexOp::mov_a_adr, lOp );
				addCmd( op_const, rOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( op_const, rOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( lOp );
				addCmd( HexOp::mov_a_atr0 );
				addCmd( op_const, rOp );
			}
		}
		else if( comb == OC::vt )
		{
			if( lOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				addCmd( HexOp::mov_a_adr, lOp );
				addCmd( op_val, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( op_val, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( lOp );
				addCmd( HexOp::mov_a_atr0 );
				addCmd( op_val, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
			}
		}
		else if( comb == OC::cv )
		{
			if( rOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				addCmd( HexOp::mov_a_const, lOp );
				addCmd( op_val, rOp );
			}
			else if( rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::mov_dptr_const, rOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( HexOp::mov_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_a_const, lOp );
				addCmd( op_val, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
			}
			else if( rOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( rOp );
				addCmd( HexOp::mov_a_const, lOp );
				addCmd( op_atr0 );
			}
		}
		else if( comb == OC::cc )
		{
			generateNotification( NT::imp_operationOnTwoConstantsCanBePrevented, token );
			addCmd( HexOp::mov_a_const, lOp );
			addCmd( op_const, rOp );
		}
		else if( comb == OC::ct )
		{
			addCmd( HexOp::mov_a_const, lOp );
			addCmd( op_val, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
		}
		else if( comb == OC::tv )
		{
			if( rOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				addCmd( op_val, rOp );
			}
			else if( rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::xch_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_dptr_const, rOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( HexOp::xch_a_adr, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( op_val, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
			}
			else if( rOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( rOp );
				addCmd( op_atr0 );
			}
		}
		else if( comb == OC::tc )
		{
			addCmd( op_const, rOp );
		}
		else if( comb == OC::tt )
		{
			addCmd( op_val, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
		}
	}

	void EIR_Parser::generateMoveAB( OperationCombination comb, std::shared_ptr<EIR_Operator> lOp, std::shared_ptr<EIR_Operator> rOp, std::shared_ptr<Token> token )
	{
		if( comb == OC::vv )
		{
			if( lOp->as<EIR_Variable>()->model == MemModel::fast &&
				rOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				addCmd( HexOp::mov_adr_adr, rOp, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_a_adr, lOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large &&
					 rOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( HexOp::mov_adr_adr, rOp, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::stack &&
				rOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				generateLoadStackR0( lOp );
				addCmd( HexOp::mov_adr_adr, rOp, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_a_atr0 );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::fast &&
					 rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::mov_dptr_const, rOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( HexOp::mov_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_a_adr, lOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large &&
					 rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::mov_dptr_const, rOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( HexOp::mov_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_a_dptr );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::stack &&
					 rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				generateLoadStackR0( lOp );
				addCmd( HexOp::mov_dptr_const, rOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( HexOp::mov_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_a_atr0 );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::fast &&
					 rOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( rOp );
				addCmd( HexOp::mov_adr_atr0, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_a_adr, lOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large &&
					 rOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( rOp );
				addCmd( HexOp::mov_adr_atr0, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_a_dptr );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::stack &&
					 rOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( rOp );
				addCmd( HexOp::mov_adr_atr0, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				generateLoadStackR0( lOp );
				addCmd( HexOp::mov_a_atr0 );
			}
		}
		else if( comb == OC::vc )
		{
			if( lOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				addCmd( HexOp::mov_adr_const, EIR_SFR::getSFR( EIR_SFR::SFR::B ), rOp );
				addCmd( HexOp::mov_a_adr, lOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::mov_adr_const, EIR_SFR::getSFR( EIR_SFR::SFR::B ), rOp );
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_a_dptr );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( lOp );
				addCmd( HexOp::mov_adr_const, EIR_SFR::getSFR( EIR_SFR::SFR::B ), rOp );
				addCmd( HexOp::mov_a_atr0 );
			}
		}
		else if( comb == OC::vt )
		{
			if( lOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				addCmd( HexOp::mov_a_adr, lOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_a_dptr );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( lOp );
				addCmd( HexOp::mov_a_atr0 );
			}
		}
		else if( comb == OC::cv )
		{
			if( rOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				addCmd( HexOp::mov_adr_adr, rOp, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_a_const, lOp );
			}
			else if( rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::mov_adr_const, rOp, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_a_dptr );
			}
			else if( rOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( rOp );
				addCmd( HexOp::mov_adr_atr0, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_a_const, lOp );
			}
		}
		else if( comb == OC::cc )
		{
			generateNotification( NT::imp_operationOnTwoConstantsCanBePrevented, token );
			addCmd( HexOp::mov_adr_const, EIR_SFR::getSFR( EIR_SFR::SFR::B ), rOp );
			addCmd( HexOp::mov_a_const, lOp );
		}
		else if( comb == OC::ct )
		{
			addCmd( HexOp::mov_a_const, lOp );
		}
		else if( comb == OC::tv )
		{
			if( rOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				addCmd( HexOp::mov_adr_adr, rOp, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
			}
			else if( rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::xch_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_dptr_const, rOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( HexOp::xch_a_adr, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
			}
			else if( rOp->as<EIR_Variable>()->model == MemModel::stack )
			{
				addCmd( HexOp::xch_a_r0 );
				generateLoadStackR0( rOp );
				addCmd( HexOp::mov_adr_atr0, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
			}
		}
		else if( comb == OC::tc )
		{
			addCmd( HexOp::mov_adr_const, EIR_SFR::getSFR( EIR_SFR::SFR::B ), rOp );
		}
		//Ignore OC::tt because it is already correct.
	}

	void EIR_Parser::generateMoveA( OperationType ot, std::shared_ptr<EIR_Operator> op )
	{
		if( ot == OT::v )
		{
			if( op->as<EIR_Variable>()->model == MemModel::fast )
			{
				addCmd( HexOp::mov_a_adr, op );
			}
			else if( op->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::mov_dptr_const, op );
				addCmd( HexOp::movx_a_dptr );
			}
			else if( op->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( op );
				addCmd( HexOp::mov_a_atr0 );
			}
		}
		else if( ot == OT::c )
		{
			addCmd( HexOp::mov_a_const, op );
		}
		//Ignore OT::t because it is already correct.
	}

	void EIR_Parser::generateUnaryLOperation( OperationType comb, HexOp op_acc, std::shared_ptr<EIR_Operator> op, std::shared_ptr<Token> token, bool changeValue, u16 iterations )
	{
		if( comb == OT::v )
		{
			if( op->as<EIR_Variable>()->model == MemModel::fast )
			{
				addCmd( HexOp::mov_a_adr, op );
				for( int i = 0 ; i < iterations ; i++ )
					addCmd( op_acc );
				if( changeValue ) addCmd( HexOp::mov_adr_a, op );
			}
			else if( op->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::mov_dptr_const, op );
				addCmd( HexOp::movx_a_dptr );
				for( int i = 0 ; i < iterations ; i++ )
					addCmd( op_acc );
				if( changeValue ) addCmd( HexOp::movx_dptr_a );
			}
			else if( op->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( op );
				addCmd( HexOp::mov_a_atr0, op );
				for( int i = 0 ; i < iterations ; i++ )
					addCmd( op_acc );
				if( changeValue ) addCmd( HexOp::mov_atr0_a );
			}
		}
		else if( comb == OT::c )
		{
			generateNotification( NT::imp_operationOnConstantCanBePrevented, token );
			addCmd( HexOp::mov_a_const, op );
			for( int i = 0 ; i < iterations ; i++ )
				addCmd( op_acc );
		}
		else if( comb == OT::t )
		{
			for( int i = 0 ; i < iterations ; i++ )
				addCmd( op_acc );
		}
	}

	void EIR_Parser::generateUnaryROperation( OperationType comb, HexOp op_acc, std::shared_ptr<EIR_Operator> op, std::shared_ptr<Token> token )
	{
		if( comb == OT::v )
		{
			if( op->as<EIR_Variable>()->model == MemModel::fast )
			{
				addCmd( HexOp::mov_a_adr, op );
				addCmd( op_acc );
				addCmd( HexOp::xch_a_adr, op );
			}
			else if( op->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::mov_dptr_const, op );
				addCmd( HexOp::movx_a_dptr );
				addCmd( HexOp::mov_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( op_acc );
				addCmd( HexOp::movx_dptr_a );
				addCmd( HexOp::mov_a_adr, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
			}
			else if( op->as<EIR_Variable>()->model == MemModel::stack )
			{
				generateLoadStackR0( op );
				addCmd( HexOp::mov_a_atr0 );
				addCmd( op_acc );
				addCmd( HexOp::xch_a_atr0 );
			}
		}
		else if( comb == OT::c )
		{
			generateNotification( NT::imp_operationOnConstantCanBePrevented, token );
			addCmd( HexOp::mov_a_const, op );
			addCmd( op_acc );
		}
		else if( comb == OT::t )
		{
			addCmd( op_acc );
		}
	}

	void EIR_Parser::generateLoadStackR0( std::shared_ptr<EIR_Operator> op )
	{
		addCmd( HexOp::mov_a_adr, EIR_SFR::getSFR( EIR_SFR::SFR::SP ) );
		addCmd( HexOp::clr_c );
		addCmd( HexOp::sub_a_const, op );
		addCmd( HexOp::xch_a_r0 );
	}

	void EIR_Parser::generateLoadStackR1( std::shared_ptr<EIR_Operator> op )
	{
		addCmd( HexOp::mov_a_adr, EIR_SFR::getSFR( EIR_SFR::SFR::SP ) );
		addCmd( HexOp::clr_c );
		addCmd( HexOp::sub_a_const, op );
		addCmd( HexOp::xch_a_r1 );
	}

	std::shared_ptr<EIR_Command> EIR_Parser::generateCmd( HexOp operation, std::shared_ptr<EIR_Operator> lOp, std::shared_ptr<EIR_Operator> rOp )
	{
		auto cmd = std::make_shared<EIR_Command>();
		cmd->op1 = lOp;
		cmd->op2 = rOp;
		cmd->operation = operation;
		return cmd;
	}

	void EIR_Parser::addCmd( HexOp operation, std::shared_ptr<EIR_Operator> lOp, std::shared_ptr<EIR_Operator> rOp )
	{
		base->eirCommands.push_back( generateCmd( operation, lOp, rOp ) );
	}
}
