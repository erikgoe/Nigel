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
			err_unexpectedToken,
			err_expectedIdentifier_atAllocation,
			err_expectedExprWithReturnValue_atAllocation,
			err_expectedEqlSign_atAllocation,
			err_expectedKnownLiteral,
			err_variableAlreadyDefined,
			err_undefinedIdentifier,
			err_noAllocationAfterVariableAttribute,

			err_unexpectedReturningBeforeIdentifier,
			err_unexpectedReturningBeforeLiteral,
			err_unexpectedReturningBeforeParenthesisBlock,
			err_unexpectedReturningBeforeBlock,
			err_expectedIdentifierBeforeOperator,
			err_expectedIdentifierAfterOperator,
			err_expectedExprWithReturnValue_atOperation,
			err_unmatchingTypeFound_atTerm,
			err_unknownBinaryOperator,
			err_unknownUnaryOperator,

			err_cannotSetAConstantLiteral,
			err_onlyConstantsAreAllowedForBitShifts,
			err_onlyPositiveConstantsAreAllowedForBitShifts,
			err_cannotSetAConstantLiteralInCombinedOperationSet,

			err_expectedIdentifierInParenthesis,
			err_expectedExprWithReturnValue_atParenthesis,
			err_expectedExprWithBooleanValue_atParenthesis,
			err_unexpectedLiteralInParenthesis,
			err_unexpectedIdentifierInParenthesis,
			err_expectedTermAfterReturnableInParenthesis,
			err_unexpectedCloseOfParenthesis,
			err_aParenthesisWasNotClosed,

			err_unexpectedCloseOfBlock,
			err_aBlockWasNotClosed,

			err_expectParanthesisAfterIfKeyword,
			err_expectBlockAfterIf,
			err_expectBlockAfterElse,

			begin_warning = 0x7fff,//All warnings will start at this position.

			warn_warningDirective,
			warn_emptyDirective,
			warn_undefinedIdentifiernAtUndefDirective,

			warn_toManyVariablesInFastRAM,


			begin_improvements = 0xffff,//All improvement notifications will start at this position.

			imp_operationOnTwoConstantsCanBePrevented,
			imp_operationOnConstantCanBePrevented,

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

		CompileNotification( Type type, std::shared_ptr<fs::path> path )
			: CompileNotification::type( type ), CompileNotification::token( nullptr ), CompileNotification::line( 0 ), CompileNotification::lineText( nullptr ), CompileNotification::file( path )
		{
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
