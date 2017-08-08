#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>


//Undefs of the standardlibrary
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

//Optimizing
#pragma inline_recursion( on )
#pragma inline_depth( 254 )//Maximum | < infinit (254)

#else // _WIN32

#endif // _WIN32



#include <time.h>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

#include <exception>
#include <stdexcept>
#include <iterator>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include <bitset>
#include <locale>
#include <codecvt>
#include <string>
#include <sstream>
#include <array>
#include <queue>
#include <stack>


#include <memory>
#include <utility>
#include <random>
#include <regex>
//#include <chrono> //del
//#include <future> //del
#include <algorithm>

#include <thread>
#include <mutex>
#include <atomic>


//Boost
#ifndef BOOST_ALL_DYN_LINK
#define BOOST_ALL_DYN_LINK//For dll-binding of all Boost-libraries
#endif
#include "boost/filesystem.hpp"

#ifdef _WIN32
//Bind libraries
//In debug- and release-mode

#ifdef _DEBUG //In debug-mode
#pragma comment(linker, "/subsystem:console")//Maintain console

#else //In release-mode

//#pragma comment(linker, "/subsystem:windows")//Remove console

#endif
#endif


//Own header
#include "HelperDefinitions.h"
#include "Version.h"
