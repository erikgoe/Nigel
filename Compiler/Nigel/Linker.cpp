#include "stdafx.h"
#include "Linker.h"

namespace nigel
{
	Linker::Linker()
	{
	}
	ExecutionResult Linker::onExecute( CodeBase &base )
	{
		std::map<u32, u16> blockBeginAddresses;//Already created blocks. Id of a block mapped to the address in hexBuffer.
		std::map<u32, u16> blockEndAddresses;//Already created blocks. Id of a block mapped to the address in hexBuffer.
		std::map<u32, std::list<u16>> missingBeginBlocks;//These addresses should be overwritten, if the corresponding block was created.
		std::map<u32, std::list<u16>> missingEndBlocks;//These addresses should be overwritten, if the corresponding block was created.
		std::map<u32, u16> conditionEnds_true;//Already created conditions. Id of a condition mapped to the address in hexBuffer.
		std::map<u32, u16> conditionEnds_false;//Already created conditions. Id of a condition mapped to the address in hexBuffer.
		std::map<u32, u16> missingConditionEnds_true;//These addresses should be overwritten, if the corresponding condition was finished.
		std::map<u32, u16> missingConditionEnds_false;//These addresses should be overwritten, if the corresponding condition was finished.



		//Initialize the machine
		base.hexBuffer.push_back( 0x75 );//mov
		base.hexBuffer.push_back( 0x07 );//BR
		base.hexBuffer.push_back( 0x00 );//0


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
			if( c->type == EIR_Command::Type::operation )
			{//Is a regular command
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
						else if( op->model == MemModel::stack )
						{
							base.hexBuffer.push_back( static_cast< u8 >( op->address ) );
						}
					}
					else if( c->op1->type == EIR_Operator::Type::sfr )
					{//Write sfr register
						base.hexBuffer.push_back( c->op1->as<EIR_SFR>()->address );
					}
					else if( c->op1->type == EIR_Operator::Type::block )
					{//Write the address of a block or put into waiting queue
						auto op = c->op1->as<EIR_Block>();
						if( op->begin )
						{//To block begin
							if( blockBeginAddresses.find( op->blockID ) == blockBeginAddresses.end() )
							{//Nothing found
								missingBeginBlocks[op->blockID].push_back( static_cast<u16>( base.hexBuffer.size() ) );
								base.hexBuffer.push_back( 0 );
								base.hexBuffer.push_back( 0 );
							}
							else
							{//Block was already created.
								base.hexBuffer.push_back( static_cast< u8 >( blockBeginAddresses[op->blockID] << 8 ) );
								base.hexBuffer.push_back( static_cast< u8 >( blockBeginAddresses[op->blockID] ) );
							}
						}
						else
						{//To block end
							if( blockEndAddresses.find( op->blockID ) == blockEndAddresses.end() )
							{//Nothing found
								missingEndBlocks[op->blockID].push_back( static_cast<u16>( base.hexBuffer.size() ) );
								base.hexBuffer.push_back( 0 );
								base.hexBuffer.push_back( 0 );
							}
							else
							{//Block was already created.
								base.hexBuffer.push_back( static_cast< u8 >( blockEndAddresses[op->blockID] << 8 ) );
								base.hexBuffer.push_back( static_cast< u8 >( blockEndAddresses[op->blockID] ) );
							}
						}
					}
					else if( c->op1->type == EIR_Operator::Type::condition )
					{//Write the address of a condition jmp
						auto op = c->op1->as<EIR_Condition>();

						if( op->isTrue )
						{//To condition true position
							if( conditionEnds_true.find( op->conditionID ) == conditionEnds_true.end() )
							{//Nothing found
								missingConditionEnds_true[op->conditionID] = static_cast< u16 >( base.hexBuffer.size() );
								base.hexBuffer.push_back( 0 );
								base.hexBuffer.push_back( 0 );
							}
							else
							{//Block was already created.
								base.hexBuffer.push_back( static_cast< u8 >( conditionEnds_true[op->conditionID] << 8 ) );
								base.hexBuffer.push_back( static_cast< u8 >( conditionEnds_true[op->conditionID] ) );
							}
						}
						else
						{//To condition false position
							if( conditionEnds_false.find( op->conditionID ) == conditionEnds_false.end() )
							{//Nothing found
								missingConditionEnds_false[op->conditionID] = static_cast< u16 >( base.hexBuffer.size() );
								base.hexBuffer.push_back( 0 );
								base.hexBuffer.push_back( 0 );
							}
							else
							{//Block was already created.
								base.hexBuffer.push_back( static_cast< u8 >( conditionEnds_false[op->conditionID] << 8 ) );
								base.hexBuffer.push_back( static_cast< u8 >( conditionEnds_false[op->conditionID] ) );
							}
						}
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
						else if( op->model == MemModel::stack )
						{
							base.hexBuffer.push_back( static_cast< u8 >( op->address ) );
						}
					}
					else if( c->op2->type == EIR_Operator::Type::sfr )
					{//Write sfr register
						base.hexBuffer.push_back( c->op2->as<EIR_SFR>()->address );
					}
					else if( c->op2->type == EIR_Operator::Type::block )
					{//Write the address of a block or put into waiting queue
						auto op = c->op2->as<EIR_Block>();
						if( op->begin )
						{//To block begin
							if( blockBeginAddresses.find( op->blockID ) == blockBeginAddresses.end() )
							{//Nothing found
								missingBeginBlocks[op->blockID].push_back( static_cast<u16>( base.hexBuffer.size() ) );
								base.hexBuffer.push_back( 0 );
								base.hexBuffer.push_back( 0 );
							}
							else
							{//Block was already created.
								base.hexBuffer.push_back( static_cast< u8 >( blockBeginAddresses[op->blockID] << 8 ) );
								base.hexBuffer.push_back( static_cast< u8 >( blockBeginAddresses[op->blockID] ) );
							}
						}
						else
						{//To block end
							if( blockEndAddresses.find( op->blockID ) == blockEndAddresses.end() )
							{//Nothing found
								missingEndBlocks[op->blockID].push_back( static_cast<u16>( base.hexBuffer.size() ) );
								base.hexBuffer.push_back( 0 );
								base.hexBuffer.push_back( 0 );
							}
							else
							{//Block was already created.
								base.hexBuffer.push_back( static_cast< u8 >( blockEndAddresses[op->blockID] << 8 ) );
								base.hexBuffer.push_back( static_cast< u8 >( blockEndAddresses[op->blockID] ) );
							}
						}
					}
					else if( c->op2->type == EIR_Operator::Type::condition )
					{//Write the address of a condition jmp
						if( !c->op2->as<EIR_Condition>()->isTrue )
							missingConditionEnds_false[c->op2->as<EIR_Condition>()->conditionID] = static_cast< u16 >( base.hexBuffer.size() );
						else missingConditionEnds_true[c->op2->as<EIR_Condition>()->conditionID] = static_cast< u16 >( base.hexBuffer.size() );
						base.hexBuffer.push_back( 0 );
						base.hexBuffer.push_back( 0 );
					}
				}
			}
			else if( c->type == EIR_Command::Type::blockBegin )
			{//Is new block
				u16 address = static_cast<u16>( base.hexBuffer.size() );
				blockBeginAddresses[c->id] = address;
				if( missingBeginBlocks.find( c->id ) != missingBeginBlocks.end() )
				{
					for( auto &a : missingBeginBlocks[c->id] )
					{
						base.hexBuffer[a] = static_cast< u8 >( address << 8 );
						base.hexBuffer[a + 1] = static_cast< u8 >( address );
					}
					missingBeginBlocks.erase( c->id );
				}
			}
			else if( c->type == EIR_Command::Type::blockEnd )
			{//End of a block
				u16 address = static_cast<u16>( base.hexBuffer.size() );
				blockEndAddresses[c->id] = address;
				if( missingEndBlocks.find( c->id ) != missingEndBlocks.end() )
				{
					for( auto &a : missingEndBlocks[c->id] )
					{
						base.hexBuffer[a] = static_cast< u8 >( address << 8 );
						base.hexBuffer[a + 1] = static_cast< u8 >( address );
					}
					missingEndBlocks.erase( c->id );
				}
			}
			else if( c->type == EIR_Command::Type::conditionEnd )
			{//End of a condition
				u16 address = static_cast<u16>( base.hexBuffer.size() );
				conditionEnds_true[c->id] = address;
				if( missingConditionEnds_true.find( c->id ) != missingConditionEnds_true.end() )
				{
					auto a = missingConditionEnds_true[c->id];
					base.hexBuffer[a] = static_cast< u8 >( address << 8 );
					base.hexBuffer[a + 1] = static_cast< u8 >( address );
					missingConditionEnds_true.erase( c->id );
				}
				address = static_cast< u16 >( base.hexBuffer.size() ) + 2;//Add two because of false jmp
				conditionEnds_false[c->id] = address;
				if( missingConditionEnds_false.find( c->id ) != missingConditionEnds_false.end() )
				{
					auto a = missingConditionEnds_false[c->id];
					base.hexBuffer[a] = static_cast< u8 >( address << 8 );
					base.hexBuffer[a + 1] = static_cast< u8 >( address );
					missingConditionEnds_false.erase( c->id );
				}
			}
		}

		//Add infinite loop
		base.hexBuffer.push_back( 128 );
		base.hexBuffer.push_back( 254 );

		if( !missingBeginBlocks.empty() || !missingBeginBlocks.empty() )
		{
			generateNotification( NT::err_blockNotFound, base.srcFile );
		}

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
