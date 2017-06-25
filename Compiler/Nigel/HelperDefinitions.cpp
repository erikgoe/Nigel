#include "stdafx.h"
#include "HelperDefinitions.h"

namespace helper
{
	StringW to_wide( const String8 &str )
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		try
		{
			return converter.from_bytes( str );
		}
		catch( std::range_error error )
		{
			log( "Conversation failure 'to_wide(const String8 &)'. Message: " + String8( error.what() ), LogLevel::Warning );
			return L"";
		}
	}

	String8 to_utf8( const StringW &str )
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		try
		{
			return converter.to_bytes( str );
		}
		catch( std::range_error error )
		{
			log( "Conversation failure 'to_utf8(const StringW &)'. Message: " + String8( error.what() ), LogLevel::Warning );
			return "";
		}
	}



	#if (_MSC_VER == 1900 || _MSC_VER == 1910) //bug-workaround (VC_2015)
	
		String8 to_utf8( const String16 &str )
		{
			std::wstring_convert<std::codecvt_utf8<int16_t>, int16_t> converter;
			try
			{
				auto p = reinterpret_cast<const int16_t *>( str.data() );
				return converter.to_bytes( p, p + str.size() );
			}
			catch( std::range_error error )
			{
				log( "Conversation failure 'to_utf8(const String16 &)'. Message: " + String8( error.what() ), LogLevel::Warning );
				return "";
			}
		}
		String8 to_utf8( const String32 &str )
		{
			std::wstring_convert<std::codecvt_utf8<int32_t>, int32_t> converter;
			try
			{
				auto p = reinterpret_cast<const int32_t *>( str.data() );
				return converter.to_bytes( p, p + str.size() );
			}
			catch( std::range_error error )
			{
				log( "Conversation failure 'to_utf8(const String32 &)'. Message: " + String8( error.what() ), LogLevel::Warning );
				return "";
			}
		}
		String16 to_utf16( const String8 &str )
		{
			std::wstring_convert<std::codecvt_utf8<int16_t>, int16_t> converter;
			try
			{
				auto p = converter.from_bytes( str );
				return String16( reinterpret_cast<const char16_t *>( p.data() ) );
			}
			catch( std::range_error error )
			{
				log( "Conversation failure 'to_utf16(const String8 &)'. Message: " + String8( error.what() ), LogLevel::Warning );
				return u"";
			}
		}
		String32 to_utf32( const String8 &str )
		{
			std::wstring_convert<std::codecvt_utf8<int32_t>, int32_t> converter;
			try
			{
				auto p = converter.from_bytes( str );
				return String32( reinterpret_cast<const char32_t *>( p.data() ) );
			}
			catch( std::range_error error )
			{
				log( "Conversation failure 'to_utf32(const String8 &)'. Message: " + String8( error.what() ), LogLevel::Warning );
				return U"";
			}
		}
	
	
	#else
	
		String8 to_utf8( const String16 &str )
		{
			std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> converter;
			try
			{
				return converter.to_bytes( str );
			}
			catch( std::range_error error )
			{
				log( "Conversation failure 'to_utf8(const String16 &)'. Message: " + String8( error.what() ), LogLevel::Warning );
				return "";
			}
		}
		String8 to_utf8( const String32 &str )
		{
			std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
			try
			{
				return converter.to_bytes( str );
			}
			catch( std::range_error error )
			{
				log( "Conversation failure 'to_utf8(const String32 &)'. Message: " + String8( error.what() ), LogLevel::Warning );
				return "";
			}
		}
		String16 to_utf16( const String8 &str )
		{
			std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> converter;
			try
			{
				return converter.from_bytes( str );
			}
			catch( std::range_error error )
			{
				log( "Conversation failure 'to_utf16(const String8 &)'. Message: " + String8( error.what() ), LogLevel::Warning );
				return u"";
			}
		}
		String32 to_utf32( const String8 &str )
		{
			std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
			try
			{
				return converter.from_bytes( str );
			}
			catch( std::range_error error )
			{
				log( "Conversation failure 'to_utf32(const String8 &)'. Message: " + String8( error.what() ), LogLevel::Warning );
				return U"";
			}
		}
	#endif

	template<>
	std::string int_to_hex( s8 i )
	{
		std::stringstream stream;
		stream << std::setfill( '0' ) << std::setw( 2 )
			<< std::hex << static_cast<int>( i );
		return stream.str();
	}
	template<>
	std::string int_to_hex( u8 i )
	{
		std::stringstream stream;
		stream << std::setfill( '0' ) << std::setw( 2 )
			<< std::hex << static_cast<int>( i );
		return stream.str();
	}

	inline void log( const String8 &text, LogLevel level )
	{
		logMultiline( text + '\n', level );
	}
	inline void logMultiline( const String8 &text, LogLevel level )
	{
		static std::recursive_mutex mx;
		std::lock_guard<std::recursive_mutex> lock( mx );

		OutputDebugStringW( to_wide( String8( level == LogLevel::Debug ? "D" : level == LogLevel::Information ? "I" : level == LogLevel::Warning ? "W" : level == LogLevel::Error ? "E" : "UKN" ) + "/ " + text ).c_str() );

		HANDLE  consoleHandle = GetStdHandle( STD_OUTPUT_HANDLE );
		if( level == LogLevel::Debug ) SetConsoleTextAttribute( consoleHandle, FOREGROUND_GREEN | FOREGROUND_INTENSITY );
		else if( level == LogLevel::Information ) SetConsoleTextAttribute( consoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY );
		else if( level == LogLevel::Warning )SetConsoleTextAttribute( consoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY );
		else if( level == LogLevel::Error ) SetConsoleTextAttribute( consoleHandle, FOREGROUND_RED | FOREGROUND_INTENSITY );

		if( level == LogLevel::Error || level == LogLevel::Warning )
		{
			std::wcerr << to_wide( text );
		}
		else
		{
			std::wcout << to_wide( text );
		}
		SetConsoleTextAttribute( consoleHandle, 7 );//Reset color
	}

	fs::path resolve( const fs::path &p, const fs::path &base )
	{
		fs::path ret;
		fs::path absolute = fs::absolute( p, base );

		for( fs::path::iterator it = absolute.begin() ; it != absolute.end() ; it++ )
		{
			if( *it == ".." )
			{//Go one dir backwars
				if( fs::is_symlink( ret ) )
				{//Check for symlinks
					ret /= *it;
				}
				else if( ret.filename() == ".." )
				{//Multiple ..s'
					ret /= *it;
				}
				// Otherwise it should be safe to resolve the parent
				else
					ret = ret.parent_path();
			}
			else if( *it == "." );//Ignore
			else
			{//Normal dirs
				ret /= *it;
			}
		}
		return ret;
	}

	bool runApp( const String8 &path, const String8 &param )
	{
		int ret = reinterpret_cast<int>( ShellExecute( 0, L"open", to_wide( path ).c_str(), to_wide( param ).c_str(), 0, SW_SHOWNORMAL ) );

		if( ret <= 32 )
		{
			switch( ret )
			{
			case ERROR_FILE_NOT_FOUND:
				log( "RunApp failed: file not found: '" + path + "'", LogLevel::Error );
				break;

			case ERROR_PATH_NOT_FOUND:
				log( "RunApp failed: path not found: '" + path + "'", LogLevel::Error );
				break;

			case ERROR_BAD_FORMAT:
				log( "RunApp failed: application is invalid or broken: '" + path + "'", LogLevel::Error );
				break;

			case SE_ERR_ACCESSDENIED:
				log( "RunApp failed: access denied: '" + path + "'", LogLevel::Error );
				break;

			case SE_ERR_DDEBUSY:
				log( "RunApp failed: DDE operation is busy. File: '" + path + "'", LogLevel::Error );
				break;

			case SE_ERR_DDEFAIL:
				log( "RunApp failed: DDE operation failed. File: '" + path + "'", LogLevel::Error );
				break;

			case SE_ERR_DDETIMEOUT:
				log( "RunApp failed: DDE operation timed out. File: '" + path + "'", LogLevel::Error );
				break;

			case SE_ERR_DLLNOTFOUND:
				log( "RunApp failed: dynamic-link library not found. File: '" + path + "'", LogLevel::Error );
				break;

			case SE_ERR_SHARE:
				log( "RunApp failed: cannot share an open file. File: '" + path + "'", LogLevel::Error );
				break;

			case SE_ERR_ASSOCINCOMPLETE:
				log( "RunApp failed: file association information not complete. File: '" + path + "'", LogLevel::Error );
				break;

			case SE_ERR_NOASSOC:
				log( "RunApp failed: file association not available. File: '" + path + "'", LogLevel::Error );
				break;

			case SE_ERR_OOM:
				log( "RunApp failed: out of memory. File: '" + path + "'", LogLevel::Error );
				break;

			case 0:
				log( "RunApp failed. out of memory. File: '" + path + "'", LogLevel::Error );
				break;

			default:
				log( "RunApp failed: Cannot open file. File: '" + path + "'", LogLevel::Error );
				break;
			}
			return false;
		}
		else
		{
			log( "RunApp file: '" + path + "'" );
			return true;
		}
	}

	bool printFile( const String8 &path )
	{
		int ret = ( int )ShellExecute( 0, L"print", to_wide( path ).c_str(), 0, 0, SW_HIDE );

		if( ret <= 32 )
		{
			switch( ret )
			{
			case ERROR_FILE_NOT_FOUND:
				log( "Print file failed: file not found: '" + path + "'", LogLevel::Error );
				break;

			case ERROR_PATH_NOT_FOUND:
				log( "Print file failed: path not found: '" + path + "'", LogLevel::Error );
				break;

			case ERROR_BAD_FORMAT:
				log( "Print file failed: application is invalid or broken: '" + path + "'", LogLevel::Error );
				break;

			case SE_ERR_ACCESSDENIED:
				log( "Print file failed: access denied: '" + path + "'", LogLevel::Error );
				break;

			case SE_ERR_DDEBUSY:
				log( "Print file failed: DDE operation is busy. File: '" + path + "'", LogLevel::Error );
				break;

			case SE_ERR_DDEFAIL:
				log( "Print file failed: DDE operation failed. File: '" + path + "'", LogLevel::Error );
				break;

			case SE_ERR_DDETIMEOUT:
				log( "Print file failed: DDE operation timed out. File: '" + path + "'", LogLevel::Error );
				break;

			case SE_ERR_DLLNOTFOUND:
				log( "Print file failed: dynamic-link library not found. File: '" + path + "'", LogLevel::Error );
				break;

			case SE_ERR_SHARE:
				log( "Print file failed: cannot share an open file. File: '" + path + "'", LogLevel::Error );
				break;

			case SE_ERR_ASSOCINCOMPLETE:
				log( "Print file failed: file association information not complete. File: '" + path + "'", LogLevel::Error );
				break;

			case SE_ERR_NOASSOC:
				log( "Print file failed: file association not available. File: '" + path + "'", LogLevel::Error );
				break;

			case SE_ERR_OOM:
				log( "Print file failed: out of memory. File: '" + path + "'", LogLevel::Error );
				break;

			case 0:
				log( "Print file failed. out of memory. File: '" + path + "'", LogLevel::Error );
				break;

			default:
				log( "Print file failed: Cannot open file. File: '" + path + "'", LogLevel::Error );
				break;
			}
			return false;
		}
		else
		{
			log( "Print file file: '" + path + "'", LogLevel::Error );
			return true;
		}
	}
}