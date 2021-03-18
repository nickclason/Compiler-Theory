all: compiler

compiler: main.o parser.o scanner.o symbolTable.o symbol.o
	clang++ -o output/compiler output/main.o output/parser.o output/scanner.o output/symbolTable.o output/symbol.o `llvm-config --cxxflags --ldflags --system-libs --libs all`

main.o: src/main.cpp include/Parser.h include/definitions.h
	clang++ -c src/main.cpp -o output/main.o `llvm-config --cxxflags --ldflags --system-libs --libs all`

parser.o: src/Parser.cpp include/Parser.h include/Scanner.h include/definitions.h include/SymbolTable.h include/Symbol.h
	clang++ -c src/Parser.cpp -o output/parser.o `llvm-config --cxxflags --ldflags --system-libs --libs all`

scanner.o: src/Scanner.cpp include/Scanner.h include/definitions.h
	clang++ -c src/Scanner.cpp -o output/scanner.o `llvm-config --cxxflags --ldflags --system-libs --libs all`

symbolTable.o: src/SymbolTable.cpp include/SymbolTable.h include/definitions.h
	clang++ -c src/SymbolTable.cpp -o output/symbolTable.o `llvm-config --cxxflags --ldflags --system-libs --libs all`

symbol.o: src/Symbol.cpp include/Symbol.h include/definitions.h
	clang++ -c src/Symbol.cpp -o output/symbol.o `llvm-config --cxxflags --ldflags --system-libs --libs all`

