//
// Created by Nick Clason on 2/1/21.
//

#include "../include/Parser.h"

// TODO:
//      resync???????                   -
//      Might need a larger error class -



Parser::Parser(token_t *tokenPtr, Scanner *scannerPtr, SymbolTable* symbolTablePtr)
{
    token = tokenPtr;
    scanner = scannerPtr;
    symbolTable = symbolTablePtr;
    errorFlag = false;
    errorCount = 0;
    warningCount = 0;

    // Parsing Starts Here
    Program();

    // Display Errors
    char errorCh = (errorCount > 1 || errorCount == 0) ? 's' : '\0';
    char warningCh = (warningCount > 1 || warningCount == 0) ? 's' : '\0';
    printf("\nParsing completed with %d error%c and %d warning%c", errorCount, errorCh, warningCount, warningCh);
    if (!errors.empty())
    {
        DisplayAllErrors();
    }

    if (!errorFlag)
    {
        printf("\nDEBUG: Parse Successful. No serious errors");
    }


}

Parser::~Parser() {
    token = nullptr;
    scanner = nullptr;
    symbolTable = nullptr;
}

void Parser::Program()
{
    // New Scope for program
    symbolTable->AddScope();
    AddIOFunctions(); // TODO: add for code gen

    if (!ProgramHeader())
    {
        ReportError("A program header was expected, but not found");
    }

    if (!ProgramBody())
    {
        ReportError("The program body was expected, but not found");
    }

    // TODO: Treat leaving of the period as a warning and not an error. May change later
    if (!ValidateToken(T_PERIOD))
    {
        ReportWarning("Expected '.' after 'end program'");
    }

    if (ValidateToken(T_EOF))
    {
        symbolTable->ExitScope();
    }
    else // If there is anything after the period, should we just ignore it? (i.e test.src)
    {
        ReportWarning("Some tokens still remain when end of the program was expected");
        symbolTable->ExitScope();
    }
}

bool Parser::ProgramHeader()
{
    if (ValidateToken(T_PROGRAM))
    {
        std::string id;
        if (IsIdentifier(id))
        {
            symbolTable->ChangeScopeName(id);
            if (ValidateToken(T_IS))
            {
                return true;
            }
            else
            {
                ReportError("'is' keyword expected after program identifier");
                return false;
            }
        }
        else
        {
            ReportError("Identifier expected after 'program");
            return false;
        }
    }

    return false;
}

bool Parser::ProgramBody()
{
    // TODO: Maybe do some resync stuff in here?
    bool isProcedureDec = false;
    while (true)
    {
        while (IsDeclaration(isProcedureDec))
        {
            if (isProcedureDec)
            {
                if (!ValidateToken(T_SEMICOLON)) {
                    // TODO: Warning or error? That is the question
                    ReportWarning("Expected ';' after procedure declaration");
                }
            }
            else if (!ValidateToken(T_SEMICOLON))
            {
                // TODO: Warning or error? That is the question
                //       ReportLineError?
                ReportError("Expected ';' after variable declaration");
            }
        }

        if (ValidateToken(T_BEGIN))
        {
            while (true)
            {
                while (IsStatement())
                {
                    if (!ValidateToken(T_SEMICOLON))
                    {
                        ReportError("Expected ';' after statement in program body");
                    }
                }

                // Get end of program body
                if (ValidateToken(T_END))
                {
                    if (ValidateToken(T_PROGRAM))
                    {
                        return true;
                    }
                    else
                    {
                        ReportError("'end program' expected to stop program execution");
                    }
                }
                else
                {
                    // TODO: ReportFatalError?
                    ReportError("Another valid statement or end of program could not be found");
                    return false;
                }
            }
        }
        else
        {
            // TODO: ReportFatalError?
            ReportError("Valid declaration or start of program execution could not be found");
            return false;
        }
    }
}

bool Parser::ValidateToken(int tokenType)
{

    // Ignore comments
    token_t* tempToken = scanner->PeekToken();
    while (tempToken->type == T_COMMENT)
    {
        token = scanner->GetToken();
        tempToken = scanner->PeekToken();
    }

    if (tempToken->type == tokenType)
    {
        token = scanner->GetToken();
        line.append(" " + token->str);
        return true;
    }
    else if (tempToken->type == T_UNKNOWN)
    {
        ReportError("Unknown token was found: " + token->str);
//        return ValidateToken(tokenType);
        return false;
    }
    else if (tempToken->type == T_EOF)
    {
        return false; // This is false because if the token type is EOF and we get here, it means we weren't expecting EOF
    }

    return false;
}

