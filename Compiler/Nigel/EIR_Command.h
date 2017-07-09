#ifndef NIGEL_EIR_COMMAND_H
#define NIGEL_EIR_COMMAND_H

#include "stdafx.h"

namespace nigel
{
		//Operations in Hex
	enum class HexOp
	{
		nop = 0x00,

		clr_a = 0xE4,

		mov_a_const = 0x74,
		mov_a_adr = 0xE5,
		mov_a_r0 = 0xE8,
		mov_a_atr0 = 0xE6,
		mov_a_atr1 = 0xE7,
		mov_adr_const = 0x75,
		mov_adr_a = 0xF5,
		mov_adr_r0 = 0x88,
		mov_adr_atr0 = 0x86,
		mov_adr_adr = 0x85,//rVal and lVal are reverse ordered
		mov_r0_const = 0x78,
		mov_r0_a = 0xF8,
		mov_r0_adr = 0xA8,
		mov_atr0_const = 0x76,
		mov_atr0_a = 0xF6,
		mov_atr1_a = 0xF7,
		mov_atr0_adr = 0xA6,
		mov_dptr_const = 0x90,
		movx_a_dptr = 0xE0,
		movx_dptr_a = 0xF0,

		push_adr = 0xC0,
		pop_adr = 0xD0,

		xch_a_r0 = 0xC8,
		xch_r0_a = 0xC8,
		xch_a_r1 = 0xC9,
		xch_r1_a = 0xC9,
		xch_a_atr0 = 0xD6,
		xch_atr0_a = 0xD6,
		xch_a_adr = 0xC5,
		xch_adr_a = 0xC5,

		clr_c = 0xC3,

		add_a_const = 0x24,
		add_a_adr = 0x25,
		add_a_r0 = 0x28,
		add_a_atr0 = 0x26,
		sub_a_const = 0x94,
		sub_a_adr = 0x95,
		sub_a_r0 = 0x98,
		sub_a_atr0 = 0x96,
		mul_a_b = 0xA4,
		div_a_b = 0x84,

		inc_a = 0x04,
		dec_a = 0x14,
		rr_a = 0x03,
		rl_a = 0x23,

		and_a_const = 0x54,
		and_a_adr = 0x55,
		and_a_atr0 = 0x56,
		or_a_const = 0x44,
		or_a_adr = 0x45,
		or_a_atr0 = 0x46,
		xor_a_const = 0x64,
		xor_a_adr = 0x65,
		xor_a_atr0 = 0x66,

		cpl_a = 0xF4,

		count
	};

		//Base class of a operator
	class EIR_Operator : public std::enable_shared_from_this<EIR_Operator>
	{
	public:
			//Type of operator
		enum class Type
		{
			constant,//Constant
			variable,//Variable
			sfr,//Special function register

			count
		} type;

		EIR_Operator( Type type )
		{
			this->type = type;
		}
		virtual ~EIR_Operator() {}

		template <typename T>
		std::shared_ptr<T> as()
		{
			return std::static_pointer_cast< T >( shared_from_this() );
		}
	};
		//Variable deklaration for the linker
	class EIR_Variable : public EIR_Operator
	{
		static u32 nextID;//Enables creation of new variables.

	public:
		MemModel model = MemModel::large;
		u32 id = 0;//Unique id of this variable
		u16 address = 0;//Address in ram, where to store the variable
		u8 size = 8;//Size of this variable in bits
		size_t scopeOffset = 0;

		static std::shared_ptr<EIR_Variable> getNew(u8 size)
		{
			return std::make_shared<EIR_Variable>( nextID++, size );
		}
		EIR_Variable(u32 id, u8 size) : EIR_Operator( EIR_Operator::Type::variable)
		{
			this->id = id;
			this->size = size;
		}
	};

		//Constant value from a literal
	class EIR_Constant : public EIR_Operator
	{
	public:
		u8 data = 0;

		static std::shared_ptr<EIR_Constant> fromAstLiteral( std::shared_ptr<AstLiteral> ast )
		{
			return std::make_shared<EIR_Constant>( ast->token->as<Token_NumberL>()->number );
		}
		static std::shared_ptr<EIR_Constant> fromConstant( u8 value )
		{
			return std::make_shared<EIR_Constant>( value );
		}

		EIR_Constant( u8 data ) : EIR_Operator( EIR_Operator::Type::constant )
		{
			this->data = data;
		}
	};

		//Special function register
	class EIR_SFR : public EIR_Operator
	{
	public:
		enum class SFR
		{
			A = 0xE0,//Accumulator-register
			B = 0xF0,//B-register
			SP = 0x81,//Stack pointer
			BR = 0x07,//Block register: special register which points to the begin of the current block, which then points to the begin of the previous block.

			count
		};
		u8 address = 0;

		static std::shared_ptr<EIR_SFR> getSFR( SFR sfr )
		{
			return std::make_shared<EIR_SFR>( sfr );
		}
		EIR_SFR( SFR address ) : EIR_Operator( EIR_Operator::Type::sfr )
		{
			this->address = static_cast<u8>( address );
		}
	};


		//Single command
	class EIR_Command
	{
	public:
		HexOp operation;//Id of the operation
		std::shared_ptr<EIR_Operator> op1 = nullptr;//First operator
		std::shared_ptr<EIR_Operator> op2 = nullptr;//Second operator

		EIR_Command() {}
	};
}

#endif
