#ifndef NIGEL_PREPROCESSOR_H
#define NIGEL_PREPROCESSOR_H

#include "BuilderTask.h"

namespace nigel
{
	class Preprocessor : public BuilderExecutable
	{
	public:
		Preprocessor();

		ExecutionResult onExecute( CodeBase &base ) override;

		void processFile( fs::path path, String &result, std::map<String, String> &definitions, size_t &ifdefCount, bool &ignoreifdef, size_t &posAtIgnore );
	};
}

#endif // !NIGEL_PREPROCESSOR_H