void Parser::DisplayAllErrors()
{
    if (!errors.empty())
    {
        std::cout << "\n\nErrors and Warnings" << std::endl;
    }
    else
    {
        return;
    }

    while (!errors.empty())
    {
        std::cout << "\t" << errors.front() << std::endl;
        errors.pop();
    }
    std::cout << std::endl;

}

void Parser::ReportError(std::string errorMsg)
{
    errors.push("Error: " + errorMsg + ". Line: " + std::to_string(token->line) + " Col: " + std::to_string(token->col));
    errorCount++;
    errorFlag = true;
}

void Parser::ReportTokenError(std::string errorMsg)
{
    // TODO:
    //      Implement method
    std::cout << errorMsg << std::endl;
}

void Parser::ReportExpectedTypeError(std::string errorMsg)
{
    // TODO:
    //      Implement method
    std::cout << errorMsg << std::endl;
}

void Parser::ReportTypeCheckError(std::string errorMsg)
{
    // TODO:
    //      Implement method
    std::cout << errorMsg << std::endl;
}

void Parser::ReportWarning(std::string warningMsg)
{
    errors.push("Warning: " + warningMsg + ". Line: " + std::to_string(token->line) + " Col: " + std::to_string(token->col));
    warningCount++;
}

bool Parser::IsIdentifier(std::string &id)
{
    if (ValidateToken(T_IDENTIFIER))
    {
        id = token->str;
        return true;
    }

    return false;
}

bool Parser::IsDeclaration(bool &isProcedureDec)
{
    bool isGlobal;
    Symbol symbol;
    symbol.parameters.clear();

    ValidateToken(T_GLOBAL) ? isGlobal = true : isGlobal = false;

    if (IsProcedureDeclaration(symbol, isGlobal))
    {
        symbolTable->ExitScope();
        symbolTable->AddSymbol(symbol.id, symbol, isGlobal);
        isProcedureDec = true;
        return true;
    }
    else if (IsVariableDeclaration(symbol, isGlobal))
    {
        symbolTable->AddSymbol(symbol.id, symbol, isGlobal);
        return true;
    }
    else if (isGlobal)
    {
        ReportError("After 'global' keyword, a valid procedure, variable declaration, or type declaration was expected"); // TODO: LineError
        return false;
    }

    return false;
}

bool Parser::IsProcedureDeclaration(Symbol &symbol, bool isGlobal)
{
    if (IsProcedureHeader(symbol, isGlobal))
    {
        if (IsProcedureBody())
        {
            return true;
        }
        else
        {
            ReportError("Procedure body was expected after procedure header"); // TODO: fatalerror
        }
    }

    return false;
}

bool Parser::IsProcedureHeader(Symbol &symbol, bool isGlobal)
{
    std::string id;
    int type;
    int size = 0;

    if (ValidateToken(T_PROCEDURE))
    {
        symbolTable->AddScope(); // Add new scope for the procedure
        if (IsIdentifier(id))
        {
            symbolTable->ChangeScopeName(id);
            if (ValidateToken(T_COLON))
            {
                if (IsTypeMark(type))
                {
                    if (ValidateToken(T_LPAREN))
                    {
                        //Symbol symbol;
                        IsParameterList(symbol);
                        if (!ValidateToken(T_RPAREN))
                        {
                            ReportError("')' was expected after parameter list");
                        }

                        // Any parameters that exist should have alredy been added by IsParameterList()
                        symbol.type = type;
                        symbol.declarationType = T_PROCEDURE;
                        symbol.size = size;
                        symbol.isGlobal = isGlobal;
                        symbol.id = id;
                        symbolTable->AddSymbol(id, symbol, isGlobal); // This is to make recursion possible

                        return true;
                    }
                    else
                    {
                        ReportError("Expected '('");
                        return true;
                    }
                }
                else
                {
                    ReportError("Expected type");
                    return true;
                }
            }
            else
            {
                ReportError("Expected ':' after identifier");
                return true;
            }
        }
        else
        {
            ReportError("Identifier expected after 'procedure' keyword");
            return true;
        }
    }

    return false;
}

bool Parser::IsParameterList(Symbol &symbol)
{
    if (IsParameter(symbol))
    {
        while (ValidateToken(T_COMMA))
        {
            if(!IsParameter(symbol))
            {
                ReportError("After ',' another parameter was expected in parameter list");
            }
        }
    }

    return true;
}

bool Parser::IsParameter(Symbol &symbol)
{
    Symbol newParameter;
    bool isGlobal;

    if (IsVariableDeclaration(newParameter, isGlobal))
    {
        // Add the parameter to the procedures scope and to the procedures symbol
        //
        symbolTable->AddSymbol(newParameter.id, newParameter, false);
        symbol.parameters.push_back(newParameter);

        return true;
    }

    return false;
}

