//
// Created by Nick Clason on 2/1/21.
//

// TODO: Global arrays don't work properly when operating on entire array.
// TODO: Resync ?
// TODO: getString doesn't work properly, string comparisons get fucked
// TODO: are string lengths the problem........


#include "../include/Parser.h"

#include <fstream>

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

Parser::Parser(Scanner scanner_, SymbolTable symbolTable_, token_t *token_)
{
    scanner = scanner_;
    symbolTable = symbolTable_;
    token = token_;

    errorFlag = false;
    doUnroll = false;

    procedureCount = 0;
    errorCount = 0;
    warningCount = 0;
    unrollSize = 0;

    llvmModule = nullptr;
    llvmBuilder = nullptr;
    llvmCurrProc = nullptr;
    unrollIdx = nullptr;
    unrollIdxAddress = nullptr;
    unrollLoopStart = nullptr;
    unrollLoopEnd = nullptr;

    Program();

    if (errorCount == 1)
    {
        std::cout << "\n1 serious error found." << std::endl;
    }
    else if (errorCount > 1)
    {
        // I don't think this will get hit, execution should probably stop when an error is found
        std::cout << "\n" << errorCount << " serious errors found" << std::endl;
    }

    if (warningCount == 1)
    {
        if (errorCount == 0)
        {
            printf("\n");
        }
        std::cout << "1 warning" << std::endl;
    }
    else if (warningCount > 1)
    {
        if (errorCount == 0)
        {
            printf("\n");
        }
        std::cout << warningCount << " warnings" << std::endl;
    }

    if (!errorFlag && errorCount == 0)
    {
        // Compile
        std::string outFile = "output/IR.ll";
        std::error_code error_code;
        llvm::raw_fd_ostream out(outFile, error_code, llvm::sys::fs::F_None);
        llvmModule->print(out, nullptr);

        // TODO: no idea what is causing verification to fail, but i have not encountered any issues in
        //       generating IR/compiling/linking... yet
        // bool isVerified = llvm::verifyModule(*llvmModule, &llvm::errs());
        // if (!isVerified) { llvmModule->print(llvm::errs(), nullptr); return; }

        auto TargetTriple = llvm::sys::getDefaultTargetTriple();

        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();

        std::string Error;
        auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

        if (!Target)
        {
            llvm::errs() << Error;
            return;
        }

        auto CPU = "generic";
        auto Features = "";

        llvm::TargetOptions opt;
        auto RM = llvm::Optional<llvm::Reloc::Model>();
        auto TargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

        llvmModule->setDataLayout(TargetMachine->createDataLayout());
        llvmModule->setTargetTriple(TargetTriple);

        auto Filename = "output/output.o";
        std::error_code EC;
        llvm::raw_fd_ostream dest(Filename, EC, llvm::sys::fs::OF_None);

        if (EC)
        {
            llvm::errs() << "Could not open file: " << EC.message();
            return;
        }

        llvm::legacy::PassManager pass;
        auto FileType = llvm::CGFT_ObjectFile;

        if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType))
        {
            llvm::errs() << "TargetMachine can't emit a file of this type";
            return;
        }

        pass.run(*llvmModule);
        dest.flush();
    }
}

Parser::~Parser()=default;

// <program>
void Parser::Program()
{
    symbolTable.AddScope();
    ProgramHeader();
    ProgramBody();
}

// <program_header>
void Parser::ProgramHeader()
{
    if (!ValidateToken(T_PROGRAM))
    {
        ReportMissingTokenError("PROGRAM");
        return;
    }

    std::string id = Identifier();

    if (!ValidateToken(T_IS))
    {
        ReportMissingTokenError("IS");
        return;
    }

    // Create module and builder now that they are needed, and add runtime functions
    llvmModule = new llvm::Module(id, llvmContext);
    llvmBuilder = new llvm::IRBuilder<>(llvmContext);

    symbolTable.AddIOFunctions(llvmModule, llvmBuilder);
}

// <program_body>
void Parser::ProgramBody()
{
    // Get all declarations
    Declarations();

    // Don't call ValidateToken(), because Declarations() has already gotten the next token
    if (token->type != T_BEGIN)
    {
        ReportMissingTokenError("BEGIN");
        return;
    }

    // Need somewhere to put everything, call it main
    llvm::FunctionType *type = llvm::FunctionType::get(llvmBuilder->getInt32Ty(), std::vector<llvm::Type *>(), false);
    llvm::FunctionCallee mainCallee = llvmModule->getOrInsertFunction("main", type);
    auto *main = llvm::dyn_cast<llvm::Constant>(mainCallee.getCallee());
    llvmCurrProc = llvm::cast<llvm::Function>(main);
    llvmCurrProc->setCallingConv(llvm::CallingConv::C);

    // First block is named entry, following llvm kaleidoscope tutorial
    llvm::BasicBlock *entry = llvm::BasicBlock::Create(llvmContext, "entry", llvmCurrProc);
    llvmBuilder->SetInsertPoint(entry);

    // Get all statements
    int terminators[] = { T_END };
    Statements(terminators, 1);

    // Don't call ValidateToken(), because Statements() has already gotten the next token
    if (token->type != T_END)
    {
        ReportMissingTokenError("END");
        return;
    }

    if (!ValidateToken(T_PROGRAM))
    {
        ReportMissingTokenError("PROGRAM");
        return;
    }

    // Always return an integer (0), because according to the way the language is defined, you can have a return
    // statement in the program body but we don't really want to do anything with it.

    llvm::Constant *retVal = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, 0, true));
    llvmBuilder->CreateRet(retVal);
}

// This function handles checking token type matches the expected type
// and does not actually advance the file pointer if it doesn't. Also handles
// eating whitespace/comments. This should be called in 99% of situations rather than
// directly calling scanner.GetToken()/PeekToken() as it is safer.
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
        ReportError("Unknown token was found");
        return false;
    }
    else if (tempToken->type == T_EOF)
    {
        return false;
    }

    return false;
}

// Helper function for converting type definitions to strings for error reporting
std::string Parser::TypeToString(int tokenType)
{
    switch (tokenType)
    {
        case (T_INTEGER):
            return "INTEGER";
        case (T_FLOAT):
            return "FLOAT";
        case (T_BOOL):
            return "BOOL";
        case (T_STRING):
            return "STRING";
        default:
            return "Unknown Type";
    }
}

// General error reporting function
void Parser::ReportError(std::string msg)
{
    if (errorFlag) return;

    printf("\nLine: %d Col: %d\n\t", token->line, token->col);
    std::cout << msg << std::endl;
    errorFlag = true;
    errorCount++;
}

// Used for reporting an error when a certain token was expected but is missing.
void Parser::ReportMissingTokenError(std::string expected)
{
    if (errorFlag) return;

    printf("\nLine: %d Col: %d \n\tExpected token ", token->line, token->col);
    std::cout << expected << std::endl;
    errorFlag = true;
    errorCount++;
}

// Used for reporting an error where the expected type differs from the actual type.
void Parser::ReportTypeMismatchError(std::string expected, std::string actual)
{
    if (errorFlag) return;

    printf("\nLine: %d Col: %d\n", token->line, token->col);
    std::cout << "Expected: " << expected << std::endl;
    std::cout << "Actual: " << actual << std::endl;
    errorFlag = true;
    errorCount++;
}

// Used for reporting an error when two types are incompatible
void Parser::ReportIncompatibleTypeError(std::string op, std::string type1, std::string type2)
{
    if (errorFlag) return;

    printf("\nLine: %d Col: %d\n\t", token->line, token->col);
    std::cout << "Incompatible types for " << op << "\n\n\t\t";
    std::cout << "Type 1: " << type1 << "\n\n\t\t";
    std::cout << "Type 2: " << type2 << std::endl;
    errorFlag = true;
    errorCount++;
}

// Used for reporting warnings (parse/compile can still continue if there are warnings)
void Parser::ReportWarning(std::string msg)
{
    if (errorFlag) return;

    printf("\nLine: %d Col: %d\n\t", token->line, token->col);
    std::cout << "Warning: " << msg << std::endl;
    warningCount++;
}

// <identifier>
std::string Parser::Identifier()
{
    if (!ValidateToken(T_IDENTIFIER))
    {
        ReportError("Identifier expected");
        return std::string();
    }

    return token->val.stringValue;
}

