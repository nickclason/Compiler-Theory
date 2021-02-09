all: compiler

compiler: main.o parser.o scanner.o symbolTable.o
	g++ -o compiler main.o parser.o scanner.o symbolTable.o

main.o: src/main.cpp include/Scanner.h include/Parser.h include/definitions.h include/SymbolTable.h
	g++ -c src/main.cpp

parser.o: src/Parser.cpp include/Parser.h include/Scanner.h include/definitions.h include/SymbolTable.h
	g++ -c src/Parser.cpp

scanner.o: src/Scanner.cpp include/Scanner.h include/definitions.h
	g++ -c src/Scanner.cpp

symbolTable.o: src/SymbolTable.cpp include/SymbolTable.h include/definitions.h
	g++ -c src/SymbolTable.cpp