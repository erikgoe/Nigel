# Nigel
The Nigel programming language
/work in progress.../

## What is it?
Nigel is a new programming language for the 8051 microcontroller, with a c-like syntax.

## Goals
* Complete compiler from source code to hex files.
* Resolve all possible expressions
* Functions (of course with a stack)
* Controll flow (if, while, etc.)
* Pointers (including dynamic heap)
* arrays
* string literals?
* Handling interrupts/timer and other 8051-specific stuff
* Intern/extern RAM
* 16+ bit types
* Compiling of multiple files (with a linker).
* Preprocessor (and header files)
* Standard library
* Local variables in sub-blocks
* Compile the compiler on linux (clang/gcc)
* Optimizing code?

## How does it work?
The Compiler is splitted in these parts:
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
|     EIR Parser     |
+--------------------+
          |
          v
         EIR
          |
          v
+--------------------+
|       Linker       |
+--------------------+
          |
          v
       HEX code
```
The resulting Hex code can be burned onto a 8051 microcontroller or be interpreted in a simulator.

## Current state
- [x] Preprocessor
- [x] Compiler structure
- [x] Lexer
- [x] AST Parser
- [x] EIR Parser
- [x] Linker

### Next development steps
* Resolving of arithmetic terms + '()'
* Blocks
* Controll flow
* 16+ types
* ptr
* 'Magic addresses' of the microcontroller (ports, etc.)

## Installation
1. Install Visual Studio (tested with VC 2017).
2. Download the repository.
3. Install the boost library (http://www.boost.org/) and add it to the project dependencies.
4. Compile and hope for the best.

## Who uses Nigel?
Hmm... maybe you?
