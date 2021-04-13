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
        //ReportUnusedVars(); // Todo: WIP
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
    }
    else
    {
        localScopes.back()[symbol.GetId()] = symbol;
    }
}

void SymbolTable::SetScopeProc(Symbol proc)
{
    localScopes.back()["_proc"] = proc;
}

Symbol SymbolTable::GetScopeProc()
{
    std::map<std::string, Symbol>::iterator it = localScopes.back().find("_proc");
    if (it != localScopes.back().end())
    {
        return it->second;
    }
    else
    {
        Symbol symbol;
        symbol.SetIsValid(false);
        return symbol;
    }
}

bool SymbolTable::DoesSymbolExist(std::string id)
{
    std::map<std::string, Symbol>::iterator it = localScopes.back().find(id);
    if (it != localScopes.back().end())
    {
        return true;
    }
    else if (scopeCount != 0)
    {
        return false;
    }

    it = globalScope.find(id);
    if (it != globalScope.end())
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

void SymbolTable::AddIOFunctions(llvm::Module *llvmModule, llvm::LLVMContext &llvmContext, llvm::IRBuilder<> *llvmBuilder)
{
    llvm::FunctionType *type = nullptr;
    llvm::Function *procedure = nullptr;

    Symbol putInteger;
    putInteger.SetId("PUTINTEGER");
    putInteger.SetType(T_BOOL);
    Symbol putIntegerArgs;
    putIntegerArgs.SetId("num");
    putIntegerArgs.SetType(T_INTEGER);
    putIntegerArgs.SetDeclarationType(T_VARIABLE);
    putInteger.GetParameters().push_back(putIntegerArgs);
    putInteger.SetIsGlobal(true);
    putInteger.SetDeclarationType(T_PROCEDURE);
    type = llvm::FunctionType::get(llvmBuilder->getInt1Ty(),{llvmBuilder->getInt32Ty()},false);
    procedure = llvm::Function::Create(type,llvm::Function::ExternalLinkage,putInteger.GetId(),llvmModule);
    putInteger.SetLLVMFunction(procedure);
    AddSymbol(putInteger);

    Symbol getInteger;
    getInteger.SetId("GETINTEGER");
    getInteger.SetType(T_INTEGER);
    getInteger.SetIsGlobal(true);
    getInteger.SetDeclarationType(T_PROCEDURE);
    type = llvm::FunctionType::get(llvmBuilder->getInt32Ty(), {},false);
    procedure = llvm::Function::Create(type, llvm::Function::ExternalLinkage, getInteger.GetId(), llvmModule);
    getInteger.SetLLVMFunction(procedure);
    AddSymbol(getInteger);

    Symbol putFloat;
    putFloat.SetId("PUTFLOAT");
    putFloat.SetType(T_BOOL);
    Symbol putFloatArgs;
    putFloatArgs.SetId("num");
    putFloatArgs.SetType(T_FLOAT);
    putFloatArgs.SetDeclarationType(T_VARIABLE);
    putFloat.GetParameters().push_back(putFloatArgs);
    putFloat.SetIsGlobal(true);
    putFloat.SetDeclarationType(T_PROCEDURE);
    type = llvm::FunctionType::get(llvmBuilder->getInt1Ty(),{llvmBuilder->getFloatTy()},false);
    procedure = llvm::Function::Create(type,llvm::Function::ExternalLinkage,putFloat.GetId(),llvmModule);
    putFloat.SetLLVMFunction(procedure);
    AddSymbol(putFloat);

    Symbol getFloat;
    getFloat.SetId("GETFLOAT");
    getFloat.SetType(T_FLOAT);
    getFloat.SetIsGlobal(true);
    getInteger.SetDeclarationType(T_PROCEDURE);
    type = llvm::FunctionType::get(llvmBuilder->getFloatTy(), {},false);
    procedure = llvm::Function::Create(type, llvm::Function::ExternalLinkage, getFloat.GetId(), llvmModule);
    getFloat.SetLLVMFunction(procedure);
    AddSymbol(getFloat);

    Symbol putBool;
    putBool.SetId("PUTBOOL");
    putBool.SetType(T_BOOL);
    Symbol putBoolArgs;
    putBoolArgs.SetId("num");
    putBoolArgs.SetType(T_BOOL);
    putBoolArgs.SetDeclarationType(T_VARIABLE);
    putBool.GetParameters().push_back(putBoolArgs);
    putBool.SetIsGlobal(true);
    putBool.SetDeclarationType(T_PROCEDURE);
    type = llvm::FunctionType::get(llvmBuilder->getInt1Ty(),{llvmBuilder->getInt1Ty()},false);
    procedure = llvm::Function::Create(type,llvm::Function::ExternalLinkage, putBool.GetId(), llvmModule);
    putBool.SetLLVMFunction(procedure);
    AddSymbol(putBool);

    Symbol getBool;
    getBool.SetId("GETBOOL");
    getBool.SetType(T_BOOL);
    getBool.SetIsGlobal(true);
    getBool.SetDeclarationType(T_PROCEDURE);
    type = llvm::FunctionType::get(llvmBuilder->getInt1Ty(), {},false);
    procedure = llvm::Function::Create(type, llvm::Function::ExternalLinkage, getBool.GetId(), llvmModule);
    getBool.SetLLVMFunction(procedure);
    AddSymbol(getBool);

    Symbol putString;
    putString.SetId("PUTSTRING");
    putString.SetType(T_BOOL);
    Symbol putStringArgs;
    putStringArgs.SetId("str");
    putStringArgs.SetType(T_STRING);
    putStringArgs.SetDeclarationType(T_VARIABLE);
    putString.GetParameters().push_back(putStringArgs);
    putString.SetIsGlobal(true);
    putString.SetDeclarationType(T_PROCEDURE);
    type = llvm::FunctionType::get(llvmBuilder->getInt1Ty(),{llvmBuilder->getInt8PtrTy()},false);
    procedure = llvm::Function::Create(type, llvm::Function::ExternalLinkage, putString.GetId(), llvmModule);
    putString.SetLLVMFunction(procedure);
    AddSymbol(putString);

    Symbol getString;
    getString.SetId("GETSTRING");
    getString.SetType(T_STRING);
    getString.SetIsGlobal(true);
    getString.SetDeclarationType(T_PROCEDURE);
    type = llvm::FunctionType::get(llvmBuilder->getInt8PtrTy(), {},false);
    procedure = llvm::Function::Create(type, llvm::Function::ExternalLinkage, getString.GetId(), llvmModule);
    getString.SetLLVMFunction(procedure);
    AddSymbol(getString);

    Symbol sqrt;
    sqrt.SetId("SQRT");
    sqrt.SetType(T_FLOAT);
    Symbol args;
    args.SetId("num");
    args.SetType(T_INTEGER);
    args.SetDeclarationType(T_VARIABLE);
    sqrt.GetParameters().push_back(args);
    sqrt.SetIsGlobal(true);
    sqrt.SetDeclarationType(T_PROCEDURE);
    type = llvm::FunctionType::get(llvmBuilder->getFloatTy(), {llvmBuilder->getInt32Ty()}, false);
    procedure = llvm::Function::Create(type, llvm::Function::ExternalLinkage, sqrt.GetId(), llvmModule);
    sqrt.SetLLVMFunction(procedure);
    AddSymbol(sqrt);

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

//void SymbolTable::ReportUnusedVars()
//{
//    for (auto &it : GetLocalScope())
//    {
//        if (!it.second.IsUsed() && it.first != "_proc")
//        {
//            std::cout << "Warning: Variable " << it.second.GetId() << " is declared but never used" << std::endl;
//        }
//    }
//}