bool Parser::IsProcedureBody()
{
    bool isProcDeclaration = false;

    while (true)
    {
        while (IsDeclaration(isProcDeclaration))
        {
            if (isProcDeclaration)
            {
                if (!ValidateToken(T_SEMICOLON))
                {
                    ReportError("Expected ';' after procedure declaration");
                }
            }
            else if (!ValidateToken(T_SEMICOLON))
            {
                ReportError("Expected ';' after variable declaration");
            }
        }

        if (ValidateToken(T_BEGIN))
        {
            while (true)
            {
                while (IsStatement())
                {
                    if (!ValidateToken(T_SEMICOLON))
                    {
                        ReportError("Expected ';' after statement in procedure");
                    }
                }

                if (ValidateToken(T_END))
                {
                    if (ValidateToken(T_PROCEDURE))
                    {
                        return true;
                    }
                    else
                    {
                        ReportError("'end procedure' was expected after end of procedure declaration");
                        return true; // TODO: Treat this as a warning?
                    }
                }
                else
                {
                    ReportError("'end procedure' was expected at the end of procedure declaration");
                    return false;
                }
            }
        }
        else
        {
            ReportError("Could not find another valid declaration or the 'begin' keyword in the procedure body");
            return false;
        }
    }
}

bool Parser::IsVariableDeclaration(Symbol &symbol, bool isGlobal)
{
    std::string id;
    int type, size;

    if (ValidateToken(T_VARIABLE))
    {
        if (ValidateToken(T_IDENTIFIER))
        {
            id = token->str;
            if (ValidateToken(T_COLON))
            {
                if (IsTypeMark(type))
                {
                    if (ValidateToken(T_LBRACKET))
                    {
                        if (ValidateToken(T_INTEGER) || ValidateToken(T_INT_LITERAL))
                        {
                            size = token->val.intValue;
                            if (!ValidateToken(T_RBRACKET))
                            {
                                ReportError("Expected ']' at end of array declaration");
                            }
                        }
                    }
                    else if (type == T_ENUM)
                    {
                        symbol.id = id;
                        symbol.type = T_ENUM;
                        symbol.declarationType = T_ENUM_DEC;
                        symbol.size = size;
                        symbol.isGlobal = isGlobal;
                        symbol.parameters = std::vector<Symbol>();

                        if (ValidateToken(T_LBRACE))
                        {

                            // TODO: Need a way to assign values to the identifiers within the enum
                            Symbol enumSym;
                            std::string enumIdentifier;

                            IsIdentifier(enumIdentifier);
                            enumSym.id = enumIdentifier;
                            enumSym.type = T_INTEGER;
                            enumSym.declarationType = T_ENUM_DEC;
                            enumSym.size = 0;
                            enumSym.isGlobal = symbol.isGlobal;

                            symbolTable->AddSymbol(enumIdentifier, enumSym, enumSym.isGlobal);

                            while (ValidateToken(T_COMMA))
                            {
                                IsIdentifier(enumIdentifier);
                                enumSym = Symbol();
                                enumSym.id = enumIdentifier;
                                enumSym.isGlobal = symbol.isGlobal;
                                enumSym.type = T_INTEGER;
                                enumSym.declarationType = T_ENUM_DEC;

                                symbolTable->AddSymbol(enumIdentifier, enumSym, enumSym.isGlobal);
                            }

                            if (!ValidateToken(T_RBRACE))
                            {
                                ReportError("Expected }");
//                                return false;
                            }

                            return true;
                        }
                        else
                        {
                            ReportError("Expected '{' after enum type declaration");
//                            return false;
                        }
                    }
                    else
                    {
                        size = 0;
                    }


                    symbol.id = id;
                    symbol.type = type;
                    symbol.declarationType = T_VARIABLE;
                    symbol.size = size;
                    symbol.isGlobal = isGlobal;
                    symbol.parameters = std::vector<Symbol>(); // Variables should not have parameters
                    return true;
                }
                else
                {
                    ReportError("TYPE expected after ':'");
                }
            }
            else
            {
                ReportError("Expected ':' after identifier");
            }
        }
        else
        {
            ReportError("Expected identifier after 'variable' keyword");
        }
    }

    return false;
}

bool Parser::IsStatement()
{
    std::string id = "";
    if (IsIfStatement()) return true;
    else if (IsLoopStatement()) return true;
    else if (IsReturnStatement()) return true;
    else if (IsAssignmentStatement(id)) return true;
    else return false;
}

