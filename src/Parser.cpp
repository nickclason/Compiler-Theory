//
// Created by Nick Clason on 2/1/21.
//

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

Parser::Parser(bool debug_, Scanner scanner_, SymbolTable symbolTable_, token_t *token_)
{
    debug = debug_;
    scanner = scanner_;
    symbolTable = symbolTable_;
    token = token_;

    stopParse = false;
    errorFlag = false;
    doCompile = true;

    procedureCount = 0;
    errorCount = 0;
    warningCount = 0;

    llvmModule = nullptr;
    llvmBuilder = nullptr;
    llvmCurrProc = nullptr;

    Program();

    if (!errorFlag && errorCount == 0)
    {
        std::cout << "Parse was successful." << std::endl;

        // Compile
        if (doCompile)
        {
            InitLLVM();
        }
    }
}

Parser::~Parser()=default;

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
        ReportMissingTokenError("PROGRAM");
        return;
    }

    std::string id = Identifier();

    llvmModule = new llvm::Module(id, llvmContext);
    llvmBuilder = new llvm::IRBuilder<>(llvmContext);

    symbolTable.AddIOFunctions(llvmModule, llvmContext, llvmBuilder);

    if (!ValidateToken(T_IS))
    {
        ReportMissingTokenError("IS");
        return;
    }
}

void Parser::ProgramBody()
{
    PrintDebugInfo("<program_body>");

    // Get all declarations
    WhileDeclarations();

    if (token->type != T_BEGIN)
    {
        ReportMissingTokenError("BEGIN");
        return;
    }

    std::vector<llvm::Type *> parameters;
    llvm::FunctionType *functionType = llvm::FunctionType::get(llvmBuilder->getInt32Ty(), parameters, false);

    llvm::FunctionCallee mainCallee = llvmModule->getOrInsertFunction("main", functionType);
    auto *main = llvm::dyn_cast<llvm::Constant>(mainCallee.getCallee());

    llvmCurrProc = llvm::cast<llvm::Function>(main);
    llvmCurrProc->setCallingConv(llvm::CallingConv::C);

    llvm::BasicBlock *entrypoint = llvm::BasicBlock::Create(llvmContext, "entrypoint", llvmCurrProc);
    llvmBuilder->SetInsertPoint(entrypoint);

    for (auto &it : symbolTable.GetLocalScope())
    {
        // removed per revision 5 of projectLanguage.pdf
        // if (it.second.GetDeclarationType() != T_VARIABLE || it.second.IsGlobal())
        if (it.second.GetDeclarationType() != T_VARIABLE)
        {
            continue;
        }

        llvm::Value *address = nullptr;
        // array
        address = llvmBuilder->CreateAlloca(GetLLVMType(it.second));
        it.second.SetLLVMAddress(address);

        symbolTable.AddSymbol(it.second); // Update symbol table
    }

    int terminators[] = { T_END };
    int terminatorsSize = 1;
    WhileStatements(terminators, terminatorsSize);

    if (!(token->type == T_END))
    {
        ReportMissingTokenError("END");
        return;
    }

    if (!ValidateToken(T_PROGRAM))
    {
        ReportMissingTokenError("PROGRAM");
        return;
    }

    if (llvmBuilder->GetInsertBlock()->getTerminator() == nullptr)
    {
        llvm::Constant *retVal = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, 0, true));
        llvmBuilder->CreateRet(retVal);
    }
}

void Parser::InitLLVM()
{
    std::string outFile = "output/IR.ll";
    std::error_code error_code;
    llvm::raw_fd_ostream out(outFile, error_code, llvm::sys::fs::F_None);
    llvmModule->print(out, nullptr);

    // These steps are from the llvm Kaleidoscope tutorial
    //bool isVerified = llvm::verifyModule(*llvmModule, &llvm::errs());

    // TODO: figure out why this fails
    //if (!isVerified) { llvmModule->print(llvm::errs(), nullptr); return; }

    auto TargetTriple = llvm::sys::getDefaultTargetTriple();

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);


    if (!Target) {
        llvm::errs() << Error;
        return;
    }

    //printf("TARGET");

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

    if (EC) {
        llvm::errs() << "Could not open file: " << EC.message();
        return;
    }

    llvm::legacy::PassManager pass;
    auto FileType = llvm::CGFT_ObjectFile;

    if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
        llvm::errs() << "TargetMachine can't emit a file of this type";
        return;
    }

    pass.run(*llvmModule);
    dest.flush();
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
        ReportError("Unknown token was found");
        return false;
    }
    else if (tempToken->type == T_EOF)
    {
        return false;
    }

    return false;
}

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

void Parser::PrintDebugInfo(std::string langID)
{
    if (debug)
    {
        std::cout << langID << std::endl;
    }
}

void Parser::ReportError(std::string msg)
{
    if (errorFlag) return;

    std::cout << "Line: " << token->line << " Col: " << token->col << " : " << msg << std::endl;
    errorFlag = true;
    errorCount++;
}

void Parser::ReportMissingTokenError(std::string expected)
{
    if (errorFlag) return;

    std::cout << "Line: " << token->line << " Col: " << token->col << " : " << "Expected token " << expected << std::endl;
    errorFlag = true;
    errorCount++;
}