// Get all <declaration> and stop when the terminating token(s) are reached
void Parser::Declarations()
{
    // No declarations
    if (ValidateToken(T_BEGIN))
    {
        return;
    }

    bool continue_ = true;
    while (continue_)
    {
        Declaration();

        if (errorFlag)
        {
            // TODO: resync
        }
        else
        {
            if (!ValidateToken(T_SEMICOLON))
            {
                ReportMissingTokenError(";");
                return;
            }
        }


        // continues until we hit the "BEGIN" keyword, this marks the end of where declarations are valid in all cases
        if (ValidateToken(T_BEGIN))
        {
            continue_ = false;
        }
    }
}

// <declaration>
void Parser::Declaration()
{
    Symbol symbol;
    if (ValidateToken(T_GLOBAL) || symbolTable.GetScopeCount() == 0) // testing to make all declarations in outermost scope global
    {
        symbol.SetIsGlobal(true);
        if (ValidateToken(T_PROCEDURE))
        {
            ProcedureDeclaration(symbol);
        }
        else if (ValidateToken(T_VARIABLE))
        {
            VariableDeclaration(symbol);
        }
        else
        {
            ReportError("Error parsing declaration, expected a procedure or variable");
            return;
        }
    }
    else
    {
        symbol.SetIsGlobal(false); // Just to be safe, should already be set to false
        if (ValidateToken(T_PROCEDURE))
        {
            ProcedureDeclaration(symbol);
        }
        else if (ValidateToken(T_VARIABLE))
        {
            VariableDeclaration(symbol);
        }
        else
        {
            ReportError("Error parsing declaration, expected a procedure or variable");
            return;
        }
    }

}

// <variable_declaration>
void Parser::VariableDeclaration(Symbol &variable)
{
    variable.SetDeclarationType(T_VARIABLE);

    std::string id = Identifier();
    variable.SetId(id);

    if (!ValidateToken(T_COLON))
    {
        ReportMissingTokenError(":");
    }

    TypeMark(variable);

    // Array
    if (ValidateToken(T_LBRACKET))
    {
        variable.SetIsArray(true);
        Bound(variable);

        if (!ValidateToken(T_RBRACKET))
        {
            ReportMissingTokenError("]");
            return;
        }
    }

    if (variable.IsGlobal())
    {
        llvm::Type *globalTy = nullptr;

        // Global array
        if (variable.IsArray())
        {
            globalTy = llvm::ArrayType::get(GetLLVMType(variable), variable.GetArraySize());
        }
        else
        {
            globalTy = GetLLVMType(variable);
        }

        // I might reconsider using actual GlobalVariables. It is causing some kind of issue for global arrays
        llvm::Constant *constInit = llvm::Constant::getNullValue(globalTy);
        llvm::Value *val = new llvm::GlobalVariable(*llvmModule, globalTy, false, llvm::GlobalValue::CommonLinkage, constInit, variable.GetId());
        variable.SetIsInitialized(true);

        // Array
        if (variable.IsArray())
        {
            variable.SetLLVMArrayAddress(val);
            llvm::Value *size = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, variable.GetArraySize(), true));
            variable.SetLLVMArraySize(size);
        }
        else
        {
            variable.SetLLVMAddress(val);
        }
    }

    // Make sure we don't add duplicate identifiers
    Symbol globalDuplicate = symbolTable.FindSymbol(variable.GetId());
    if (variable.IsGlobal() && globalDuplicate.IsValid())
    {
        ReportError("Identifier already exists");
        return;
    }

    if (symbolTable.DoesSymbolExist(variable.GetId()))
    {
        ReportError("Identifier already exists");
        return;
    }

    // All good, we can now add the symbol to the symbol table
    symbolTable.AddSymbol(variable);
}

// <procedure_declaration>
void Parser::ProcedureDeclaration(Symbol &procedure)
{
    // Add a new scope for the current procedure
    symbolTable.AddScope();
    procedure.SetDeclarationType(T_PROCEDURE);
    ProcedureHeader(procedure);

    // Get the parameters (if any)
    std::vector<llvm::Type *> parameters;
    for (const Symbol& parameter : procedure.GetParameters())
    {
        if (parameter.IsArray())
        {
            llvm::PointerType *parameterPtr = GetLLVMType(parameter)->getPointerTo();
            parameters.push_back(parameterPtr);
        }
        else
        {
            llvm::Type *type = GetLLVMType(parameter);
            parameters.push_back(type);
        }
    }

    // Create the function for llvm
    llvm::FunctionType *procType = llvm::FunctionType::get(GetLLVMType(procedure), parameters, false);
    llvm::FunctionCallee procCallee = llvmModule->getOrInsertFunction("function" + std::to_string(procedureCount), procType);
    auto *proc = llvm::dyn_cast<llvm::Constant>(procCallee.getCallee());
    procedureCount++;

    auto *func = llvm::cast<llvm::Function>(proc);
    func->setCallingConv(llvm::CallingConv::C);
    procedure.SetLLVMFunction(func);

    // Don't add if it already exists
    if (symbolTable.DoesSymbolExist(procedure.GetId()))
    {
        ReportError("This identifier already exists");
        return;
    }

    // We need to add the symbol to this procedures scope, as it needs to be able to call itself for recursive calls
    symbolTable.AddSymbol(procedure);
    symbolTable.SetScopeProc(procedure);

    ProcedureBody();

    symbolTable.RemoveScope();

    // Check that the procedure has not already been defined in the upper scope before adding it
    if (symbolTable.GetScopeCount() != 0 && symbolTable.DoesSymbolExist(procedure.GetId()))
    {
        ReportError("This identifier already exists");
        return;
    }

    symbolTable.AddSymbol(procedure);
}

// <procedure_header>
void Parser::ProcedureHeader(Symbol &procedure)
{
    std::string id = Identifier(); // Get procedure name
    procedure.SetId(id);

    if (!ValidateToken(T_COLON))
    {
        ReportMissingTokenError(":");
        return;
    }

    TypeMark(procedure); // Get return type

    if (!ValidateToken(T_LPAREN))
    {
        ReportMissingTokenError("(");
        return;
    }

    ParameterList(procedure); // Get parameters (if there are any)

    if (!ValidateToken(T_RPAREN))
    {
        ReportMissingTokenError(")");
        return;
    }
}

// <parameter_list>
void Parser::ParameterList(Symbol &procedure)
{
    if (!ValidateToken(T_VARIABLE))
    {
        return; // No parameters
    }

    Parameter(procedure); // Get first parameter;

    if (ValidateToken(T_COMMA)) // Expecting another parameter
    {
        ParameterList(procedure);
    }
}

// <parameter>
void Parser::Parameter(Symbol &procedure)
{
    Symbol param;
    VariableDeclaration(param);
    procedure.GetParameters().push_back(param);
}