bool Parser::IsIfStatement()
{
    int size, type;
    bool statementFlag;

    if (!ValidateToken(T_IF))
    {
        return false;
    }

    if (!ValidateToken(T_LPAREN))
    {
        ReportError("'(' is expected before condition in if statement");;
    }
    else if (!IsExpression(size, type))
    {
        ReportError("Condition expected for if statement");
    }
    else if (type != T_BOOL)
    {
        ReportError("Expression must evaluate to bool in if statement condition");
    }
    else if (!ValidateToken(T_RPAREN))
    {
        ReportError("')' expected after condition in if statement");
    }

    if (ValidateToken(T_THEN))
    {
        statementFlag = false;
        while (true)
        {
            while (IsStatement())
            {
                statementFlag = true;
                if (!ValidateToken(T_SEMICOLON))
                {
                    ReportError("Expected ';' after statement in 'if' condition");
                }
            }

            if (!statementFlag)
            {
                ReportError("At least one statement is expected after 'then'");
            }

            if (ValidateToken(T_ELSE))
            {
                statementFlag = false;
                while (true)
                {
                    while (IsStatement())
                    {
                        statementFlag = true;
                        if (!ValidateToken(T_SEMICOLON))
                        {
                            ReportError("Expected ';' after statement in 'else' condition");
                        }
                    }

                    if (ValidateToken(T_END))
                    {
                        if (!statementFlag)
                        {
                            ReportError("At least one statement expected after 'else'");
                        }

                        if (!ValidateToken(T_IF))
                        {
                            ReportError("Missing 'if' in 'end if' closure"); // todo: fatal error
                        }

                        return true;
                    }
                    else
                    {
                        ReportError("Unable to find valid statement"); // todo: fatal error
                        return true;
                    }
                }
            }
            else if (ValidateToken(T_END))
            {
                if (!ValidateToken(T_IF))
                {
                    ReportError("Missing 'if' in 'end if' closure"); //todo: fatal error
                }
                return true;
            }
            else
            {
                ReportError("Unable to find valid statement"); // todo: fatal error
                return true;
            }
        }
    }

    ReportError("'then' expected after condition in if statement"); // TODO: fatal error
}

bool Parser::IsLoopStatement()
{
    int size, type;
    std::string id;

    if (!ValidateToken(T_FOR))
    {
        return false;
    }

    if (!ValidateToken(T_LPAREN))
    {
        ReportError("'(' expected before for loop statement");
    }

    if (!IsAssignmentStatement(id))
    {
        ReportError("An assignment statement was expected after '('");
    }

    if (!ValidateToken(T_SEMICOLON))
    {
        ReportError("';' expected after assignment statement in for loop");
    }

    if (!IsExpression(size, type))
    {
        ReportError("An expression was expected after assignment in for loop");
    }

    if (!ValidateToken(T_RPAREN))
    {
        ReportError("')' expected after expression in for loop");
    }

    // Loop Body
    while (true)
    {
        while (IsStatement())
        {
            if (!ValidateToken(T_SEMICOLON))
            {
                ReportError("Expected ';' after statement");
            }
        }

        if (ValidateToken(T_END))
        {
            if (ValidateToken(T_FOR))
            {
                return true;
            }
            else
            {
                ReportError("'for' is missing in 'end for' loop closure");
            }
        }
        else
        {
            ReportError("'end for' expected at end of for loop");
            return false;
        }
    }
}

bool Parser::IsReturnStatement()
{
    int size, type;
    if (ValidateToken(T_RETURN))
    {
        // TODO: add stuff for code gen
        if (IsExpression(size, type))
        {
            return true;
        }
        return true;
    }

    return false;
}

bool Parser::IsAssignmentStatement(std::string &id)
{
    Symbol symbol;
    int size, type, checkSize, checkType;
    bool isFound;
    bool isGlobal;
    bool isValidAssignment = IsValidAssignment(id, isFound, isGlobal, symbol, checkSize, checkType);

    if (!isValidAssignment)
    {
        return false;
    }

    if (ValidateToken(T_ASSIGNMENT))
    {
        IsExpression(size, type);
        if (isFound)
        {
            if ((size != checkSize) && (size > 1) && (checkSize <= 1))
            {
                ReportError("Invalid assignment: size of expression must match destination size");
            }

            if (type != checkType)
            {
                // 0 is false, all other int values are converted to true
                if ((checkType == T_INTEGER) && ((type == T_BOOL) || type == T_FLOAT))
                {
                    // probably some codegen stuff here
                    // convert bool/float to integers
                    type = T_INTEGER;
                }
                else if((checkType == T_BOOL) && (type == T_INTEGER))
                {
                    // probably some codegen stuff here
                    // convert integers to bool
                    type = T_INTEGER;
                }
                else if ((checkType == T_FLOAT) && (type == T_INTEGER))
                {
                    // code gen stuff
                    // convert integers to floats
                    type = T_FLOAT;
                }
                else if (((type == T_STRING && checkType == T_STRING_LITERAL) || (checkType == T_STRING && type == T_STRING_LITERAL))) // support string comparisons
                {
                    type = T_STRING;
                }
                else
                {
                    ReportError("Type mismatch in expression");
                }
            }
        }

        return true;
    }
    else
    {
        ReportError("Expected ':=' after '" + id + "' in assignment statement");
        return true;
    }
}

