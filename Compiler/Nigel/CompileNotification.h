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

			err_unclosedIfdef,
			err_elseifWithoutIfdef,
			err_endifWithoutIfdef,
			err_incompleteDefineDirective,
			err_incompleteUndefineDirective,
			err_incompleteIfdefDirective,
			err_incompleteIfndefDirective,
			err_incompleteIncludeDirective,
			err_incompletePragma,
			err_incompleteMemModelPragma,
			err_unknownMemModelPragma,
			err_unknownPragma,
			err_unknownDirective,
			err_errorDirective,

			err_reachedEOF_unfinishedExpression,
			err_unexpectedToken,//What did you tend to achive with this token?
			err_expectedIdentifier_atAllocation,
			err_expectedExprWithReturnValue_atAllocation,
			err_expectedEqlSign_atAllocation,
			err_expectedKnownLiteral,
			err_variableAlreadyDefined,
			err_undefinedIdentifier,

			err_unexpectedIdentifierAfterIdentifier,
			err_expectedIdentifierBeforeOperator,
			err_expectedIdentifierAfterOperator,
			err_expectedExprWithReturnValue_atOperation,
			err_unmatchingTypeFound_atTerm,

			err_cannotSetAConstantLiteral,//Did you mean "==" ?

			begin_warning = 0x7fff,//All warnings will start at this position.

			warn_emptyDirective,

			begin_improvements = 0xffff,//All improvement notifications will start at this position.

			imp_addingTwoConstantsCanBePrevented,

			count
		} type;

		std::shared_ptr<fs::path> file;
		std::shared_ptr<String> lineText;

		//Either token or line
		const std::shared_ptr<Token> token;
		const size_t line;

		u64 getCode()
		{
			return static_cast< u64 >( type );
		}

		CompileNotification( Type type, std::shared_ptr<Token> token )
			: CompileNotification::type(type), CompileNotification::token(token), CompileNotification::line(token->lineNo), CompileNotification::lineText(token->line), CompileNotification::file(token->path)
		{
		}
		CompileNotification( Type type, size_t line, std::shared_ptr<String> lineText, std::shared_ptr<fs::path> file )
			: CompileNotification::type( type ), CompileNotification::token( nullptr ), CompileNotification::line(line), CompileNotification::lineText( lineText ), CompileNotification::file( file )
		{
		}
	};
}

#endif // !NIGEL_COMPILE_ERROR_H
