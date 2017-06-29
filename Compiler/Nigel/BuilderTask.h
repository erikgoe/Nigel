#ifndef NIGEL_BUILDER_TASK_H
#define NIGEL_BUILDER_TASK_H

#include "CodeBase.h"

namespace nigel
{
		//Return type of an builder execution
	enum class ExecutionResult
	{
		unknownError = -1,
		success,

		internalError,
		unknownInternalError,

		astParsingFailed,

		count
	};

		//Single execution process. E. g. lexer, parse ast
	class BuilderExecutable
	{
	public:
		virtual ~BuilderExecutable() {}
		virtual ExecutionResult onExecute( CodeBase &base ) = 0;
	};

		//E. g. build, compile
	class BuilderTask
	{
	protected:
		String name;
		String description;
		String helpText;
		std::list<std::shared_ptr<BuilderExecutable>> executables;

	public:
		BuilderTask( String name, String description, String helpText, std::list<std::shared_ptr<BuilderExecutable>> executables );
		virtual ~BuilderTask() {}

		String getName() { return name; }
		String getDescription() { return description; }
		String getHelp() { return helpText; }
		ExecutionResult execute( std::map<String, std::list<String>> parameters);
	};
}

#endif // !NIGEL_BUILDER_TASK_H
