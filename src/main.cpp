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

int main()
{
//    std::string fileName = GetFileName();
    std::string fileName = "/Users/nick/Documents/Compiler-Theory/testPgms/correct/test.src";
    Scanner scanner;
    SymbolTable symbolTable;

    // Initialize Scanner and get first token
    scanner.InitScanner(fileName);
    token_t *token = new token_t();

    Parser *p = new Parser(fileName, true, scanner, symbolTable, token);

    return 0;
}