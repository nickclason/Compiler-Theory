//
// Created by Nick Clason on 2/1/21.
//

#ifndef COMPILER_THEORY_PARSER_H
#define COMPILER_THEORY_PARSER_H

#include "../include/definitions.h"
#include "../include/Scanner.h"
#include "../include/SymbolTable.h"

#include <queue>

class Parser {
public:
    token_t* token;
    Scanner* scanner;
    SymbolTable* symbolTable;

    // Constructor & Destructor
    Parser(token_t* tokenPtr, Scanner* scannerPtr, SymbolTable* symbolTablePtr);
    ~Parser();

private:

    std::string line;

    // TODO:
    //      Add to error functions as needed
    std::queue<std::string> errors;
    void DisplayAllErrors();
    void ReportError(std::string errorMsg);
    void ReportTokenError(std::string errorMsg);
    void ReportExpectedTypeError(std::string errorMsg);
    void ReportTypeCheckError(std::string errorMsg);
    void ReportWarning(std::string warningMsg);


    void Program();
    bool ProgramHeader();
    bool ProgramBody();
    bool ValidateToken(int tokenType);
    bool IsIdentifier(std::string &ident);

    bool IsDeclaration(bool &isProcedureDec);
    bool IsProcedureDeclaration();
    bool IsVariableDeclaration();

    bool IsStatement();
    bool IsIfStatement();
    bool IsLoopStatement();
    bool IsReturnStatement();
    bool IsAssignmentStatement();
    bool IsProcedureCall();


};



#endif //COMPILER_THEORY_PARSER_H