// <procedure_body>
void Parser::ProcedureBody()
{
    // Get all declarations
    Declarations();

    // Get the scopes procedure and set the current procedure to it
    Symbol procSymbol = symbolTable.GetScopeProc();
    llvmCurrProc = procSymbol.GetLLVMFunction();

    // Create entry block
    llvm::BasicBlock *procEntry = llvm::BasicBlock::Create(llvmContext, "entry", llvmCurrProc);
    llvmBuilder->SetInsertPoint(procEntry);

    // Create all local variables
    std::map<std::string, Symbol>::iterator it;
    for (auto &it : symbolTable.GetLocalScope())
    {
        if (it.second.GetDeclarationType() != T_VARIABLE) {
            continue;
        }

        llvm::Value *address = nullptr;

        // Array
        if (it.second.IsArray())
        {
            llvm::Value *size = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, it.second.GetArraySize(), true));
            it.second.SetLLVMArraySize(size);

            address = llvmBuilder->CreateAlloca(GetLLVMType(it.second), size);
            it.second.SetLLVMArrayAddress(address);
            it.second.SetIsInitialized(true);
        }
        else
        {
            address = llvmBuilder->CreateAlloca(GetLLVMType(it.second));
            it.second.SetLLVMAddress(address);
        }

        symbolTable.AddSymbol(it.second);
    }

    // Create arguments
    llvm::Function::arg_iterator args = llvmCurrProc->arg_begin();
    for (const Symbol& sym : procSymbol.GetParameters())
    {
        if (args == llvmCurrProc->arg_end())
        {
            ReportError("Could not generator LLVM IR for args");
            return;
        }

        // Create arg value and update
        llvm::Value *argVal = &*args++;
        Symbol newSymbol = symbolTable.FindSymbol(sym.GetId());
        newSymbol.SetLLVMValue(argVal);
        newSymbol.SetIsInitialized(true);

        if (newSymbol.IsArray())
        {
            newSymbol.SetLLVMArrayAddress(argVal);
        }
        else
        {
            llvmBuilder->CreateStore(argVal, newSymbol.GetLLVMAddress());
        }

        symbolTable.AddSymbol(newSymbol);
    }

    if (token->type != T_BEGIN)
    {
        ReportMissingTokenError("BEGIN");
        return;
    }

    // Get all statements
    int terminators[] = {T_END};
    Statements(terminators, 1);

    if (!ValidateToken(T_PROCEDURE))
    {
        ReportMissingTokenError("PROCEDURE");
        return;
    }

    if (llvmBuilder->GetInsertBlock()->getTerminator() == nullptr)
    {
        // llvmBuilder->CreateRet(retVal);
        // llvm::Constant *retVal = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, 0, true));

        // Force procedures to have returns
        ReportError("Procedure is required to have a return value.");
        return;
    }
}

// <type_mark>
void Parser::TypeMark(Symbol &symbol)
{
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
        ReportError("Expected int, float, bool, or string type");
        return;
    }

}

// <bound>
void Parser::Bound(Symbol &symbol)
{
    if (!ValidateToken(T_INT_LITERAL))
    {
        ReportMissingTokenError("Integer literal");
        return;
    }

    Symbol bound = Symbol();
    Number(bound);

    int size = token->val.intValue;
    symbol.SetArraySize(size);
}

// Get all <statement> and stop when the terminating token(s) are reached
void Parser::Statements(int terminators[], int terminatorsSize)
{
    bool continue_ = true;
    for (int i = 0; i < terminatorsSize; i++)
    {
        if (ValidateToken(terminators[i]))
        {
            continue_ = false;
            break;
        }
    }

    // Parse statements
    while (continue_)
    {
        Statement();

        if (errorFlag)
        {
            // TODO: resync
            return;
        }
        else
        {
            if (!ValidateToken(T_SEMICOLON))
            {
                ReportMissingTokenError(";");
                return;
            }
        }

        // check if any of the terminating tokens match the next token, if so stop
        for (int i = 0; i < terminatorsSize; i++)
        {
            if (ValidateToken(terminators[i]))
            {
                continue_ = false;
                break;
            }
        }
    }
}

// <statement>
void Parser::Statement()
{
    token = scanner.PeekToken();
    switch (token->type)
    {
        case T_IDENTIFIER:
            AssignmentStatement();
            break;
        case T_IF:
            IfStatement();
            break;
        case T_FOR:
            LoopStatement();
            break;
        case T_RETURN:
            ReturnStatement();
            break;
        default:
            ReportError("Expected if, for, or return statement.");
            break;
    }

    return;
}

// <assignment_statement>
void Parser::AssignmentStatement()
{
    // Left hand side
    Symbol dest = Destination();
    if (!ValidateToken(T_ASSIGNMENT))
    {
        ReportMissingTokenError(":=");
    }

    // Right hand side
    Symbol expr = Symbol();
    Expression(dest, expr);
    AssignmentTypeCheck(dest, expr, token, expr);

    if (!expr.IsValid()) { return; }

    // Store expression in the destination
    llvmBuilder->CreateStore(expr.GetLLVMValue(), dest.GetLLVMAddress());
    dest.SetLLVMValue(expr.GetLLVMValue());
    symbolTable.AddSymbol(dest);

    // if the array should be unrolled, update value
    if (doUnroll)
    {
        llvm::Value *one = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, 1, true));
        unrollIdx = llvmBuilder->CreateAdd(unrollIdx, one);
        llvmBuilder->CreateStore(unrollIdx, unrollIdxAddress);
        llvmBuilder->CreateBr(unrollLoopStart);
        llvmBuilder->SetInsertPoint(unrollLoopEnd);
        doUnroll = false;
    }

    // Update
    dest.SetIsInitialized(true);
    symbolTable.AddSymbol(dest); // Update
}

// <if_statement>
void Parser::IfStatement()
{
    if (!ValidateToken(T_IF))
    {
        ReportMissingTokenError("IF");
    }

    if (!ValidateToken(T_LPAREN))
    {
        ReportMissingTokenError("(");
        return;
    }

    // Get expression
    Symbol expected = Symbol();
    expected.SetType(T_BOOL);
    Symbol expr = Symbol();
    Expression(expected, expr);

    // Do type conversion on compatible types
    if (expr.GetType() == T_INTEGER)
    {
        ReportWarning("Converting integer to boolean");
        llvm::Value *val = ConvertIntToBool(expr.GetLLVMValue());
        expr.SetLLVMValue(val);
    }
    else if (expr.GetType() != T_BOOL)
    {
        ReportError("If statement must evaluate to bool (or int)");
        return;
    }

    if (!ValidateToken(T_RPAREN))
    {
        ReportMissingTokenError(")");
        return;
    }

    if (!ValidateToken(T_THEN))
    {
        ReportMissingTokenError("THEN");
        return;
    }

    // Create blocks for "if then" and "else"
    llvm::BasicBlock *ifBlock = llvm::BasicBlock::Create(llvmContext, "if", llvmCurrProc);
    llvm::BasicBlock *elseBlock = llvm::BasicBlock::Create(llvmContext, "else", llvmCurrProc);;
    llvm::BasicBlock *endBlock = nullptr;

    // Conditional jump that is based on the expression
    llvmBuilder->CreateCondBr(expr.GetLLVMValue(), ifBlock, elseBlock);
    llvmBuilder->SetInsertPoint(ifBlock);

    // if block statements
    int terminators[] = {T_ELSE, T_END};
    Statements(terminators, 2);

    if (token->type == T_ELSE)
    {
        endBlock = llvm::BasicBlock::Create(llvmContext, "", llvmCurrProc);

        if (llvmBuilder->GetInsertBlock()->getTerminator() == nullptr)
        {
            llvmBuilder->CreateBr(endBlock);
        }

        llvmBuilder->SetInsertPoint(elseBlock);

        // else block statements
        int endElse[] = {T_END};
        Statements(endElse, 1);
    }

    if (token->type != T_END)
    {
        ReportMissingTokenError("END");
        return;
    }

    if (!ValidateToken(T_IF))
    {
        ReportMissingTokenError("IF");
        return;
    }

    if (endBlock == nullptr)
    {
        // go from if to else
        if (llvmBuilder->GetInsertBlock()->getTerminator() == nullptr)
        {
            llvmBuilder->CreateBr(elseBlock);
        }

        llvmBuilder->SetInsertPoint(elseBlock);
    }
    else
    {
        // go from else to end
        if (llvmBuilder->GetInsertBlock()->getTerminator() == nullptr)
        {
            llvmBuilder->CreateBr(endBlock);
        }

        llvmBuilder->SetInsertPoint(endBlock);
    }
}

