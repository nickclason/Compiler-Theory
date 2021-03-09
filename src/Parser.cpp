//
// Created by Nick Clason on 2/1/21.
//

#include "../include/Parser.h"
#include "../include/Scanner.h"





Parser::Parser(std::string fileName, bool debug_, Scanner scanner_, SymbolTable symbolTable_, token_t *token_)
{
    debug = debug_;
    scanner = scanner_;
    symbolTable = symbolTable_;
    token = token_;

    stopParse = false;
    errorFlag = false;

    procedureCount = 0;

    llvmModule = nullptr;
    llvmBuilder = nullptr;
    llvmCurrProc = nullptr;

    // TODO: start parsing
    Program();
    if (debug)
    {
        llvmModule->print(llvm::errs(), nullptr);
    }
}

Parser::~Parser()
{
    if (llvmBuilder != nullptr)
    {
        delete llvmBuilder;
    }

    if (llvmCurrProc != nullptr)
    {
        delete llvmCurrProc;
    }

    if (llvmModule != nullptr)
    {
        delete llvmModule;
    }
}

void Parser::Program()
{
    symbolTable.AddScope();
    ProgramHeader();
    ProgramBody();
}

void Parser::ProgramHeader()
{
    PrintDebugInfo("<program_header>");

    if (!ValidateToken(T_PROGRAM))
    {
        YieldMissingTokenError("PROGRAM", *token);
        return;
    }

    std::string id = Identifier();

    llvmModule = new llvm::Module(id, llvmContext);
    llvmBuilder = new llvm::IRBuilder<>(llvmContext);

    // TODO: insert runtime functions

    if (!ValidateToken(T_IS))
    {
        YieldMissingTokenError("IS", *token);
        return;
    }

    return;
}

void Parser::ProgramBody()
{
    PrintDebugInfo("<program_body>");

    // Get all declarations
    WhileDeclarations();

    if (!ValidateToken(T_BEGIN))
    {
        YieldMissingTokenError("BEGIN", *token);
        return;
    }

    std::vector<llvm::Type *> parameters;
    llvm::FunctionType *functionType = llvm::FunctionType::get(llvmBuilder->getInt32Ty(), parameters, false);

    llvm::FunctionCallee mainCallee = llvmModule->getOrInsertFunction("main", functionType);
    llvm::Constant *main = llvm::dyn_cast<llvm::Constant>(mainCallee.getCallee());

    llvmCurrProc = llvm::cast<llvm::Function>(main);
    llvmCurrProc->setCallingConv(llvm::CallingConv::C);

    llvm::BasicBlock *entrypoint = llvm::BasicBlock::Create(llvmContext, "entrypoint", llvmCurrProc);
    llvmBuilder->SetInsertPoint(entrypoint);

    for (auto &it : symbolTable.GetLocalScope())
    {
        if (it.second.GetDeclarationType() != T_VARIABLE || it.second.IsGlobal())
        {
            continue;
        }

        llvm::Value *address = nullptr;
        // array
        address = llvmBuilder->CreateAlloca(GetLLVMType(it.second));
        it.second.SetLLVMAddress(address);

        symbolTable.AddSymbol(it.second); // Update symbol table
    }



}

bool Parser::ValidateToken(int tokenType)
{

    // Ignore comments
    token_t* tempToken = scanner.PeekToken();
    while (tempToken->type == T_COMMENT)
    {
        token = scanner.GetToken();
        tempToken = scanner.PeekToken();
    }

    if (tempToken->type == tokenType)
    {
        token = scanner.GetToken();
        return true;
    }
    else if (tempToken->type == T_UNKNOWN)
    {
        std::string errMsg = "Unknown token was found: " + token->str;
        YieldError(errMsg, *token);
        return false;
    }
    else if (tempToken->type == T_EOF)
    {
        return false;
    }

    return false;
}

void Parser::PrintDebugInfo(std::string langID)
{
    if (debug)
    {
        std::cout << langID << std::endl;
    }
}

void Parser::YieldError(token_t token)
{
    if (errorFlag) return;

    std::cout << "Line: " << token.line << " Col: " << token.col << std::endl << std::endl;
    errorFlag = true;
}

void Parser::YieldError(std::string msg, token_t token)
{
    if (errorFlag) return;

    std::cout << "Line: " << token.line << " Col: " << token.col << " : " << msg << std::endl;
    errorFlag = true;
}

void Parser::YieldMissingTokenError(std::string expected, token_t token)
{
    if (errorFlag) return;

    std::cout << "Line: " << token.line << " Col: " << token.col << " : " << "Expected token " << expected << std::endl;
    errorFlag = true;
}

