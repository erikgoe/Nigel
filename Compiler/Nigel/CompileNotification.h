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

			err_unexpectedReturningBeforeByteKeyword,
			err_unexpectedReturningBeforeFastKeyword,
			err_unexpectedReturningBeforeNormKeyword,
			err_unexpectedReturningBeforeUnsignedKeyword,
			err_unexpectedReturningBeforeIdentifier,
			err_unexpectedReturningBeforeLiteral,
			err_unexpectedReturningBeforeParenthesisBlock,
			err_unexpectedReturningBeforeBlock,
			err_unexpectedReturningBeforeFunctionDeklaration,
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

			err_expectParenthesisAfterIfKeyword,
			err_expectConditionInParanthesis,
			err_expectBlockAfterIf,
			err_expectBlockAfterElse,

			err_expectedExprWithConditionalValue_atOperation,

			err_expectParenthesisAfterWhileKeyword,
			err_expectBlockAfterWhile,
			err_comparisonConditionCannotBeThisRValue,
			err_unexpectedIdentifierBeforeNotOperator,

			err_operationsAreNotAllowedInGlobalScope,
			err_functionDefinitionsAreNotAllowedInLocalScope,

			err_unknownTypeAtFunction,
			err_expectedIdentifier_atFunction,
			err_expectedOpeningParenthesis_atFunction,
			err_unknownTypeAtFunctionParameter,
			err_unknownTokenAfterFunctionParameter,
			err_expectedBlockAfterFunctionHead,
			err_functionIdentifierAlreadyAssigned,
			err_expectedReturningExpression_atFunctionCall,
			err_expectedOpeningParenthesis_atFunctionCall,
			err_unknownTypeAtFunctionCallParameter,
			err_expectedReturningExpression_AtReturn,
			err_returnHasToBeInTheOuterScope,
			err_notFoundMatchingFunctionDeclaration,

			err_expectedVariableForReference,
			err_expectedVariableForDeferencing,
			err_cannotGetRefOfLargeGlobal,

			err_unknownASTExpr,
			err_cannotBreakAtThisPosition,

			err_internal_blockNotFound,
			err_internal_conditionalNotFound,
			err_functionDefinitionNotFound,
			err_mainEntryPointNotFound,
			err_symbolIsAlreadyDefined,


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
			: type( type ), token( nullptr ), line( 0 ), lineText( nullptr ), file( path )
		{
		}
		CompileNotification( Type type, std::shared_ptr<Token> token )
			: type(type), token(token), line(token->lineNo), lineText(token->line), file(token->path)
		{
		}
		CompileNotification( Type type, size_t line, std::shared_ptr<String> lineText, std::shared_ptr<fs::path> file )
			: type( type ), token( nullptr ), line(line), lineText( lineText ), file( file )
		{
		}
	};
}

#endif // !NIGEL_COMPILE_ERROR_H
