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
		std::map<u32, u16> blockFinishAddresses;//Already created blocks. Id of a block mapped to the address in hexBuffer.
		std::map<u32, std::list<u16>> missingBeginBlocks;//These addresses should be overwritten, if the corresponding block was created.
		std::map<u32, std::list<u16>> missingEndBlocks;//These addresses should be overwritten, if the corresponding block was created.
		std::map<u32, std::list<u16>> missingFinishBlocks;//These addresses should be overwritten, if the corresponding block was created.
		
		std::map<u32, u16> conditionEnds_true;//Already created conditions. Id of a condition mapped to the address in hexBuffer.
		std::map<u32, u16> conditionEnds_false;//Already created conditions. Id of a condition mapped to the address in hexBuffer.
		std::map<u32, u16> missingConditionEnds_true;//These addresses should be overwritten, if the corresponding condition was finished.
		std::map<u32, u16> missingConditionEnds_false;//These addresses should be overwritten, if the corresponding condition was finished.

		std::map<String, u16> symbolAddresses;//Already created symbols/functions. Id of a function mapped to the address in hexBuffer.
		std::map<String, std::list<u16>> missingSymbols;//These addresses should be overwritten, if the corresponding function was created.


		{//Initialize the machine
			base.hexBuffer.resize( 0x33 );//Insert empty space for interrupts, etc.

			//Jmp to init code
			base.hexBuffer[0x00] = 0x80;//jmp_rel
			base.hexBuffer[0x01] = 0x29;//to init code

			//<reserved for interrupt calls>

			//Init, call main and then loop infinitely
			base.hexBuffer[0x2B] = 0x75;//mov_const
			base.hexBuffer[0x2C] = 0x07;//BR
			//<reserved for main call>
			base.hexBuffer[0x31] = 0x80;//jmp_rel
			base.hexBuffer[0x32] = 0xFE;//to itselfe
		}


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
						else if( op->isOnStack() )
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
							if( op->symbol != "" )
							{//Function-like
								if( symbolAddresses.find( op->symbol ) == symbolAddresses.end() )
								{//Nothing found
									missingSymbols[op->symbol].push_back( static_cast< u16 >( base.hexBuffer.size() ) );
									base.hexBuffer.push_back( 0 );
									base.hexBuffer.push_back( 0 );
								}
								else
								{//Block was already created.
									base.hexBuffer.push_back( static_cast< u8 >( symbolAddresses[op->symbol] << 8 ) );
									base.hexBuffer.push_back( static_cast< u8 >( symbolAddresses[op->symbol] ) );
								}
							}
							else
							{//Scope-like
								if( blockBeginAddresses.find( op->blockID ) == blockBeginAddresses.end() )
								{//Nothing found
									missingBeginBlocks[op->blockID].push_back( static_cast< u16 >( base.hexBuffer.size() ) );
									base.hexBuffer.push_back( 0 );
									base.hexBuffer.push_back( 0 );
								}
								else
								{//Block was already created.
									base.hexBuffer.push_back( static_cast< u8 >( blockBeginAddresses[op->blockID] << 8 ) );
									base.hexBuffer.push_back( static_cast< u8 >( blockBeginAddresses[op->blockID] ) );
								}
							}
						}
						else
						{
							if( op->finish )
							{//Finish block
								if( blockFinishAddresses.find( op->blockID ) == blockFinishAddresses.end() )
								{//Nothing found
									missingFinishBlocks[op->blockID].push_back( static_cast< u16 >( base.hexBuffer.size() ) );
									base.hexBuffer.push_back( 0 );
									base.hexBuffer.push_back( 0 );
								}
								else
								{//Block was already created.
									base.hexBuffer.push_back( static_cast< u8 >( blockFinishAddresses[op->blockID] << 8 ) );
									base.hexBuffer.push_back( static_cast< u8 >( blockFinishAddresses[op->blockID] ) );
								}
							}
							else
							{//To block end
								if( blockEndAddresses.find( op->blockID ) == blockEndAddresses.end() )
								{//Nothing found
									missingEndBlocks[op->blockID].push_back( static_cast< u16 >( base.hexBuffer.size() ) );
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
						else if( op->isOnStack() )
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
						{
							if( op->finish )
							{//Finish block
								if( blockFinishAddresses.find( op->blockID ) == blockFinishAddresses.end() )
								{//Nothing found
									missingFinishBlocks[op->blockID].push_back( static_cast< u16 >( base.hexBuffer.size() ) );
									base.hexBuffer.push_back( 0 );
									base.hexBuffer.push_back( 0 );
								}
								else
								{//Block was already created.
									base.hexBuffer.push_back( static_cast< u8 >( blockFinishAddresses[op->blockID] << 8 ) );
									base.hexBuffer.push_back( static_cast< u8 >( blockFinishAddresses[op->blockID] ) );
								}
							}
							else
							{//To block end
								if( blockEndAddresses.find( op->blockID ) == blockEndAddresses.end() )
								{//Nothing found
									missingEndBlocks[op->blockID].push_back( static_cast< u16 >( base.hexBuffer.size() ) );
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

				if( c->symbol != "" )
				{
					if( symbolAddresses.find( c->symbol ) != symbolAddresses.end() )
					{//Already defined symbol found
						generateNotification( NT::err_symbolIsAlreadyDefined, std::make_shared<String>( c->symbol ), -1, base.srcFile );
					}
					symbolAddresses[c->symbol] = address;
					if( missingSymbols.find( c->symbol ) != missingSymbols.end() )
					{
						for( auto &a : missingSymbols[c->symbol] )
						{
							base.hexBuffer[a] = static_cast< u8 >( address << 8 );
							base.hexBuffer[a + 1] = static_cast< u8 >( address );
						}
						missingSymbols.erase( c->symbol );
					}
				}


				if( c->symbol == "main" )
				{//Main function
					base.hexBuffer[0x2E] = 0x12;//call_abs
					base.hexBuffer[0x2F] = static_cast< u8 >( address << 8 );
					base.hexBuffer[0x30] = static_cast< u8 >( address );
				}
				else if( c->symbol == "interrupt0" )
				{//Interrupt 0 without EA disabled
					base.hexBuffer[0x03] = 0x12;//call_abs
					base.hexBuffer[0x04] = static_cast< u8 >( address << 8 );
					base.hexBuffer[0x05] = static_cast< u8 >( address );
					base.hexBuffer[0x06] = 0x32;//reti
				}
				else if( c->symbol == "interrupt0_ea" )
				{//Interrupt 0 with EA disabled
					base.hexBuffer[0x03] = 0xC2;//clr_bit
					base.hexBuffer[0x04] = 0xAF;//EA
					base.hexBuffer[0x05] = 0x12;//call_abs
					base.hexBuffer[0x06] = static_cast< u8 >( address << 8 );
					base.hexBuffer[0x07] = static_cast< u8 >( address );
					base.hexBuffer[0x08] = 0xD2;//set_bit
					base.hexBuffer[0x09] = 0xAF;//EA
					base.hexBuffer[0x0A] = 0x32;//reti
				}
				else if( c->symbol == "timer0" )
				{//Timer 0 without EA disabled
					base.hexBuffer[0x0B] = 0x12;//call_abs
					base.hexBuffer[0x0C] = static_cast< u8 >( address << 8 );
					base.hexBuffer[0x0D] = static_cast< u8 >( address );
					base.hexBuffer[0x0E] = 0x32;//reti
				}
				else if( c->symbol == "timer0_ea" )
				{//Timer 0 with EA disabled
					base.hexBuffer[0x0B] = 0xC2;//clr_bit
					base.hexBuffer[0x0C] = 0xAF;//EA
					base.hexBuffer[0x0D] = 0x12;//call_abs
					base.hexBuffer[0x0E] = static_cast< u8 >( address << 8 );
					base.hexBuffer[0x0F] = static_cast< u8 >( address );
					base.hexBuffer[0x10] = 0xD2;//set_bit
					base.hexBuffer[0x11] = 0xAF;//EA
					base.hexBuffer[0x12] = 0x32;//reti
				}
				else if( c->symbol == "interrupt1" )
				{//Interrupt 1 without EA disabled
					base.hexBuffer[0x13] = 0x12;//call_abs
					base.hexBuffer[0x14] = static_cast< u8 >( address << 8 );
					base.hexBuffer[0x15] = static_cast< u8 >( address );
					base.hexBuffer[0x16] = 0x32;//reti
				}
				else if( c->symbol == "interrupt1_ea" )
				{//Interrupt 1 with EA disabled
					base.hexBuffer[0x13] = 0xC2;//clr_bit
					base.hexBuffer[0x14] = 0xAF;//EA
					base.hexBuffer[0x15] = 0x12;//call_abs
					base.hexBuffer[0x16] = static_cast< u8 >( address << 8 );
					base.hexBuffer[0x17] = static_cast< u8 >( address );
					base.hexBuffer[0x18] = 0xD2;//set_bit
					base.hexBuffer[0x19] = 0xAF;//EA
					base.hexBuffer[0x1A] = 0x32;//reti
				}
				else if( c->symbol == "timer1" )
				{//Timer 1 without EA disabled
					base.hexBuffer[0x1B] = 0x12;//call_abs
					base.hexBuffer[0x1C] = static_cast< u8 >( address << 8 );
					base.hexBuffer[0x1D] = static_cast< u8 >( address );
					base.hexBuffer[0x1E] = 0x32;//reti
				}
				else if( c->symbol == "timer1_ea" )
				{//Timer 1 with EA disabled
					base.hexBuffer[0x1B] = 0xC2;//clr_bit
					base.hexBuffer[0x1C] = 0xAF;//EA
					base.hexBuffer[0x1D] = 0x12;//call_abs
					base.hexBuffer[0x1E] = static_cast< u8 >( address << 8 );
					base.hexBuffer[0x1F] = static_cast< u8 >( address );
					base.hexBuffer[0x20] = 0xD2;//set_bit
					base.hexBuffer[0x21] = 0xAF;//EA
					base.hexBuffer[0x22] = 0x32;//reti
				}
				else if( c->symbol == "serialio" )
				{//Serial I/O without EA disabled
					base.hexBuffer[0x23] = 0x12;//call_abs
					base.hexBuffer[0x24] = static_cast< u8 >( address << 8 );
					base.hexBuffer[0x25] = static_cast< u8 >( address );
					base.hexBuffer[0x26] = 0x32;//reti
				}
				else if( c->symbol == "serialio_ea" )
				{//Serial I/O with EA disabled
					base.hexBuffer[0x23] = 0xC2;//clr_bit
					base.hexBuffer[0x24] = 0xAF;//EA
					base.hexBuffer[0x25] = 0x12;//call_abs
					base.hexBuffer[0x26] = static_cast< u8 >( address << 8 );
					base.hexBuffer[0x27] = static_cast< u8 >( address );
					base.hexBuffer[0x28] = 0xD2;//set_bit
					base.hexBuffer[0x29] = 0xAF;//EA
					base.hexBuffer[0x2A] = 0x32;//reti
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
			else if( c->type == EIR_Command::Type::blockFinish )
			{//Finish the block
				u16 address = static_cast<u16>( base.hexBuffer.size() );
				blockFinishAddresses[c->id] = address;
				if( missingFinishBlocks.find( c->id ) != missingFinishBlocks.end() )
				{
					for( auto &a : missingFinishBlocks[c->id] )
					{
						base.hexBuffer[a] = static_cast< u8 >( address << 8 );
						base.hexBuffer[a + 1] = static_cast< u8 >( address );
					}
					missingFinishBlocks.erase( c->id );
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


		//Error reporting
		if( !missingBeginBlocks.empty() || !missingEndBlocks.empty() || !missingFinishBlocks.empty() )
		{
			generateNotification( NT::err_internal_blockNotFound, base.srcFile );
		}
		if( !missingConditionEnds_true.empty() || !missingConditionEnds_false.empty() )
		{
			generateNotification( NT::err_internal_conditionalNotFound, base.srcFile );
		}
		if( !missingSymbols.empty() )
		{
			generateNotification( NT::err_functionDefinitionNotFound, base.srcFile );
		}
		if( symbolAddresses.find( "main" ) == symbolAddresses.end() )
		{
			generateNotification( NT::err_mainEntryPointNotFound, base.srcFile );
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
