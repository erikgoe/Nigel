#include "stdafx.h"
#include "Builder.h"

#include "Preprocessor.h"
#include "Lexer.h"
#include "AST_Parser.h"
#include "IMC_Generator.h"
#include "Linker.h"

namespace nigel
{
	Builder::Builder()
	{
		loadBuilderTasks();
	}

	void Builder::loadBuilderTasks()
	{
		{//Lexer
			std::list<std::shared_ptr<BuilderExecutable>> e;
			e.push_back( std::make_shared<Preprocessor>() );
			e.push_back( std::make_shared<Lexer>() );
			builderTasks["lexer"] = std::make_shared<BuilderTask>( "Lexer", "Creates the lexer-structure from a soucecode file.", "lexer [-b] [--pl] --c [sourcePath] --o [destinationPath]", e );
		}
		{//Builder
			std::list<std::shared_ptr<BuilderExecutable>> e;
			e.push_back( std::make_shared<Preprocessor>() );
			e.push_back( std::make_shared<Lexer>() );
			e.push_back( std::make_shared<AST_Parser>() );
			e.push_back( std::make_shared<IMC_Generator>() );
			e.push_back( std::make_shared<Linker>() );
			builderTasks["build"] = std::make_shared<BuilderTask>( "Builder", "Creates a hex file from a soucecode file.", "build [-b] [--pl] [--pa] [--pe] [--pasm] --c [sourcePath] --o [destinationPath]", e );
		}
	}


	int Builder::buildFromCL( int argc, char ** argv )
	{
		//Args
		String operation;
		std::set<char> flags;
		std::map<String, std::list<String>> args;
		fs::path currentPath = fs::path( argv[0] );
		std::list<String> otherParams;

		if( argc < 2 ) return 1;
		operation = argv[1];

		{//Create args-list from argc & argv
			String lastParameter = "";
			for( size_t i = 2 ; i < static_cast<size_t>( argc ) ; i++ )
			{
				String v = String( argv[i] );
				if( v.size() > 1 )
				{
					if( v[0] == '-' )
					{
						if( v[1] == '-' )
						{//Found argument
							lastParameter = v.substr( 2 );
							args.insert( std::pair<String, std::list<String>>( lastParameter, std::list<String>() ) );
						}
						else
						{//Found flag
							for( size_t j = 1 ; j < v.size() ; j++ ) flags.insert( v[j] );
						}
					}
					else if( args.empty() )
					{
						otherParams.push_back( v );
					}
					else args[lastParameter].push_back( v );
				}
			}
		}


		//Decide which operation should be executed
		if( operation == "help" || operation == "--help" )
		{//Print help text.
			if( otherParams.empty() )
			{//General help
				log( "Nigel is a compiler for the nigel programing language.\nIt can create executable hex files for the 8051 microcontroller.\n" );
				log( "nigel --help\nnigel help                Print this information text. Type nigel --help [command] to get more \n                          information about a specific command." );
				log( "nigel --version\nnigel version             Print the version information." );
				for( auto c : builderTasks )
				{
					logMultiline( "nigel " + c.first );
					for( size_t i = 0 ; i < 20 - c.first.size() ; i++ ) logMultiline( " " );
					log( c.second->getDescription() );
				}
			}
			else
			{
				String command = otherParams.front();
				if( command == "help [command]" )
				{
					log( "Prints out a list of all available sub-commands." );
				}
				else if( command == "version" )
				{
					log( "Will print out the version of this application." );
				}
				else if( builderTasks.find( command ) != builderTasks.end() )
				{//Print help text for a command
					BuilderTask &task = *builderTasks.find( command )->second;
					log( "Nigel " + task.getName() );
					log( task.getDescription() );
					log( "\nnigel " + task.getHelp() + "\n" );

					log( "   sourcePath             Path to the source file." );
					log( "   destinationPath        File to write to." );
					log( "   pl                     Print lexer structure." );
					log( "   pa                     Print AST." );
					log( "   pi                     Print IMC." );
					log( "   pasm                   Print assembly code." );
					log( "   b                      Pause if an error occurred." );
				}
			}
		}
		else if( operation == "version" || operation == "--version" )
		{//Print version information.
			log( "Nigel compiler version " + to_string( NIGEL_VERSION ) );
		}
		else if( builderTasks.find( operation ) != builderTasks.end() )
		{//Execute other operation. Found operation.
			ExecutionResult result;
			/*try //todo uncomment
			{*/
				result = builderTasks.find( operation )->second->execute( args, flags );
			/*}
			catch( std::exception &e )
			{
				log( "An internel compiler error occourred: " + String( e.what() ), LogLevel::Error );
				result = ExecutionResult::internalError;
			}
			catch( ... )
			{
				log( "An unknown internel compiler error occourred!", LogLevel::Error );
				result = ExecutionResult::unknownInternalError;
			}*/
			if( result != ExecutionResult::success )
			{//Panic
				log( "An error occurred while processing '" + operation + "'", LogLevel::Error );
				if( flags.find( 'b' ) != flags.end() ) std::cin.ignore();
				return static_cast< int >( result );
			}
		}

		return 0;
	}
}
