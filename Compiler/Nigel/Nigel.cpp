// Nigel.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "stdafx.h"
#include "Builder.h"

int main(int argc, char **argv)
{
	nigel::Builder builder;
	return builder.buildFromCL( argc, argv );
}

