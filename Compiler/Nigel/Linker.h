#ifndef NIGEL_LINKER_H
#define NIGEL_LINKER_H

#include "BuilderTask.h"

namespace nigel
{
		//Linkes EIR-structure into a hex file.
	class Linker : public BuilderExecutable
	{
	public:
		Linker();

		ExecutionResult onExecute( CodeBase &base ) override;

	};
}

#endif // !NIGEL_LINKER_H

