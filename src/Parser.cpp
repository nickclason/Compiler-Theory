//
// Created by Nick Clason on 2/1/21.
//

#include "../include/Parser.h"

// TODO:
//      resync???????                   -
//      Might need a larger error class -


Parser::Parser(token_t *tokenPtr, Scanner *scannerPtr, SymbolTable* symbolTablePtr)
{
    token = tokenPtr; // TODO: Might be able to remove tokenPtr parameter with some future changes
    scanner = scannerPtr;
    symbolTable = symbolTablePtr;
    errorFlag = false;
    errorCount = 0;
    warningCount = 0;

    // Parsing Starts Here
    Program();

    char errorCh = (errorCount > 1 || errorCount == 0) ? 's' : '\0';
    char warningCh = (warningCount > 1 || warningCount == 0) ? 's' : '\0';
    printf("\nParsing Completed with %d error%c and %d warning%c", errorCount, errorCh, warningCount, warningCh);
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

    if (!ProgramHeader())
    {
        ReportError("Program header expected"); // TODO: Change msg
    }

    if (!ProgramBody())
    {
        ReportError("Program body expected"); // TODO: Change msg
    }

    // TODO: Treat leaving of the period as a warning and not an error. May change later
//    token = scanner->GetToken();
    if (!ValidateToken(T_PERIOD))
    {
        ReportWarning("Expected '.' at the end of program"); // TODO: Change msg
    }

//    token = scanner->GetToken();
    if (ValidateToken(T_EOF))
    {
        // TODO: Exit scope, I dont think i really need to do anything explicitly to exit this scope?
        //       We do have a ChangeScope() method if needed.
        return;
    }
    else
    {
        ReportError("Some tokens remain when end of program was expected"); // TODO: Change msg
    }
}

