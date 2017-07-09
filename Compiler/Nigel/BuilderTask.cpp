#include "stdafx.h"
#include "BuilderTask.h"

namespace nigel
{
	void BuilderTask::printErrorLog( std::list<std::shared_ptr<CompileNotification>> notificationList )
	{
		size_t errorCount = 0, warningCount = 0, notificationCount = 0;
		String outStr = "";
		LogLevel level = LogLevel::Information;

		for( auto &n : notificationList )
		{
			outStr = n->file->generic_string() + "(";
			if( n->token != nullptr ) outStr += to_string( n->token->lineNo ) + ", " + to_string( n->token->columnNo );
			else if( n->lineText != nullptr ) outStr += to_string( n->line );
			outStr += "): ";

			if( n->type > NT::begin_err && n->type < NT::begin_warning )
			{//error
				outStr += "error";
				level = LogLevel::Error;
				errorCount++;
			}
			else if( n->type > NT::begin_warning && n->type < NT::begin_improvements )
			{//warning
				outStr += "warning";
				level = LogLevel::Warning;
				warningCount++;
			}
			else if( n->type > NT::begin_improvements && n->type < NT::count )
			{//notification
				outStr += "notification";
				level = LogLevel::Information;
				notificationCount++;
			}

			outStr += " " + to_string( n->getCode() ) + " \"" + notificationTexts[n->type] + "\"" ;
			if( n->token != nullptr ) outStr += " at token '" + n->token->toString() + "'";
			if( n->lineText != nullptr )
			{
				outStr += ": \n";
				outStr += *n->lineText;
			}

			if( n->token != nullptr )
			{
				size_t pos = n->token->columnNo;
				if( n->token->toString() == "_!EOF!_" ) pos = n->lineText->size() + 1;

				for( size_t i = 1 ; i < pos ; i++ ) outStr += ' ';

				if( n->token->toString() == "-!-UNKNOWN-!-" ||
					n->token->toString() == "_!EOF!_" ) outStr += '^';
				else for( size_t i = 0 ; i < n->token->toString().size() ; i++ ) outStr += '^';
			}

			log( outStr, level );
		}
		if( errorCount > 0 )
		{
			log( "FAILED " + to_string( errorCount ) + ( errorCount>1 ? " ERRORS" : " ERROR" ) + " OCCURRED! " + to_string( warningCount ) + " warnings occurred. " + to_string( notificationCount ) + " improvements available." );
		}
		else if( warningCount > 0 || notificationCount > 0 )
		{
			log( "Finshed with " + to_string( errorCount ) + " errors, " + to_string( warningCount ) + ( warningCount == 1 ? " warning" : " warnings" ) + " and " + to_string( notificationCount ) + ( warningCount == 1 ? " improvement" : " improvements" ) + "." );
		}
	}
	BuilderTask::BuilderTask( String name, String description, String helpText, std::list<std::shared_ptr<BuilderExecutable>> executables )
	{
		this->name = name;
		this->description = description;
		this->helpText = helpText;
		this->executables = executables;

		//Load notification texts
		notificationTexts[NT::err_unclosedIfdef] = "An ifdef or ifndef is never closed.";
		notificationTexts[NT::err_elseifWithoutIfdef] = "Found elseif without prior ifdef/ifndef.";
		notificationTexts[NT::err_endifWithoutIfdef] = "Found endif without prior ifdef/ifndef.";
		notificationTexts[NT::err_incompleteDefineDirective] = "Incomplete define-directive found. Use '#define <identifier> [definition]'.";
		notificationTexts[NT::err_incompleteUndefineDirective] = "Incomplete undef-directive found. Use '#undef <identifier>'.";
		notificationTexts[NT::err_incompleteIfdefDirective] = "Incomplete ifdef-directive found. Use '#ifdef <identifier>'.";
		notificationTexts[NT::err_incompleteIfndefDirective] = "Incomplete ifndef-directive found. Use '#ifndef <identifier>'.";
		notificationTexts[NT::err_incompleteIncludeDirective] = "Incomplete include-directive found. Use '#include \"<path_to_file>\"'.";
		notificationTexts[NT::err_incompletePragma] = "Incomplete pragma-directive found. Use '#pragma <command> [parameter [...]]'.";
		notificationTexts[NT::err_incompleteMemModelPragma] = "Incomplete memmodel pragma-directive found. Use '#pragma memmodel <model>'.";
		notificationTexts[NT::err_unknownMemModelPragma] = "Unknown memory model defined in pragma. Use fast or large";
		notificationTexts[NT::err_unknownPragma] = "Unknown pragma found.";
		notificationTexts[NT::err_unknownDirective] = "Unknown preprocessor directive found.";
		notificationTexts[NT::err_errorDirective] = "error directive";

		notificationTexts[NT::err_reachedEOF_unfinishedExpression] = "Reached end of file with unfinished expression.";
		notificationTexts[NT::err_unexpectedToken] = "Unexpectd token found. What did you tend to achive with this token?";
		notificationTexts[NT::err_expectedIdentifier_atAllocation] = "An identifier was expected.";
		notificationTexts[NT::err_expectedExprWithReturnValue_atAllocation] = "Expected expression or value at a variable allocation.";
		notificationTexts[NT::err_expectedEqlSign_atAllocation] = "Expected '=' at allocation.";
		notificationTexts[NT::err_expectedKnownLiteral] = "Expected known literal.";
		notificationTexts[NT::err_variableAlreadyDefined] = "The variable identifier is already defined.";
		notificationTexts[NT::err_undefinedIdentifier] = "The identifiern is not defined.";
		notificationTexts[NT::err_noAllocationAfterVariableAttribute] = "Expected allocation after attribute keyword.";

		notificationTexts[NT::err_unexpectedReturningBeforeIdentifier] = "Unexpected returnable before identifier.";
		notificationTexts[NT::err_unexpectedReturningBeforeLiteral] = "Unexpected returnable before literal.";
		notificationTexts[NT::err_unexpectedReturningBeforeParenthesisBlock] = "Unexpected retunrable before parenthesis block.";
		notificationTexts[NT::err_unexpectedReturningBeforeBlock] = "Unexpected retunrable before block.";
		notificationTexts[NT::err_expectedIdentifierBeforeOperator] = "Exptected Identifiern before operation.";
		notificationTexts[NT::err_expectedIdentifierAfterOperator] = "Exptected Identifiern after operation.";
		notificationTexts[NT::err_expectedExprWithReturnValue_atOperation] = "Expected expression or value as rValue at operation.";
		notificationTexts[NT::err_unmatchingTypeFound_atTerm] = "Found unmatching type at Operation.";

		notificationTexts[NT::err_cannotSetAConstantLiteral] = "Cannot set a constant literal or term. Did you mean '=='?";
		notificationTexts[NT::err_onlyConstantsAreAllowedForBitShifts] = "Only positive constants are allowed for binary shifts, due to processor limitations.";
		notificationTexts[NT::err_onlyPositiveConstantsAreAllowedForBitShifts] = "Only positive constants are allowed for binary shifts. Use the opperation antagonist instead.";
		notificationTexts[NT::err_cannotSetAConstantLiteralInCombinedOperationSet] = "Cannot set a constant literal or term with the operation result.";


		notificationTexts[NT::err_expectedIdentifierInParenthesis] = "Unexpected identifier in parenthesis block.";
		notificationTexts[NT::err_expectedExprWithReturnValue_atParenthesis] = "Expected returnable in parenthesis block.";
		notificationTexts[NT::err_unexpectedLiteralInParenthesis] = "Unexpected literal in parenthesis block.";
		notificationTexts[NT::err_unexpectedIdentifierInParenthesis] = "Unexpected identifier in parenthesis block.";
		notificationTexts[NT::err_expectedTermAfterReturnableInParenthesis] = "Expected term after returnable in parenthesis block.";
		notificationTexts[NT::err_unexpectedCloseOfParenthesis] = "Unexpected closing of parenthesis block.";
		notificationTexts[NT::err_aParenthesisWasNotClosed] = "A parenthesis block was not closed.";

		notificationTexts[NT::err_unexpectedCloseOfBlock] = "Unexpected closing of block.";
		notificationTexts[NT::err_aBlockWasNotClosed] = "A block was not closed.";


		notificationTexts[NT::warn_warningDirective] = "waring directive.";
		notificationTexts[NT::warn_emptyDirective] = "Preprocessor directive is empty. You can safely remove this line.";
		notificationTexts[NT::warn_undefinedIdentifiernAtUndefDirective] = "Undef directive used for undefined identifier.";

		notificationTexts[NT::warn_toManyVariablesInFastRAM] = "To many variables were declared for fast memory. All other variables will be saved in normal memory.";


		notificationTexts[NT::imp_operationOnTwoConstantsCanBePrevented] = "The operation is obsolete and could be replaced by a single constant.";
		notificationTexts[NT::imp_operationOnConstantCanBePrevented] = "The operation is obsolete and could be replaced by a single constant.";

	}