bool Parser::IsProcedureCall(std::string &id, int &size, int &type)
{
    std::vector<Symbol> parameterList;
    Symbol procedureCall;
    bool isGlobal;

    // check that id != ""
    // check if symbol exists
    // get argument list (aka parse '(variable x : integer) )'
    // if (found)
    //      compare arguments, make sure sizes/types match
    //      ~ codegen stuff ~
    //      return true
    // else
    //      proc not declared in this scope

    token_t* tempToken = scanner->PeekToken();
    id = tempToken->str;
    if (tempToken->type != T_IDENTIFIER)
    {
        return false;
    }

    if (symbolTable->DoesSymbolExist(id, procedureCall, isGlobal))
    {
        if (procedureCall.declarationType == T_PROCEDURE)
        {
            if (ValidateToken(T_IDENTIFIER));
            else return false;
        }
        else
        {
            // todo: error msg?
            return false;
        }
    }
    else
    {
        ReportError(id + " is not defined in the current scope");
    }

    size = procedureCall.size;
    type = procedureCall.type;

    if (ValidateToken(T_LPAREN))
    {
        IsArgumentList(parameterList, procedureCall);
        if (!ValidateToken(T_RPAREN))
        {
            ReportError("')' was expected following procedure arguments");
        }
        else
        {
            return true; // TODO: ???
        }
    }
    else
    {
        ReportError("'(' was expected following procedure call");
    }

    return false;
}

bool Parser::IsArgumentList(std::vector<Symbol> &parameterList, Symbol &procedureCall)
{
    parameterList.clear(); // Ensure parameterList is empty to begin with

    Symbol param;
    param.parameters.clear();

    std::vector<Symbol>::iterator it = procedureCall.parameters.begin();
    if (it == procedureCall.parameters.end())
    {
        // Not found
        return false;
    }

    if (IsExpression(param.size, param.type))
    {
        // TODO: code gen
        ++it;
        parameterList.push_back(param);
        while (ValidateToken(T_COMMA))
        {
            if (it == procedureCall.parameters.end())
            {
                // Not Found
                return false;
            }

            if (IsExpression(param.size, param.type))
            {
                parameterList.push_back(param);
                // TODO: code gen
                ++it;
            }
            else
            {
                ReportError("Expected an additional argument after ',' in argument list");
            }
        }
    }

    return true;
}

bool Parser::IsTypeMark(int &type)
{
    if (ValidateToken(T_INTEGER) ||
        ValidateToken(T_FLOAT) ||
        ValidateToken(T_BOOL) ||
        ValidateToken(T_STRING))
    {
        type = token->type;
        return true;
    }

    return false;
}

bool Parser::IsValidAssignment(std::string &id, bool &isFound, bool &isGlobal, Symbol &symbol, int &checkSize, int &checkType)
{
    int size, type;

    if (IsIdentifier(id))
    {
        isFound = symbolTable->DoesSymbolExist(id, symbol, isGlobal);
        if (isFound && symbol.declarationType == T_PROCEDURE)
        {
            return false;
        }
        else if (!isFound)
        {
            checkSize = 0;
            checkType = T_UNKNOWN;
            ReportError(id + " was not declared in this scope");
        }
        else
        {
            checkSize = symbol.size;
            checkType = symbol.type;
        }

        if (ValidateToken(T_LBRACKET))
        {
            if (IsExpression(size, type))
            {
                if (size != 0 || ((type != T_FLOAT) && (type != T_INTEGER) && (type != T_BOOL)))
                {
                    ReportError("Index must be a numerical value");
                }
                else
                {
                    checkSize = 0;
                }

                if (ValidateToken(T_RBRACKET))
                {
                    return true;
                }
                else
                {
                    ReportError("Expected numerical expression for array index");
                    return true;
                }
            }
        }

        return true;
    }

    return false;
}

