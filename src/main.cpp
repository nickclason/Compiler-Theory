//
// Created by Nick Clason on 1/21/21.
//

#include "../include/definitions.h"
#include "../include/Parser.h"

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

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "No file found" << std::endl;
        return 0;
    }

    std::string fileName = argv[1];
    Scanner scanner;
    SymbolTable symbolTable;

    // Initialize Scanner and get first token
    scanner.InitScanner(fileName);
    token_t *token = new token_t();


    Parser p(scanner, symbolTable, token);

    return 0;
}