void Parser::ReportTypeMismatchError(std::string expected, std::string actual)
{
    if (errorFlag) return;

    std::cout << "Line: " << token->line << " Col: " << token->col << " : " << std::endl;
    std::cout << "\tExpected Type: " << expected << std::endl;
    std::cout << "\tFound Type: " << actual << std::endl;
    errorFlag = true;
    errorCount++;
}

void Parser::ReportOpTypeCheckError(std::string op, std::string type1, std::string type2)
{
    if (errorFlag) return;

    std::cout << "Line:" << token->line << " Col:" << token->col << " - ";
    std::cout << "Incompatible types for " << op << ":" << std::endl;
    std::cout << "Type 1: " << type1 << std::endl;
    std::cout << "Type 2: " << type2 << std::endl << std::endl;
    errorFlag = true;
    errorCount++;
}

void Parser::ReportWarning(std::string msg)
{
    if (errorFlag) return;
    std::cout << "Line:" << token->line << " Col:" << token->col << " : ";
    std::cout << "Warning:" << std::endl << msg << std::endl << std::endl;
    warningCount++;
}

std::string Parser::Identifier()
{
    PrintDebugInfo("<identifier>");

    if (!ValidateToken(T_IDENTIFIER))
    {
        ReportError("Identifier expected");
        return std::string();
    }

    return token->val.stringValue;
}

void Parser::WhileDeclarations()
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

        if (ValidateToken(T_BEGIN))
        {
            continue_ = false;
        }
    }
}

void Parser::Declaration()
{
    PrintDebugInfo("<declaration>");

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

void Parser::VariableDeclaration(Symbol &variable)
{
    PrintDebugInfo("<variable_declaration>");

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

        // Array
        if (variable.IsArray())
        {
            globalTy = llvm::ArrayType::get(GetLLVMType(variable), variable.GetArraySize());
        }
        else
        {
            globalTy = GetLLVMType(variable);
        }

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

    symbolTable.AddSymbol(variable);
}

void Parser::ProcedureDeclaration(Symbol &procedure) {
    PrintDebugInfo("<procedure_declaration>");

    symbolTable.AddScope();
    procedure.SetDeclarationType(T_PROCEDURE);
    ProcedureHeader(procedure);

    std::vector<llvm::Type *> parameters;
    for (const Symbol& param : procedure.GetParameters()) {
        if (param.IsArray()) {
            parameters.push_back(GetLLVMType(param)->getPointerTo());
        } else {
            parameters.push_back(GetLLVMType(param));
        }
    }

    llvm::FunctionType *functionType = llvm::FunctionType::get(GetLLVMType(procedure), parameters, false);
    llvm::FunctionCallee procCallee = llvmModule->getOrInsertFunction("proc_" + std::to_string(procedureCount),
                                                                      functionType);
    auto *proc = llvm::dyn_cast<llvm::Constant>(procCallee.getCallee());
    procedureCount++;
    auto *func = llvm::cast<llvm::Function>(proc);
    func->setCallingConv(llvm::CallingConv::C);
    procedure.SetLLVMFunction(func);

    if (symbolTable.DoesSymbolExist(procedure.GetId())) {
        ReportError("This identifier already exists");
        return;
    }

    symbolTable.AddSymbol(procedure);
    symbolTable.SetScopeProc(procedure);

    ProcedureBody();

    symbolTable.RemoveScope();

    if (symbolTable.GetScopeCount() != 0 && symbolTable.DoesSymbolExist(procedure.GetId())) {
        ReportError("This identifier already exists");
        return;
    }


    symbolTable.AddSymbol(procedure);

}

void Parser::ProcedureHeader(Symbol &procedure)
{
    PrintDebugInfo("<procedure_header>");

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

void Parser::Parameter(Symbol &procedure)
{
    Symbol param;
    VariableDeclaration(param);
    procedure.GetParameters().push_back(param);
}

void Parser::ProcedureBody() {
    PrintDebugInfo("<procedure_body>");

    WhileDeclarations();

    Symbol procSymbol = symbolTable.GetScopeProc();
    llvmCurrProc = procSymbol.GetLLVMFunction();

    llvm::BasicBlock *procEntry = llvm::BasicBlock::Create(llvmContext, "entrypoint", llvmCurrProc);
    llvmBuilder->SetInsertPoint(procEntry);

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

    llvm::Function::arg_iterator args = llvmCurrProc->arg_begin();
    for (const Symbol& sym : procSymbol.GetParameters()) {
        if (args == llvmCurrProc->arg_end()) {
            ReportError("Could not generator LLVM IR for args");
            return;
        }

        llvm::Value *val = &*args++;
        Symbol newSymbol = symbolTable.FindSymbol(sym.GetId());
        newSymbol.SetLLVMValue(val);
        newSymbol.SetIsInitialized(true);

        if (newSymbol.IsArray())
        {
            newSymbol.SetLLVMArrayAddress(val); // TODO: need to pass arrays by value, so the array needs to be copied instead of just reusing the address?
        }
        else
        {
            llvmBuilder->CreateStore(val, newSymbol.GetLLVMAddress());
        }

        symbolTable.AddSymbol(newSymbol);
    }

    if (token->type != T_BEGIN) {
        ReportMissingTokenError("BEGIN");
        return;
    }

    int terminators[] = {T_END};
    int terminatorSize = 1;
    WhileStatements(terminators, terminatorSize);

    if (!ValidateToken(T_PROCEDURE))
    {
        ReportMissingTokenError("PROCEDURE");
        return;
    }

    if (llvmBuilder->GetInsertBlock()->getTerminator() == nullptr)
    {
        llvm::Constant *retVal = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, 0, true));
        llvmBuilder->CreateRet(retVal);
    }
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
        ReportError("Expected int, float, bool, or string type");
        return;
    }

}

