//
// Created by Nick Clason on 2/1/21.
//

#ifndef COMPILER_THEORY_PARSER_H
#define COMPILER_THEORY_PARSER_H

#include "../include/definitions.h"
#include "../include/Scanner.h"
#include "../include/Symbol.h"
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
        bool TypeCheck(int &type);
        bool IsValidAssignment(std::string &id, bool &isFound, bool &isGlobal, Symbol &symbol, int &checkSize, int &checkType);


        bool IsIdentifier(std::string &id);


        bool IsDeclaration(bool &isProcedureDec);
        bool IsVariableDeclaration(Symbol &symbol, bool isGlobal);
        bool IsTypeDelaration(Symbol &symbol, bool isGlobal);
        bool IsProcedureDeclaration(Symbol &symbol, bool isGlobal);
        bool IsProcedureHeader(Symbol &symbol, bool isGlobal);
        bool IsProcedureBody();
        bool IsParameter(Symbol &symbol);
        bool IsParameterList(Symbol &symbol);


        bool IsStatement();
        bool IsIfStatement();
        bool IsLoopStatement();
        bool IsReturnStatement();
        bool IsAssignmentStatement(std::string &id);


        bool IsExpression(int &size, int &type);
        bool IsExpressionPrime(int &size, int &type);

        bool IsArithOp(int &size, int &type);
        bool IsArithOpPrime(int &size, int &type);

        bool IsRelation(int &size, int &type);
        bool IsRelationPrime(int &size, int &type);

        bool IsTerm(int &size, int &type);
        bool IsTermPrime(int &size, int &type);

        bool IsFactor(int &size, int &type);

        bool IsProcedureCall(std::string &id, int &size, int &type);
        bool IsArgumentList(std::vector<Symbol> &parameterList, Symbol &procedureCall);



        bool IsNumber(int &type);
        bool IsName(int &size, int &type);
        bool IsInt();
        bool IsFloat();
        bool IsString();
        bool IsBool();
        bool IsEnum();


        void AddIOFunctions();


};



#endif //COMPILER_THEORY_PARSER_H
