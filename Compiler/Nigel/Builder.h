#ifndef NIGEL_BUILDER_H
#define NIGEL_BUILDER_H

#include "stdafx.h"
#include "BuilderTask.h"

namespace nigel
{
		//Class to build a project, including operations for command line.
	class Builder
	{
		std::map<String, std::shared_ptr<BuilderTask>> builderTasks;//Commands mapped to their execution task

	public:
		Builder();

		void loadBuilderTasks();

		//Creates a project from command line. Returns the result.
		int buildFromCL( int argc, char **argv );
	};
}

#endif // !NIGEL_BUILDER_H
