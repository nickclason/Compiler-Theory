all: compiler

compiler: main.o parser.o scanner.o symbolTable.o symbol.o
	clang++ -o compiler main.o parser.o scanner.o symbolTable.o symbol.o `llvm-config --cxxflags --ldflags --system-libs --libs all`

main.o: src/main.cpp include/Parser.h include/definitions.h
	clang++ -c src/main.cpp -o main.o `llvm-config --cxxflags --ldflags --system-libs --libs all`

parser.o: src/Parser.cpp include/Parser.h include/Scanner.h include/definitions.h include/SymbolTable.h include/Symbol.h
	clang++ -c src/Parser.cpp -o parser.o `llvm-config --cxxflags --ldflags --system-libs --libs all`

scanner.o: src/Scanner.cpp include/Scanner.h include/definitions.h
	clang++ -c src/Scanner.cpp -o scanner.o `llvm-config --cxxflags --ldflags --system-libs --libs all`

symbolTable.o: src/SymbolTable.cpp include/SymbolTable.h include/definitions.h
	clang++ -c src/SymbolTable.cpp -o symbolTable.o `llvm-config --cxxflags --ldflags --system-libs --libs all`

symbol.o: src/Symbol.cpp include/Symbol.h include/definitions.h
	clang++ -c src/Symbol.cpp -o symbol.o `llvm-config --cxxflags --ldflags --system-libs --libs all`