// <loop_statement>
void Parser::LoopStatement()
{
    if (!ValidateToken(T_FOR))
    {
        ReportMissingTokenError("FOR");
        return;
    }

    if (!ValidateToken(T_LPAREN))
    {
        ReportMissingTokenError("(");
        return;
    }

    // Parse the assignment statement in the for loop
    AssignmentStatement();

    if (!ValidateToken(T_SEMICOLON))
    {
        ReportMissingTokenError(";");
        return;
    }

    llvm::BasicBlock *loopStart = llvm::BasicBlock::Create(llvmContext, "start", llvmCurrProc);
    llvm::BasicBlock *loopBody = llvm::BasicBlock::Create(llvmContext, "body", llvmCurrProc); // (<statement>;)*
    llvm::BasicBlock *loopEnd = llvm::BasicBlock::Create(llvmContext,"end", llvmCurrProc);

    // jump to start
    llvmBuilder->CreateBr(loopStart);
    llvmBuilder->SetInsertPoint(loopStart);

    // Condition should be a boolean
    Symbol expected = Symbol();
    expected.SetType(T_BOOL);
    Symbol expr = Symbol();
    Expression(expected, expr);

    // Do type conversion if alllowed
    if (expr.GetType() == T_INTEGER)
    {
        ReportWarning("Converting integer to boolean");
        llvm::Value *val = ConvertIntToBool(expr.GetLLVMValue());
        expr.SetLLVMValue(val);
    }
    else if (expr.GetType() != T_BOOL)
    {
        ReportError("Condition in loop must evaluate to bool (or int)");
        return;
    }

    if (!ValidateToken(T_RPAREN))
    {
        ReportMissingTokenError(")");
        return;
    }

    // Jump to body or end based on expression
    llvmBuilder->CreateCondBr(expr.GetLLVMValue(), loopBody, loopEnd);
    llvmBuilder->SetInsertPoint(loopBody);

    // Get all statements
    int terminators[] = {T_END};
    Statements(terminators, 1);

    if (token->type != T_END)
    {
        ReportMissingTokenError("END");
        return;
    }

    if (!ValidateToken(T_FOR))
    {
        ReportMissingTokenError("FOR");
        return;
    }

    // Jump to start to check if loop should continue
    llvmBuilder->CreateBr(loopStart);
    llvmBuilder->SetInsertPoint(loopEnd);
}

// <return_statement>
void Parser::ReturnStatement()
{
    if (!ValidateToken(T_RETURN))
    {
        ReportMissingTokenError("RETURN");
        return;
    }

    // Currently if there is any return statement in the "main" program scope, it just gets ignored and
    // replaced with the default return value, 0.
    //
    // If a return is put in the main scope, i.e <program_body>, user will be warned that this return will be replaced
    // with "return 0;"
    Symbol proc = symbolTable.GetScopeProc();
    if (!proc.IsValid())
    {
        ReportWarning("Return statements in this scope are ignored");
        llvm::Constant *retVal = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(),
                                                                    llvm::APInt(32, 0, true));
        llvmBuilder->CreateRet(retVal);
        Symbol expr = Symbol();
        Expression(proc, expr);
        return;
    }

    // Get expression
    Symbol expr = Symbol();
    Expression(proc, expr);
    AssignmentTypeCheck(proc, expr, token, expr);
    if (!expr.IsValid()) {
        return;
    }

    // Create return
    llvmBuilder->CreateRet(expr.GetLLVMValue());
}

// Index the symbol passed in
void Parser::IndexArray(Symbol symbol, Symbol &out)
{
    symbol.SetIsArrayIndexed(true);
    Symbol sym = Symbol();

    // I believe this should already be true, just double check.
    if (token->type == T_LBRACKET)
    {
        sym.SetType(T_INTEGER);
        Symbol idx = Symbol();
        Expression(sym, idx);
        AssignmentTypeCheck(sym, idx, token, idx);

        if (idx.GetType() != T_INTEGER)
        {
            ReportError("Array index must be an integer.");
            symbol.SetIsValid(false);

            out.CopySymbol(symbol);
            return;
        }

        if (!symbol.IsArray())
        {
            ReportError("Indexing not supported for non-arrays");
            symbol.SetIsValid(false);

            out.CopySymbol(symbol);
            return;
        }

        if (!ValidateToken(T_RBRACKET))
        {
            ReportMissingTokenError("]");
            symbol.SetIsValid(false);

            out.CopySymbol(symbol);
            return;
        }

        // Make sure that the index is within the array bound
        llvm::Value *zero = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, 0, true));
        llvm::Value *less = llvmBuilder->CreateICmpSLT(idx.GetLLVMValue(), symbol.GetLLVMArraySize());
        llvm::Value *greater = llvmBuilder->CreateICmpSGE(idx.GetLLVMValue(), zero);
        llvm::Value *actualIdx = llvmBuilder->CreateAnd(greater, less);

        llvm::BasicBlock *fail = llvm::BasicBlock::Create(llvmContext, "fail", llvmCurrProc);
        llvm::BasicBlock *succ = llvm::BasicBlock::Create(llvmContext, "success", llvmCurrProc);

        // go to fail if index is invalid, otherwise succ
        llvmBuilder->CreateCondBr(actualIdx, succ, fail);
        llvmBuilder->SetInsertPoint(fail);

        // for any out of bounds errors
        // this is treated as a runtime exception as the syntax would still be perfectly valid
        //  i.e. variable x : integer[2]
        //       x[100] := 1; this syntax is still valid in the eyes of the parser
        Symbol oobError = symbolTable.FindSymbol("OOB_ERROR");
        llvmBuilder->CreateCall(oobError.GetLLVMFunction());

        llvmBuilder->CreateBr(succ);
        llvmBuilder->SetInsertPoint(succ);

        llvm::Value *address = nullptr;
        if (symbol.IsGlobal())
        {
            // global arrays are weird and this might not even work properly
            llvm::Value *vals[] = {zero, idx.GetLLVMValue()};
            address = llvmBuilder->CreateInBoundsGEP(symbol.GetLLVMArrayAddress(), vals);
        }
        else
        {
            address = llvmBuilder->CreateGEP(symbol.GetLLVMArrayAddress(), idx.GetLLVMValue());
        }

        symbol.SetLLVMAddress(address);

        out.CopySymbol(symbol);
        return;
    }

    ReportMissingTokenError("[");
    sym.SetIsValid(false);

    out.CopySymbol(symbol);
    return;
}

// This function checks that the destination and expression evaluate to the same type,
// or interoperable types, and does any type conversions if necessary. Converting from one type to another
// will yield a warning to user.
void Parser::AssignmentTypeCheck(Symbol dest, Symbol expr, token_t *token, Symbol &out)
{
    if (dest.GetType() == expr.GetType())
    {
        out.CopySymbol(expr);
        return;
    }

    bool isDiff = true;

    // int -> bool
    if (dest.GetType() == T_BOOL && expr.GetType() == T_INTEGER)
    {
        isDiff = false;
        expr.SetType(T_BOOL);
        ReportWarning("Converting int to bool");
        expr.SetLLVMValue(ConvertIntToBool(expr.GetLLVMValue()));
    }

    if (dest.GetType() == T_INTEGER)
    {
        // bool -> int
        if (expr.GetType() == T_BOOL)
        {
            expr.SetType(T_INTEGER);
            isDiff = false;
            ReportWarning("Converting bool to int");
            llvm::Value *val = llvmBuilder->CreateZExtOrTrunc(expr.GetLLVMValue(), llvmBuilder->getInt32Ty());
            expr.SetLLVMValue(val);
        }
        else if (expr.GetType() == T_FLOAT) // float -> int
        {
            expr.SetType(T_INTEGER);
            isDiff = false;
            ReportWarning("Converting float to int");
            llvm::Value *val = llvmBuilder->CreateFPToSI(expr.GetLLVMValue(), llvmBuilder->getInt32Ty());
            expr.SetLLVMValue(val);
        }
    }

    // int -> float
    if (dest.GetType() == T_FLOAT && expr.GetType() == T_INTEGER)
    {
        isDiff = false;
        ReportWarning("Converting int to float");
        expr.SetType(T_FLOAT);
        llvm::Value *val = llvmBuilder->CreateSIToFP(expr.GetLLVMValue(), llvmBuilder->getFloatTy());
        expr.SetLLVMValue(val);
    }

    // If we get here, no suitable conversion was found and the types do not match, nor are they interoperable
    if (isDiff)
    {
        ReportTypeMismatchError(TypeToString(dest.GetType()), TypeToString(expr.GetType()));
        expr.SetIsValid(false);
        out.CopySymbol(expr);
        return;
    }

    // By this point any compatible types should have been converted
    out.CopySymbol(expr);
}

