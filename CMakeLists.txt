cmake_minimum_required (VERSION 3.0)
project (nigel)

# version
set( VERSION_MAJOR 1 )
set( VERSION_MINOR 0 )

# packages
find_package(Boost REQUIRED COMPONENTS filesystem system)

set(CMAKE_CXX_STANDARD 14)

# subdirectories
add_subdirectory(Compiler)

