#ifndef NIGEL_EIR_COMMAND_H
#define NIGEL_EIR_COMMAND_H

#include "stdafx.h"

namespace nigel
{
		//Operations in Hex
	enum class HexOp
	{
		clr_a = 0xE4,

		mov_a_const = 0x74,
		mov_adr_const = 0x75,
		mov_a_adr = 0xE5,
		mov_adr_a = 0xF5,
		mov_adr_adr = 0x85,
		mov_r0_a = 0xF8,
		mov_a_r0 = 0xE8,

		add_a_const = 0x24,
		add_a_adr = 0x25,
		add_a_r0 = 0x28,
		sub_a_const = 0x94,
		sub_a_adr = 0x95,
		sub_a_r0 = 0x98,

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
		u32 id = 0;//Unique id of this variable
		u16 adress = 0;//Adress in ram, where to store the variable
		u8 size = 8;//Size of this variable in bits

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

	class EIR_Constant : public EIR_Operator
	{
	public:
		u8 data = 0;

		static std::shared_ptr<EIR_Constant> fromAstLiteral( std::shared_ptr<AstLiteral> ast )
		{
			return std::make_shared<EIR_Constant>( ast->token->as<Token_NumberL>()->number );
		}

		EIR_Constant( u8 data ) : EIR_Operator( EIR_Operator::Type::constant )
		{
			this->data = data;
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
