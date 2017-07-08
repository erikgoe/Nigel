#ifndef NIGEL_HELPER_DEFINITIONS_H
#define NIGEL_HELPER_DEFINITIONS_H

namespace helper
{
	//Basic types
	using u8 = unsigned char;
	using u16 = unsigned short;
	using u32 = unsigned int;
	using u64 = unsigned long long;

	using s8 = signed char;
	using s16 = signed short;
	using s32 = signed int;
	using s64 = signed long long;

	using f32 = float;
	using f64 = double;


	//Helperfunctions to in-/decrement enumerated types. E. g. in for-loops
	template<typename T>
	//Increments a enumerated type.
	void incEnum( T &e )
	{
		e = static_cast<T>( static_cast<size_t>( e ) + 1 );
	}
	//Increments a enumerated type with a signed int.
	template<typename T>
	void incEnumSInt( T &e )
	{
		e = static_cast<T>( static_cast<signed int>( e ) + 1 );
	}
	//Decrements a enumerated type.
	template<typename T>
	void decEnum( T &e )
	{
		e = static_cast<T>( static_cast<size_t>( e ) - 1 );
	}
	//Decrements a enumerated type with a signed int.
	template<typename T>
	void decEnumSInt( T &e )
	{
		e = static_cast<T>( static_cast<signed int>( e ) - 1 );
	}


	//String-deklarations
	using String = std::string;
	using String8 = std::string;
	using String16 = std::u16string;
	using String32 = std::u32string;
	using StringW = std::wstring;
	using StringA = std::string;

	//String-conversions. Pass \p converter to speedup the conversation.
	StringW to_wide( const String8 &str );
	String8 to_utf8( const StringW &str );

	String8 to_utf8( const String16 &str );
	String8 to_utf8( const String32 &str );
	String16 to_utf16( const String8 &str );
	String32 to_utf32( const String8 &str );

	//Translate integer to hex-string
	template< typename T >
	std::string int_to_hex( T i )
	{
		std::stringstream stream;
		stream << std::setfill( '0' ) << std::setw( sizeof( T ) * 2 )
			<< std::hex << i;
		return stream.str();
	}
	template<>
	std::string int_to_hex( u8 i );
		template<>
	std::string int_to_hex( s8 i );

	using std::to_string;

	using std::stoi;
	using std::stol;
	using std::stoll;
	using std::stoul;
	using std::stoull;
	using std::stof;
	using std::stod;
	using std::stold;

	//String-helper-functions
	template<typename T>
	void replace( T &searchIn, const T &searchFor, const T &replaceWith )
	{
		size_t pos = 0;
		while( pos != T::npos )
		{
			pos = searchIn.find( searchFor, pos );
			if( pos != T::npos )
			{
				searchIn.replace( pos, searchFor.size(), replaceWith );
				pos += replaceWith.size();
			}
		}
	}


		//Enum to define the log type.
	enum class LogLevel
	{
		Debug,
		Information,
		Warning,
		Error,

		count
	};

		//Log the specified utf-8-message with the user-defined logger-function. If no function has been defined, std-streams will be used. Not multithreading-safe! A newline will be automatically attached.
	inline void log( const String8 &text, LogLevel level = LogLevel::Information );
		//Log the specified utf-8-message with the user-defined logger-function. If no function has been defined, std-streams will be used. Not multithreading-safe! No newline will be automatically attached.
	inline void logMultiline( const String8 &text, LogLevel level = LogLevel::Information );

	//Using boost namespace
	namespace fs = boost::filesystem;

	//Resolve path. "/x/../y" -> "/y".
	fs::path resolve( const fs::path &p, const fs::path &base = fs::current_path() );

	//Start a Windows-application in its own thread. To open a file set the applicationpath (e. g. "c:\\editor.exe"  or "c:\\explorer.exe"(starts default application) on windows) and the file as parameter. Returns true application successfully started, otherwise errorlog will be created.
	bool runApp( const String8 &FileName, const String8 &FileParam = "" );

}
using namespace helper;

#endif // !NIGEL_HELPER_DEFINITIONS_H
