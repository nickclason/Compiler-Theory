# Compiler
Compiler for EECE 5183/6083

## Requirements
```
clang++
gcc
llvm 11.1.0
make
```

## Build Instructions
```
make
```

## How to use
```
./output/compiler (you will then be prompted to enter a file name)

Then to link the runtime with the generated code run: 

gcc output/output.o src/runtime.c 

Run the executable:
./a.out 
```