void Parser::Bound(Symbol &symbol)
{
    PrintDebugInfo("<bound>");

    if (!ValidateToken(T_INT_LITERAL))
    {
        ReportMissingTokenError("Integer literal");
        return;
    }

    Symbol bound = Number();
    int size = token->val.intValue;
    bound.SetArraySize(size);
}

void Parser::WhileStatements(int terminators[], int terminatorsSize)
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

void Parser::Statement() {
    PrintDebugInfo("<statement>");

    token = scanner.PeekToken();
    if (token->type == T_IDENTIFIER)
    {
        AssignmentStatement();
    }
    else if (token->type == T_IF)
    {
        IfStatement();
    }
    else if (token->type == T_FOR)
    {
        LoopStatement();
    }
    else if (token->type == T_RETURN)
    {
        ReturnStatement();
    }
    else
    {
        ReportError("Expected if, for, or return statement");
        return;
    }

}

void Parser::AssignmentStatement()
{
    PrintDebugInfo("<assignment_statement>");

    Symbol dest = Destination();
    if (!ValidateToken(T_ASSIGNMENT))
    {
        ReportMissingTokenError(":=");
    }

    Symbol expr = Expression(dest);

    expr = AssignmentTypeCheck(dest, expr, token);

    if (!expr.IsValid())
    {
        return;
    }

    llvmBuilder->CreateStore(expr.GetLLVMValue(), dest.GetLLVMAddress());
    dest.SetLLVMValue(expr.GetLLVMValue());
    // symbolTable.AddSymbol(dest); // Update symbol

    dest.SetIsInitialized(true);
    symbolTable.AddSymbol(dest); // Update
}

void Parser::IfStatement()
{
    PrintDebugInfo("<if_statement>");

    if (!ValidateToken(T_IF))
    {
        ReportMissingTokenError("IF");
    }

    if (!ValidateToken(T_LPAREN))
    {
        ReportMissingTokenError("(");
        return;
    }

    Symbol expected = Symbol();
    expected.SetType(T_BOOL);
    Symbol expr = Expression(expected);

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

    llvm::BasicBlock *true_ = nullptr;
    llvm::BasicBlock *false_ = nullptr;
    llvm::BasicBlock *end = nullptr;

    true_ = llvm::BasicBlock::Create(llvmContext, "", llvmCurrProc);
    false_ = llvm::BasicBlock::Create(llvmContext, "", llvmCurrProc);

    llvmBuilder->CreateCondBr(expr.GetLLVMValue(), true_, false_);
    llvmBuilder->SetInsertPoint(true_);

    int terminators[] = {T_ELSE, T_END};
    int terminatorSize = 2;
    WhileStatements(terminators, terminatorSize);

    // std::cout << std::to_string(token->type) << std::endl;
    if (token->type == T_ELSE)
    {
        end = llvm::BasicBlock::Create(llvmContext, "", llvmCurrProc);

        if (llvmBuilder->GetInsertBlock()->getTerminator() == nullptr)
        {
            llvmBuilder->CreateBr(end);
        }

        llvmBuilder->SetInsertPoint(false_);

        int endElse[] = {T_END};
        terminatorSize = 1;
        WhileStatements(endElse, terminatorSize);
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

    if (end == nullptr)
    {
        if (llvmBuilder->GetInsertBlock()->getTerminator() == nullptr)
        {
            llvmBuilder->CreateBr(false_);
        }

        llvmBuilder->SetInsertPoint(false_);
    }
    else
    {
        if (llvmBuilder->GetInsertBlock()->getTerminator() == nullptr)
        {
            llvmBuilder->CreateBr(end);
        }

        llvmBuilder->SetInsertPoint(end);
    }
}

void Parser::LoopStatement()
{
    PrintDebugInfo("<loop_statement>");

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

    AssignmentStatement();

    if (!ValidateToken(T_SEMICOLON))
    {
        ReportMissingTokenError(";");
        return;
    }

    llvm::BasicBlock *loopStart = nullptr;
    llvm::BasicBlock *loopBody = nullptr;
    llvm::BasicBlock *loopEnd = nullptr;

    loopStart = llvm::BasicBlock::Create(llvmContext,"", llvmCurrProc);
    loopBody = llvm::BasicBlock::Create(llvmContext,"", llvmCurrProc);
    loopEnd = llvm::BasicBlock::Create(llvmContext,"", llvmCurrProc);

    llvmBuilder->CreateBr(loopStart);
    llvmBuilder->SetInsertPoint(loopStart);

    Symbol expected = Symbol();
    expected.SetType(T_BOOL);
    Symbol expr = Expression(expected);

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

    llvmBuilder->CreateCondBr(expr.GetLLVMValue(), loopBody, loopEnd);
    llvmBuilder->SetInsertPoint(loopBody);

    int terminators[] = {T_END};
    int terminatorSize = 1;
    WhileStatements(terminators, terminatorSize);

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

    llvmBuilder->CreateBr(loopStart);
    llvmBuilder->SetInsertPoint(loopEnd);
}

void Parser::ReturnStatement()
{
    PrintDebugInfo("<return_statement>");

    if (!ValidateToken(T_RETURN))
    {
        ReportMissingTokenError("RETURN");
        return;
    }

    // Currently if there is any return statement in the "main" program scope, it just gets ignored and
    // replaced with the default return value, 0.
    Symbol proc = symbolTable.GetScopeProc();
    if (!proc.IsValid()) {
        ReportWarning("Return statements in this scope are ignored");
        llvm::Constant *retVal = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, 0, true));
        llvmBuilder->CreateRet(retVal);
        Symbol expr = Expression(proc);
        return;
    }

    Symbol expr = Expression(proc);
    expr = AssignmentTypeCheck(proc, expr, token);
    if (!expr.IsValid()) {
        return;
    }

    llvmBuilder->CreateRet(expr.GetLLVMValue());
}

