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
    token = scanner->GetToken();
    if (!ValidateToken(T_PERIOD))
    {
        ReportWarning("Expected '.' at the end of program"); // TODO: Change msg
    }

    token = scanner->GetToken();
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
    token = scanner->GetToken();
    if (ValidateToken(T_PROGRAM))
    {
        std::string ident;
        if (IsIdentifier(ident))
        {
            token = scanner->GetToken();
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
            token = scanner->GetToken();
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
                    token = scanner->GetToken();
                    if (!ValidateToken(T_SEMICOLON))
                    {
                        ReportError("Expected ';' after expression in program body"); // TODO: Change msg
                    }
                }

                // Get end of program body
                token = scanner->GetToken();
                if (ValidateToken(T_END))
                {
                    token = scanner->GetToken();
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
    // TODO:
    //      rewrite using a switch on token->type. might not work actually

    // Ignore comments
    while (token->type == T_COMMENT)
    {
        token = scanner->GetToken();
    }

    if (token->type == tokenType)
    {
        line.append(" " + token->str);
        return true;
    }
    else if (token->type == T_UNKNOWN)
    {
        ReportError("Unknown Token Error: " + token->str); // TODO: Change msg
        return ValidateToken(tokenType);
    }
    else if (token->type == T_EOF)
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
    token = scanner->GetToken();
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

    token = scanner->GetToken();
    ValidateToken(T_GLOBAL) ? isGlobal = true : isGlobal = false;

    if (IsProcedureDeclaration(id, type, isGlobal))
    {
        // TODO: add new scope? and symbol?
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
    // TODO: Implement method
    //       ifStatement, loopStatement, returnStatement, assignment, procCall (all bool methods)

    return false;
}

bool Parser::IsProcedureDeclaration(std::string &id, int &type, bool isGlobal)
{
    // TODO: Implement method

    if (IsProcedureHeader(id, type, isGlobal))
    {
        // temporary
        return true;
    }

    return false;
}

bool Parser::IsVariableDeclaration(std::string &id, int &type, bool isGlobal)
{
    // TODO: Add array and enum? support

    if (isGlobal)
    {
        token = scanner->GetToken();
        if (ValidateToken(T_VARIABLE))
        {
            token = scanner->GetToken();
            if (ValidateToken(T_IDENTIFIER))
            {
                id = token->str;
                token = scanner->GetToken();
                if (ValidateToken(T_COLON))
                {
                    token = scanner->GetToken();
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
        token = scanner->GetToken();
        if (ValidateToken(T_VARIABLE))
        {
            token = scanner->GetToken();
            if (ValidateToken(T_IDENTIFIER))
            {
                id = token->str;
                token = scanner->GetToken();
                if (ValidateToken(T_COLON))
                {
                    token = scanner->GetToken();
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

bool Parser::IsAssignmentStatement()
{
    // TODO: Implement method
    return false;
}

bool Parser::IsProcedureCall()
{
    // TODO: Implement method
    return false;
}

bool Parser::IsProcedureHeader(std::string &id, int &type, bool isGlobal)
{
    // TODO: Add error messages and fix all this
    if (isGlobal)
    {
        token = scanner->GetToken();
    }

    if (ValidateToken(T_PROCEDURE))
    {
        // Add new scope for procedure
        symbolTable->AddScope();
        if (IsIdentifier(id))
        {
            token = scanner->GetToken();
            if (ValidateToken(T_COLON))
            {
                token = scanner->GetToken();
                if (TypeCheck())
                {
                    type = token->type;
                    token = scanner->GetToken();
                    if (ValidateToken(T_LPAREN))
                    {
                        token = scanner->GetToken();
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
                                token = scanner->GetToken();
                                if (ValidateToken(T_COLON))
                                {
                                    token = scanner->GetToken();
                                    if (TypeCheck())
                                    {
                                        int varType = token->type;
                                        token = scanner->GetToken();
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
                                            symbolTable->AddSymbol(varId, type, args, isGlobal);

                                            return true;
                                        }

                                    }
                                }
                            }

                        }
                        else
                        {
                            ReportError("Expected ')'"); // TODO: Change msg
                        }
                    }
                }
            }
        }

    }

    return false;
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