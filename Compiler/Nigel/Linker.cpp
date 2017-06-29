#include "stdafx.h"
#include "Linker.h"

namespace nigel
{
	Linker::Linker()
	{
	}
	ExecutionResult Linker::onExecute( CodeBase &base )
	{
		//Write values to hex buffer
		for( auto v : base.eirValues )
		{
			v.second->adress = base.hexBuffer.size();
			for( u8 i = 0 ; i < v.second->size ; i++ )
				base.hexBuffer.push_back( 0 );
		}

		//Write code to hex buffer
		for( auto c : base.eirCommands )
		{
			base.hexBuffer.push_back( static_cast< u8 >( c->operation ) );
			if( c->op1 != nullptr )
			{
				if( c->op1->type == EIR_Operator::Type::constant )
				{//Write constant
					base.hexBuffer.push_back( c->op1->as<EIR_Constant>()->data );
				}
				else if( c->op1->type == EIR_Operator::Type::variable )
				{//Write variable
					base.hexBuffer.push_back( static_cast< u8 >( c->op1->as<EIR_Variable>()->adress ) );
				}
			}
			if( c->op2 != nullptr )
			{
				if( c->op2->type == EIR_Operator::Type::constant )
				{//Write constant
					base.hexBuffer.push_back( c->op2->as<EIR_Constant>()->data );
				}
				else if( c->op2->type == EIR_Operator::Type::variable )
				{//Write variable
					base.hexBuffer.push_back( static_cast< u8 >( c->op2->as<EIR_Variable>()->adress ) );
				}
			}
		}

		return ExecutionResult::success;
	}
}