Symbol Parser::IndexArray(Symbol symbol)
{
    symbol.SetIsArrayIndexed(true);

    Symbol sym = Symbol();
    if (ValidateToken(T_LBRACKET))
    {
        ReportMissingTokenError("[");

        sym.SetIsValid(false);
        return sym;
    }

    sym.SetType(T_INTEGER);
    Symbol idx = Expression(sym);
    idx = AssignmentTypeCheck(sym, idx, token);

    if (idx.GetType() != T_INTEGER)
    {
        ReportError("Array index must be an integer.");
        symbol.SetIsValid(false);
        return symbol;
    }

    if (!symbol.IsArray())
    {
        ReportError("Indexing not supported for non-arrays");
        symbol.SetIsValid(false);
        return symbol;
    }

    if (!ValidateToken(T_RBRACKET))
    {
        ReportMissingTokenError("]");
        symbol.SetIsValid(false);
        return symbol;
    }

    llvm::Value *zero = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, 0, true));
    llvm::Value *greater = llvmBuilder->CreateICmpSGE(idx.GetLLVMValue(), zero);
    llvm::Value *less = llvmBuilder->CreateICmpSLT(idx.GetLLVMValue(), symbol.GetLLVMArraySize());
    llvm::Value *actualIdx = llvmBuilder->CreateAnd(greater, less);

    llvm::BasicBlock *fail = llvm::BasicBlock::Create(llvmContext, "", llvmCurrProc);
    llvm::BasicBlock *succ = llvm::BasicBlock::Create(llvmContext, "", llvmCurrProc);

    llvmBuilder->CreateCondBr(actualIdx, succ, fail);
    llvmBuilder->SetInsertPoint(fail);

    // TODO: Need to somehow call a function to quit running if there is an index error?

    llvmBuilder->CreateBr(succ);
    llvmBuilder->SetInsertPoint(succ);

    llvm::Value *address = nullptr;
    if (symbol.IsGlobal())
    {
        llvm::Value *vals[] = {zero, idx.GetLLVMValue()};
        address = llvmBuilder->CreateInBoundsGEP(symbol.GetLLVMArrayAddress(), vals);
    }
    else
    {
        address = llvmBuilder->CreateGEP(symbol.GetLLVMArrayAddress(), idx.GetLLVMValue());
    }

    symbol.SetLLVMAddress(address);

    return symbol;
}

Symbol Parser::AssignmentTypeCheck(Symbol dest, Symbol expr, token_t *token)
{
    if (dest.GetType() != expr.GetType())
    {
        bool isDiff = true;
        if (dest.GetType() == T_BOOL && expr.GetType() == T_INTEGER)
        {
            isDiff = false;
            expr.SetType(T_BOOL);
            ReportWarning("Converting int to bool");
            llvm::Value *val = ConvertIntToBool(expr.GetLLVMValue());
            expr.SetLLVMValue(val);
        }

        if (dest.GetType() == T_INTEGER)
        {
            if (expr.GetType() == T_BOOL)
            {
                expr.SetType(T_INTEGER);
                isDiff = false;
                ReportWarning("Converting bool to int");
                llvm::Value *val = llvmBuilder->CreateZExtOrTrunc(expr.GetLLVMValue(), llvmBuilder->getInt32Ty());
                expr.SetLLVMValue(val);
            }
            else if (expr.GetType() == T_FLOAT)
            {
                expr.SetType(T_INTEGER);
                isDiff = false;
                ReportWarning("Converting float to int");
                llvm::Value *val = llvmBuilder->CreateFPToSI(expr.GetLLVMValue(), llvmBuilder->getInt32Ty());
                expr.SetLLVMValue(val);
            }
        }

        if (dest.GetType() == T_FLOAT && expr.GetType() == T_INTEGER)
        {
            isDiff = false;
            ReportWarning("Converting int to float");
            expr.SetType(T_FLOAT);
            llvm::Value *val = llvmBuilder->CreateSIToFP(expr.GetLLVMValue(), llvmBuilder->getFloatTy());
            expr.SetLLVMValue(val);

        }

        if (isDiff)
        {
            ReportTypeMismatchError(TypeToString(dest.GetType()), TypeToString(expr.GetType()));
            expr.SetIsValid(false);
            return expr;
        }
    }

    return expr;
}