	ExecutionResult BuilderTask::execute( std::map<String, std::list<String>> parameters, std::set<char> flags )
	{
		CodeBase codeBase;

		if( parameters.find( "c" ) != parameters.end() ) codeBase.srcFile = std::make_shared<fs::path>( parameters["c"].front() );
		if( parameters.find( "o" ) != parameters.end() ) codeBase.destFile = std::make_shared<fs::path>( parameters["o"].front() );

		if( parameters.find( "pl" ) != parameters.end() ) codeBase.printLexer = true;
		if( parameters.find( "pa" ) != parameters.end() ) codeBase.printAST = true;
		if( parameters.find( "pe" ) != parameters.end() ) codeBase.printEIR = true;

		std::list<std::shared_ptr<CompileNotification>> notificationList;
		ExecutionResult executionResult = ExecutionResult::success;

		for( auto e : executables )
		{
			ExecutionResult result = e->onExecute( codeBase );
			notificationList.insert( notificationList.end(), e->getNotifications().begin(), e->getNotifications().end() );
			if( result != ExecutionResult::success )
			{
				executionResult = result;
				break;
			}
		}

		printErrorLog( notificationList );
		return executionResult;
	}
	void BuilderExecutable::generateNotification( NT error, std::shared_ptr<fs::path> path )
	{
		notificationList.push_back( std::make_shared<CompileNotification>( error, path ) );
	}
	void BuilderExecutable::generateNotification( NT error, std::shared_ptr<Token> token )
	{
		notificationList.push_back( std::make_shared<CompileNotification>( error, token ) );
	}
	void BuilderExecutable::generateNotification( NT error, std::shared_ptr<String> lineText, size_t line, std::shared_ptr<fs::path> path )
	{
		notificationList.push_back( std::make_shared<CompileNotification>( error, line, lineText, path ) );
	}
	const std::list<std::shared_ptr<CompileNotification>> &BuilderExecutable::getNotifications()
	{
		return notificationList;
	}
}