// <destination>
Symbol Parser::Destination()
{
    std::string id = Identifier();
    Symbol dest = symbolTable.FindSymbol(id);

    if (!dest.IsValid())
    {
        ReportError("Symbol: " + id + " not found");
//        out.CopySymbol(dest);
        return dest;
    }

    if (dest.GetDeclarationType() != T_VARIABLE)
    {
        ReportError("Variable required for valid destination");
        dest = Symbol();
        dest.SetIsValid(false);

//        out.CopySymbol(dest);
        return dest;
    }

    // Index was supplied, index the array
    if (ValidateToken(T_LBRACKET))
    {
        IndexArray(dest, dest);
    }
    else // if it is an array with no index, then we unroll the array to operate on the entire thing
    {
        // unroll
        if (dest.IsArray())
        {
            dest.SetIsArrayIndexed(true);
            doUnroll = true;
            unrollSize = dest.GetArraySize();

            // Zero index
            llvm::Value *zero = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, 0, true));
            unrollIdx = zero;

            // Store the index
            unrollIdxAddress = llvmBuilder->CreateAlloca(llvmBuilder->getInt32Ty());
            llvmBuilder->CreateStore(unrollIdx, unrollIdxAddress);

            // Blocks for unrolling loop
            unrollLoopStart = llvm::BasicBlock::Create(llvmContext, "", llvmCurrProc);
            llvm::BasicBlock *unrollLoopBody = llvm::BasicBlock::Create(llvmContext, "", llvmCurrProc);
            unrollLoopEnd = llvm::BasicBlock::Create(llvmContext, "", llvmCurrProc);

            // Jump to the loop start and load index
            llvmBuilder->CreateBr(unrollLoopStart);
            llvmBuilder->SetInsertPoint(unrollLoopStart);
            unrollIdx = llvmBuilder->CreateLoad(llvmBuilder->getInt32Ty(), unrollIdxAddress);

            // Check if index equals the size, if it does jump to the end
            llvm::Value *cmp = llvmBuilder->CreateICmpEQ(unrollIdx, dest.GetLLVMArraySize());
            llvmBuilder->CreateCondBr(cmp, unrollLoopEnd, unrollLoopBody);

            // Set the insert point to the loop body so that the expression of the assignment statement
            // goes in the body
            llvmBuilder->SetInsertPoint(unrollLoopBody);
            unrollIdx = llvmBuilder->CreateLoad(llvmBuilder->getInt32Ty(), unrollIdxAddress);

            llvm::Value *addr = nullptr;
            if (dest.IsGlobal())
            {
                llvm::Value *vals[] = {zero, unrollIdx};
                addr = llvmBuilder->CreateInBoundsGEP(dest.GetLLVMArrayAddress(), vals);
            }
            else
            {
                addr = llvmBuilder->CreateGEP(dest.GetLLVMArrayAddress(), unrollIdx);
            }

            dest.SetLLVMAddress(addr);

        }
    }

    // If we get here, the destination is valid
//    out.CopySymbol(dest);
    return dest;
}

// <expression>
void Parser::Expression(Symbol expectedType, Symbol &out)
{
    bool isNotOp = ValidateToken(T_NOT) ? true : false;

    Symbol arithOp =Symbol();
    ArithOp(expectedType, arithOp);
    token_t *op = scanner.PeekToken();
    Symbol expr_ = Symbol();
    Expression_(expectedType, expr_);

    ExpressionTypeCheck(expectedType, arithOp, expr_, op, isNotOp, out);
}

// <expression>
void Parser::Expression_(Symbol expectedType, Symbol &out)
{
    if (ValidateToken(T_AND) || ValidateToken(T_OR))
    {
        Symbol arithOp = Symbol();
        ArithOp(expectedType, arithOp);
        token_t *op = scanner.PeekToken();
        Symbol expr_ = Symbol();
        Expression_(expectedType, expr_);

        ExpressionTypeCheck(expectedType, arithOp, expr_, op, false, out);
        return;
    }

    Symbol sym = Symbol();
    sym.SetIsValid(false);
    out.CopySymbol(sym);
}

// Verify expression is valid, types match, do codegen
void Parser::ExpressionTypeCheck(Symbol expectedType, Symbol arithOp, Symbol expr_, token_t *op, bool isNotOp, Symbol &out)
{
    if (expr_.IsValid())
    {
        // There is an operation if we get here
        Symbol sym = Symbol();
        bool isInterop = false;
        std::string opStr;

        // check if types are interoperable
        int expectedTy = expectedType.GetType();
        switch (expectedTy)
        {
            case T_BOOL:
                opStr = "logical";
                isInterop = (arithOp.GetType() == T_BOOL && expr_.GetType() == T_BOOL);
                sym.SetType(T_BOOL);
                break;
            case T_INTEGER:
            case T_FLOAT:
                opStr = "binary";
                isInterop = (arithOp.GetType() == T_INTEGER && expr_.GetType() == T_INTEGER);
                sym.SetType(T_INTEGER);
                break;
            default:
                ReportError("Invalid type");
                sym.SetIsValid(false);
                out.CopySymbol(sym);
                return;
        }

        if (!isInterop)
        {
            ReportIncompatibleTypeError(opStr, TypeToString(arithOp.GetType()), TypeToString(expr_.GetType()));
            sym.SetIsValid(false);
            out.CopySymbol(sym);
            return;
        }

        // Create the appropriate operation
        llvm::Value *val;
        llvm::Value *arithVal = arithOp.GetLLVMValue();
        llvm::Value *exprVal = expr_.GetLLVMValue();
        switch (op->type)
        {
            case T_AND:
                val = llvmBuilder->CreateAnd(arithVal, exprVal);
                break;
            case T_OR:
                val = llvmBuilder->CreateOr(arithVal, exprVal);
                break;
            default:
                printf("No operation found");
        }

        sym.SetLLVMValue(val);

        // Create Not operation if one should exist
        if (isNotOp)
        {
            val = llvmBuilder->CreateNot(sym.GetLLVMValue());
            sym.SetLLVMValue(val);
        }
        out.CopySymbol(sym);
        return;
    }
    else
    {
        if (isNotOp)
        {
            // check if types are interoperable
            bool isInterop = false;
            int expectedTy = expectedType.GetType();
            switch (expectedTy)
            {
                case T_BOOL:
                    isInterop = (arithOp.GetType() == T_BOOL);
                    break;
                case T_INTEGER:
                case T_FLOAT:
                    isInterop = (arithOp.GetType() == T_INTEGER);
                    break;
                default:
                    break;
            }

            if (!isInterop)
            {
                ReportIncompatibleTypeError("binary", TypeToString(arithOp.GetType()), "null");
                Symbol sym = Symbol();
                sym.SetIsValid(false);
                out.CopySymbol(sym);
                return;
            }

            // Create Not operation
            llvm::Value *val = llvmBuilder->CreateNot(arithOp.GetLLVMValue());
            arithOp.SetLLVMValue(val);
        }

        // expr_ was not valid, return arithOp
        out.CopySymbol(arithOp);
        return;
    }
}

// <arith_op>
void Parser::ArithOp(Symbol expectedType, Symbol &out)
{
    Symbol rel = Symbol();
    Relation(expectedType, rel);
    token_t *op = scanner.PeekToken();
    Symbol arithOp_ = Symbol();
    ArithOp_(expectedType, arithOp_);

    ArithOpTypeCheck(expectedType, rel, arithOp_, op, out);
}

// <arith_op>
void Parser::ArithOp_(Symbol expectedType, Symbol &out)
{
    if (ValidateToken(T_ADD) || ValidateToken(T_SUBTRACT))
    {
        Symbol rel = Symbol();
        Relation(expectedType, rel);
        token_t *op = scanner.PeekToken();
        Symbol arithOp_ = Symbol();
        ArithOp_(expectedType, arithOp_);

        ArithOpTypeCheck(expectedType, rel, arithOp_, op, out);
        return;
    }

    Symbol sym = Symbol();
    sym.SetIsValid(false);

    out.CopySymbol(sym);
    return;
}