Symbol Parser::Destination()
{
    PrintDebugInfo("<destination>");

    std::string id = Identifier();
    Symbol dest = symbolTable.FindSymbol(id);

    if (!dest.IsValid())
    {
        ReportError("Symbol: " + id + " not found");
        return dest;
    }

    if (dest.GetDeclarationType() != T_VARIABLE)
    {
        ReportError("Variable required for valid destination");
        dest = Symbol();
        dest.SetIsValid(false);
        return dest;
    }

    if (ValidateToken(T_LBRACKET))
    {
        // get index
        dest = IndexArray(dest);
    }
    else
    {
        // TODO: no index was supplied; array unwrapping
    }

    // If we get here, the destination is valid and not an array
    return dest;
}

Symbol Parser::Expression(Symbol expectedType)
{
    PrintDebugInfo("<expression>");

    bool isNotOp = ValidateToken(T_NOT) ? true : false;

    Symbol arithOp = ArithOp(expectedType);
    token_t *op = scanner.PeekToken();
    Symbol expr_ = Expression_(expectedType);

    return ExpressionTypeCheck(expectedType, arithOp, expr_, op, isNotOp);
}

Symbol Parser::Expression_(Symbol expectedType)
{
    PrintDebugInfo("<expression_tail>");

    if (ValidateToken(T_AND) || ValidateToken(T_OR))
    {
        Symbol arithOp = ArithOp(expectedType);
        token_t *op = scanner.PeekToken();
        Symbol expr_ = Expression_(expectedType);

        return ExpressionTypeCheck(expectedType, arithOp, expr_, op, false);
    }

    Symbol sym = Symbol();
    sym.SetIsValid(false);

    return sym;
}

Symbol Parser::ExpressionTypeCheck(Symbol expectedType, Symbol arithOp, Symbol expr_, token_t *op, bool isNotOp)
{
    if (expr_.IsValid())
    {
        Symbol sym = Symbol();
        bool isInterop = false;
        std::string opStr;

        if (expectedType.GetType() == T_BOOL)
        {
            opStr = "logical";
            isInterop = (arithOp.GetType() == T_BOOL && expr_.GetType() == T_BOOL);
            sym.SetType(T_BOOL);
        }
        else if (expectedType.GetType() == T_INTEGER || expectedType.GetType() == T_FLOAT)
        {
            isInterop = (arithOp.GetType() == T_INTEGER && expr_.GetType() == T_INTEGER);
            opStr = "binary";
            sym.SetType(T_INTEGER);
        }
        else
        {
            ReportError("Invalid type");
            sym.SetIsValid(false);

            return sym;
        }

        if (!isInterop)
        {
            ReportOpTypeCheckError(opStr, TypeToString(arithOp.GetType()), TypeToString(expr_.GetType()));
            sym.SetIsValid(false);

            return sym;
        }

        llvm::Value *val;
        switch (op->type)
        {
            case T_AND:
                val = llvmBuilder->CreateAnd(arithOp.GetLLVMValue(), expr_.GetLLVMValue());
                break;
            case T_OR:
                val = llvmBuilder->CreateOr(arithOp.GetLLVMValue(), expr_.GetLLVMValue());
                break;
            default:
                std::cout << "error in codegen for operation" << std::endl; // TODO: fix msg
        }

        sym.SetLLVMValue(val);

        if (isNotOp)
        {
            val = llvmBuilder->CreateNot(sym.GetLLVMValue());
            sym.SetLLVMValue(val);
        }

        return sym;
    }
    else
    {
        if (isNotOp)
        {
            bool isInterop = false;
            if (expectedType.GetType() == T_BOOL)
            {
                isInterop = (arithOp.GetType() == T_BOOL);
            }
            else if (expectedType.GetType() == T_INTEGER || expectedType.GetType() == T_FLOAT)
            {
                isInterop = (arithOp.GetType() == T_INTEGER);
            }

            if (!isInterop)
            {
                ReportOpTypeCheckError("binary", TypeToString(arithOp.GetType()), "null");

                Symbol sym = Symbol();
                sym.SetIsValid(false);

                return sym;
            }

            llvm::Value *val = llvmBuilder->CreateNot(arithOp.GetLLVMValue());
            arithOp.SetLLVMValue(val);
        }

        return arithOp;
    }
}

Symbol Parser::ArithOp(Symbol expectedType)
{
    PrintDebugInfo("<arithOp>");

    Symbol rel = Relation(expectedType);
    token_t *op = scanner.PeekToken();
    Symbol arithOp_ = ArithOp_(expectedType);

    return ArithOpTypeCheck(expectedType, rel, arithOp_, op);
}

Symbol Parser::ArithOp_(Symbol expectedType)
{
    PrintDebugInfo("<arithOp_tail>");

    if (ValidateToken(T_ADD) || ValidateToken(T_SUBTRACT))
    {
        Symbol rel = Relation(expectedType);
        token_t *op = scanner.PeekToken();
        Symbol arithOp_ = ArithOp_(expectedType);

        return ArithOpTypeCheck(expectedType, rel, arithOp_, op);
    }

    Symbol sym = Symbol();
    sym.SetIsValid(false);

    return sym;
}

