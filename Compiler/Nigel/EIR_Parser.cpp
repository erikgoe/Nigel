#include "stdafx.h"
#include "EIR_Parser.h"

namespace nigel
{
	using OC = EIR_Parser::OperationCombination;
	using OT = EIR_Parser::OperationType;
	u32 EIR_Variable::nextID { 0 };
	u32 EIR_Condition::nextConditionPos { 0 };

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
			String out;
			if( c->type == EIR_Command::Type::operation )
			{
				out = int_to_hex( static_cast< u8 >( c->operation ) );

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
					else if( c->op1->type == EIR_Operator::Type::block )
					{
						std::shared_ptr<EIR_Block> var = c->op1->as<EIR_Block>();
						if( var->symbol != "" ) out += "B::" + var->symbol;
						else out += "B" + int_to_hex( var->blockID ) + ( var->begin ? "B" : "E" );
					}
					else if( c->op1->type == EIR_Operator::Type::condition )
					{
						std::shared_ptr<EIR_Condition> var = c->op1->as<EIR_Condition>();
						out += "D" + int_to_hex( var->conditionID );
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
						else if( c->op2->type == EIR_Operator::Type::block )
						{
							std::shared_ptr<EIR_Block> var = c->op1->as<EIR_Block>();
							if( var->symbol != "" ) out += "B::" + var->symbol;
							else out += "B" + int_to_hex( var->blockID ) + ( var->begin ? "B" : "E" );
						}
						else if( c->op2->type == EIR_Operator::Type::condition )
						{
							std::shared_ptr<EIR_Condition> var = c->op2->as<EIR_Condition>();
							out += "D" + int_to_hex( var->conditionID );
						}
					}
				}
			}
			else if( c->type == EIR_Command::Type::blockBegin )
			{
				if( c->symbol != "" ) out = " :blockB, " + c->symbol + ", ";
				else out = " :blockB, ";
				out += int_to_hex( c->id );
			}
			else if( c->type == EIR_Command::Type::blockEnd )
			{
				out = " :blockE, " + int_to_hex( c->id );
			}
			else if( c->type == EIR_Command::Type::blockFinish )
			{
				out = " :blockF, " + int_to_hex( c->id );
			}
			else if( c->type == EIR_Command::Type::conditionEnd )
			{
				out = " :condition, " + int_to_hex( c->id );
			}
			log( out );
		}

		log( "EIR end" );
	}

	void EIR_Parser::printAssembly( CodeBase & base )
	{
		log( "__ASM__" );

		String tabs = "";

		for( auto c : base.eirCommands )
		{//Print all commands
			String out = tabs;
			if( c->type == EIR_Command::Type::operation )
			{
				if( c->operation == HexOp::nop )					out += "NOP";

				else if( c->operation == HexOp::clr_a )				out += "CLR A";
				else if( c->operation == HexOp::clr_c )				out += "CLR C";

				else if( c->operation == HexOp::mov_a_const )		out += "MOV A, " + operatorToString( c->op1 );
				else if( c->operation == HexOp::mov_a_adr )			out += "MOV A, " + operatorToString( c->op1 );
				else if( c->operation == HexOp::mov_a_r0 )			out += "MOV A, R0";
				else if( c->operation == HexOp::mov_a_atr0 )		out += "MOV A, @R0";
				else if( c->operation == HexOp::mov_a_atr1 )		out += "MOV A, @R1";
				else if( c->operation == HexOp::mov_adr_const )		out += "MOV " + operatorToString( c->op1 ) + ", " + operatorToString( c->op2 );
				else if( c->operation == HexOp::mov_adr_a )			out += "MOV " + operatorToString( c->op1 ) + ", A";
				else if( c->operation == HexOp::mov_adr_r0 )		out += "MOV " + operatorToString( c->op1 ) + ", R0";
				else if( c->operation == HexOp::mov_adr_atr0 )		out += "MOV " + operatorToString( c->op1 ) + ", @R0";
				else if( c->operation == HexOp::mov_adr_adr )		out += "MOV " + operatorToString( c->op1 ) + ", " + operatorToString( c->op2 ) + " ; r into l";
				else if( c->operation == HexOp::mov_r0_const )		out += "MOV R0, " + operatorToString( c->op1 );
				else if( c->operation == HexOp::mov_r0_a )			out += "MOV R0, A";
				else if( c->operation == HexOp::mov_r0_adr )		out += "MOV R0, " + operatorToString( c->op1 );
				else if( c->operation == HexOp::mov_atr0_const )	out += "MOV @R0, " + operatorToString( c->op1 );
				else if( c->operation == HexOp::mov_atr0_a )		out += "MOV @R0, A";
				else if( c->operation == HexOp::mov_atr1_a )		out += "MOV @R1, A";
				else if( c->operation == HexOp::mov_atr0_adr )		out += "MOV @R0, " + operatorToString( c->op1 );
				else if( c->operation == HexOp::mov_dptr_const )	out += "MOV DPTR, " + operatorToString( c->op1 );
				else if( c->operation == HexOp::movx_a_dptr )		out += "MOVX A, @DPTR";
				else if( c->operation == HexOp::movx_dptr_a )		out += "MOVX @DPTR, A";

				else if( c->operation == HexOp::push_adr )			out += "PUSH " + operatorToString( c->op1 );
				else if( c->operation == HexOp::pop_adr )			out += "POP " + operatorToString( c->op1 );

				else if( c->operation == HexOp::xch_a_r0 )			out += "XCH A, R0";
				else if( c->operation == HexOp::xch_r0_a )			out += "XCH R0, A";
				else if( c->operation == HexOp::xch_a_r1 )			out += "XCH A, R1";
				else if( c->operation == HexOp::xch_r1_a )			out += "XCH R1, A";
				else if( c->operation == HexOp::xch_a_atr0 )		out += "XCH A, @R0";
				else if( c->operation == HexOp::xch_atr0_a )		out += "XCH @R0, A";
				else if( c->operation == HexOp::xch_a_adr )			out += "XCH A, " + operatorToString( c->op1 );
				else if( c->operation == HexOp::xch_adr_a )			out += "XCH " + operatorToString( c->op1 ) + ", A";

				else if( c->operation == HexOp::add_a_const )		out += "ADD A, " + operatorToString( c->op1 );
				else if( c->operation == HexOp::add_a_adr )			out += "ADD A, " + operatorToString( c->op1 );
				else if( c->operation == HexOp::add_a_r0 )			out += "ADD A, R0";
				else if( c->operation == HexOp::add_a_atr0 )		out += "ADD A, @R0";
				else if( c->operation == HexOp::sub_a_const )		out += "SUBB A, " + operatorToString( c->op1 );
				else if( c->operation == HexOp::sub_a_adr )			out += "SUBB A, " + operatorToString( c->op1 );
				else if( c->operation == HexOp::sub_a_r0 )			out += "SUBB A, R0";
				else if( c->operation == HexOp::sub_a_atr0 )		out += "SUBB A, @R0";
				else if( c->operation == HexOp::mul_a_b )			out += "MUL AB";
				else if( c->operation == HexOp::div_a_b )			out += "DIV AB";

				else if( c->operation == HexOp::inc_a )				out += "INC A";
				else if( c->operation == HexOp::dec_a )				out += "DEC A";
				else if( c->operation == HexOp::rr_a )				out += "RR A";
				else if( c->operation == HexOp::rl_a )				out += "RL A";

				else if( c->operation == HexOp::and_a_const )		out += "ANL A, #" + operatorToString( c->op1 );
				else if( c->operation == HexOp::and_a_adr )			out += "ANL A, " + operatorToString( c->op1 );
				else if( c->operation == HexOp::and_a_atr0 )		out += "ANL A, @R0";
				else if( c->operation == HexOp::or_a_const )		out += "ORL A, " + operatorToString( c->op1 );
				else if( c->operation == HexOp::or_a_adr )			out += "ORL A, " + operatorToString( c->op1 );
				else if( c->operation == HexOp::or_a_atr0 )			out += "ORL A, @R0";
				else if( c->operation == HexOp::xor_a_const )		out += "XRL A, " + operatorToString( c->op1 );
				else if( c->operation == HexOp::xor_a_adr )			out += "XRL A, " + operatorToString( c->op1 );
				else if( c->operation == HexOp::xor_a_atr0 )		out += "XRL A, @R0";

				else if( c->operation == HexOp::cpl_a )				out += "CPL A";

				else if( c->operation == HexOp::cpl_bit )			out += "CPL " + operatorToString( c->op1 );
				else if( c->operation == HexOp::clr_bit )			out += "CLR " + operatorToString( c->op1 );
				else if( c->operation == HexOp::set_bit )			out += "SETB " + operatorToString( c->op1 );

				else if( c->operation == HexOp::jmp_abs )			out += "LJMP " + operatorToString( c->op1 );
				else if( c->operation == HexOp::jmp_rel )			out += "SJMP " + operatorToString( c->op1 );
				else if( c->operation == HexOp::jmp_c_rel )			out += "JC " + operatorToString( c->op1 );
				else if( c->operation == HexOp::jmp_nc_rel )		out += "JNC " + operatorToString( c->op1 );
				else if( c->operation == HexOp::jmp_z_rel )			out += "JZ " + operatorToString( c->op1 );
				else if( c->operation == HexOp::jmp_nz_rel )		out += "JNZ " + operatorToString( c->op1 );

				else if( c->operation == HexOp::call_abs )			out += "LCALL " + operatorToString( c->op1 );
				else if( c->operation == HexOp::ret )				out += "RET";
				else if( c->operation == HexOp::reti )				out += "RETI";

				else if( c->operation == HexOp::reti )				out += "-!-UNKNOWN_COMMAND-!-";
			}
			else if( c->type == EIR_Command::Type::blockBegin )
			{
				if( c->symbol != "" ) out += c->symbol + ", ";
				out += "begin" + to_string( c->id ) + ":";
				tabs += '\t';
			}
			else if( c->type == EIR_Command::Type::blockEnd )
			{
				out += "end" + to_string( c->id ) + ":";
				tabs.pop_back();
			}
			else if( c->type == EIR_Command::Type::blockFinish )
			{
				out += "finish" + to_string( c->id ) + ":";
			}
			else if( c->type == EIR_Command::Type::conditionEnd )
			{
				out += "true" + to_string( c->id ) + ":";
			}

			log( out );
		}

		log( "__END__" );
	}

	String EIR_Parser::operatorToString( std::shared_ptr<EIR_Operator> op )
	{
		if( op->type == EIR_Operator::Type::constant ) return "#" + int_to_hex( op->as<EIR_Constant>()->data );
		else if( op->type == EIR_Operator::Type::variable ) return "v" + to_string( op->as<EIR_Variable>()->id );
		else if( op->type == EIR_Operator::Type::sfr )
		{
			if( op->as<EIR_SFR>()->address == static_cast< u8 >( EIR_SFR::SFR::A ) ) return "A";
			else if( op->as<EIR_SFR>()->address == static_cast< u8 >( EIR_SFR::SFR::B ) ) return "B";
			else if( op->as<EIR_SFR>()->address == static_cast< u8 >( EIR_SFR::SFR::SP ) ) return "SP";
			else if( op->as<EIR_SFR>()->address == static_cast< u8 >( EIR_SFR::SFR::BR ) ) return "BR";
			else if( op->as<EIR_SFR>()->address == static_cast< u8 >( EIR_SFR::SFR::SW ) ) return "SW";
			else return "-!-UNKNOWN_SFR-!-";
		}
		else if( op->type == EIR_Operator::Type::block )
		{
			if( op->as<EIR_Block>()->symbol != "" ) return op->as<EIR_Block>()->symbol;
			else return ( op->as<EIR_Block>()->begin ? "begin" : op->as<EIR_Block>()->finish ? "finish" : "end" ) + to_string( op->as<EIR_Block>()->blockID );
		}
		else if( op->type == EIR_Operator::Type::condition )
			return ( op->as<EIR_Condition>()->isTrue ? "true" : "false" ) + to_string( op->as<EIR_Condition>()->conditionID );
		else return "-!-UNKNOWN-!-";
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
			varList[v.first.first] = EIR_Variable::getNew( sizeOfType( v.first.second->retType ) );
			varList[v.first.first]->model = v.first.second->model;
			base.eirValues[varList[v.first.first]->id] = varList[v.first.first];
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
		if( base.printAssembly ) printAssembly( base );

		if( hasError ) return ExecutionResult::eirParsingFailed;
		else return ExecutionResult::success;
	}
	void EIR_Parser::parseAst( std::shared_ptr<AstExpr> ast, std::map<String, std::shared_ptr<EIR_Variable>> varList )
	{
		if( ast->type == AstExpr::Type::block )
		{
			std::shared_ptr<AstBlock> a = ast->as<AstBlock>();
			std::map<String, std::shared_ptr<EIR_Variable>> localVarList;

			for( auto &v : varList )
			{
				if( v.second->isOnStack() )//Copy if is stack, because the scope offset may change
					localVarList[v.first] = std::make_shared<EIR_Variable>( *v.second );
				else localVarList[v.first] = v.second;//Otherwise just copy the reference to prevent linker errors.
				localVarList[v.first]->scopeOffset++;
			}
			u8 stackInc = 0, preStacking = 0;
			for( auto &v : a->newVariables )
			{
				localVarList[v.first] = EIR_Variable::getNew( sizeOfType( v.second->retType ) );
				localVarList[v.first]->model = MemModel::stack;
				localVarList[v.first]->address = stackInc + 1;
				stackInc += sizeOfType( v.second->retType ) / 8;
			}
			for( auto &v : a->parameters )
			{
				localVarList[v.first] = EIR_Variable::getNew( sizeOfType( v.second->retType ) );
				localVarList[v.first]->model = MemModel::param;
				preStacking -= sizeOfType( v.second->retType ) / 8;
				localVarList[v.first]->address = preStacking - 2;
			}
			a->parameters.clear();

			bool isBlock = true;
			{//Add begin of block
				auto blockBegin = std::make_shared<EIR_Command>();
				blockBegin->type = EIR_Command::Type::blockBegin;
				blockBegin->id = a->id;
				blockBegin->symbol = currSymbol;
				base->eirCommands.push_back( blockBegin );

				if( currSymbol == "" ) breakableIDs.push( a->id );
				else
				{
					funcIDs.push( a->id );
					isBlock = false;
				}
				currSymbol = "";
			}

			//Increment SP
			addCmd( HexOp::push_adr, EIR_SFR::getSFR( EIR_SFR::SFR::BR ) );
			addCmd( HexOp::mov_a_adr, EIR_SFR::getSFR( EIR_SFR::SFR::SP ) );
			addCmd( HexOp::mov_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::BR ) );
			addCmd( HexOp::add_a_const, EIR_Constant::fromConstant( stackInc ) );
			addCmd( HexOp::mov_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::SP ) );

			for( auto &a : a->content )
			{//Iterate the ast.
				parseAst( a, localVarList );
			}


			{//Add finish of block
				auto blockFinish = std::make_shared<EIR_Command>();;
				blockFinish->type = EIR_Command::Type::blockFinish;
				blockFinish->id = a->id;
				base->eirCommands.push_back( blockFinish );
			}

			//Decrement SP
			addCmd( HexOp::mov_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::SW ) );
			addCmd( HexOp::mov_a_adr, EIR_SFR::getSFR( EIR_SFR::SFR::SP ) );
			addCmd( HexOp::add_a_const, EIR_Constant::fromConstant( ~stackInc + 1 ) );
			addCmd( HexOp::mov_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::SP ) );
			addCmd( HexOp::pop_adr, EIR_SFR::getSFR( EIR_SFR::SFR::BR ) );
			addCmd( HexOp::mov_a_adr, EIR_SFR::getSFR( EIR_SFR::SFR::SW ) );

			//Move return value from r0 into a
			//if( !isBlock ) addCmd( HexOp::xch_r0_a );

			{//Add end of block
				auto blockEnd = std::make_shared<EIR_Command>();;
				blockEnd->type = EIR_Command::Type::blockEnd;
				blockEnd->id = a->id;
				base->eirCommands.push_back( blockEnd );

				if( isBlock ) breakableIDs.pop();
				else funcIDs.pop();
			}


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
					 a->rVal->type == AstExpr::Type::parenthesis ||
					 a->rVal->type == AstExpr::Type::functionCall ||
					 a->rVal->type == AstExpr::Type::refer ||
					 a->rVal->type == AstExpr::Type::derefer )
			{
				parseAst( a->rVal, varList );
				if( a->lVal->type == AstExpr::Type::term ||
					a->lVal->type == AstExpr::Type::unary ||
					a->lVal->type == AstExpr::Type::parenthesis ||
					a->lVal->type == AstExpr::Type::functionCall ||
					a->lVal->type == AstExpr::Type::refer ||
					a->lVal->type == AstExpr::Type::derefer )//push to stack if OC::tt
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
						 a->rVal->type == AstExpr::Type::parenthesis ||
						 a->rVal->type == AstExpr::Type::functionCall ||
						 a->rVal->type == AstExpr::Type::refer ||
						 a->rVal->type == AstExpr::Type::derefer ) comb = OC::vt;
			}
			else if( a->lVal->type == AstExpr::Type::literal )
			{//lValue is a constant
				lOp = EIR_Constant::fromAstLiteral( a->lVal->as<AstLiteral>() );
				if( a->rVal->type == AstExpr::Type::variable ) comb = OC::cv;
				else if( a->rVal->type == AstExpr::Type::literal ) comb = OC::cc;
				else if( a->rVal->type == AstExpr::Type::term ||
						 a->rVal->type == AstExpr::Type::unary ||
						 a->rVal->type == AstExpr::Type::parenthesis ||
						 a->rVal->type == AstExpr::Type::functionCall ||
						 a->rVal->type == AstExpr::Type::refer ||
						 a->rVal->type == AstExpr::Type::derefer ) comb = OC::ct;
			}
			else if( a->lVal->type == AstExpr::Type::term ||
					 a->lVal->type == AstExpr::Type::unary ||
					 a->lVal->type == AstExpr::Type::parenthesis ||
					 a->lVal->type == AstExpr::Type::functionCall ||
					 a->lVal->type == AstExpr::Type::refer ||
					 a->lVal->type == AstExpr::Type::derefer )
			{//lValue is a term
				if( a->lVal->type == AstExpr::Type::derefer ) lOp = std::make_shared<EIR_Operator>( EIR_Operator::Type::derefer );
				parseAst( a->lVal, varList );
				if( a->rVal->type == AstExpr::Type::variable ) comb = OC::tv;
				else if( a->rVal->type == AstExpr::Type::literal ) comb = OC::tc;
				else if( a->rVal->type == AstExpr::Type::term ||
						 a->rVal->type == AstExpr::Type::unary ||
						 a->rVal->type == AstExpr::Type::parenthesis ||
						 a->rVal->type == AstExpr::Type::functionCall ||
						 a->rVal->type == AstExpr::Type::refer ||
						 a->rVal->type == AstExpr::Type::derefer )
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
			else if( a->op == Token::Type::op_set_get )
			{// = (The operation itselve is a returning, so it has to copy the value into the acc)
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


			else generateNotification( NT::err_unknownBinaryOperator, a->token );

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
			else if( a->val->type == AstExpr::Type::term ||
					 a->val->type == AstExpr::Type::parenthesis ||
					 a->val->type == AstExpr::Type::unary ||
					 a->val->type == AstExpr::Type::functionCall ||
					 a->val->type == AstExpr::Type::refer ||
					 a->val->type == AstExpr::Type::derefer )
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
				else generateNotification( NT::err_unknownUnaryOperator, a->token );
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
			else if( a->content->type == AstExpr::Type::term ||
					 a->content->type == AstExpr::Type::parenthesis ||
					 a->content->type == AstExpr::Type::unary ||
					 a->content->type == AstExpr::Type::functionCall ||
					 a->content->type == AstExpr::Type::refer ||
					 a->content->type == AstExpr::Type::derefer )
			{
				ot = OT::t;
				parseAst( a->content, varList );
			}

			generateMoveA( ot, op );
		}
		else if( ast->type == AstExpr::Type::ifStat )
		{//if statement
			std::shared_ptr<AstIf> a = ast->as<AstIf>();

			if( a->condition->type != AstExpr::Type::booleanParenthesis )
				generateNotification( NT::err_expectParenthesisAfterIfKeyword, a->condition->token );
			else
			{
				parseCondition( a->condition, varList );

				if( a->elseCase == nullptr )
				{//Without else block
					addCmd( HexOp::jmp_rel, EIR_Constant::fromConstant( 3 ) );//true
					addCmd( HexOp::jmp_abs, EIR_Block::getBlockEnd( a->ifCase->id ) );//false
					parseAst( a->ifCase, varList );
				}
				else
				{//With else block
					addCmd( HexOp::jmp_rel, EIR_Constant::fromConstant( 3 ) );//true
					addCmd( HexOp::jmp_abs, EIR_Block::getBlockBegin( a->elseCase->id ) );//false
					parseAst( a->ifCase, varList );
					addCmd( HexOp::jmp_abs, EIR_Block::getBlockEnd( a->elseCase->id ) );
					parseAst( a->elseCase, varList );
				}
			}
		}
		else if( ast->type == AstExpr::Type::whileStat )
		{//while statement
			std::shared_ptr<AstWhile> a = ast->as<AstWhile>();

			if( a->condition->type != AstExpr::Type::booleanParenthesis )
				generateNotification( NT::err_expectParenthesisAfterWhileKeyword, a->condition->token );
			else
			{
				if( a->isDoWhile )
				{//Is do-while
					u32 idB = EIR_Condition::getNextConditionPos();

					auto conditionPos = std::make_shared<EIR_Command>();
					conditionPos->type = EIR_Command::Type::conditionEnd;
					conditionPos->id = idB;
					base->eirCommands.push_back( conditionPos );

					parseAst( a->block, varList );
					parseCondition( a->condition, varList );
					addCmd( HexOp::jmp_rel, EIR_Constant::fromConstant( 2 ) );//true
					addCmd( HexOp::jmp_rel, EIR_Constant::fromConstant( 3 ) );//false
					addCmd( HexOp::jmp_abs, EIR_Condition::getNew( idB, true ) );
				}
				else
				{//Is normal while statement
					u32 idB = EIR_Condition::getNextConditionPos();
					u32 idE = EIR_Condition::getNextConditionPos();

					auto conditionPos = std::make_shared<EIR_Command>();
					conditionPos->type = EIR_Command::Type::conditionEnd;
					conditionPos->id = idB;
					base->eirCommands.push_back( conditionPos );

					parseCondition( a->condition, varList );
					addCmd( HexOp::jmp_rel, EIR_Constant::fromConstant( 3 ) );//true
					addCmd( HexOp::jmp_abs, EIR_Condition::getNew( idE, true ) );//false
					parseAst( a->block, varList );
					addCmd( HexOp::jmp_abs, EIR_Condition::getNew( idB, true ) );

					conditionPos = std::make_shared<EIR_Command>();
					conditionPos->type = EIR_Command::Type::conditionEnd;
					conditionPos->id = idE;
					base->eirCommands.push_back( conditionPos );
				}
			}
		}
		else if( ast->type == AstExpr::Type::breakStat )
		{//Break statement
			if( !breakableIDs.empty() )
			{
				addCmd( HexOp::jmp_abs, EIR_Block::getBlockFinish( breakableIDs.top() ) );
			}
			else generateNotification( NT::err_cannotBreakAtThisPosition, ast->token );
		}
		else if( ast->type == AstExpr::Type::functionDefinition )
		{//Definition of a function
			std::shared_ptr<AstFunction> a = ast->as<AstFunction>();

			currSymbol = a->symbol;
			parseAst( a->content, varList );

			addCmd( HexOp::ret );
		}
		else if( ast->type == AstExpr::Type::functionCall )
		{//Calling a function
			std::shared_ptr<AstFunctionCall> a = ast->as<AstFunctionCall>();
			u8 stackInc = 0;

			//Handle parameters
			for( auto p : a->parameters )
			{
				OperationType ot;//Operator type combination
				std::shared_ptr<EIR_Operator> op;

				//Memorize val
				if( p->type == AstExpr::Type::variable )
				{
					ot = OT::v;
					op = varList[p->as<AstVariable>()->name];
				}
				else if( p->type == AstExpr::Type::literal )
				{
					ot = OT::c;
					op = EIR_Constant::fromAstLiteral( p->as<AstLiteral>() );
				}
				else if( p->type == AstExpr::Type::term ||
						 p->type == AstExpr::Type::parenthesis ||
						 p->type == AstExpr::Type::unary ||
						 p->type == AstExpr::Type::functionCall ||
						 p->type == AstExpr::Type::refer ||
						 p->type == AstExpr::Type::derefer )
				{
					ot = OT::t;
					parseAst( p, varList );
				}
				generateMoveA( ot, op );
				addCmd( HexOp::push_adr, EIR_SFR::getSFR( EIR_SFR::SFR::A ) );

				stackInc += sizeOfType( p->as<AstReturning>()->retType ) / 8;
			}

			//Call function
			addCmd( HexOp::call_abs, EIR_Block::getBlockBegin( a->symbol ) );

			//Decrement SP
			addCmd( HexOp::mov_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::SW ) );
			addCmd( HexOp::mov_a_adr, EIR_SFR::getSFR( EIR_SFR::SFR::SP ) );
			addCmd( HexOp::add_a_const, EIR_Constant::fromConstant( ~stackInc + 1 ) );
			addCmd( HexOp::mov_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::SP ) );
			addCmd( HexOp::mov_a_adr, EIR_SFR::getSFR( EIR_SFR::SFR::SW ) );

		}
		else if( ast->type == AstExpr::Type::returnStat )
		{//Return a value
			std::shared_ptr<AstReturnStatement> a = ast->as<AstReturnStatement>();
			OperationType ot;//Operator type combination
			std::shared_ptr<EIR_Operator> op;

			//Memorize val
			if( a->expr->type == AstExpr::Type::variable )
			{
				ot = OT::v;
				op = varList[a->expr->as<AstVariable>()->name];
			}
			else if( a->expr->type == AstExpr::Type::literal )
			{
				ot = OT::c;
				op = EIR_Constant::fromAstLiteral( a->expr->as<AstLiteral>() );
			}
			else if( a->expr->type == AstExpr::Type::term ||
					 a->expr->type == AstExpr::Type::parenthesis ||
					 a->expr->type == AstExpr::Type::unary ||
					 a->expr->type == AstExpr::Type::functionCall ||
					 a->expr->type == AstExpr::Type::refer ||
					 a->expr->type == AstExpr::Type::derefer )
			{
				ot = OT::t;
				parseAst( a->expr, varList );
			}

			if( !breakableIDs.empty() )
				generateNotification( NT::err_returnHasToBeInTheOuterScope, ast->token );

			generateMoveA( ot, op );
			//addCmd( HexOp::xch_r0_a );todo del
			addCmd( HexOp::jmp_abs, EIR_Block::getBlockFinish( funcIDs.top() ) );
		}
		else if( ast->type == AstExpr::Type::refer )
		{//Get the address of a variable
			std::shared_ptr<AstRefer> a = ast->as<AstRefer>();
			std::shared_ptr<EIR_Operator> op = varList[a->var->name];

			if( a->var->model == MemModel::fast )
			{
				addCmd( HexOp::mov_a_const, op );
			}
			else if( a->var->model == MemModel::large )
			{
				generateNotification( NT::err_cannotGetRefOfLargeGlobal, ast->token );
			}
			else if( a->var->model == MemModel::stack ||
					 a->var->model == MemModel::param )
			{
				generateLoadStackR0( op );
			}
		}
		else if( ast->type == AstExpr::Type::derefer )
		{//Get the value from a address
			std::shared_ptr<AstDerefer> a = ast->as<AstDerefer>();
			OperationType ot;//Operator type combination
			std::shared_ptr<EIR_Operator> op;

			//Memorize val
			if( a->expr->type == AstExpr::Type::variable )
			{
				ot = OT::v;
				op = varList[a->expr->as<AstVariable>()->name];
			}
			else if( a->expr->type == AstExpr::Type::literal )
			{
				ot = OT::c;
				op = EIR_Constant::fromAstLiteral( a->expr->as<AstLiteral>() );
			}
			else if( a->expr->type == AstExpr::Type::term ||
					 a->expr->type == AstExpr::Type::parenthesis ||
					 a->expr->type == AstExpr::Type::unary ||
					 a->expr->type == AstExpr::Type::functionCall ||
					 a->expr->type == AstExpr::Type::refer ||
					 a->expr->type == AstExpr::Type::derefer )
			{
				ot = OT::t;
				parseAst( a->expr, varList );
			}

			generateMoveA( ot, op );
			addCmd( HexOp::mov_r0_a );
			addCmd( HexOp::mov_a_atr0 );
		}
		else if( ast->type != AstExpr::Type::allocation &&
				 ast->type != AstExpr::Type::variable &&
				 ast->type != AstExpr::Type::literal ) generateNotification( NT::err_unknownASTExpr, ast->token );
	}

	void EIR_Parser::parseCondition( std::shared_ptr<AstExpr> ast, std::map<String, std::shared_ptr<EIR_Variable>> varList )
	{
		if( ast->type == AstExpr::Type::booleanParenthesis )
		{//Parenthesis block
			parseCondition( ast->as<AstBooleanParenthesis>()->content, varList );
		}
		else if( ast->type == AstExpr::Type::keywordCondition )
		{//Keyword (true/false)
			if( !ast->as<AstKeywordCondition>()->val )
			{//Is false
				addCmd( HexOp::jmp_rel, EIR_Constant::fromConstant( 2 ) );
			}
		}
		else if( ast->type == AstExpr::Type::arithmenticCondition )
		{//Arithmetic expression will be converted into boolean expression (0=false)
			auto ret = ast->as<AstArithmeticCondition>()->ret;
			OperationType ot;//Operator type combination
			std::shared_ptr<EIR_Operator> op;

			//Memorize val
			if( ret->type == AstExpr::Type::variable )
			{
				ot = OT::v;
				op = varList[ret->as<AstVariable>()->name];
			}
			else if( ret->type == AstExpr::Type::literal )
			{
				ot = OT::c;
				op = EIR_Constant::fromAstLiteral( ret->as<AstLiteral>() );
			}
			else if( ret->type == AstExpr::Type::term ||
					 ret->type == AstExpr::Type::unary ||
					 ret->type == AstExpr::Type::parenthesis ||
					 ret->type == AstExpr::Type::functionCall ||
					 ret->type == AstExpr::Type::refer ||
					 ret->type == AstExpr::Type::derefer )
			{
				ot = OT::t;
				parseAst( ret, varList );
			}

			generateMoveA( ot, op );
			addCmd( HexOp::jmp_z_rel, EIR_Constant::fromConstant( 2 ) );
		}
		else if( ast->type == AstExpr::Type::comparisonCondition )
		{//Comparison of two returnables
			auto a = ast->as<AstComparisonCondition>();
			OC comb;//Operator type combination
			std::shared_ptr<EIR_Operator> lOp;
			std::shared_ptr<EIR_Operator> rOp;

			//Memorize rVal
			if( a->rVal->type == AstExpr::Type::variable ) rOp = varList[a->rVal->as<AstVariable>()->name];
			else if( a->rVal->type == AstExpr::Type::literal ) rOp = EIR_Constant::fromAstLiteral( a->rVal->as<AstLiteral>() );
			else if( a->rVal->type == AstExpr::Type::term ||
					 a->rVal->type == AstExpr::Type::unary ||
					 a->rVal->type == AstExpr::Type::parenthesis ||
					 a->rVal->type == AstExpr::Type::functionCall ||
					 a->rVal->type == AstExpr::Type::refer ||
					 a->rVal->type == AstExpr::Type::derefer )
			{
				parseAst( a->rVal, varList );
				if( a->lVal->type == AstExpr::Type::term ||
					a->lVal->type == AstExpr::Type::unary ||
					a->lVal->type == AstExpr::Type::parenthesis ||
					a->lVal->type == AstExpr::Type::functionCall ||
					a->lVal->type == AstExpr::Type::refer ||
					a->lVal->type == AstExpr::Type::derefer )//push to stack if OC::tt
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
						 a->rVal->type == AstExpr::Type::parenthesis ||
						 a->rVal->type == AstExpr::Type::functionCall ||
						 a->rVal->type == AstExpr::Type::refer ||
						 a->rVal->type == AstExpr::Type::derefer ) comb = OC::vt;
			}
			else if( a->lVal->type == AstExpr::Type::literal )
			{//lValue is a constant
				lOp = EIR_Constant::fromAstLiteral( a->lVal->as<AstLiteral>() );
				if( a->rVal->type == AstExpr::Type::variable ) comb = OC::cv;
				else if( a->rVal->type == AstExpr::Type::literal ) comb = OC::cc;
				else if( a->rVal->type == AstExpr::Type::term ||
						 a->rVal->type == AstExpr::Type::unary ||
						 a->rVal->type == AstExpr::Type::parenthesis ||
						 a->rVal->type == AstExpr::Type::functionCall ||
						 a->rVal->type == AstExpr::Type::refer ||
						 a->rVal->type == AstExpr::Type::derefer ) comb = OC::ct;
			}
			else if( a->lVal->type == AstExpr::Type::term ||
					 a->lVal->type == AstExpr::Type::unary ||
					 a->lVal->type == AstExpr::Type::parenthesis ||
					 a->lVal->type == AstExpr::Type::functionCall ||
					 a->lVal->type == AstExpr::Type::refer ||
					 a->lVal->type == AstExpr::Type::derefer )
			{//lValue is a term
				parseAst( a->lVal, varList );
				if( a->rVal->type == AstExpr::Type::variable ) comb = OC::tv;
				else if( a->rVal->type == AstExpr::Type::literal ) comb = OC::tc;
				else if( a->rVal->type == AstExpr::Type::term ||
						 a->rVal->type == AstExpr::Type::unary ||
						 a->rVal->type == AstExpr::Type::parenthesis ||
						 a->rVal->type == AstExpr::Type::functionCall ||
						 a->rVal->type == AstExpr::Type::refer ||
						 a->rVal->type == AstExpr::Type::derefer )
				{//pop from stack to B
					comb = OC::tt;
					addCmd( HexOp::pop_adr, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				}
			}

			//Handle operation
			if( a->op == Token::Type::op_eql )
			{// ==
				addCmd( HexOp::clr_c );
				generateOperation( comb, HexOp::sub_a_adr, HexOp::sub_a_const, HexOp::sub_a_atr0, lOp, rOp, a->token );
				addCmd( HexOp::jmp_nz_rel, EIR_Constant::fromConstant( 2 ) );
			}
			else if( a->op == Token::Type::op_not_eql )
			{// !=
				addCmd( HexOp::clr_c );
				generateOperation( comb, HexOp::sub_a_adr, HexOp::sub_a_const, HexOp::sub_a_atr0, lOp, rOp, a->token );
				addCmd( HexOp::jmp_z_rel, EIR_Constant::fromConstant( 2 ) );
			}
			else if( a->op == Token::Type::op_less )
			{// <
				addCmd( HexOp::clr_c );
				generateOperation( comb, HexOp::sub_a_adr, HexOp::sub_a_const, HexOp::sub_a_atr0, lOp, rOp, a->token );
				addCmd( HexOp::jmp_nc_rel, EIR_Constant::fromConstant( 2 ) );
			}
			else if( a->op == Token::Type::op_more )
			{// >
				addCmd( HexOp::clr_c );
				generateOperation( comb, HexOp::sub_a_adr, HexOp::sub_a_const, HexOp::sub_a_atr0, lOp, rOp, a->token );
				addCmd( HexOp::jmp_c_rel, EIR_Constant::fromConstant( 4 ) );
				addCmd( HexOp::jmp_z_rel, EIR_Constant::fromConstant( 2 ) );
			}
			else if( a->op == Token::Type::op_less_eql )
			{// <=
				addCmd( HexOp::clr_c );
				generateOperation( comb, HexOp::sub_a_adr, HexOp::sub_a_const, HexOp::sub_a_atr0, lOp, rOp, a->token );
				addCmd( HexOp::jmp_c_rel, EIR_Constant::fromConstant( 2 ) );//Is true -> ignore next
				addCmd( HexOp::jmp_nz_rel, EIR_Constant::fromConstant( 2 ) );
			}
			else if( a->op == Token::Type::op_more_eql )
			{// >=
				addCmd( HexOp::clr_c );
				generateOperation( comb, HexOp::sub_a_adr, HexOp::sub_a_const, HexOp::sub_a_atr0, lOp, rOp, a->token );
				addCmd( HexOp::jmp_c_rel, EIR_Constant::fromConstant( 2 ) );
			}

		}
		else if( ast->type == AstExpr::Type::combinationCondition )
		{//Combination of two conditionals
			auto a = ast->as<AstComparisonCondition>();

			if( a->op == Token::Type::op_and_log )
			{// &&
				u32 id = EIR_Condition::getNextConditionPos();

				parseCondition( a->lVal, varList );
				addCmd( HexOp::jmp_rel, EIR_Constant::fromConstant( 3 ) );//true
				addCmd( HexOp::jmp_abs, EIR_Condition::getNew( id, false ) );//false
				parseCondition( a->rVal, varList );

				auto conditionPos = std::make_shared<EIR_Command>();
				conditionPos->type = EIR_Command::Type::conditionEnd;
				conditionPos->id = id;
				base->eirCommands.push_back( conditionPos );
			}
			else if( a->op == Token::Type::op_or_log )
			{// ||
				u32 id = EIR_Condition::getNextConditionPos();

				parseCondition( a->lVal, varList );
				addCmd( HexOp::jmp_rel, EIR_Constant::fromConstant( 2 ) );//true. Has to be a rel jmp because next command (2byte) will be false-jmp-destination.
				addCmd( HexOp::jmp_rel, EIR_Constant::fromConstant( 3 ) );
				addCmd( HexOp::jmp_abs, EIR_Condition::getNew( id, true ) );//true
				parseCondition( a->rVal, varList );

				auto conditionPos = std::make_shared<EIR_Command>();
				conditionPos->type = EIR_Command::Type::conditionEnd;
				conditionPos->id = id;
				base->eirCommands.push_back( conditionPos );
			}
			else if( a->op == Token::Type::op_not )
			{// !
				parseCondition( a->rVal, varList );
				addCmd( HexOp::jmp_rel, EIR_Constant::fromConstant( 2 ) );
			}
		}
		else
		{//Unknown condition ast
			generateNotification( NT::err_expectConditionInParanthesis, ast->token );
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
			else if( lOp->as<EIR_Variable>()->isOnStack() &&
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
			else if( lOp->as<EIR_Variable>()->isOnStack() &&
					 rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				generateLoadStackR0( lOp );
				addCmd( HexOp::mov_dptr_const, rOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( HexOp::mov_atr0_a );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::fast &&
					 rOp->as<EIR_Variable>()->isOnStack() )
			{
				generateLoadStackR0( rOp );
				if( copyInAcc )
				{
					addCmd( HexOp::mov_a_atr0 );
					addCmd( HexOp::mov_adr_a, lOp );
				}
				else addCmd( HexOp::mov_adr_atr0, lOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large &&
					 rOp->as<EIR_Variable>()->isOnStack() )
			{
				generateLoadStackR0( rOp );
				addCmd( HexOp::mov_a_atr0 );
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_dptr_a );
			}
			else if( lOp->as<EIR_Variable>()->isOnStack() &&
					 rOp->as<EIR_Variable>()->isOnStack() )
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
			else if( lOp->as<EIR_Variable>()->isOnStack() )
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
			else if( lOp->as<EIR_Variable>()->isOnStack() )
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
		else
		{
			if( lOp->type == EIR_Operator::Type::derefer )
			{//Dereferencing
				if( comb == OC::tv )
				{
					if( rOp->as<EIR_Variable>()->model == MemModel::fast )
					{
						if( copyInAcc )
						{
							addCmd( HexOp::mov_a_adr, rOp );
							addCmd( HexOp::mov_atr0_a );
						}
						else addCmd( HexOp::mov_atr0_adr, rOp );
					}
					else if( rOp->as<EIR_Variable>()->model == MemModel::large )
					{
						addCmd( HexOp::mov_dptr_const, rOp );
						addCmd( HexOp::movx_a_dptr );
						addCmd( HexOp::mov_atr0_a );
					}
					else if( rOp->as<EIR_Variable>()->isOnStack() )
					{
						generateLoadStackR1( rOp );
						addCmd( HexOp::mov_a_atr1 );
						addCmd( HexOp::mov_atr0_a );
					}
				}
				else if( comb == OC::tc )
				{
					if( copyInAcc )
					{
						addCmd( HexOp::mov_a_const, rOp );
						addCmd( HexOp::mov_atr0_a );
					}
					else addCmd( HexOp::mov_atr0_const, rOp );
				}
				else if( comb == OC::tt )
				{
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
	}

	void EIR_Parser::generateSetAfterOp( OperationCombination comb, std::shared_ptr<EIR_Operator> lOp, std::shared_ptr<Token> lValToken )
	{
		if( comb == OC::vv || comb == OC::vc || comb == OC::vt )
		{//Is the same, because after a operation the value will be located in the acc.
			if( lOp->as<EIR_Variable>()->model == MemModel::fast )
			{
				addCmd( HexOp::mov_adr_a, lOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large )
			{
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_dptr_a );
			}
			else if( lOp->as<EIR_Variable>()->isOnStack() )
			{
				addCmd( HexOp::mov_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::SW ) );
				generateLoadStackR0( lOp );
				addCmd( HexOp::mov_atr0_adr, EIR_SFR::getSFR( EIR_SFR::SFR::SW ) );
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
			else if( lOp->as<EIR_Variable>()->isOnStack() &&
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
			else if( lOp->as<EIR_Variable>()->isOnStack() &&
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
					 rOp->as<EIR_Variable>()->isOnStack() )
			{
				generateLoadStackR0( rOp );
				addCmd( HexOp::mov_a_adr, lOp );
				addCmd( op_atr0 );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large &&
					 rOp->as<EIR_Variable>()->isOnStack() )
			{
				generateLoadStackR0( rOp );
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( op_atr0 );
			}
			else if( lOp->as<EIR_Variable>()->isOnStack() &&
					 rOp->as<EIR_Variable>()->isOnStack() )
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
			else if( lOp->as<EIR_Variable>()->isOnStack() )
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
			else if( lOp->as<EIR_Variable>()->isOnStack() )
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
			else if( rOp->as<EIR_Variable>()->isOnStack() )
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
			else if( rOp->as<EIR_Variable>()->isOnStack() )
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
			else if( lOp->as<EIR_Variable>()->isOnStack() &&
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
			else if( lOp->as<EIR_Variable>()->isOnStack() &&
					 rOp->as<EIR_Variable>()->model == MemModel::large )
			{
				generateLoadStackR0( lOp );
				addCmd( HexOp::mov_dptr_const, rOp );
				addCmd( HexOp::movx_a_dptr );
				addCmd( HexOp::mov_adr_a, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_a_atr0 );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::fast &&
					 rOp->as<EIR_Variable>()->isOnStack() )
			{
				generateLoadStackR0( rOp );
				addCmd( HexOp::mov_adr_atr0, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_a_adr, lOp );
			}
			else if( lOp->as<EIR_Variable>()->model == MemModel::large &&
					 rOp->as<EIR_Variable>()->isOnStack() )
			{
				generateLoadStackR0( rOp );
				addCmd( HexOp::mov_adr_atr0, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::mov_dptr_const, lOp );
				addCmd( HexOp::movx_a_dptr );
			}
			else if( lOp->as<EIR_Variable>()->isOnStack() &&
					 rOp->as<EIR_Variable>()->isOnStack() )
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
			else if( lOp->as<EIR_Variable>()->isOnStack() )
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
			else if( lOp->as<EIR_Variable>()->isOnStack() )
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
			else if( rOp->as<EIR_Variable>()->isOnStack() )
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
			else if( rOp->as<EIR_Variable>()->isOnStack() )
			{
				addCmd( HexOp::push_adr, EIR_SFR::getSFR( EIR_SFR::SFR::A ) );
				generateLoadStackR0( rOp );
				addCmd( HexOp::mov_adr_atr0, EIR_SFR::getSFR( EIR_SFR::SFR::B ) );
				addCmd( HexOp::pop_adr, EIR_SFR::getSFR( EIR_SFR::SFR::A ) );
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
			else if( op->as<EIR_Variable>()->isOnStack() )
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
			else if( op->as<EIR_Variable>()->isOnStack() )
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
			else if( op->as<EIR_Variable>()->isOnStack() )
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
		addCmd( HexOp::mov_a_adr, EIR_SFR::getSFR( EIR_SFR::SFR::BR ) );
		for( size_t i = 0 ; i < op->as<EIR_Variable>()->scopeOffset ; i++ )
		{//Resolve scope offset
			addCmd( HexOp::xch_a_r0 );
			addCmd( HexOp::mov_a_atr0 );
		}
		addCmd( HexOp::add_a_const, op );
		addCmd( HexOp::clr_c );//Clear carry because some operations require cleared carry.
		addCmd( HexOp::mov_r0_a );
	}

	void EIR_Parser::generateLoadStackR1( std::shared_ptr<EIR_Operator> op )
	{
		addCmd( HexOp::mov_a_adr, EIR_SFR::getSFR( EIR_SFR::SFR::BR ) );
		for( size_t i = 0 ; i < op->as<EIR_Variable>()->scopeOffset ; i++ )
		{//Resolve scope offset
			addCmd( HexOp::xch_a_r1 );
			addCmd( HexOp::mov_a_atr1 );
		}
		addCmd( HexOp::add_a_const, op );
		addCmd( HexOp::clr_c );//Clear carry because some operations require cleared carry.
		addCmd( HexOp::mov_r0_a );
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
