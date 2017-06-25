#ifndef NIGEL_COMPILE_NOTIFICATION_H
#define NIGEL_COMPILE_NOTIFICATION_H

#include "stdafx.h"
#include "Token.h"

namespace nigel
{
		//A Error or warning, which can be shown to the user.
	class CompileNotification
	{
	public:
		const enum class Type
		{
			begin_err = 0,//All errors will start at this position.
			err_reachedEOF_unfinishedExpression,
			err_unexpectedToken,//What did you tend to achive with this token?
			err_expectedIdentifier_atAllocation,
			err_expectedExprWithReturnValue_atAllocation,
			err_expectedEqlSign_atAllocation,
			err_expectedKnownLiteral,
			err_variableAlreadyDefined,
			err_undefinedIdentifier,

			err_expectedExprWithReturnValue_atOperation,
			err_unmatchingTypeFound_atTerm,

			begin_warning = 0x7fff,//All warnings will start at this position.

			begin_improvements = 0xffff,//All improvement notifications will start at this position.

			count
		} type;

		const fs::path file;
		const Token token;

		u64 getCode()
		{
			return static_cast< u64 >( type );
		}

		CompileNotification( Type type, Token token, fs::path file )
			: CompileNotification::type(type), CompileNotification::token(token), CompileNotification::file(file)
		{
		}
	};
}

#endif // !NIGEL_COMPILE_ERROR_H