Symbol Parser::ArithOpTypeCheck(Symbol expectedType, Symbol rel, Symbol arithOp_, token_t *op)
{
    if (arithOp_.IsValid())
    {
        Symbol sym = Symbol();
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
            ReportOpTypeCheckError("arith", TypeToString(rel.GetType()), TypeToString(arithOp_.GetType()));
            sym.SetIsValid(false);

            return sym;
        }

        bool isFloatOp = (rel.GetType() == T_FLOAT || arithOp_.GetType() == T_FLOAT);

        llvm::Value *val;
        switch (op->type)
        {
            case T_ADD:
                if (isFloatOp)
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::FAdd, rel.GetLLVMValue(), arithOp_.GetLLVMValue());
                }
                else
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::Add, rel.GetLLVMValue(), arithOp_.GetLLVMValue());
                }
                break;
            case T_SUBTRACT:
                if (isFloatOp)
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::FSub, rel.GetLLVMValue(), arithOp_.GetLLVMValue());
                }
                else
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::Sub, rel.GetLLVMValue(), arithOp_.GetLLVMValue());
                }
                break;
            case T_MULTIPLY:
                if (isFloatOp)
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::FMul, rel.GetLLVMValue(), arithOp_.GetLLVMValue());
                }
                else
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::Mul, rel.GetLLVMValue(), arithOp_.GetLLVMValue());
                }
                break;
            case T_DIVIDE:
                if (isFloatOp)
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::FDiv, rel.GetLLVMValue(), arithOp_.GetLLVMValue());
                }
                else
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::SDiv, rel.GetLLVMValue(), arithOp_.GetLLVMValue());
                }
                break;
            default:
                std::cout << "Error finding operation" << std::endl; // todo: fix msg
        }

        sym.SetLLVMValue(val);

        if (expectedType.GetType() == T_FLOAT)
        {
            sym.SetType(T_FLOAT);
            if (!isFloatOp)
            {
                llvm::Value *val = llvmBuilder->CreateSIToFP(sym.GetLLVMValue(), llvmBuilder->getFloatTy());
                sym.SetLLVMValue(val);
            }
        }
        else if (expectedType.GetType() == T_INTEGER)
        {
            sym.SetType(T_INTEGER);
            if (isFloatOp)
            {
                llvm::Value *val = llvmBuilder->CreateSIToFP(sym.GetLLVMValue(), llvmBuilder->getInt32Ty());
                sym.SetLLVMValue(val);
            }
        }
        else
        {
            if (rel.GetType() == T_FLOAT || arithOp_.GetType() == T_FLOAT)
            {
                sym.SetType(T_FLOAT);
            }
            else
            {
                sym.SetType(T_INTEGER);
            }
        }

        return sym;
    }
    else
    {
        return rel;
    }
}

Symbol Parser::Relation(Symbol expectedType)
{
    PrintDebugInfo("<relation>");

    Symbol term = Term(expectedType);
    token_t *op = scanner.PeekToken();
    Symbol relation_ = Relation_(expectedType);

    return RelationTypeCheck(expectedType, term, relation_, op);
}

Symbol Parser::Relation_(Symbol expectedType)
{
    PrintDebugInfo("<relation_tail>");

    if (ValidateToken(T_LESSTHAN) ||
        ValidateToken(T_GREATERTHAN) ||
        ValidateToken(T_LTEQ) ||
        ValidateToken(T_GTEQ) ||
        ValidateToken(T_EQEQ) ||
        ValidateToken(T_NOTEQ))
    {
        Symbol term = Term(expectedType);
        token_t *op = scanner.PeekToken();
        Symbol relation_ = Relation_(expectedType);

        return RelationTypeCheck(expectedType, term, relation_, op);
    }

    Symbol sym = Symbol();
    sym.SetIsValid(false);

    return sym;
}

