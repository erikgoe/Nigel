#ifndef NIGEL_BUILDER_TASK_H
#define NIGEL_BUILDER_TASK_H

#include "CodeBase.h"
#include "CompileNotification.h"

namespace nigel
{
	using NT = CompileNotification::Type;

		//Return type of an builder execution
	enum class ExecutionResult
	{
		unknownError = -1,
		success,

		internalError,
		unknownInternalError,

		preprocessorFailed,
		astParsingFailed,
		eirParsingFailed,
		linkerFailed,

		count
	};

		//Single execution process. E. g. lexer, parse ast
	class BuilderExecutable
	{
	protected:
		std::list<std::shared_ptr<CompileNotification>> notificationList;

		void generateNotification( NT error, std::shared_ptr<Token> token );
		void generateNotification( NT error, std::shared_ptr<String> lineText, size_t line, std::shared_ptr<fs::path> path );


	public:
		virtual ~BuilderExecutable() {}
		virtual ExecutionResult onExecute( CodeBase &base ) = 0;
		const std::list<std::shared_ptr<CompileNotification>> &getNotifications();
	};

		//E. g. build, compile
	class BuilderTask
	{
	protected:
		String name;
		String description;
		String helpText;
		std::list<std::shared_ptr<BuilderExecutable>> executables;

		std::map<NT, String> notificationTexts;

		void printErrorLog( std::list<std::shared_ptr<CompileNotification>> notificationList );

	public:
		BuilderTask( String name, String description, String helpText, std::list<std::shared_ptr<BuilderExecutable>> executables );
		virtual ~BuilderTask() {}

		String getName() { return name; }
		String getDescription() { return description; }
		String getHelp() { return helpText; }
		ExecutionResult execute( std::map<String, std::list<String>> parameters, std::set<char> flags );
	};
}

#endif // !NIGEL_BUILDER_TASK_H