void Parser::YieldTypeMismatchError(std::string expected, std::string actual, token_t token)
{
    if (errorFlag) return;

    std::cout << "Line: " << token.line << " Col: " << token.col << " : " << std::endl;
    std::cout << "Expected Type: " << expected << std::endl;
    std::cout << "Found Type: " << actual << std::endl;
    errorFlag = true;
}

void Parser::YieldOpTypeCheckError(std::string op, std::string type1, std::string type2, token_t token)
{
    if (errorFlag) return;

    std::cout << "Line:" << token.line << " Col:" << token.col << " - ";
    std::cout << "Incompatible types for " << op << ":" << std::endl;
    std::cout << "Type 1: " << type1 << std::endl;
    std::cout << "Type 2: " << type2 << std::endl << std::endl;
    errorFlag = true;
}

void Parser::YieldWarning(std::string msg, token_t token)
{
    if (errorFlag) return;
    std::cout << "Line:" << token.line << " Col:" << token.col << " : ";
    std::cout << "Warning:" << std::endl << msg << std::endl << std::endl;
}

std::string Parser::Identifier()
{
    PrintDebugInfo("<identifier>");

    if (!ValidateToken(T_IDENTIFIER))
    {
        YieldError("Identifier expected", *token);
        return std::string();
    }

    return token->val.stringValue;
}

void Parser::WhileDeclarations()
{
    bool continue_ = false;
    while (!continue_)
    {
        Declaration();

        if (errorFlag)
        {
            // TODO: resync
            return;
        }
        else
        {
            if (!ValidateToken(T_SEMICOLON))
            {
                YieldMissingTokenError(";", *token);
                return;
            }
        }

        token = scanner.PeekToken();
        if (token->type == T_BEGIN)
        {
            continue_ = true;
        }
    }
}

void Parser::Declaration()
{
    PrintDebugInfo("<declaration>");

    Symbol symbol;

    token = scanner.PeekToken();
    if (token->type == T_GLOBAL)
    {
        ValidateToken(T_GLOBAL);
        symbol.SetIsGlobal(true);
        token = scanner.PeekToken();
    }

    if (token->type == T_PROCEDURE)
    {
        //ProcedureDeclaration(symbol);
    }
    else if (token->type == T_VARIABLE)
    {
        VariableDeclaration(symbol);
    }
    else
    {
        YieldError("Error parsing declaration, expected a procedure or variable", *token);
        return;
    }
}

void Parser::VariableDeclaration(Symbol &variable)
{
    PrintDebugInfo("<variable_declaration>");

    variable.SetDeclarationType(T_VARIABLE);

    if (!ValidateToken(T_VARIABLE))
    {
        YieldMissingTokenError("VARIABLE", *token);
        return;
    }

    std::string id = Identifier();
    variable.SetId(id);

    if (!ValidateToken(T_COLON))
    {
        YieldMissingTokenError(":", *token);
    }

    TypeMark(variable);

    // TODO: array

    if (variable.IsGlobal())
    {
        llvm::Type *globalTy = nullptr;

        // array

        globalTy = GetLLVMType(variable);
        llvm::Constant *constInit = llvm::Constant::getNullValue(globalTy);
        llvm::Value *val = new llvm::GlobalVariable(*llvmModule, globalTy, false, llvm::GlobalValue::CommonLinkage, constInit, variable.GetId());
        variable.SetIsInitialized(true);

        // array
        variable.SetLLVMAddress(val);
    }

    if (symbolTable.DoesSymbolExist(variable.GetId()))
    {
        YieldError("Identifier already exists", *token);
        return;
    }

    symbolTable.AddSymbol(variable);
}

void Parser::TypeMark(Symbol &symbol)
{
    PrintDebugInfo("<type_mark>");

    if (ValidateToken(T_INTEGER))
    {
        symbol.SetType(T_INTEGER);
        return;
    }
    else if (ValidateToken(T_FLOAT))
    {
        symbol.SetType(T_FLOAT);
        return;
    }
    else if (ValidateToken(T_BOOL))
    {
        symbol.SetType(T_BOOL);
        return;
    }
    else if (ValidateToken(T_STRING))
    {
        symbol.SetType(T_STRING);
        return;
    }
    else
    {
        YieldError("Expected int, float, bool, or string type", *token);
        return;
    }

}

llvm::Type *Parser::GetLLVMType(Symbol symbol) {
    int type = symbol.GetType();

    switch (type)
    {
        case T_BOOL:
            return llvmBuilder->getInt1Ty();
        case T_INTEGER:
            return llvmBuilder->getInt32Ty();
        case T_FLOAT:
            return llvmBuilder->getFloatTy();
        case T_STRING:
            return llvmBuilder->getInt8PtrTy();
        default:
            std::cout << "Unknown Type" << std::endl;
            return llvm::IntegerType::getInt1Ty(llvmModule->getContext());
    }
}
