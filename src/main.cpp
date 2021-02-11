//
// Created by Nick Clason on 1/21/21.
//

#include "../include/definitions.h"
#include "../include/Parser.h"
#include "../include/Scanner.h"
#include "../include/SymbolTable.h"

#include <iostream>

std::string GetFileName()
{
    std::string fileName;
    std::cout << "Enter file name: ";
    std::cin >> fileName;

    return fileName;
}

void ScannerTest(std::string fileName)
{
    Scanner* s = new Scanner();
    s->InitScanner(fileName);

    token_t* token = new token_t();

    while (token->type != T_EOF)
    {
        token = s->GetToken();
        std::cout << "<" << token->type << ", " << token->str << ">, Line: " << token->line << " Col: " << token->col << std::endl;
    }
}

void PeekTest(std::string fileName)
{
    Scanner* s = new Scanner();
    s->InitScanner(fileName);

    token_t* token = new token_t();

    int i = 1;
    while (token->type != T_EOF)
    {
        if (i % 2 == 0)
        {
            token = s->PeekToken();
        }
        else
        {
            token = s->GetToken();
        }

        std::cout << "<" << token->type << ", " << token->str << ">, Line: " << token->line << " Col: " << token->col << std::endl;
        i++;
    }
}
/*
void SymbolTableTest()
{
    SymbolTable* symbolTable = new SymbolTable();

    Symbol s = {T_INTEGER, T_VARIABLE, 0, true, "X", std::vector<Symbol>()};
    Symbol s1 = {T_INTEGER, T_PROCEDURE, 0, true, "myProc", std::vector<Symbol>()};
    Symbol s2 = {T_BOOL, T_VARIABLE, 0, false, "isValid", std::vector<Symbol>()};
    Symbol s3 = {T_BOOL, T_VARIABLE, 0, false, "isValid", std::vector<Symbol>()};

    symbolTable->AddScope();
    symbolTable->AddSymbol(s);
    symbolTable->AddSymbol(s1);

    symbolTable->AddScope();
    symbolTable->AddSymbol(s3);

    symbolTable->AddScope();
    symbolTable->AddSymbol(s2);

    std::cout << "Printing all scopes: " << std::endl;
    symbolTable->PrintScopes();

    std::cout << "Printing global scope: " << std::endl;
    symbolTable->PrintScope(0);
}
*/
//void ParserTest(std::string fileName)
//{
//}



int main()
{
    std::string fileName = GetFileName();
    token_t* tokenPtr = new token_t();
    Scanner* scannerPtr = new Scanner();
    SymbolTable* symbolTablePtr = new SymbolTable();

    // Initialize Scanner and get first token
    scannerPtr->InitScanner(fileName);

    Parser* p = new Parser(tokenPtr, scannerPtr, symbolTablePtr);

    return 0;
}