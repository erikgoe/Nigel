#ifndef NIGEL_LINKER_H
#define NIGEL_LINKER_H

#include "BuilderTask.h"

namespace nigel
{
		//Linkes IMC into a hex file.
	class Linker : public BuilderExecutable
	{
	public:
		Linker();

		ExecutionResult onExecute( CodeBase &base ) override;

		void printToFile( const std::vector<u8> &data, fs::path file );
	};
}

#endif // !NIGEL_LINKER_H