bool Parser::IsExpression(int &size, int &type)
{
    bool isNotOp;
    ValidateToken(T_NOT) ? isNotOp = true : isNotOp = false;

    if (IsArithOp(size, type))
    {
        if ((isNotOp) && (type != T_BOOL) && (type != T_INTEGER))
        {
            ReportError("'NOT' operator is only defined for bool and integer types");
        }
        else if (isNotOp)
        {
            // TODO: code gen?
        }

        IsExpressionPrime(size, type);
        return true;
    }
    else if (isNotOp)
    {
        ReportError("Expected integer/bool arithmetic operation following 'NOT'"); // TODO: FatalError?
        return true;
    }

    return false;
}

bool Parser::IsExpressionPrime(int &size, int &type)
{
    int opSize, opType;
    std::string op = token->str; // for code gen

    if (ValidateToken(T_OR) || ValidateToken(T_AND))
    {
        ValidateToken(T_NOT); // optional
        if (IsArithOp(opSize, opType))
        {
            if ((type == T_INTEGER) && (opType != T_INTEGER))
            {
                ReportError("Only integer arithmetic operators can be used with bitwise '|' and '&'");
            }
            else if ((type == T_BOOL) && (opType != T_BOOL))
            {
                ReportError("Only boolean arithmetic operators can be used with boolean '|' and '&'");
            }
            else
            {
                ReportError("Only integer / boolean operators can be used for bitwise / boolean operators '|' and '&'.");
            }

            if ((size != opSize) && (size != 0) && (opSize != 0))
            {
                ReportError("Expected arithmetic op size: " + std::to_string(size) + ", Found size of: " + std::to_string(opSize));
            }
            else if (opSize !=0 )
            {
                size = opSize;
            }
        }
        else
        {
            ReportError("Expected arithmetic op after '|' or '&' ");
        }

        IsExpression(size, type);

        return true;
    }

    return false;
}

bool Parser::IsArithOp(int &size, int &type)
{
    if (IsRelation(size, type))
    {
        IsArithOpPrime(size, type);
        return true;
    }

    return false;
}

bool Parser::IsArithOpPrime(int &size, int &type)
{
    int relSize;
    int relType;

    std::string op = token->str; // for code gen

    if (ValidateToken(T_ADD));
    else if (ValidateToken(T_SUBTRACT));
    else return false;

    if (IsRelation(relSize, relType))
    {
        if (!IsNumber(relType) || !IsNumber(type))
        {
            ReportError("Value must be integer or float for arithmetic operations");
        }

        if ((size != relSize) && (size != 0) && (relSize != 0))
        {
            ReportError("Relation size expected: " + std::to_string(size) + ", Size Found: " + std::to_string(relSize));
        }
        else if (relSize != 0)
        {
            size = relSize;
        }
    }
    else
    {
        ReportError("Expected a relation after arithmetic operator");
    }

    IsArithOpPrime(size, type);

    return true;
}

bool Parser::IsRelation(int &size, int &type)
{
    if (IsTerm(size, type))
    {
        if (IsRelationPrime(size, type))
        {
            type = T_BOOL;
        }
        return true;
    }

    return false;
}

bool Parser::IsRelationPrime(int &size, int &type)
{
    int termSize, termType, opType;
    std::string op = token->str; // for code gen

    if (ValidateToken(T_LESSTHAN) || ValidateToken(T_GREATERTHAN) || ValidateToken(T_LTEQ) || ValidateToken(T_GTEQ) || ValidateToken(T_EQEQ))
    {
        op = token->str;
        opType = token->type;
        if (IsTerm(termSize, termType))
        {
            if (((type == T_STRING || type == T_STRING_LITERAL) && (termType == T_STRING || termType == T_STRING_LITERAL)))
            {
                // '==' is the only support relational operator for strings
                if (opType != T_EQEQ)
                {
                    ReportError("Relational operator '" + op + "' is not supported for the string type");
                }
            }
            else if (((type != T_BOOL) && (type != T_INTEGER)) || ((termType != T_BOOL) && (termType != T_INTEGER)))
            {
                ReportError("Relational ops are only valid for types of bool or integers('0' or '1')");
            }

            if ((size != termSize) && (size != 0) && (termSize != 0))
            {
                ReportError("Expected term size: " + std::to_string(size) + ", Found size:  " + std::to_string(termSize));
            }
            else if (termSize != 0)
            {
                size = termSize;
            }
        }
        else
        {
            ReportError("Term expected after relational operator");
        }

        IsRelation(size, type);

        return true;
    }

    return false;
}

