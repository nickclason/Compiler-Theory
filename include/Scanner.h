//
// Created by Nick Clason on 1/21/21.
//

#ifndef COMPILER_THEORY_SCANNER_H
#define COMPILER_THEORY_SCANNER_H


#include "definitions.h"

#include <cstdio>
#include <map>

class Scanner
{

public:

    // Constructor and Destructor
    //
    Scanner();
    ~Scanner();

    bool InitScanner(std::string fileName);
    token_t* GetToken();

private:
    FILE* filePtr;
    int lineCount; // starts at 1
    int colCount; // starts at 1
    int prevColCount;
    std::map<std::string, int> reservedTable;

    std::map<std::string, int> GenerateReservedTable();
    int ScanOneToken(FILE* filePtr, token_t* token);
    bool isNum(char c);
    bool isAlpha(char c);
    bool isSingleToken(char c);
    bool isSpace(char c);
    char ScanNextChar();
    void UndoScan(char c);
};

#endif //COMPILER_THEORY_SCANNER_H