bool Parser::ProgramHeader()
{
    // TODO: Rewrite/Cleanup/Optimize
//    token = scanner->GetToken();
    if (ValidateToken(T_PROGRAM))
    {
        std::string ident;
        if (IsIdentifier(ident))
        {
//            token = scanner->GetToken();
            if (ValidateToken(T_IS))
            {
                return true;
            }
            else
            {
                ReportError("Expected 'is' after program identifier"); // TODO: Change msg
                return false;
            }
        }
        else
        {
            ReportError("Expected identifier after 'program"); // TODO: Change msg
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
//            token = scanner->GetToken();
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

        //token = scanner->GetToken();
        if (ValidateToken(T_BEGIN))
        {
            while (true)
            {
                while (IsStatement())
                {
//                    token = scanner->GetToken();
                    if (!ValidateToken(T_SEMICOLON))
                    {
                        ReportError("Expected ';' after expression in program body"); // TODO: Change msg
                    }
                }

                // Get end of program body
//                token = scanner->GetToken();
                if (ValidateToken(T_END))
                {
//                    token = scanner->GetToken();
                    if (ValidateToken(T_PROGRAM))
                    {
                        return true;
                    }
                    else
                    {
                        ReportError("Expected 'end program' to end program execution"); // TODO: Change msg
                    }
                }
                else
                {
                    // TODO: ReportFatalError?
                    ReportError("Could not find another valid statement or end of program"); // TODO: Change msg
                    return false;
                }
            }
        }
        else
        {
            // TODO: ReportFatalError?

            ReportError("Could not find another valid declaration or start of program execution"); // TODO: Change msg
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
        ReportError("Unknown Token Error: " + token->str); // TODO: Change msg
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

    return;
}

void Parser::ReportError(std::string errorMsg)
{
    // TODO:
    //      Implement method
    //      add more detailed info, line, line #, col #, etc.
    errors.push("Error: " + errorMsg);
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
    // TODO:
    //      Implement method
    //      add more detailed info, line, line #, col #, etc.
    errors.push("Warning: " + warningMsg);
    warningCount++;
}

bool Parser::IsIdentifier(std::string &ident)
{
//    token = scanner->GetToken();
    if (ValidateToken(T_IDENTIFIER))
    {
        ident = token->str;
        return true;
    }

    return false;
}

bool Parser::IsDeclaration(bool &isProcedureDec)
{
    bool isGlobal;
    std::string id;
    int type;

//    token = scanner->GetToken();
    ValidateToken(T_GLOBAL) ? isGlobal = true : isGlobal = false;

    if (IsProcedureDeclaration(id, type, isGlobal))
    {
        return true;
    }
    else if (IsVariableDeclaration(id, type, isGlobal))
    {
        symbolTable->AddSymbol(id, type, std::vector<Node>(), isGlobal);
        return true;
    }
    else if (isGlobal)
    {
        ReportError("Bad Line. Expected either a valid procedure or variable declaration of 'global' keyword"); // TODO: Change msg
        return false;
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
    else if (IsProcedureCall(id)) return true;
    else return false;
}

bool Parser::IsProcedureDeclaration(std::string &id, int &type, bool isGlobal)
{
    // TODO: Implement method

    if (IsProcedureHeader(id, type, isGlobal))
    {
        if (IsProcedureBody())
        {
            return true;
        }
        else
        {
            ReportError("Expected procedure body after procedure header"); // TODO: Change msg & make fatalerror
        }
    }

    return false;
}

bool Parser::IsVariableDeclaration(std::string &id, int &type, bool isGlobal)
{
    // TODO: Add array and enum? support

    if (isGlobal)
    {
//        token = scanner->GetToken();
        if (ValidateToken(T_VARIABLE))
        {
//            token = scanner->GetToken();
            if (ValidateToken(T_IDENTIFIER))
            {
                id = token->str;
//                token = scanner->GetToken();
                if (ValidateToken(T_COLON))
                {
//                    token = scanner->GetToken();
                    if (TypeCheck())
                    {
                        type = token->type;
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
        else
        {
            ReportError("Expected 'variable' keyword after global identifier");
        }
    }
    else // i think this could be made else if(T_VARIABLE)
    {
//        token = scanner->GetToken();
        if (ValidateToken(T_VARIABLE))
        {
//            token = scanner->GetToken();
            if (ValidateToken(T_IDENTIFIER))
            {
                id = token->str;
//                token = scanner->GetToken();
                if (ValidateToken(T_COLON))
                {
//                    token = scanner->GetToken();
                    if (TypeCheck())
                    {
                        type = token->type;
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
        else // Not a variable
        {
            return false;
        }
    }

    return false;
}

bool Parser::IsIfStatement()
{
    // TODO: Implement method
    return false;
}

bool Parser::IsLoopStatement()
{
    // TODO: Implement method
    return false;
}

bool Parser::IsReturnStatement()
{
    // TODO: Implement method
    return false;
}

bool Parser::IsAssignmentStatement(std::string &id)
{
    Node n;
    int size, type, checkSize, checkType;
    bool isFound;
    bool isGlobal;
    bool isValidAssignment = IsValidAssignment(id, isFound, isGlobal, n, checkSize, checkType);

    if (!isValidAssignment)
    {
        return false;
    }

    if (ValidateToken(T_ASSIGNMENT))
    {
        IsExpression(size, type);
        if (isFound)
        {
            if (size != checkSize && (size > 1) && (checkSize <= 1))
            {
                ReportError("Invalid assignment, size of expression must match destination size"); // todo: msg
            }
            if ((type != checkType) && ((!IsNumber(type)) || (!IsNumber(checkType))))
            {
                ReportError("Invalid assignment, type of expression must match destination type"); // todo: msg
            }

        }

        return true;
    }
    else
    {
        ReportError("Expected ':=' after '" + id + "' in assignment statement");
        return true; // TODO: Not sure about this
    }
}

bool Parser::IsProcedureCall(std::string &id)
{
    // TODO: Implement method
    return false;
}

bool Parser::IsProcedureHeader(std::string &id, int &type, bool isGlobal)
{
    // TODO: Add error messages and fix all this

    if (ValidateToken(T_PROCEDURE))
    {
        // Add new scope for procedure
        symbolTable->AddScope();
        if (IsIdentifier(id))
        {
//            token = scanner->GetToken();
            if (ValidateToken(T_COLON))
            {
//                token = scanner->GetToken();
                if (TypeCheck())
                {
                    type = token->type;
//                    token = scanner->GetToken();
                    if (ValidateToken(T_LPAREN))
                    {
//                        token = scanner->GetToken();
                        if (ValidateToken(T_RPAREN))
                        {
                            // No parameters
                            symbolTable->AddSymbol(id, type, std::vector<Node>(), isGlobal);
                            return true;
                        }
                        else if (ValidateToken(T_VARIABLE))
                        {
                            // TODO: I think there can only be 1 parameter per procedure, might need to change
                            //       if this is not true

                            //token = scanner->GetToken();
                            std::string varId;
                            if (IsIdentifier(varId))
                            {
//                                token = scanner->GetToken();
                                if (ValidateToken(T_COLON))
                                {
//                                    token = scanner->GetToken();
                                    if (TypeCheck())
                                    {
                                        int varType = token->type;
//                                        token = scanner->GetToken();
                                        if (ValidateToken(T_RPAREN))
                                        {
                                            Node paramNode;
                                            paramNode.id = varId;
                                            paramNode.size = 0;
                                            paramNode.isGlobal = false;
                                            paramNode.type = varType;
                                            paramNode.args = std::vector<Node>();

                                            std::vector<Node> args;
                                            args.push_back(paramNode);
                                            symbolTable->AddSymbol(id, type, args, isGlobal);

                                            return true;
                                        }
                                        else
                                        {
                                            ReportError("Expected ')'");
                                        }
                                    }
                                    else
                                    {
                                        ReportError("Type Expected: Missing or invalid type"); // TODO: Change msg
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
                        else
                        {
                            ReportError("Expected ')'"); // TODO: Change msg
                        }
                    }
                    else
                    {
                        ReportError("Expected ')'"); // TODO: Change msg
                    }
                }
                else
                {
                    ReportError("Expected type");
                }
            }
            else
            {
                ReportError("Expected ':' after identifier");
            }
        }
        else
        {
            ReportError("Identifier expected after 'procedure' keyword");
        }

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
                        ReportError("Expected 'end procedure' at end of procedure declaration"); // TODO: Change msg
                        return true; // TODO: Treat this as a warning?
                    }
                }
                else
                {
                    ReportError("Expected 'end procedure' at the end of procedure declaration");
                    return false;
                }
            }
        }
        else
        {
            ReportError("Could not find valid declaration or 'begin' keyword in procedure body"); // TODO: Modify
            return false;
        }
    }
}

bool Parser::TypeCheck()
{
    if (ValidateToken(T_INTEGER) ||
        ValidateToken(T_FLOAT) ||
        ValidateToken(T_BOOL) ||
        ValidateToken(T_STRING) ||
        ValidateToken(T_ENUM))
    {
        return true;
    }

    return false;
}

bool Parser::IsValidAssignment(std::string &id, bool &isFound, bool &isGlobal, Node &n, int &checkSize, int &checkType)
{
    int size, type;

    if (IsIdentifier(id))
    {
        isFound = symbolTable->DoesSymbolExist(id, isGlobal, n);
        if (isFound && n.type == T_PROCEDURE)
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
            checkSize = n.size;
            checkType = n.type;
        }

        // TODO: Arrays
//        if (ValidateToken(T_LBRACKET)) {}
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
            if ((type == T_INTEGER) && (opType != T_INTEGER)) // TODO: Messages
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
            ReportError("Expected arithmetic op after '|' or '&' "); // todo: improve message
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

    if (!ValidateToken(T_ADD) || !ValidateToken(T_SUBTRACT))
    {
        return false;
    }

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
            return true; // not sure if this should be outside this condition
        }
    }

    return false;
}

bool Parser::IsRelationPrime(int &size, int &type)
{
    int termSize, termType;
    std::string op = token->str; // for code gen

    if (ValidateToken(T_LESSTHAN) || ValidateToken(T_GREATERTHAN) || ValidateToken(T_LTEQ) || ValidateToken(T_GTEQ))
    {
        if (IsTerm(termSize, termType))
        {
            if (((type != T_BOOL) && (type != T_INTEGER)) || ((termType != T_BOOL) && (termType != T_INTEGER)))
            {
                ReportError("Relational ops are only valid for types of bool or integers('0' or '1')"); // TODO: msg
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
            ReportError("Expected term after relational operator"); // todo: msg
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
            ReportError("Only integer and float values are allowed for arithmetic ops in terms"); // TODO: Msg
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
        ReportError("Expected factor after arithmetic operator"); // TODO: Msg
    }

    IsTermPrime(size, type);

    return true;
}

bool Parser::IsFactor(int &size, int &type)
{
    int tempSize, tempType;

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
                ReportError("Expected ')' around expression"); // TODO msg and fatal error
            }
        }
        else
        {
            ReportError("Expected expression in parenthesis"); // TODO: msg / fatal error
        }
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
                ReportError("'-' only valid before integer and floats"); // TODO: msg
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
        type = T_STRING;
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
    Node n;

    if (IsIdentifier(id))
    {
        bool isFound = symbolTable->DoesSymbolExist(id, isGlobal, n);
        if (isFound)
        {
            if (n.type == T_PROCEDURE)
            {
                ReportError(id + " is not a variable in this scope"); // todo: msg
            }
            else
            {
                size = n.size;
                type = n.type;
            }
        }
        else
        {
            ReportError(id + " is not declared in this scope"); // todo: msg
            size = 0;
            type = T_UNKNOWN;
        }

        // TODO: implement array stuff
//        if (ValidateToken(T_LBRACKET)) {}

        return true;
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
    // TODO: Might need to fix these
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
    // TODO: might need to fix these
    std::string str = token->str;

    if (ValidateToken(T_ENUM))
    {
        return true;
    }

    return false;
}