bool Parser::IsTerm(int &size, int &type)
{
    if (IsFactor(size, type))
    {
        IsTermPrime(size, type);
        return true;
    }

    return false;
}

bool Parser::IsTermPrime(int &size, int &type)
{
    int factorSize, factorType;
    std::string op = token->str; // for code gen

    if (!ValidateToken(T_MULTIPLY) || !ValidateToken(T_DIVIDE))
    {
        return false;
    }

    if (IsFactor(factorSize, factorType))
    {
        if (!IsNumber(type) || !IsNumber(factorType))
        {
            ReportError("Integer and float values are the only allowed types for arithmetic ops in terms");
        }

        if ((size != factorSize) && ((size !=0 ) && (factorSize != 0)))
        {
            ReportError("Expected factor size: " + std::to_string(size) + ", Found size: " + std::to_string(factorSize));
        }
        else if (factorSize != 0)
        {
            size = factorSize;
        }
    }
    else
    {
        ReportError("Factor expected after arithmetic operator");
    }

    IsTermPrime(size, type);

    return true;
}

bool Parser::IsFactor(int &size, int &type)
{
    int tempSize, tempType;
    std::string id;

    if (ValidateToken(T_LPAREN))
    {
        if (IsExpression(tempSize, tempType))
        {
            size = tempSize;
            type = tempType;
            if (ValidateToken(T_RPAREN))
            {
                return true;
            }
            else
            {
                ReportError("Expected ')' around expression");
            }
        }
        else
        {
            ReportError("Expected expression in parenthesis");
        }
    }
    else if (IsProcedureCall(id, tempSize, tempType))
    {
        size = tempSize;
        type = tempType;
        return true;
    }
    else if (ValidateToken(T_SUBTRACT))
    {
        if (IsInt())
        {
            type = T_INTEGER;
            size = 0;
            return true;
        }
        else if (IsFloat())
        {
            type = T_FLOAT;
            size = 0;
            return true;
        }
        else if (IsName(tempSize, tempType))
        {
            size = tempSize;
            type = tempType;

            if (!IsNumber(type))
            {
                ReportError("'-' is only valid before integer and floats");
            }

            return true;
        }

        return false;
    }
    else if (IsName(tempSize, tempType))
    {
        size = tempSize;
        type = tempType;
        return true;
    }
    else if (IsInt())
    {
        size = 0;
        type = T_INTEGER;
        return true;
    }
    else if (IsFloat())
    {
        size = 0;
        type = T_FLOAT;
        return true;
    }
    else if (IsBool())
    {
        size = 0;
        type = T_BOOL;
        return true;
    }
    else if (IsString())
    {
        size = 0;
//        type = T_STRING;
        type = token->type;
        return true;
    }
    else if (IsEnum())
    {
        size = 0;
        type = T_ENUM;
        return true;
    }

    return false;
}

bool Parser::IsNumber(int &type)
{
    if ((type == T_INTEGER) || (type == T_FLOAT))
    {
        return true;
    }

    return false;
}

bool Parser::IsName(int &size, int &type)
{
    std::string id;
    bool isGlobal;
    Symbol symbol;

    if (IsIdentifier(id))
    {
        bool isFound = symbolTable->DoesSymbolExist(id, symbol, isGlobal);
        if (isFound)
        {
            if (symbol.declarationType == T_PROCEDURE)
            {
                ReportError(id + " is not declared as a variable in this scope");
            }
            else
            {
                size = symbol.size;
                type = symbol.type;
            }
        }
        else
        {
            ReportError(id + " is not declared in this scope");
            size = 0;
            type = T_UNKNOWN;
        }

        if (ValidateToken(T_LBRACKET))
        {
            if (symbol.size == 0 && symbol.declarationType != T_PROCEDURE)
            {
                ReportError(id + " is not an array");
            }

            int arrSize, arrType;
            if (IsExpression(arrSize, arrType))
            {
                if ((arrSize > 1) || ((arrType != T_INTEGER) && (arrType != T_FLOAT) && (arrType != T_BOOL)))
                {
                    ReportError("Array index must be a numerical value");
                }

                size = 0;
                if (ValidateToken(T_RBRACKET))
                {
                    // code gen stuff
                    return true;
                }
                else
                {
                    ReportError("Expected ']' after expression");
                }
            }
            else
            {
                ReportError("An expression was expected between '[ ]'");
            }

        }
        else
        {
            return true;
        }
    }

    return false;
}

bool Parser::IsInt()
{
    std::string str = token->str;

    if (ValidateToken(T_INT_LITERAL))
    {
        return true;
    }

    return false;
}

