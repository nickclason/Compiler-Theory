//
// Created by Nick Clason on 2/1/21.
//

#include "../include/Parser.h"

// TODO:
//      Add error flag
//      Error/Warning counters

Parser::Parser(token_t *tokenPtr, Scanner *scannerPtr, SymbolTable* symbolTablePtr)
{
    token = tokenPtr;
    scanner = scannerPtr;
    symbolTable = symbolTablePtr;

    // TODO: Implement functionality as needed
    // Parsing Starts Here
    Program();

    DisplayAllErrors();
}

Parser::~Parser() {
    token = nullptr;
    scanner = nullptr;
    symbolTable = nullptr;
}

void Parser::Program()
{
    // TODO: Start new scope

    if (!ProgramHeader())
    {
        ReportError("Program header expected"); // TODO: Change msg
    }

    if (!ProgramBody())
    {
        ReportError("Program body expected"); // TODO: Change msg
    }

    // TODO: Treat leaving of the period as a warning and not an error. May change later
    if (!ValidateToken(T_PERIOD))
    {
        ReportWarning("Expected '.' at the end of program"); // TODO: Change msg
    }

    if (ValidateToken(T_EOF))
    {
        // TODO: Exit scope
    }
    else
    {
        ReportError("Some tokens remain when end of program was expected"); // TODO: Change msg
    }
}

bool Parser::ProgramHeader()
{
    // TODO: Implement Method

    if (ValidateToken(T_PROGRAM))
    {
        std::string ident;

        if (IsIdentifier(ident))
        {
            // TODO: some way to manage scope?
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
    bool isProcedureDec = false;
    while (true)
    {
        while (IsDeclaration(isProcedureDec))
        {
            if (isProcedureDec)
            {
                if (!ValidateToken(T_SEMICOLON))
                {
                    // TODO: Warning or error? That is the question
                    ReportWarning("Expected ';' after procedure declaration");
                }
                else if (!ValidateToken(T_SEMICOLON))
                {
                    // TODO: Warning or error? That is the question
                    //       ReportLineError?
                    ReportError("Expected ';' after variable declaration");
                }
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
                        ReportError("Expected ';' after expression in program body"); // TODO: Change msg
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
    //      Implement method

    // Ignore comments
    while (token->type == T_COMMENT)
    {
        token = scanner->GetToken();
    }

    if (token->type == tokenType)
    {
        line.append(" " + token->str);
        token = scanner->GetToken();
        return true;
    }
    else if (token->type == T_UNKNOWN)
    {
        ReportError("Unknown Token Error: " + token->str); // TODO: Change msg
        token = scanner->GetToken();
        return ValidateToken(tokenType);
    }
    else if (token->type == T_EOF)
    {
        return false;
    }

    return false;
}

void Parser::DisplayAllErrors()
{
    // TODO:
    //      Implement method

    if (!errors.empty())
    {
        std::cout << "\nErrors and Warnings\n" << std::endl;
    }
    else
    {
        return;
    }

    while (!errors.empty())
    {
        std::cout << errors.front() << "\n" << std::endl;
        errors.pop();
    }

    return;
}

void Parser::ReportError(std::string errorMsg)
{
    // TODO:
    //      Implement method
    //      add more detailed info, line, line #, col #, etc.
    errors.push("Error: " + errorMsg);
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
}

bool Parser::IsIdentifier(std::string &ident) {
    // TODO: Implement Method
    std::string temp = token->str;
    if (ValidateToken(T_IDENTIFIER))
    {
        ident = temp;
        return true;
    }

    return false;
}

bool Parser::IsDeclaration(bool &isProcedureDec)
{
    // TODO: Need to store symbols somehow
    bool isGlobal;
    ValidateToken(T_GLOBAL) ? isGlobal = true : isGlobal = false;

    // TODO: Implement functionality
    if (IsProcedureDeclaration())
    {
        // scope stuff
    }
    else if (IsVariableDeclaration())
    {
        // scope stuff
    }
    else if (isGlobal)
    {
        ReportError("Bad Line. Expected either a valid procedure or variable declaration of 'global' keyword"); // TODO: Change msg
    }

    return false;
}

bool Parser::IsStatement()
{
    // TODO: Implement method
    //       ifStatement, loopStatement, returnStatement, assignment, procCall (all bool methods)

    return false;
}

bool Parser::IsProcedureDeclaration()
{
    // TODO: Implement method
    return false;
}

bool Parser::IsVariableDeclaration()
{
    // TODO: Implement method
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
