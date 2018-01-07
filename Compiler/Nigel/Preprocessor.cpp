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

		processFile( base, base.srcFile, base.fileCont, definitions, ifdefCount, ignoreifdef, posAtIgnore );

		if( ifdefCount > 0 )
			generateNotification( NT::err_unclosedIfdef, base.fileCont.back().content, base.fileCont.size(), base.srcFile );

		bool hasError = false;
		for( auto &n : notificationList ) if( n->type > NT::begin_err && n->type < NT::begin_warning )
		{
			hasError = true;
			break;
		}

		if( hasError ) return ExecutionResult::imParsingFailed;
		else return ExecutionResult::success;
	}
	void Preprocessor::processFile( CodeBase &base, std::shared_ptr<fs::path> path, std::vector<CodeBase::LineContent> &result, std::map<String, String> &definitions, size_t &ifdefCount, bool &ignoreifdef, size_t &posAtIgnore )
	{
		std::ifstream filestream( path->string(), std::ios_base::binary );
		String str;
		size_t lineCount = 0;


		while( filestream && std::getline( filestream, str ) )
		{
			lineCount++;

			//Remove whitespaces
			while( str.size() > 0 && ( str[0] == ' ' || str[0] == '\t' || str[0] == '\r' || str[0] == '\n' ) ) str = str.substr( 1 );

			if( str.size() > 0 )
			{
				if( str[0] == '#' )
				{//Is pp directive
					if( str.size() > 1 )
					{
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
								else generateNotification( NT::err_elseifWithoutIfdef, std::make_shared<String>( str ), lineCount, path );
							}
							else if( str.find( "endif" ) == 1 )
							{//endif
								if( ifdefCount > 0 )
								{
									if( posAtIgnore == ifdefCount )
										ignoreifdef = false;
									ifdefCount--;
								}
								else generateNotification( NT::err_endifWithoutIfdef, std::make_shared<String>( str ), lineCount, path );
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
								else generateNotification( NT::err_incompleteDefineDirective, std::make_shared<String>( str ), lineCount, path );
							}
							else if( str.find( "undef" ) == 1 )
							{//undef-directive
								if( str.size() > 7 )
								{
									size_t pos = str.find( '\n' );
									if( str.find( '\r' ) < pos ) pos = str.find( '\r' );
									String identifier = str.substr( 7, pos - 7 );
									if( definitions.find( identifier ) != definitions.end() ) definitions.erase( identifier );
									else generateNotification( NT::warn_undefinedIdentifiernAtUndefDirective, std::make_shared<String>( str ), lineCount, path );
								}
								else generateNotification( NT::err_incompleteUndefineDirective, std::make_shared<String>( str ), lineCount, path );
							}
							else if( str.find( "ifdef" ) == 1 )
							{//ifdef-directive
								if( str.size() > 7 )
								{
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
								else generateNotification( NT::err_elseifWithoutIfdef, std::make_shared<String>( str ), lineCount, path );
							}
							else if( str.find( "ifndef" ) == 1 )
							{//ifndef-directive
								if( str.size() > 8 )
								{
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
								else generateNotification( NT::err_elseifWithoutIfdef, std::make_shared<String>( str ), lineCount, path );
							}
							else if( str.find( "elseif" ) == 1 )
							{//elseif-directive
								if( ifdefCount > 0 )
								{
									if( !ignoreifdef )
									{
										ignoreifdef = true;
										posAtIgnore = ifdefCount;
									}
									else ignoreifdef = false;
								}
								else generateNotification( NT::err_elseifWithoutIfdef, std::make_shared<String>( str ), lineCount, path );
							}
							else if( str.find( "endif" ) == 1 )
							{//endif
								if( ifdefCount > 0 )
								{
									if( posAtIgnore == ifdefCount )
										ignoreifdef = false;
									ifdefCount--;
								}
								else generateNotification( NT::err_endifWithoutIfdef, std::make_shared<String>( str ), lineCount, path );
							}
							else if( str.find( "error" ) == 1 )
							{//error
								size_t pos = str.find( '\n' );
								if( str.find( '\r' ) < pos ) pos = str.find( '\r' );
								String errorInfo = str.substr( 8, pos - 8 );

								generateNotification( NT::err_errorDirective, std::make_shared<String>( str ), lineCount, path );
							}
							else if( str.find( "warning" ) == 1 )
							{//warning
								size_t pos = str.find( '\n' );
								if( str.find( '\r' ) < pos ) pos = str.find( '\r' );
								String errorInfo = str.substr( 8, pos - 8 );

								generateNotification( NT::warn_warningDirective, std::make_shared<String>( str ), lineCount, path );
							}
							else if( str.find( "include" ) == 1 )
							{//include-directive
								if( str.size() > 10 )
								{
									String filename = str.substr( str.find( '\"', 9 ) + 1 );
									fs::path subPath = fs::path( filename.substr( 0, filename.find( '\"' ) ) );
									if( path->has_parent_path() ) subPath = fs::absolute( subPath, path->parent_path() );
									processFile( base, std::make_shared<fs::path>( subPath ), result, definitions, ifdefCount, ignoreifdef, posAtIgnore );
								}
								else generateNotification( NT::err_incompleteIncludeDirective, std::make_shared<String>( str ), lineCount, path );
							}
							else if( str.find( "pragma" ) == 1 )
							{//pragma-directive
								if( str.size() > 8 )
								{
									size_t pos = str.find( '\n' );
									if( str.find( '\r' ) < pos ) pos = str.find( '\r' );
									if( str.find( ' ', 8 ) < pos ) pos = str.find( ' ', 8 );
									else if( str.find( '=', 8 ) < pos ) pos = str.find( '=', 8 );
									String identifier = str.substr( 8, pos - 8 );

									String def;
									if( str.size() > 9 + identifier.size() )
									{
										size_t pos = str.find( '\n' );
										if( str.find( '\r' ) < pos ) pos = str.find( '\r' );
										def = str.substr( 9 + identifier.size(), pos - 9 - identifier.size() );
									}

									if( identifier == "memmodel" )
									{
										if( def.empty() )
										{
											generateNotification( NT::err_incompleteMemModelPragma, std::make_shared<String>( str ), lineCount, path );
										}
										else if( def == "fast" )
										{
											definitions["byte"] = "fast byte";
											definitions["int"] = "fast int";
										}
										else if( def == "norm" )
										{
											definitions["byte"] = "norm byte";
											definitions["int"] = "norm int";
										}
										else generateNotification( NT::err_unknownMemModelPragma, std::make_shared<String>( str ), lineCount, path );
									}
									else generateNotification( NT::err_unknownPragma, std::make_shared<String>( str ), lineCount, path );
								}
								else generateNotification( NT::err_incompletePragma, std::make_shared<String>( str ), lineCount, path );
							}
							else if( str == "##" || str == "#\r" || str == "##\r" )
								generateNotification( NT::warn_emptyDirective, std::make_shared<String>( str ), lineCount, path );
							else generateNotification( NT::err_unknownDirective, std::make_shared<String>( str ), lineCount, path );
						}
					}
					else generateNotification( NT::warn_emptyDirective, std::make_shared<String>( str ), lineCount, path );
				}
				else
				{
					if( !ignoreifdef )
					{//Is not ignored
						//Remove obsolete carriage returns
						replace( str, String( "\r" ), String() );
						str += '\n';

						//Replace defined constructs
						for( auto &s : definitions )
						{//todo: should not be replaced in string literals!
							replace( str, s.first, s.second );
						}

						//Replace debuggable information
						replace( str, String( "__FILE__" ), path->generic_string() );
						replace( str, String( "__LINE__" ), to_string( lineCount ) );
						replace( str, String( "__TIME__" ), String( "" ) );//todo
						replace( str, String( "__DATE__" ), String( "" ) );//todo

						result.push_back( CodeBase::LineContent( std::make_shared<String>( str ), path, lineCount ) );
					}
				}
			}
		}
	}
}
