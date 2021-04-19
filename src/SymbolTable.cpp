//
// Created by Nick Clason on 2/2/21.
//

#include "../include/definitions.h"
#include "../include/SymbolTable.h"


SymbolTable::SymbolTable()
{
    scopeCount = -1;
}

SymbolTable::~SymbolTable()=default;

void SymbolTable::AddScope()
{
    localScopes.push_back(std::map<std::string, Symbol>());
    scopeCount++;
}

void SymbolTable::RemoveScope()
{
    if (localScopes.size() > 0)
    {
        localScopes.pop_back();
    }
    else
    {
        std::cout << "Error: No scope to remove" << std::endl;
    }


    scopeCount--;
}

void SymbolTable::AddSymbol(Symbol &symbol)
{
    if (symbol.IsGlobal())
    {
        globalScope[symbol.GetId()] = symbol;
        return;
    }

    localScopes.back()[symbol.GetId()] = symbol;
}

void SymbolTable::SetScopeProc(Symbol proc)
{
    localScopes.back()["_procedure"] = proc;
}

Symbol SymbolTable::GetScopeProc()
{
    if (localScopes.back().find("_procedure") != localScopes.back().end())
    {
        return localScopes.back().find("_procedure")->second;
    }
    else
    {
        Symbol symbol = Symbol();
        symbol.SetIsValid(false);
        return symbol;
    }
}

bool SymbolTable::DoesSymbolExist(std::string id)
{
    if (localScopes.back().find(id) != localScopes.back().end())
    {
        return true;
    }
    else if (scopeCount != 0) // TODO: this is concerning... lets me redefine get/put methods
    {
        return false;
    }

    if (globalScope.find(id) != globalScope.end())
    {
        return true;
    }

    if (id.compare("main") == 0)
    {
        return true;
    }

    return false;
}

Symbol SymbolTable::FindSymbol(std::string id) {
    std::map<std::string, Symbol>::iterator it = localScopes.back().find(id);
    if (it != localScopes.back().end())
    {
        return it->second;
    }

    it = globalScope.find(id);
    if (it != globalScope.end())
    {
        return it->second;
    }
    Symbol symbol;
    symbol.SetIsValid(false);;
    return symbol;
}

std::map<std::string, Symbol> SymbolTable::GetLocalScope()
{
    return localScopes.back();
}

void SymbolTable::AddIOFunctions(llvm::Module *llvmModule, llvm::IRBuilder<> *llvmBuilder)
{
    llvm::FunctionType *type = nullptr;
    llvm::Function *procedure = nullptr;
    Symbol args;

    // putInteger
    args = Symbol();
    args.SetId("num");
    args.SetType(T_INTEGER);
    args.SetDeclarationType(T_VARIABLE);
    Symbol putInt = GeneratePutSymbol("PUTINTEGER", T_BOOL, args, llvmModule, llvmBuilder,{llvmBuilder->getInt32Ty()});
    AddSymbol(putInt);

    // getInteger
    Symbol getInt = GenerateGetSymbol("GETINTEGER", T_INTEGER, llvmModule, llvmBuilder, llvmBuilder->getInt32Ty());
    AddSymbol(getInt);

    // putFloat
    args = Symbol();
    args.SetId("num");
    args.SetType(T_FLOAT);
    args.SetDeclarationType(T_VARIABLE);
    Symbol putFloat = GeneratePutSymbol("PUTFLOAT", T_BOOL, args, llvmModule, llvmBuilder, {llvmBuilder->getFloatTy()});
    AddSymbol(putFloat);

    // getFloat
    Symbol getFloat = GenerateGetSymbol("GETFLOAT", T_FLOAT, llvmModule, llvmBuilder, llvmBuilder->getFloatTy());
    AddSymbol(getFloat);

    // putBool
    args = Symbol();
    args.SetId("num");
    args.SetType(T_BOOL);
    args.SetDeclarationType(T_VARIABLE);
    Symbol putBool = GeneratePutSymbol("PUTBOOL", T_BOOL, args, llvmModule, llvmBuilder, {llvmBuilder->getInt1Ty()});
    AddSymbol(putBool);

    // getBool
    Symbol getBool = GenerateGetSymbol("GETBOOL", T_BOOL, llvmModule, llvmBuilder, llvmBuilder->getInt1Ty());
    AddSymbol(getBool);

    // putString
    args = Symbol();
    args.SetId("str");
    args.SetType(T_STRING);
    args.SetDeclarationType(T_VARIABLE);
    Symbol putString = GeneratePutSymbol("PUTSTRING", T_BOOL, args, llvmModule, llvmBuilder, {llvmBuilder->getInt8PtrTy()});
    AddSymbol(putString);

    // getString
    Symbol getString = GenerateGetSymbol("GETSTRING", T_BOOL, llvmModule, llvmBuilder, llvmBuilder->getInt8PtrTy());
    AddSymbol(getString);

    // sqrt
    args = Symbol();
    args.SetId("num");
    args.SetType(T_INTEGER);
    args.SetDeclarationType(T_VARIABLE);
    Symbol sqrt = GeneratePutSymbol("SQRT", T_FLOAT, args, llvmModule, llvmBuilder, {llvmBuilder->getInt32Ty()});
    AddSymbol(sqrt);

    // Array out-of-bounds error
    Symbol oobError;
    oobError.SetId("OOB_ERROR");
    oobError.SetType(T_BOOL);
    oobError.SetIsGlobal(true);
    oobError.SetDeclarationType(T_PROCEDURE);
    type = llvm::FunctionType::get(llvmBuilder->getVoidTy(), {}, false);
    procedure = llvm::Function::Create(type, llvm::Function::ExternalLinkage, oobError.GetId(), llvmModule);
    oobError.SetLLVMFunction(procedure);
    AddSymbol(oobError);
}

int SymbolTable::GetScopeCount()
{
    return scopeCount;
}

Symbol SymbolTable::GeneratePutSymbol(std::string id, int type, Symbol args, llvm::Module *llvmModule,
                                      llvm::IRBuilder<> *llvmBuilder, llvm::ArrayRef<llvm::Type *> llvmArgType)
{
    llvm::FunctionType *llvmType = nullptr;
    llvm::Function *procedure = nullptr;

    Symbol put;
    put.SetId(id);
    put.SetType(type);

    put.GetParameters().push_back(args);
    put.SetIsGlobal(true);
    put.SetDeclarationType(T_PROCEDURE);

    if (id == "SQRT")
    {
        llvmType = llvm::FunctionType::get(llvmBuilder->getFloatTy(), llvmArgType, false);
    }
    else
    {
        llvmType = llvm::FunctionType::get(llvmBuilder->getInt1Ty(), llvmArgType, false);
    }
    procedure = llvm::Function::Create(llvmType, llvm::Function::ExternalLinkage, put.GetId(), llvmModule);
    put.SetLLVMFunction(procedure);

    return put;
}

Symbol SymbolTable::GenerateGetSymbol(std::string id, int type, llvm::Module *llvmModule,
                                      llvm::IRBuilder<> *llvmBuilder, llvm::Type *llvmTy)
{
    llvm::FunctionType *llvmType = nullptr;
    llvm::Function *procedure = nullptr;

    Symbol get;
    get.SetId(id);
    get.SetType(type);
    get.SetIsGlobal(true);
    get.SetDeclarationType(T_PROCEDURE);
    llvmType = llvm::FunctionType::get(llvmTy, {},false);
    procedure = llvm::Function::Create(llvmType, llvm::Function::ExternalLinkage, get.GetId(), llvmModule);
    get.SetLLVMFunction(procedure);

    return get;
}