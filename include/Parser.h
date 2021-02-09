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
        void ReportError(std::string errorMsg); // TODO: make reference or const
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
        bool IsAssignmentStatement(std::string &id);
        bool IsProcedureCall(std::string &id);
        bool IsProcedureHeader(std::string &id, int &type, bool isGlobal);
        bool IsProcedureBody();
        bool TypeCheck();
        bool IsValidAssignment(std::string &id, bool &isFound, bool &isGlobal, Node &n, int &checkSize, int &checkType);
        bool IsExpression(int &size, int &type);
        bool IsExpressionPrime(int &size, int &type);
        bool IsArithOp(int &size, int &type);
        bool IsArithOpPrime(int &size, int &type);
        bool IsRelation(int &size, int &type);
        bool IsRelationPrime(int &size, int &type);
        bool IsTerm(int &size, int &type);
        bool IsTermPrime(int &size, int &type);
        bool IsFactor(int &size, int &type);

        bool IsNumber(int &type);
        bool IsName(int &size, int &type);
        bool IsInt();
        bool IsFloat();
        bool IsString();
        bool IsBool();
        bool IsEnum();


};



#endif //COMPILER_THEORY_PARSER_H