Symbol Parser::RelationTypeCheck(Symbol expectedType, Symbol term, Symbol relation_, token_t *op)
{
    if (relation_.IsValid())
    {
        Symbol sym = Symbol();
        bool isInterop = false;
        bool isFloatOp = false;

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
            ReportOpTypeCheckError("relational", TypeToString(term.GetType()), TypeToString(relation_.GetType()));
            sym.SetIsValid(false);
            return sym;
        }

        llvm::Value *val ;
        if (term.GetType() == T_STRING)
        {
            llvm::Value *idxAddr = llvmBuilder->CreateAlloca(llvmBuilder->getInt32Ty());
            llvm::Value *idx = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, 0, true));
            llvmBuilder->CreateStore(idx, idxAddr);

            llvm::BasicBlock *strCmpBlock = llvm::BasicBlock::Create(llvmContext, "", llvmCurrProc);
            llvm::BasicBlock *strCmpBlockEnd = llvm::BasicBlock::Create(llvmContext, "", llvmCurrProc);

            llvmBuilder->CreateBr(strCmpBlock);
            llvmBuilder->SetInsertPoint(strCmpBlock);

            llvm::Value *inc = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, 1, true));
            idx = llvmBuilder->CreateLoad(llvmBuilder->getInt32Ty(), idxAddr);
            idx = llvmBuilder->CreateBinOp(llvm::Instruction::Add, idx, inc);

            llvmBuilder->CreateStore(idx, idxAddr);

            llvm::Value *termAddr = llvmBuilder->CreateGEP(term.GetLLVMValue(), idx);
            llvm::Value *termChar = llvmBuilder->CreateLoad(llvmBuilder->getInt8Ty(), termAddr);
            llvm::Value *tailAddr = llvmBuilder->CreateGEP(relation_.GetLLVMValue(), idx);
            llvm::Value *tailChar = llvmBuilder->CreateLoad(llvmBuilder->getInt8Ty(), tailAddr);
            llvm::Value *cmp = llvmBuilder->CreateICmpEQ(termChar, tailChar);

            llvm::Value *zero = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt8Ty(), llvm::APInt(8, 0, true));
            llvm::Value *noEnd = llvmBuilder->CreateICmpNE(termChar, zero);
            llvm::Value *cont = llvmBuilder->CreateAnd(cmp, noEnd);

            llvmBuilder->CreateCondBr(cont, strCmpBlock, strCmpBlockEnd);
            llvmBuilder->SetInsertPoint(strCmpBlockEnd);

            if (op->type == T_EQEQ)
            {
                val = cmp;
            }
            else
            {
                val = llvmBuilder->CreateNot(cmp);
            }
        }
        else
        {
            switch (op->type)
            {
                case T_EQEQ:
                    if (isFloatOp)
                    {

                        val = llvmBuilder->CreateFCmpOEQ(term.GetLLVMValue(), relation_.GetLLVMValue());
                    }
                    else
                    {
                        val = llvmBuilder->CreateICmpEQ(term.GetLLVMValue(), relation_.GetLLVMValue());
                    }
                    break;
                case T_NOTEQ:
                    if (isFloatOp)
                    {

                        val = llvmBuilder->CreateFCmpONE(term.GetLLVMValue(), relation_.GetLLVMValue());
                    }
                    else
                    {
                        val = llvmBuilder->CreateICmpNE(term.GetLLVMValue(), relation_.GetLLVMValue());
                    }
                    break;
                case T_LESSTHAN:
                    if (isFloatOp)
                    {

                        val = llvmBuilder->CreateFCmpOLT(term.GetLLVMValue(), relation_.GetLLVMValue());
                    }
                    else
                    {
                        val = llvmBuilder->CreateICmpSLT(term.GetLLVMValue(), relation_.GetLLVMValue());
                    }
                    break;
                case T_LTEQ:
                    if (isFloatOp)
                    {
                        val = llvmBuilder->CreateFCmpOLE(term.GetLLVMValue(), relation_.GetLLVMValue());
                    }
                    else
                    {
                        val = llvmBuilder->CreateICmpSLE(term.GetLLVMValue(), relation_.GetLLVMValue());
                    }
                    break;
                case T_GREATERTHAN:
                    if (isFloatOp)
                    {

                        val = llvmBuilder->CreateFCmpOGT(term.GetLLVMValue(), relation_.GetLLVMValue());
                    }
                    else
                    {
                        val = llvmBuilder->CreateICmpSGT(term.GetLLVMValue(), relation_.GetLLVMValue());
                    }
                    break;
                case T_GTEQ:
                    if (isFloatOp)
                    {

                        val = llvmBuilder->CreateFCmpOGE(term.GetLLVMValue(), relation_.GetLLVMValue());
                    }
                    else
                    {
                        val = llvmBuilder->CreateICmpSGE(term.GetLLVMValue(), relation_.GetLLVMValue());
                    }
                    break;
                default:
                    std::cout << "Operation error in codegen" << std::endl; // todo: fix msg
            }
        }

        sym.SetLLVMValue(val);
        sym.SetType(T_BOOL);

        return sym;
    }
    else
    {
        return term;
    }
}

Symbol Parser::Term(Symbol expectedType)
{
    PrintDebugInfo("<term>");

    Symbol factor = Factor(expectedType);
    token_t *op = scanner.PeekToken();
    Symbol termTail = Term_(expectedType);

    return ArithOpTypeCheck(expectedType, factor, termTail, op);
}

Symbol Parser::Term_(Symbol expectedType)
{
    PrintDebugInfo("<term_tail>");

    if (ValidateToken(T_DIVIDE) || ValidateToken(T_MULTIPLY))
    {
        Symbol factor = Factor(expectedType);
        token_t *op = scanner.PeekToken();
        Symbol termTail = Term_(expectedType);

        return ArithOpTypeCheck(expectedType, factor, termTail, op);
    }

    Symbol sym = Symbol();
    sym.SetIsValid(false);

    return sym;
}