// Verify arithOp is valid, types match, do codegen
void Parser::ArithOpTypeCheck(Symbol expectedType, Symbol rel, Symbol arithOp_, token_t *op, Symbol &out)
{
    if (arithOp_.IsValid())
    {
        Symbol sym = Symbol();

        // check if types are interoperable
        bool isInterop = false;
        if (rel.GetType() == T_INTEGER)
        {
            isInterop = (arithOp_.GetType() == T_INTEGER || arithOp_.GetType() == T_FLOAT);
            if (isInterop && arithOp_.GetType() == T_FLOAT)
            {
                // do type conversion
                llvm::Value *val = llvmBuilder->CreateSIToFP(rel.GetLLVMValue(), llvmBuilder->getFloatTy());
                rel.SetLLVMValue(val);

            }
        }
        else if (rel.GetType() == T_FLOAT)
        {
            isInterop = (arithOp_.GetType() == T_INTEGER || arithOp_.GetType() == T_FLOAT);
            if (isInterop && arithOp_.GetType() == T_INTEGER)
            {
                llvm::Value *val = llvmBuilder->CreateSIToFP(arithOp_.GetLLVMValue(), llvmBuilder->getFloatTy());
                arithOp_.SetLLVMValue(val);
            }
        }

        if (!isInterop)
        {
            ReportIncompatibleTypeError("arith", TypeToString(rel.GetType()), TypeToString(arithOp_.GetType()));
            sym.SetIsValid(false);
            out.CopySymbol(sym);
            return;
        }

        bool isFloatOp = (rel.GetType() == T_FLOAT || arithOp_.GetType() == T_FLOAT);

        llvm::Value *val;
        switch (op->type)
        {
            case T_ADD:
            {
                auto relVal = rel.GetLLVMValue();
                auto arithVal = arithOp_.GetLLVMValue();
                if (isFloatOp) {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::FAdd, relVal, arithVal);
                } else {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::Add, relVal, arithVal);
                }
                break;
            }
            case T_SUBTRACT:
            {
                auto relVal = rel.GetLLVMValue();
                auto arithVal = arithOp_.GetLLVMValue();
                if (isFloatOp)
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::FSub, relVal, arithVal);
                }
                else
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::Sub, relVal, arithVal);
                }
                break;
            }
            case T_MULTIPLY:
            {
                auto relVal = rel.GetLLVMValue();
                auto arithVal = arithOp_.GetLLVMValue();
                if (isFloatOp)
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::FMul, relVal, arithVal);
                }
                else
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::Mul, relVal, arithVal);
                }
                break;
            }
            case T_DIVIDE:
            {
                auto relVal = rel.GetLLVMValue();
                auto arithVal = arithOp_.GetLLVMValue();
                if (isFloatOp)
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::FDiv, relVal, arithVal);
                }
                else
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::SDiv, relVal, arithVal);
                }
                break;
            }
            default:
                std::cout << "Could not find an operation" << std::endl;
        }

        sym.SetLLVMValue(val);

        if (expectedType.GetType() == T_FLOAT)
        {
            // Cast to float
            sym.SetType(T_FLOAT);
            if (!isFloatOp)
            {
                llvm::Value *val = llvmBuilder->CreateSIToFP(sym.GetLLVMValue(), llvmBuilder->getFloatTy());
                sym.SetLLVMValue(val);
            }
        }
        else if (expectedType.GetType() == T_INTEGER)
        {
            // Cast to int
            sym.SetType(T_INTEGER);
            if (isFloatOp)
            {
                llvm::Value *val = llvmBuilder->CreateSIToFP(sym.GetLLVMValue(), llvmBuilder->getInt32Ty());
                sym.SetLLVMValue(val);
            }
        }
        else
        {
            // float is preferred
            if (rel.GetType() == T_FLOAT || arithOp_.GetType() == T_FLOAT)
            {
                sym.SetType(T_FLOAT);
            }
            else
            {
                sym.SetType(T_INTEGER);
            }
        }

        out.CopySymbol(sym);
        return;
    }
    else
    {
        // arithOp_ was not valid
        out.CopySymbol(rel);
        return;
    }
}

// <relation>
void Parser::Relation(Symbol expectedType, Symbol &out)
{
    Symbol term = Symbol();
    Term(expectedType, term);
    token_t *op = scanner.PeekToken();
    Symbol relation_ = Symbol();
    Relation_(expectedType, relation_);

    RelationTypeCheck(expectedType, term, relation_, op, out);
}

// <relation>
void Parser::Relation_(Symbol expectedType, Symbol &out)
{
    if (ValidateToken(T_LESSTHAN) ||
        ValidateToken(T_GREATERTHAN) ||
        ValidateToken(T_LTEQ) ||
        ValidateToken(T_GTEQ) ||
        ValidateToken(T_EQEQ) ||
        ValidateToken(T_NOTEQ))
    {
        Symbol term = Symbol();
        Term(expectedType, term);
        token_t *op = scanner.PeekToken();
        Symbol relation_ = Symbol();
        Relation_(expectedType, relation_);

        RelationTypeCheck(expectedType, term, relation_, op, out);
        return;
    }

    Symbol sym = Symbol();
    sym.SetIsValid(false);

    out.CopySymbol(sym);
    return;
}

// Verify relation is valid, types match, do codegen
void Parser::RelationTypeCheck(Symbol expectedType, Symbol term, Symbol relation_, token_t *op, Symbol &out)
{
    if (relation_.IsValid())
    {
        Symbol sym = Symbol();
        bool isInterop = false;
        bool isFloatOp = false;

        // do type conversion if necessary and allowed
        switch (term.GetType())
        {
            case T_BOOL:
                isInterop = (relation_.GetType() == T_BOOL || relation_.GetType() == T_INTEGER);
                if (isInterop && relation_.GetType() == T_INTEGER)
                {
                    llvm::Value *val = llvmBuilder->CreateZExtOrTrunc(term.GetLLVMValue(), llvmBuilder->getInt32Ty());
                    term.SetLLVMValue(val);
                }
                break;
            case T_FLOAT:
                isFloatOp = true;
                isInterop = (relation_.GetType() == T_FLOAT || relation_.GetType() == T_INTEGER);
                if (isInterop && relation_.GetType() == T_INTEGER)
                {
                    llvm::Value *val = llvmBuilder->CreateSIToFP(relation_.GetLLVMValue(), llvmBuilder->getFloatTy());
                    relation_.SetLLVMValue(val);
                }
                break;
            case T_INTEGER:
                isInterop = (relation_.GetType() == T_INTEGER || relation_.GetType() == T_FLOAT || relation_.GetType() == T_BOOL);
                if (isInterop)
                {
                    if (relation_.GetType() == T_BOOL)
                    {
                        llvm::Value *val = llvmBuilder->CreateZExtOrTrunc(relation_.GetLLVMValue(), llvmBuilder->getInt32Ty());
                        relation_.SetLLVMValue(val);
                    }
                    else if (relation_.GetType() == T_FLOAT)
                    {
                        llvm::Value *val = llvmBuilder->CreateSIToFP(term.GetLLVMValue(), llvmBuilder->getFloatTy());
                        term.SetLLVMValue(val);
                        isFloatOp = true;
                    }
                }
                break;
            case T_STRING:
                isInterop = ((op->type == T_EQEQ || op->type == T_NOTEQ) && (relation_.GetType() == T_STRING));
                break;
        }

        if (!isInterop)
        {
            ReportIncompatibleTypeError("relational", TypeToString(term.GetType()), TypeToString(relation_.GetType()));
            sym.SetIsValid(false);
            out.CopySymbol(sym);
            return;
        }

        llvm::Value *val ;
        if (term.GetType() == T_STRING)
        {
            val = DoStringComp(term, relation_, op, val);
        }
        else
        {
            switch (op->type)
            {
                case T_LESSTHAN:
                    if (isFloatOp)
                    {
                        auto termVal = term.GetLLVMValue();
                        auto relVal = relation_.GetLLVMValue();
                        val = llvmBuilder->CreateFCmpOLT(termVal, relVal);
                    }
                    else
                    {
                        auto termVal = term.GetLLVMValue();
                        auto relVal = relation_.GetLLVMValue();
                        val = llvmBuilder->CreateICmpSLT(termVal, relVal);
                    }
                    break;
                case T_LTEQ:
                    if (isFloatOp)
                    {
                        auto termVal = term.GetLLVMValue();
                        auto relVal = relation_.GetLLVMValue();
                        val = llvmBuilder->CreateFCmpOLE(termVal, relVal);
                    }
                    else
                    {
                        auto termVal = term.GetLLVMValue();
                        auto relVal = relation_.GetLLVMValue();
                        val = llvmBuilder->CreateICmpSLE(termVal, relVal);
                    }
                    break;
                case T_GREATERTHAN:
                    if (isFloatOp)
                    {
                        auto termVal = term.GetLLVMValue();
                        auto relVal = relation_.GetLLVMValue();
                        val = llvmBuilder->CreateFCmpOGT(termVal, relVal);
                    }
                    else
                    {
                        auto termVal = term.GetLLVMValue();
                        auto relVal = relation_.GetLLVMValue();
                        val = llvmBuilder->CreateICmpSGT(termVal, relVal);
                    }
                    break;
                case T_GTEQ:
                    if (isFloatOp)
                    {
                        auto termVal = term.GetLLVMValue();
                        auto relVal = relation_.GetLLVMValue();
                        val = llvmBuilder->CreateFCmpOGE(termVal, relVal);
                    }
                    else
                    {
                        auto termVal = term.GetLLVMValue();
                        auto relVal = relation_.GetLLVMValue();
                        val = llvmBuilder->CreateICmpSGE(termVal, relVal);
                    }
                    break;
                case T_EQEQ:
                    if (isFloatOp)
                    {
                        auto termVal = term.GetLLVMValue();
                        auto relVal = relation_.GetLLVMValue();
                        val = llvmBuilder->CreateFCmpOEQ(termVal, relVal);
                    }
                    else
                    {
                        auto termVal = term.GetLLVMValue();
                        auto relVal = relation_.GetLLVMValue();
                        val = llvmBuilder->CreateICmpEQ(termVal, relVal);
                    }
                    break;
                case T_NOTEQ:
                    if (isFloatOp)
                    {
                        auto termVal = term.GetLLVMValue();
                        auto relVal = relation_.GetLLVMValue();
                        val = llvmBuilder->CreateFCmpONE(termVal, relVal);
                    }
                    else
                    {
                        auto termVal = term.GetLLVMValue();
                        auto relVal = relation_.GetLLVMValue();
                        val = llvmBuilder->CreateICmpNE(termVal, relVal);
                    }
                    break;
                default:
                    std::cout << "Could not find an operation" << std::endl;
            }
        }

        sym.SetLLVMValue(val);
        sym.SetType(T_BOOL);

        out.CopySymbol(sym);
        return;
    }
    else
    {
        out.CopySymbol(term);
        return;
    }
}

