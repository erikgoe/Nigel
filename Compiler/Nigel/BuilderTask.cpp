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
			else outStr += to_string( n->line );
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
			outStr += ": \n";

			outStr += *n->lineText + "\n";

			if( n->token != nullptr )
			{
				for( size_t i = 1 ; i < n->token->columnNo ; i++ ) outStr += ' ';
				for( size_t i = 0 ; i < n->token->toString().size() ; i++ ) outStr += '^';
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

		//todo load notification texts
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