bool Parser::IsFloat()
{
    std::string str = token->str;

    if (ValidateToken(T_FLOAT_LITERAL))
    {
        return true;
    }

    return false;
}

bool Parser::IsString()
{
    std::string str = token->str;

    if (ValidateToken(T_STRING_LITERAL))
    {
        return true;
    }

    return false;
}

bool Parser::IsBool()
{
    std::string str = token->str;

    if (ValidateToken(T_TRUE))
    {
        return true;
    }

    if (ValidateToken(T_FALSE))
    {
        return true;
    }

    return false;
}

bool Parser::IsEnum()
{
    std::string str = token->str;

    if (ValidateToken(T_ENUM))
    {
        return true;
    }

    return false;
}

void Parser::AddIOFunctions()
{

//    int type;               // int, float, bool, enum, string
//    int declarationType;    // variable declaration, procedure declaration, or user type declaration
//    int size;               // size of arrays, 0 non-arrays
//    bool isGlobal;          // True if this symbol is in the global scope; false otherwise
//
//    std::string id;
//    std::vector<Symbol> parameters; // For procedures

    int type = T_BOOL;
    int decType = T_PROCEDURE;
    int size = 0;
    bool isGlobal = true;
    std::string id = "GETBOOL";
    std::vector<Symbol> params;
    params.clear();

    Symbol *s = new Symbol(type, decType, size, isGlobal, id, params);
    symbolTable->AddSymbol(id, *s, isGlobal);

    type = T_INTEGER;
    id = "GETINTEGER";
    Symbol *s2 = new Symbol(type, decType, size, isGlobal, id, params);
    symbolTable->AddSymbol(id, *s2, isGlobal);

    type = T_FLOAT;
    id = "GETFLOAT";
    Symbol *s3 = new Symbol(type, decType, size, isGlobal, id, params);
    symbolTable->AddSymbol(id, *s3, isGlobal);

    type = T_STRING;
    id = "GETSTRING";
    Symbol *s4 = new Symbol(type, decType, size, isGlobal, id, params);
    symbolTable->AddSymbol(id, *s4, isGlobal);

    type = T_BOOL;
    id = "PUTBOOL";
    int pType = T_BOOL;
    int pDecType = T_VARIABLE;
    bool pIsGlobal = false;
    std::string pId = "VALUE";
    Symbol *p = new Symbol(pType, pDecType, size, pIsGlobal, pId, std::vector<Symbol>());
    params.push_back(*p);
    Symbol *s5 = new Symbol(type, decType, size, isGlobal, id, params);
    symbolTable->AddSymbol(id, *s5, isGlobal);

    type = T_BOOL;
    id = "PUTINTEGER";
    pType = T_INTEGER;
    pDecType = T_VARIABLE;
    pIsGlobal = false;
    pId = "VALUE";
    Symbol *p2 = new Symbol(pType, pDecType, size, pIsGlobal, pId, std::vector<Symbol>());
    params.clear();
    params.push_back(*p2);
    Symbol *s6 = new Symbol(type, decType, size, isGlobal, id, params);
    symbolTable->AddSymbol(id, *s6, isGlobal);

    type = T_BOOL;
    id = "PUTFLOAT";
    pType = T_FLOAT;
    pDecType = T_VARIABLE;
    pIsGlobal = false;
    pId = "VALUE";
    Symbol *p3 = new Symbol(pType, pDecType, size, pIsGlobal, pId, std::vector<Symbol>());
    params.clear();
    params.push_back(*p3);
    Symbol *s7 = new Symbol(type, decType, size, isGlobal, id, params);
    symbolTable->AddSymbol(id, *s7, isGlobal);

    type = T_BOOL;
    id = "PUTSTRING";
    pType = T_STRING;
    pDecType = T_VARIABLE;
    pIsGlobal = false;
    pId = "VALUE";
    Symbol *p4 = new Symbol(pType, pDecType, size, pIsGlobal, pId, std::vector<Symbol>());
    params.clear();
    params.push_back(*p4);
    Symbol *s8 = new Symbol(type, decType, size, isGlobal, id, params);
    symbolTable->AddSymbol(id, *s8, isGlobal);

    type = T_FLOAT;
    id = "SQRT";
    pType = T_INTEGER;
    pDecType = T_VARIABLE;
    pIsGlobal = false;
    pId = "VALUE";
    Symbol *p5 = new Symbol(pType, pDecType, size, pIsGlobal, pId, std::vector<Symbol>());
    params.clear();
    params.push_back(*p5);
    Symbol *s9 = new Symbol(type, decType, size, isGlobal, id, params);
    symbolTable->AddSymbol(id, *s9, isGlobal);
}

