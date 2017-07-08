#include "stdafx.h"
#include "Linker.h"

namespace nigel
{
	Linker::Linker()
	{
	}
	ExecutionResult Linker::onExecute( CodeBase &base )
	{
		{//Set address of values
			u16 fastAdr = 0x80;
			u16 largeAdr = 0;
			bool onlyNormal = false;

			for( auto &v : base.eirValues )
			{
				if( v.second->model == MemModel::fast && !onlyNormal )
				{
					if( ( fastAdr - v.second->size / 8 ) < 0x20 )
					{
						generateNotification( NT::warn_toManyVariablesInFastRAM, base.srcFile );
						onlyNormal = true;
					}
					else
					{
						fastAdr -= v.second->size / 8;
						v.second->address = fastAdr;
					}
				}
				if( v.second->model == MemModel::large || onlyNormal )
				{
					v.second->address = largeAdr;
					largeAdr += v.second->size / 8;
				}
			}
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
					auto op = c->op1->as<EIR_Variable>();
					if( op->model == MemModel::fast )
						base.hexBuffer.push_back( static_cast< u8 >( op->address ) );
					else if( op->model == MemModel::large )
					{
						base.hexBuffer.push_back( static_cast< u8 >( op->address << 8 ) );
						base.hexBuffer.push_back( static_cast< u8 >( op->address ) );
					}
				}
				else if( c->op1->type == EIR_Operator::Type::sfr )
				{//Write sfr register
					base.hexBuffer.push_back( c->op1->as<EIR_SFR>()->address );
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
					auto op = c->op2->as<EIR_Variable>();
					if( op->model == MemModel::fast )
						base.hexBuffer.push_back( static_cast< u8 >( op->address ) );
					else if( op->model == MemModel::large )
					{
						base.hexBuffer.push_back( static_cast< u8 >( op->address << 8 ) );
						base.hexBuffer.push_back( static_cast< u8 >( op->address ) );
					}
				}
				else if( c->op2->type == EIR_Operator::Type::sfr )
				{//Write sfr register
					base.hexBuffer.push_back( c->op2->as<EIR_SFR>()->address );
				}
			}
		}

		//Add infinite loop
		base.hexBuffer.push_back( 128 );
		base.hexBuffer.push_back( 254 );

		printToFile( base.hexBuffer, *base.destFile );

		return ExecutionResult::success;
	}
	void Linker::printToFile( const std::vector<u8>& data, fs::path file )
	{
		String fileStr = "";
		String out;
		u16 startAddr = 0;
		u16 checksum = 0;
		for( size_t i = 0 ; i < data.size() ; i++ )
		{
			out += int_to_hex( data[i] );
			checksum += data[i];

			if( out.size() >= 32 )
			{
				fileStr += ":" + int_to_hex( static_cast< u8 >( out.size() / 2 ) ) + int_to_hex( startAddr ) + "00" + out + int_to_hex( static_cast< u8 >( ~( checksum % 256 )+1 ) ) + "\r\n";
				checksum = 0;
				startAddr += static_cast< u16 >( out.size() / 2 );
				out = "";
			}
		}
		if( out.size() > 0 )
		{
			fileStr += ":" + int_to_hex( static_cast< u8 >( out.size() / 2 ) ) + int_to_hex( startAddr ) + "00" + out + int_to_hex( static_cast< u8 >( ~( checksum % 256 )+1 ) ) + "\r\n";
		}
		fileStr += ":00000001FF";


		std::basic_ofstream<u8> filestream( file.string(), std::ios_base::binary );

		if( filestream )
		{
			filestream << fileStr.c_str();
			filestream.close();
		}
	}
}
