#include "stdafx.h"
#include "EIR_Parser.h"

namespace nigel
{
	using OC = EIR_Parser::OperationCombination;
	u32 EIR_Variable::nextID { 0 };

	void EIR_Parser::printEIR( CodeBase & base )
	{
		log( "EIR:\n#VARDEF" );

		for( auto v : base.eirValues )
		{//Print all variables
			log( int_to_hex( v.second->id ) + "-" + int_to_hex( v.second->adress ) + "-" + int_to_hex( v.second->size ) );
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
					out += "v" + int_to_hex( var->id ) + "-" + int_to_hex( var->adress ) + "-" + int_to_hex( var->size );
				}
				else if( c->op1->type == EIR_Operator::Type::constant )
				{
					std::shared_ptr<EIR_Constant> var = c->op1->as<EIR_Constant>();
					out += "c" + int_to_hex( var->data );
				}

				if( c->op2 != nullptr )
				{//Has two operators
					out += ", ";
					if( c->op2->type == EIR_Operator::Type::variable )
					{
						std::shared_ptr<EIR_Variable> var = c->op2->as<EIR_Variable>();
						out += "v" + int_to_hex( var->id ) + "-" + int_to_hex( var->adress ) + "-" + int_to_hex( var->size );
					}
					else if( c->op2->type == EIR_Operator::Type::constant )
					{
						std::shared_ptr<EIR_Constant> var = c->op2->as<EIR_Constant>();
						out += "c" + int_to_hex( var->data );
					}
				}
			}
			log( out );
		}

		log( "EIR end" );
	}

	void EIR_Parser::generateSet( OperationCombination comb, std::shared_ptr<EIR_Operator> lOp, std::shared_ptr<EIR_Operator> rOp, std::shared_ptr<Token> lValtoken )
	{
		if( comb == OC::vv )
		{
			if( lOp->as<EIR_Variable>()->model == MemModel::fast &&
				rOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				base->eirCommands.push_back( generateCmd( HexOp::mov_adr_adr, rOp, lOp ) );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large &&
					 rOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				base->eirCommands.push_back( generateCmd( HexOp::mov_a_adr, rOp ) );
				base->eirCommands.push_back( generateCmd( HexOp::mov_dptr_const, lOp ) );
				base->eirCommands.push_back( generateCmd( HexOp::movx_dptr_a ) );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::fast &&
					 rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				base->eirCommands.push_back( generateCmd( HexOp::mov_dptr_const, rOp ) );
				base->eirCommands.push_back( generateCmd( HexOp::movx_a_dptr ) );
				base->eirCommands.push_back( generateCmd( HexOp::mov_adr_a, lOp ) );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large &&
					 rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				base->eirCommands.push_back( generateCmd( HexOp::mov_dptr_const, rOp ) );
				base->eirCommands.push_back( generateCmd( HexOp::movx_a_dptr ) );
				base->eirCommands.push_back( generateCmd( HexOp::mov_dptr_const, lOp ) );
				base->eirCommands.push_back( generateCmd( HexOp::movx_dptr_a ) );
			}
		}
		else if( comb == OC::vc )
		{
			if( lOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				base->eirCommands.push_back( generateCmd( HexOp::mov_adr_const, lOp, rOp ) );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large )
			{
				base->eirCommands.push_back( generateCmd( HexOp::mov_a_const, rOp ) );
				base->eirCommands.push_back( generateCmd( HexOp::mov_dptr_const, lOp ) );
				base->eirCommands.push_back( generateCmd( HexOp::movx_dptr_a ) );
			}
		}
		else if( comb == OC::vt )
		{
			if( lOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				base->eirCommands.push_back( generateCmd( HexOp::mov_adr_r0, lOp ) );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large )
			{
				base->eirCommands.push_back( generateCmd( HexOp::mov_a_r0 ) );
				base->eirCommands.push_back( generateCmd( HexOp::mov_dptr_const, lOp ) );
				base->eirCommands.push_back( generateCmd( HexOp::movx_dptr_a ) );
			}
		}
		else generateNotification( NT::err_cannotSetAConstantLiteral, lValtoken );
	}

	void EIR_Parser::generateOperation( OperationCombination comb, HexOp op_val, HexOp op_const, HexOp op_r0, std::shared_ptr<EIR_Operator> lOp, std::shared_ptr<EIR_Operator> rOp, std::shared_ptr<Token> token )
	{
		if( comb == OC::vv )
		{
			if( lOp->as<EIR_Variable>()->model == MemModel::fast &&
				rOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				base->eirCommands.push_back( generateCmd( HexOp::mov_a_adr, lOp ) );
				base->eirCommands.push_back( generateCmd( op_val, rOp ) );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large &&
					 rOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				base->eirCommands.push_back( generateCmd( HexOp::mov_dptr_const, lOp ) );
				base->eirCommands.push_back( generateCmd( HexOp::movx_a_dptr ) );
				base->eirCommands.push_back( generateCmd( op_val, rOp ) );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::fast &&
					 rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				base->eirCommands.push_back( generateCmd( HexOp::mov_dptr_const, rOp ) );
				base->eirCommands.push_back( generateCmd( HexOp::movx_a_dptr ) );
				base->eirCommands.push_back( generateCmd( HexOp::mov_r0_a ) );
				base->eirCommands.push_back( generateCmd( HexOp::mov_a_adr, lOp ) );
				base->eirCommands.push_back( generateCmd( op_r0 ) );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large &&
					 rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				base->eirCommands.push_back( generateCmd( HexOp::mov_dptr_const, rOp ) );
				base->eirCommands.push_back( generateCmd( HexOp::movx_a_dptr ) );
				base->eirCommands.push_back( generateCmd( HexOp::mov_r0_a ) );
				base->eirCommands.push_back( generateCmd( HexOp::mov_dptr_const, lOp ) );
				base->eirCommands.push_back( generateCmd( HexOp::movx_a_dptr ) );
				base->eirCommands.push_back( generateCmd( op_r0 ) );
			}
		}
		else if( comb == OC::vc )
		{
			if( lOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				base->eirCommands.push_back( generateCmd( HexOp::mov_a_adr, lOp ) );
				base->eirCommands.push_back( generateCmd( op_const, rOp ) );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large )
			{
				base->eirCommands.push_back( generateCmd( HexOp::mov_dptr_const, lOp ) );
				base->eirCommands.push_back( generateCmd( HexOp::movx_a_dptr ) );
				base->eirCommands.push_back( generateCmd( op_const, rOp ) );
			}
		}
		else if( comb == OC::vt )
		{
			if( lOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				base->eirCommands.push_back( generateCmd( HexOp::mov_a_adr, lOp ) );
				base->eirCommands.push_back( generateCmd( op_r0 ) );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large )
			{
				base->eirCommands.push_back( generateCmd( HexOp::mov_dptr_const, lOp ) );
				base->eirCommands.push_back( generateCmd( HexOp::movx_a_dptr ) );
				base->eirCommands.push_back( generateCmd( op_r0 ) );
			}
		}
		else if( comb == OC::cv )
		{
			if( rOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				base->eirCommands.push_back( generateCmd( HexOp::mov_a_const, lOp ) );
				base->eirCommands.push_back( generateCmd( op_val, rOp ) );
			}
			else if( rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				base->eirCommands.push_back( generateCmd( HexOp::mov_dptr_const, rOp ) );
				base->eirCommands.push_back( generateCmd( HexOp::movx_a_dptr ) );
				base->eirCommands.push_back( generateCmd( HexOp::mov_r0_a ) );
				base->eirCommands.push_back( generateCmd( HexOp::mov_a_const, lOp ) );
				base->eirCommands.push_back( generateCmd( op_r0 ) );
			}
		}
		else if( comb == OC::cc )
		{
			generateNotification( NT::imp_operationOnTwoConstantsCanBePrevented, token );
			base->eirCommands.push_back( generateCmd( HexOp::mov_a_const, lOp ) );
			base->eirCommands.push_back( generateCmd( op_const, rOp ) );
		}
		else if( comb == OC::ct )
		{
			base->eirCommands.push_back( generateCmd( HexOp::mov_a_const, lOp ) );
			base->eirCommands.push_back( generateCmd( op_r0 ) );
		}
		else if( comb == OC::tv )
		{
			if( rOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				base->eirCommands.push_back( generateCmd( op_val, rOp ) );
			}
			else if( rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				base->eirCommands.push_back( generateCmd( HexOp::mov_r0_a ) );
				base->eirCommands.push_back( generateCmd( HexOp::mov_dptr_const, rOp ) );
				base->eirCommands.push_back( generateCmd( HexOp::movx_a_dptr ) );
				base->eirCommands.push_back( generateCmd( HexOp::xch_a_r0 ) );
				base->eirCommands.push_back( generateCmd( op_r0 ) );
			}
		}
		else if( comb == OC::tc )
		{
			base->eirCommands.push_back( generateCmd( op_const, rOp ) );
		}
		else if( comb == OC::tt )
		{
			base->eirCommands.push_back( generateCmd( op_r0 ) );
		}
	}

	EIR_Parser::EIR_Parser()
	{
	}
	ExecutionResult EIR_Parser::onExecute( CodeBase &base )
	{
		this->base = &base;
		std::map<String, std::shared_ptr<EIR_Variable>> varList;

		for( auto &v : base.globalAst->variables )
		{//Add all list of the global block.
			varList[v.first] = EIR_Variable::getNew(
				v.second->retType == BasicType::tByte ? 8 :
				v.second->retType == BasicType::tInt ? 16 :
				v.second->retType == BasicType::tUbyte ? 8 :
				v.second->retType == BasicType::tUint ? 16 : 0 );
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
		if( ast->type == AstExpr::Type::allocation )
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
			std::shared_ptr<EIR_Command> newCmd;

			OC comb;//Operator type combination
			std::shared_ptr<EIR_Operator> lOp;
			std::shared_ptr<EIR_Operator> rOp;

			//Memorize rVal
			if( a->rVal->type == AstExpr::Type::variable ) rOp = varList[a->rVal->as<AstVariable>()->name];
			else if( a->rVal->type == AstExpr::Type::literal ) rOp = EIR_Constant::fromAstLiteral( a->rVal->as<AstLiteral>() );
			else if( a->rVal->type == AstExpr::Type::term )
			{
				parseAst( a->rVal, varList );
				base->eirCommands.push_back( generateCmd( HexOp::mov_r0_a ) );
			}
			//Check lValue and combination
			if( a->lVal->type == AstExpr::Type::variable )
			{//lValue is a variable
				lOp = varList[a->lVal->as<AstVariable>()->name];
				if( a->rVal->type == AstExpr::Type::variable ) comb = OC::vv;
				else if( a->rVal->type == AstExpr::Type::literal ) comb = OC::vc;
				else if( a->rVal->type == AstExpr::Type::term ) comb = OC::vt;
			}
			else if( a->lVal->type == AstExpr::Type::literal )
			{//lValue is a constant
				lOp = EIR_Constant::fromAstLiteral( a->lVal->as<AstLiteral>() );
				if( a->rVal->type == AstExpr::Type::variable ) comb = OC::cv;
				else if( a->rVal->type == AstExpr::Type::literal ) comb = OC::cc;
				else if( a->rVal->type == AstExpr::Type::term ) comb = OC::ct;
			}
			else if( a->lVal->type == AstExpr::Type::term )
			{//lValue is a term
				parseAst( a->lVal, varList );
				if( a->rVal->type == AstExpr::Type::variable ) comb = OC::tv;
				else if( a->rVal->type == AstExpr::Type::literal ) comb = OC::tc;
				else if( a->rVal->type == AstExpr::Type::term ) comb = OC::tt;
			}



			//Handle operation
			if( a->op == Token::Type::op_set )
			{// =
				generateSet( comb, lOp, rOp, a->lVal->token );
			}
			else if( a->op == Token::Type::op_add )
			{// +
				generateOperation( comb, HexOp::add_a_adr, HexOp::add_a_const, HexOp::add_a_r0, lOp, rOp, a->token );
			}
			else if( a->op == Token::Type::op_sub )
			{// -
				generateOperation( comb, HexOp::sub_a_adr, HexOp::sub_a_const, HexOp::sub_a_r0, lOp, rOp, a->token );
			}
		}
	}
	std::shared_ptr<EIR_Command> EIR_Parser::generateCmd( HexOp operation, std::shared_ptr<EIR_Operator> lOp, std::shared_ptr<EIR_Operator> rOp )
	{
		auto cmd = std::make_shared<EIR_Command>();
		cmd->op1 = lOp;
		cmd->op2 = rOp;
		cmd->operation = operation;
		return cmd;
	}
}
