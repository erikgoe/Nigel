#include "stdafx.h"
#include "BuilderTask.h"

namespace nigel
{
	BuilderTask::BuilderTask( String name, String description, String helpText, std::list<std::shared_ptr<BuilderExecutable>> executables )
	{
		this->name = name;
		this->description = description;
		this->helpText = helpText;
		this->executables = executables;
	}

	ExecutionResult BuilderTask::execute( std::map<String, std::list<String>> parameters )
	{
		CodeBase codeBase;

		if( parameters.find( "c" ) != parameters.end() ) codeBase.srcFile = parameters["c"].front();
		if( parameters.find( "o" ) != parameters.end() ) codeBase.destFile = parameters["o"].front();


		for( auto e : executables )
		{
			ExecutionResult result = e->onExecute( codeBase );
			if( result != ExecutionResult::success )
			{
				return result;
			}
		}
		return ExecutionResult::success;
	}
}
