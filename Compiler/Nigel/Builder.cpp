#include "stdafx.h"
#include "Builder.h"

#include "Lexer.h"
#include "AST_Parser.h"
#include "EIR_Parser.h"

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
			e.push_back( std::make_shared<Lexer>() );
			e.push_back( std::make_shared<AST_Parser>() );
			e.push_back( std::make_shared<EIR_Parser>() );
			builderTasks["lexer"] = std::make_shared<BuilderTask>( "Lexer", "Creates the lexer-structure from a soucecode file.", "\n--lexer", e );
		}
	}


	int Builder::buildFromCL( int argc, char ** argv )
	{
		//Args
		std::set<char> flags;
		std::map<String, std::list<String>> args;
		fs::path currentPath = fs::path( argv[0] ).generic( );

		{//Create args-list from argc & argv
			String lastParameter = "";
			for( size_t i = 1 ; i < static_cast<size_t>( argc ) ; i++ )
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
					else
					{
						if( args.empty() )
						{//Add parameter to args
							lastParameter = v.substr( 1 );
							args.insert( std::pair<String, std::list<String>>( lastParameter, std::list<String>() ) );
						}
						else
						{//Found parameter
							args[lastParameter].push_back( v );
						}
					}
				}
			}
		}


		//Decide which operation should be executed
		if( args.find( "help" ) != args.end() )
		{//Print help text.
			if( args.find( "help" )->second.empty() )
			{//General help
				log( "Nigel is a compiler for the nigel programing language.\nIt can create executable hex files for the 8051 microcontroller.\n" );
				for( auto c : builderTasks )
				{
					logMultiline( "nigel --" + c.first );
					for( size_t i = 0 ; i < 20 - c.first.size() ; i++ ) logMultiline( " " );
					log( c.second->getDescription() );
				}
			}
			else
			{
				String command = args.find( "help" )->second.front();
				if( command == "help" )
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
					log( task.getName() );
					log( task.getDescription() );
					log( task.getHelp() );
				}
			}
		}
		else if( args.find( "version" ) != args.end() )
		{//Print version information.
			log( "Nigel compiler version " + to_string( NIGEL_VERSION ) );
		}
		else 
		{//Execute other operation.
			for( auto command : args )
			{
				if( command.first != "help" && command.first != "version" && builderTasks.find( command.first ) != builderTasks.end() )
				{//Found operation
					ExecutionResult result = builderTasks.find( command.first )->second->execute( command.second );
					if( result != ExecutionResult::success )
					{//Panic
						log( "An error occurred while processing '" + command.first + "'", LogLevel::Error );
						return static_cast<int>(result);
					}
				}
			}
		}


		return 0;
	}
}