Symbol Parser::Factor(Symbol expectedType)
{
    PrintDebugInfo("<factor>");

    Symbol sym = Symbol();
    if (ValidateToken(T_LPAREN))
    {
        sym = Expression(expectedType);
        if (!ValidateToken(T_RPAREN))
        {
            ReportMissingTokenError(")");
            sym.SetIsValid(false);
            return sym;
        }
    }
    else if (ValidateToken(T_IDENTIFIER))
    {
        sym = ProcedureCallOrName();
    }
    else if (ValidateToken(T_SUBTRACT))
    {
        if (ValidateToken(T_INT_LITERAL) || (ValidateToken(T_FLOAT_LITERAL)))
        {
            sym = Number();
        }
        else if (ValidateToken(T_IDENTIFIER))
        {
            sym = ProcedureCallOrName();
        }
        else
        {
            ReportError("Expected an identifier or number literal");
            sym.SetIsValid(false);
            return sym;
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
        sym = Number();
    }
    else if (ValidateToken(T_STRING_LITERAL))
    {
        sym = String();
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
        return sym;
    }

    return sym;
}

Symbol Parser::ProcedureCallOrName()
{
    PrintDebugInfo("<proc_call/name>");

    Symbol sym = Symbol();
    std::string id = token->val.stringValue;
    sym = symbolTable.FindSymbol(id);

    if (!sym.IsValid())
    {
        ReportError("Symbol not found: " + id);
        return sym;
    }

    if (ValidateToken(T_LPAREN))
    {
        // procedure
        if (sym.GetDeclarationType() != T_PROCEDURE)
        {
            ReportError("Cannot call non procedure type");
            sym = Symbol();
            sym.SetIsValid(false);

            return sym;
        }

        Symbol procSym = sym;
        sym = Symbol();
        sym.SetType(procSym.GetType());

        token_t *tmp = scanner.PeekToken();
        if (tmp->type == T_NOT || tmp->type == T_LPAREN ||
            tmp->type == T_SUBTRACT || tmp->type == T_INT_LITERAL ||
            tmp->type == T_FLOAT_LITERAL || tmp->type == T_IDENTIFIER ||
            tmp->type == T_STRING_LITERAL || tmp->type == T_TRUE ||
            tmp->type == T_FALSE)
        {
            std::vector<llvm::Value *> arguments = ArgumentList(procSym.GetParameters().begin(), procSym.GetParameters().end());
            llvm::Value *val = llvmBuilder->CreateCall(procSym.GetLLVMFunction(), arguments);
            sym.SetLLVMValue(val);
        }
        else
        {
            if (!procSym.GetParameters().empty())
            {
                ReportError("Missing arguments for procedure");
                sym.SetIsValid(false);

                return sym;
            }

            llvm::Value *val = llvmBuilder->CreateCall(procSym.GetLLVMFunction());
            sym.SetLLVMValue(val);
        }

        if (!ValidateToken(T_RPAREN))
        {
            ReportMissingTokenError(")");
            sym.SetIsValid(false);
            return sym;
        }
    }
    else
    {
        // name
        if (sym.GetDeclarationType() != T_VARIABLE)
        {
            ReportError("Name must be a variable");
            sym.SetIsValid(false);

            return sym;
        }

        if (ValidateToken(T_LBRACKET))
        {
            if (sym.GetDeclarationType() != T_VARIABLE)
            {
                ReportError("Indexing is not supported for non-arrays.");
                sym = Symbol();
                sym.SetIsValid(false);
                return sym;
            }

            sym = IndexArray(sym);
        }
//        else if (unwrapArray)
//        {
            // TODO: Array unwrapping
//        }
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

                return sym;
            }

            if (sym.IsArray() && !sym.IsArrayIndexed())
            {
                // TODO: copy array into the val?
            }
            else
            {
                llvm::Value *val = llvmBuilder->CreateLoad(GetLLVMType(sym), sym.GetLLVMAddress());
                sym.SetLLVMValue(val);
            }
        }
    }

    return sym;
}

Symbol Parser::Number()
{
    PrintDebugInfo("<number>");

    Symbol sym = Symbol();
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

    return sym;
}

Symbol Parser::String()
{
    PrintDebugInfo("<string>");

    Symbol sym = Symbol();
    if (token->type != T_STRING_LITERAL)
    {
        ReportError("String literal expected.");
        return sym;
    }

    sym.SetType(T_STRING);
    llvm::Value *val = llvmBuilder->CreateGlobalStringPtr(token->val.stringValue);
    sym.SetLLVMValue(val);

    return sym;
}

std::vector<llvm::Value *> Parser::ArgumentList(std::vector<Symbol>::iterator curr, std::vector<Symbol>::iterator end)
{
    PrintDebugInfo("<argument_list>");

    bool continue_ = false;
    std::vector<llvm::Value *> arguments;
    do {
        if (curr == end)
        {
            ReportError("Too many arguments");
            return arguments;
        }

        Symbol expr = Expression(*curr);
        expr = AssignmentTypeCheck(*curr, expr, token);

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
            ReportError("Invalid argument: array not expected");
            return arguments;
        }


        if (!expr.IsValid())
        {
            return arguments;
        }

        if (expr.IsArray() && !expr.IsArrayIndexed())
        {
            if (expr.IsGlobal())
            {
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

    } while (continue_);

    return arguments;
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

llvm::Value *Parser::ConvertIntToBool(llvm::Value *intVal)
{
    llvm::Value *zero = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, 0, true));
    llvm::Value *cmp = llvmBuilder->CreateICmpEQ(intVal, zero);

    llvm::Value *true_ = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt1Ty(), llvm::APInt(1, 1, true));
    llvm::Value *false_ = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt1Ty(), llvm::APInt(1, 0, true));

    llvm::Value *boolVal = llvmBuilder->CreateSelect(cmp, false_, true_);

    return boolVal;
}
