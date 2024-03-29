# Compiler
Compiler for EECE 5183/6083
- - - -
## Requirements
* llvm 
* clang/clang++
* make
- - - -

## Setup Environment
In have tested this project on both Mac OS and Ubuntu 20.04.

To set up the correct environment based on a default Linux installation, the following steps were taken:
1. sudo apt install clang llvm
2. sudo apt install make

This should handle all dependencies.
- - - -
## Build Instructions
```
make
```
- - - -
## How to use
```
./compiler <file_name>

Then to link the runtime with the generated code run: 

clang output.o src/runtime.c -lm

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

### Features
* Track errors and reports the number of errors with detailed error messages including line and column numbers.
* Track warnings and reports the number of warnings with line and column number of warning location.

### Design Notes
* Symbol table/scoping is accomplished via std::map. There are 2 "scopes" (global/local). The global
scope is a single std::map, and the local scopes is constructed with a std::vector\<std::map>.

* As a result of the way I designed scope management, the built-in get/put IO functions CAN be overridden
by the user. This is due to how I track scope and could likely be easily fixed, but given the time constraint
  I do not want to risk breaking anything else. But as noted in the class Teams discussion this is acceptable.
  
* Everything is handled through the Parser class. It has an instance of token_t, Scanner, and SymbolTable.
Once constructed, the parser will begin parsing/scanning, and take care of everything from there.

### General Notes
* Important note: when running ./compiler, I have encountered an error message 
  
```
free(): double free detected in tcache 2
Aborted (core dumped)  
```
However I have not ran into any issues and have not been able to track down the source of this. There error is a result
of something trying to be freed that already has been freed. Regardless, the IR and .o file are generated and should be
fine to link with the runtime. For purposes of compiling and running, this error can be ignored.
* Resyncronization is attempted in 2 places, in \<declaration> and \<statement>. If the parser successfully
recovers, parsing will attempt to continue. In the cases of [test1.src](testPgms/incorrect/test1.src) and
  [test1b.src](testPgms/incorrect/test1b.src), the parser is able to recover and continue parsing, and the
  IR generated is usable and runs with no issues. This is definitely something I could improve, and I think if I better
  understood the purpose of trying to resync, I would be able to do this in a better manner. Currently though I don't
  really see the point in trying to recover from these errors.
  
* Leaving off the '.' after 'end program' will issue a warning, but the code will be considered valid
and will be compiled and can still be ran.


