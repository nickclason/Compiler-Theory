# Compiler
Compiler for EECE 5183/6083
- - - -
## Requirements

* clang++
* gcc
* llvm 11.1.0
* make
- - - -
## Build Instructions
```
make
```
- - - -
## How to use
```
./output/compiler <file_name>

Then to link the runtime with the generated code run: 

gcc output/output.o src/runtime.c 

Run the executable:
./a.out 
```
- - - -
## Documentation
### Introduction
I've chosen to use C/C++ as it is the language I am most comfortable with and felt that it provided the
best set of features to utilize for writing my compiler. Particularly the LLVM C++ API for creating an
in-memory representation of the LLVM IR was the main reason for my language choice.

### About the parser
This is a single pass, LL(1) recursive descent parser, which is meant to parse the language defined in
[projectLanugage.pdf](projectLanguage.pdf). There are 2 resync points to attempt to recover from a
parsing error.

### Interesting Features
* Tracks unused variables