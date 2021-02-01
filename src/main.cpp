//
// Created by Nick Clason on 1/21/21.
//

#include "../include/definitions.h"
#include "../include/Scanner.h"
#include "../include/Parser.h"

#include <iostream>

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

int main() {
    std::string fileName;
    std::cout << "Enter file name: ";
    std::cin >> fileName;

    // Testing Scanner functionality
    ScannerTest(fileName);

    return 0;
}