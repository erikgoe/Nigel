#include "stdafx.h"
#include "Preprocessor.h"

namespace nigel
{
	Preprocessor::Preprocessor()
	{
	}
	ExecutionResult Preprocessor::onExecute( CodeBase & base )
	{
		std::map<String, String> definitions;
		size_t ifdefCount = 0;
		bool ignoreifdef = false;
		size_t posAtIgnore = 0;

		//Default definitions
		definitions["__PLATFORM__"] = "ATMEL_8051";
		definitions["__COMPILER__"] = "NIGEL_" + to_string( NIGEL_VERSION );

		processFile( base.srcFile, base.fileCont, definitions, ifdefCount, ignoreifdef, posAtIgnore );

		if( ifdefCount > 0 )
		{
			//todo error: some remaining open ifdef/ifndef
		}

		return ExecutionResult::success;
	}
	void Preprocessor::processFile( fs::path path, String &result, std::map<String, String> &definitions, size_t &ifdefCount, bool &ignoreifdef, size_t &posAtIgnore )
	{
		std::ifstream filestream( path.string(), std::ios_base::binary );
		String str;
		size_t lineCount = 0;


		while( filestream && std::getline( filestream, str ) )
		{
			lineCount++;

			//Remove whitespaces
			while( str.size() > 0 && ( str[0] == ' ' || str[0] == '\t' || str[0] == '\r' || str[0] == '\n' ) ) str = str.substr( 1 );

			if( str.size() > 0 && str[0] == '#' )
			{//Is pp directive
				if( ignoreifdef )
				{//Is ignoring

					if( str.find( "ifdef" ) == 1 || str.find( "ifndef" ) == 1 )
					{//ifdef/ifndef-directive
						ifdefCount++;
					}
					if( str.find( "elseif" ) == 1 )
					{//elseif-directive
						if( ifdefCount > 0 )
						{
							if( posAtIgnore == ifdefCount )
								ignoreifdef = false;
						}
						else
						{
							//todo error: elseif uses non-existing ifdef/ifndef
						}
					}
					else if( str.find( "endif" ) == 1 )
					{//endif
						if( ifdefCount > 0 )
						{
							if( posAtIgnore == ifdefCount )
								ignoreifdef = false;
							ifdefCount--;
						}
						else
						{
							//todo error: endif closes non-existing ifdef/ifndef
						}
					}
				}
				else
				{
					if( str.find( "define" ) == 1 )
					{//define-directive
						if( str.size() > 8 )
						{
							size_t pos = str.find( '\n' );
							if( str.find( '\r' ) < pos ) pos = str.find( '\r' );
							if( str.find( ' ', 8 ) < pos ) pos = str.find( ' ', 8 );
							String identifier = str.substr( 8, pos - 8 );

							String def;
							if( str.size() > 9 + identifier.size() )
							{
								size_t pos = str.find( '\n' );
								if( str.find( '\r' ) < pos ) pos = str.find( '\r' );
								def = str.substr( 9 + identifier.size(), pos - 9 - identifier.size() );
							}
							definitions[identifier] = def;
						}
						else
						{
							//todo error: unresolvable pp-definition
						}
					}
					else if( str.find( "undef" ) == 1 )
					{//undef-directive
						if( str.size() > 7 )
						{
							size_t pos = str.find( '\n' );
							if( str.find( '\r' ) < pos ) pos = str.find( '\r' );
							String identifier = str.substr( 7, pos - 7 );
							if( definitions.find( identifier ) != definitions.end() ) definitions.erase( identifier );
						}
						else
						{
							//todo error: unresolvable pp-definition
						}
					}
					else if( str.find( "ifdef" ) == 1 )
					{//ifdef-directive
						size_t pos = str.find( '\n' );
						if( str.find( '\r' ) < pos ) pos = str.find( '\r' );
						String identifier = str.substr( 7, pos - 7 );

						ifdefCount++;
						if( definitions.find( identifier ) == definitions.end() )
						{
							ignoreifdef = true;
							posAtIgnore = ifdefCount;
						}
					}
					else if( str.find( "ifndef" ) == 1 )
					{//ifndef-directive
						size_t pos = str.find( '\n' );
						if( str.find( '\r' ) < pos ) pos = str.find( '\r' );
						String identifier = str.substr( 8, pos - 8 );

						ifdefCount++;
						if( definitions.find( identifier ) != definitions.end() )
						{
							ignoreifdef = true;
							posAtIgnore = ifdefCount;
						}
					}
					else if( str.find( "elseif" ) == 1 )
					{//elseif-directive
						if( ifdefCount > 0 )
						{
							if( !ignoreifdef ) ignoreifdef = true;
							else ignoreifdef = false;
						}
						else
						{
							//todo error: elseif uses non-existing ifdef/ifndef
						}
					}
					else if( str.find( "endif" ) == 1 )
					{//endif
						if( ifdefCount > 0 )
						{
							ifdefCount--;
							ignoreifdef = false;
						}
						else
						{
							//todo error: endif closes non-existing ifdef/ifndef
						}
					}
					else if( str.find( "error" ) == 1 )
					{//endif
						size_t pos = str.find( '\n' );
						if( str.find( '\r' ) < pos ) pos = str.find( '\r' );
						String errorInfo = str.substr( 8, pos - 8 );
						//todo error: print the error information
					}
					else if( str.find( "include" ) == 1 )
					{//include-directive
						if( str.size() > 10 )
						{
							String filename = str.substr( str.find( '\"', 9 ) + 1 );
							fs::path subPath = fs::path( filename.substr( 0, filename.find( '\"' ) ) );
							if( path.has_parent_path() ) subPath = fs::absolute( subPath, path.parent_path() );
							processFile( subPath, result, definitions, ifdefCount, ignoreifdef, posAtIgnore );
						}

					}
					else
					{
						//todo error: unknown pp-directive
					}
				}
			}
			else
			{
				if( !ignoreifdef )
				{//Is not ignored
					//Replace defined constructs
					for( auto &s : definitions )
					{//todo: should not be replaced in literals!
						replace( str, s.first, s.second );
					}

					//Replace debuggable information
					replace( str, String( "__FILE__" ), path.generic_string() );
					replace( str, String( "__LINE__" ), to_string( lineCount ) );
					replace( str, String( "__TIME__" ), String( "" ) );//todo
					replace( str, String( "__DATE__" ), String( "" ) );//todo

					result += str + '\n';
				}
			}

		}
	}
}
