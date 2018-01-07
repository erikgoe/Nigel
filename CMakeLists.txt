cmake_minimum_required (VERSION 3.0)
project (nigel)

# version
set( VERSION_MAJOR 1 )
set( VERSION_MINOR 0 )

# packages
find_package(Boost REQUIRED COMPONENTS filesystem system)

set(CMAKE_CXX_STANDARD 14)

SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
SET(BUILD_SHARED_LIBRARIES OFF)
SET(CMAKE_EXE_LINKER_FLAGS "-static")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

# subdirectories
add_subdirectory(Compiler)

