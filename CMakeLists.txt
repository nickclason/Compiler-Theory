cmake_minimum_required(VERSION 3.17)
project(Compiler_Theory)

set(CMAKE_CXX_STANDARD 14)

add_executable(Compiler_Theory src/main.cpp src/Scanner.cpp include/Scanner.h src/Parser.cpp include/Parser.h include/definitions.h src/SymbolTable.cpp include/SymbolTable.h)