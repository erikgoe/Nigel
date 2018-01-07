cmake_minimum_required (VERSION 3.0)
project (nigel)

add_executable(nigel
    AST_Parser.cpp
    Builder.cpp
    BuilderTask.cpp
    CodeBase.cpp
    IMC_Generator.cpp
    HelperDefinitions.cpp
    Lexer.cpp
    Linker.cpp
    Nigel.cpp
    Preprocessor.cpp
    stdafx.cpp
    Token.cpp
)

target_include_directories(nigel
    PUBLIC
        ${Boost_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(nigel
    ${Boost_LIBRARIES}
)

