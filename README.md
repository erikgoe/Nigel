# Nigel
The Nigel programming language

## What is it?
Nigel is a new programming language for the 8051 microcontroller, with a c-like syntax.

## Goals
- [x] Complete compilation from source code to hex files.
- [x] Resolve complex expressions
- [x] Functions
- [x] Control flow (if, while, etc.)
- [x] Pointers
- [x] Intern/extern RAM
- [x] Preprocessor
- [x] Compile the compiler on linux (clang/gcc)
- [x] Interrupts
- [x] 8051-specific stuff (ports, timer, etc.)

## Nice to have
* arrays (can be emulated with a pointer)
* string literals
* structs
* optimizing code

## How does it work?
The Compiler is divided into these parts:
```
     Source code
          |
          v
+--------------------+
|    Preprocessor    |
+--------------------+
          |
          v
    Line textcode
          |
          v
+--------------------+
|       Lexer        |
+--------------------+
          |
          v
     Lexer code
          |
          v
+--------------------+
|     AST Parser     |
+--------------------+
          |
          v
         AST
          |
          v
+--------------------+
|    IM Generator    |
+--------------------+
          |
          v
         IMC
          |
          v
+--------------------+
|       Linker       |
+--------------------+
          |
          v
       HEX code
```
The resulting Hex code can be uploaded onto a 8051 microcontroller or be interpreted in a simulator.

### Next development steps
* new, delete (dynamic memory)

* std-library

## Getting started
### Building
#### On Windows
Pre-generated solution:
1. Install Visual Studio (tested with VC 2017).
2. Install the boost library:
    1. Download the library from http://www.boost.org/ and add it to the project dependencies (make sure filesystem and system are compiled)
    2. __or__ use vcpkg (https://github.com/Microsoft/vcpkg/) and install boost with 
       ```<vcpkg_install_dir>\vcpkg install boost-filesystem boost-system```.
       This might be the easier way.
3. Clone this repository.
4. Open Compiler\Nigel.sln, compile and hope for the best.
The output binaries will be in Nigel\Compiler\Release\ or Nigel\Compiler\Debug\ depending on your configuration.

CMake (static build):
1. Install cmake
2. Install the boost library with vcpkg as above
use ```<vcpkg_install_dir>\vcpkg install --triplet x86-windows-static boost-filesystem boost-system``` for static build
3. Clone this repository.
4. create the project
```
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=<vcpkg_install_dir>/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x86-windows-static
```
5. Open build\Nigel.sln and compile

#### On Linux
1. Install cmake (https://cmake.org/ or just ```sudo apt-get install cmake```).
2. Install boost with ```sudo apt-get install libboost-all-dev```.
3. Clone this repository (```git clone https://github.com/erikgoe/nigel```).
    1. Use clang (recommanded):
       Install clang-4.0 (google it for your distribution). This should work for ubuntu 16.04 xenial:
       ```sudo apt-add-repository "deb http://llvm.org/apt/xenial/ llvm-toolchain-xenial-4.0 main"```
       ```sudo apt-get update```
       ```sudo apt-get install clang-4.0 lldb-4.0```
       Then just run 'build_clang.sh' script from this repository.
    2. __or__ use gcc:
       ```mkdir build | cd```
       ```cmake ..```
       ```make```
The output binaries will be in Nigel/Compiler/build/Nigel/.

### Usage
```cd <binaries_output>```,

then, with cmake
```nigel build --c ../../tests/code/functions.nig --o ../../tests/code/functions.hex```
or without (on Windows)
```nigel build --c ..\tests\code\functions.nig --o ..\tests\code\functions.hex```
to compile a test program

or 
```nigel help```
to get further information on how to use nigel.

Alternatively you can add the binaries output directory to your PATH environment variable on windows.

## Who uses Nigel?
Hmm... maybe you?