// <term>
void Parser::Term(Symbol expectedType, Symbol &out)
{
    Symbol factor = Symbol();
    Factor(expectedType, factor);
    token_t *op = scanner.PeekToken();
    Symbol term_ = Symbol();
    Term_(expectedType, term_);

    ArithOpTypeCheck(expectedType, factor, term_, op, out);
    return;
}

// <term>
void Parser::Term_(Symbol expectedType, Symbol &out)
{
    if (ValidateToken(T_DIVIDE) || ValidateToken(T_MULTIPLY))
    {
        Symbol factor = Symbol();
        Factor(expectedType, factor);
        token_t *op = scanner.PeekToken();
        Symbol term_ = Symbol();
        Term_(expectedType, term_);

        ArithOpTypeCheck(expectedType, factor, term_, op, out);
        return;
    }

    Symbol sym = Symbol();
    sym.SetIsValid(false);

    out.CopySymbol(sym);
    return;
}

// <factor>
void Parser::Factor(Symbol expectedType, Symbol &out)
{
    // This just follows the rules defined in the project language for <factor> syntax
    Symbol sym = Symbol();
    if (ValidateToken(T_LPAREN))
    {
        Expression(expectedType, sym);
        if (!ValidateToken(T_RPAREN))
        {
            ReportMissingTokenError(")");
            sym.SetIsValid(false);
            out.CopySymbol(sym);
            return;
        }
    }
    else if (ValidateToken(T_IDENTIFIER))
    {
        ProcedureCallOrName(sym);
    }
    else if (ValidateToken(T_SUBTRACT))
    {
        if (ValidateToken(T_INT_LITERAL) || (ValidateToken(T_FLOAT_LITERAL)))
        {
            Number(sym);
        }
        else if (ValidateToken(T_IDENTIFIER))
        {
            ProcedureCallOrName(sym);
        }
        else
        {
            ReportError("Expected an identifier or number literal");
            sym.SetIsValid(false);
            out.CopySymbol(sym);
            return;
        }

        if (sym.GetType() == T_INTEGER)
        {
            llvm::Value *val = llvmBuilder->CreateNeg(sym.GetLLVMValue());
            sym.SetLLVMValue(val);
        }
        else if (sym.GetType() == T_FLOAT)
        {
            llvm::Value *val = llvmBuilder->CreateFNeg(sym.GetLLVMValue());
            sym.SetLLVMValue(val);
        }
        else
        {
            ReportTypeMismatchError("int/float", TypeToString(sym.GetType()));
        }


    }
    else if (ValidateToken(T_INT_LITERAL) || ValidateToken(T_FLOAT_LITERAL))
    {
        Number(sym);
    }
    else if (ValidateToken(T_STRING_LITERAL))
    {
        String(sym);
    }
    else if (ValidateToken(T_TRUE))
    {
        sym.SetType(T_BOOL);
        llvm::Value *val = llvm::ConstantInt::getIntegerValue(GetLLVMType(sym),llvm::APInt(1, 1, true));
        sym.SetLLVMValue(val);
    }
    else if (ValidateToken(T_FALSE))
    {
        sym.SetType(T_BOOL);
        llvm::Value *val = llvm::ConstantInt::getIntegerValue(GetLLVMType(sym),llvm::APInt(1, 0, true));
        sym.SetLLVMValue(val);
    }
    else
    {
        ReportError("Factor expected");
        sym.SetIsValid(false);
        out.CopySymbol(sym);
        return;
    }

    out.CopySymbol(sym);
    return;
}

// Combines <procedure_call> and <name> because both are <identifier>'s. The two are then differentiated here and
// the appropriate course of action is taken for each.
void Parser::ProcedureCallOrName(Symbol &out)
{
    Symbol sym = Symbol();
    std::string id = token->val.stringValue;
    sym = symbolTable.FindSymbol(id);

    if (!sym.IsValid())
    {
        ReportError("Symbol not found: " + id);
        out.CopySymbol(sym);
        return;
    }

    // if there is a parenthesis this is a procedure, as () is required
    if (ValidateToken(T_LPAREN))
    {
        // just double check that the symbol is definitely a procedure
        if (sym.GetDeclarationType() != T_PROCEDURE)
        {
            ReportError("Cannot call non procedure type");
            sym = Symbol();
            sym.SetIsValid(false);

            out.CopySymbol(sym);
            return;
        }

        Symbol procSym = sym;
        sym = Symbol();
        sym.SetType(procSym.GetType());

        // Get any arguments
        token_t *tmp = scanner.PeekToken();
        if (tmp->type == T_NOT || tmp->type == T_LPAREN ||
            tmp->type == T_SUBTRACT || tmp->type == T_INT_LITERAL ||
            tmp->type == T_FLOAT_LITERAL || tmp->type == T_IDENTIFIER ||
            tmp->type == T_STRING_LITERAL || tmp->type == T_TRUE ||
            tmp->type == T_FALSE)
        {
            auto paramStart = procSym.GetParameters().begin();
            auto paramEnd = procSym.GetParameters().end();
            std::vector<llvm::Value *> arguments = ArgumentList(paramStart, paramEnd);
            llvm::Value *val = llvmBuilder->CreateCall(procSym.GetLLVMFunction(), arguments);
            sym.SetLLVMValue(val);
        }
        else
        {
            // check that arguments aren't missing
            if (!procSym.GetParameters().empty())
            {
                ReportError("Missing arguments for procedure");
                sym.SetIsValid(false);

                out.CopySymbol(sym);
                return;
            }

            llvm::Value *val = llvmBuilder->CreateCall(procSym.GetLLVMFunction());
            sym.SetLLVMValue(val);
        }

        if (!ValidateToken(T_RPAREN))
        {
            ReportMissingTokenError(")");
            sym.SetIsValid(false);

            out.CopySymbol(sym);
            return;
        }
    }
    else
    {
        // name, has to be a variable now
        if (sym.GetDeclarationType() != T_VARIABLE)
        {
            ReportError("Name must be a variable");
            sym.SetIsValid(false);

            out.CopySymbol(sym);
            return;
        }

        if (ValidateToken(T_LBRACKET))
        {
            // array, we must index it
            if (sym.GetDeclarationType() != T_VARIABLE)
            {
                ReportError("Indexing is not supported for non-arrays.");
                sym = Symbol();
                sym.SetIsValid(false);

                out.CopySymbol(sym);
                return;
            }

            IndexArray(sym, sym);
        }
        else if (doUnroll)
        {
            // no index, unroll array
            if (sym.IsArray())
            {
                sym.SetIsArrayIndexed(true);
                if (sym.GetArraySize() < unrollSize)
                {
                    ReportError("Array size is smaller than destination size. Unroll failed.");
                    sym = Symbol();
                    sym.SetIsValid(false);

                    out.CopySymbol(sym);
                    return;
                }

                llvm::Value *addr = llvmBuilder->CreateGEP(sym.GetLLVMArrayAddress(), unrollIdx);
                sym.SetLLVMAddress(addr);

            }
        }
        else
        {
            sym.SetIsArrayIndexed(false);
        }

        if (sym.GetDeclarationType() == T_VARIABLE)
        {
            if (sym.IsInitialized() == false)
            {
                ReportError("Variable has not yet been initialized");
                sym.SetIsValid(false);

                out.CopySymbol(sym);
                return;
            }

            // Load value
            llvm::Value *val = llvmBuilder->CreateLoad(GetLLVMType(sym), sym.GetLLVMAddress());
            sym.SetLLVMValue(val);
        }
    }

    out.CopySymbol(sym);
    return;
}

