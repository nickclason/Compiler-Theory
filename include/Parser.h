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

        int errorCount;
        int warningCount;
        bool errorFlag;
        std::string line;
        std::queue<std::string> errors;

        // TODO:
        //      Add to error functions as needed
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
        bool IsProcedureDeclaration(std::string &id, int &type, bool isGlobal);
        bool IsVariableDeclaration(std::string &id, int &type, bool isGlobal);

        bool IsStatement();
        bool IsIfStatement();
        bool IsLoopStatement();
        bool IsReturnStatement();
        bool IsAssignmentStatement();
        bool IsProcedureCall();
        bool IsProcedureHeader(std::string &id, int &type, bool isGlobal);
        bool IsProcedureBody();
        bool TypeCheck();

};



#endif //COMPILER_THEORY_PARSER_H