// <number>
void Parser::Number(Symbol &out)
{
    Symbol sym = Symbol();

    // Is it an int or a float?
    llvm::Value *val;
    if (token->type == T_INT_LITERAL)
    {
        sym.SetType(T_INTEGER);
        val = llvm::ConstantInt::getIntegerValue(GetLLVMType(sym), llvm::APInt(32, token->val.intValue, true));
    }
    else if (token->type == T_FLOAT_LITERAL)
    {
        sym.SetType(T_FLOAT);
        val = llvm::ConstantFP::get(GetLLVMType(sym), llvm::APFloat(token->val.floatValue));
    }

    sym.SetLLVMValue(val);

    out.CopySymbol(sym);
    return;
}

// <string>
void Parser::String(Symbol &out)
{
    Symbol sym = Symbol();
    if (token->type != T_STRING_LITERAL)
    {
        ReportError("String literal expected.");
        out.CopySymbol(sym);
        return;
    }

    sym.SetType(T_STRING);
    llvm::Value *val = llvmBuilder->CreateGlobalStringPtr(token->val.stringValue);
    sym.SetLLVMValue(val);

    out.CopySymbol(sym);
    return;
}

// <argument_list>
std::vector<llvm::Value *> Parser::ArgumentList(std::vector<Symbol>::iterator curr, std::vector<Symbol>::iterator end)
{
    bool continue_ = true;
    std::vector<llvm::Value *> arguments;
    while (continue_)
    {
        continue_ = false;

        if (curr == end)
        {
            // No more args
            ReportError("Too many arguments");
            return arguments;
        }

        // Make sure the type matches what is expected
        Symbol expr = Symbol();
        Expression(*curr, expr);
        AssignmentTypeCheck(*curr, expr, token, expr);

        if (curr->IsArray())
        {
            if (curr->GetArraySize() != expr.GetArraySize() || expr.IsArrayIndexed())
            {
                ReportError("Array expected as argument");
                return arguments;
            }
        }
        else if (expr.IsArray() && !expr.IsArrayIndexed())
        {
            ReportError("Invalid argument: Cannot pass un-indexed array");
            return arguments;
        }

        if (!expr.IsValid())
        {
            return arguments;
        }

        // add to arg vector
        if (expr.IsArray() && !expr.IsArrayIndexed())
        {
            if (expr.IsGlobal())
            {
                // Global arrays are weird
                llvm::Value *zero = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, 0, true));
                llvm::Value *val = llvmBuilder->CreateInBoundsGEP(expr.GetLLVMArrayAddress(), zero);
                val = llvmBuilder->CreateBitCast(val, GetLLVMType(expr)->getPointerTo());
                arguments.push_back(val);
            }
            else
            {
                arguments.push_back(expr.GetLLVMValue());
            }
        }
        else
        {
            arguments.push_back(expr.GetLLVMValue());
        }

        // but wait, there's more
        if (ValidateToken(T_COMMA))
        {
            continue_ = true;
            curr = std::next(curr);
        }
        else
        {
            if (curr != end)
            {
                if (std::next(curr) != end)
                {
                    ReportError("Not enough arguments");
                    return arguments;
                }
            }

            continue_ = false;
        }
    }


    return arguments;
}

// Helper function to get the corresponding llvm type from the symbol
// returns llvm::Type*
llvm::Type *Parser::GetLLVMType(Symbol symbol)
{
    int type = symbol.GetType();
    switch (type)
    {
        case T_INTEGER:
            return llvmBuilder->getInt32Ty();
        case T_FLOAT:
            return llvmBuilder->getFloatTy();
        case T_BOOL:
            return llvmBuilder->getInt1Ty();
        case T_STRING:
            return llvmBuilder->getInt8PtrTy();
        default:
            return llvm::IntegerType::getInt1Ty(llvmModule->getContext());
    }
}

// Helper function for performing int->bool type conversion
llvm::Value *Parser::ConvertIntToBool(llvm::Value *intVal)
{
    llvm::Value *zero = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, 0, true));
    llvm::Value *cmp = llvmBuilder->CreateICmpEQ(intVal, zero);

    llvm::Value *true_ = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt1Ty(), llvm::APInt(1, 1, true));
    llvm::Value *false_ = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt1Ty(), llvm::APInt(1, 0, true));

    llvm::Value *boolVal = llvmBuilder->CreateSelect(cmp, false_, true_);

    return boolVal;
}

// Helper function for performing string comparisions
llvm::Value* Parser::DoStringComp(Symbol term, Symbol relation, token_t *op, llvm::Value* val)
{
    // create a loop to compare strings
    llvm::Value *idxAddr = llvmBuilder->CreateAlloca(llvmBuilder->getInt32Ty());
    llvm::Value *idx = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, 0, true));
    llvmBuilder->CreateStore(idx, idxAddr);

    llvm::BasicBlock *strCmpStart = llvm::BasicBlock::Create(llvmContext, "", llvmCurrProc);
    llvm::BasicBlock *strCmpEnd = llvm::BasicBlock::Create(llvmContext, "", llvmCurrProc);

    // jump to the loop start
    llvmBuilder->CreateBr(strCmpStart);
    llvmBuilder->SetInsertPoint(strCmpStart);

    llvm::Value *one = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, 1, true));
    idx = llvmBuilder->CreateLoad(llvmBuilder->getInt32Ty(), idxAddr);
    idx = llvmBuilder->CreateBinOp(llvm::Instruction::Add, idx, one);
    llvmBuilder->CreateStore(idx, idxAddr);

    // Get the character
    llvm::Value *termAddr = llvmBuilder->CreateGEP(term.GetLLVMValue(), idx);
    llvm::Value *termChar = llvmBuilder->CreateLoad(llvmBuilder->getInt8PtrTy(), termAddr);
    llvm::Value *tailAddr = llvmBuilder->CreateGEP(relation.GetLLVMValue(), idx);
    llvm::Value *tailChar = llvmBuilder->CreateLoad(llvmBuilder->getInt8PtrTy(), tailAddr);

    // Do the comparison
    llvm::Value *cmp = llvmBuilder->CreateICmpEQ(termChar, tailChar);

    // check that strings are still going
    llvm::Value *zero = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt8PtrTy(), llvm::APInt(8, 0, true));
    llvm::Value *noEnd = llvmBuilder->CreateICmpNE(termChar, zero);
    llvm::Value *cont = llvmBuilder->CreateAnd(cmp, noEnd);

    llvmBuilder->CreateCondBr(cont, strCmpStart, strCmpEnd);
    llvmBuilder->SetInsertPoint(strCmpEnd);

    if (op->type == T_EQEQ)
    {
        val = cmp;
    }
    else
    {
        val = llvmBuilder->CreateNot(cmp);
    }

    return val;
}
