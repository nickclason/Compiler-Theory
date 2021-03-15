//
// Created by Nick Clason on 2/2/21.
//

#include "../include/definitions.h"
#include "../include/SymbolTable.h"


SymbolTable::SymbolTable()
{
    scopeCount = -1;
}

SymbolTable::~SymbolTable()
{

}

void SymbolTable::AddScope() {
    localScopes.push_back(std::map<std::string, Symbol>());
    scopeCount++;
}

void SymbolTable::RemoveScope() {
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

void SymbolTable::AddSymbol(Symbol &symbol) {
    // Debug only
    std::cout << "\tInserting symbol: ";
    symbol.PrintDebugInfo();

    if (symbol.IsGlobal())
    {
        globalScope[symbol.GetId()] = symbol;
    }
    else
    {
        localScopes.back()[symbol.GetId()] = symbol;
    }
}

void SymbolTable::SetScopeProc(Symbol proc) {
    localScopes.back()["_proc"] = proc;
}

Symbol SymbolTable::GetScopeProc() {
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

bool SymbolTable::DoesSymbolExist(std::string id) {
    std::map<std::string, Symbol>::iterator it = localScopes.back().find(id);
    if (it != localScopes.back().end())
    {
        return true;
    }
    else if (scopeCount != 0)
    {
        // This might solve the issue of variables that were prefixed with the global keyword not being able
        // to be redefined in a local scope
        return false;
    }

    it = globalScope.find(id);
    if (it != globalScope.end()) // TODO: global variables can be overridden in local scopes, need to fix
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
        // Debug
        std::cout << "Found the following symbol:" << std::endl;
        it->second.PrintDebugInfo();

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

std::map<std::string, Symbol> SymbolTable::GetLocalScope() {
    return localScopes.back();
}

void SymbolTable::AddIOFunctions(llvm::Module *llvmModule, llvm::LLVMContext &llvmContext, llvm::IRBuilder<> *llvmBuilder)
{
    // putInteger(integer value) : bool
    Symbol put_integer;
    put_integer.SetId("PUTINTEGER");
    put_integer.SetType(T_BOOL);
    Symbol put_integer_arg;
    put_integer_arg.SetId("value");
    put_integer_arg.SetType(T_INTEGER);
    put_integer_arg.SetDeclarationType(T_VARIABLE);
    put_integer.GetParameters().push_back(put_integer_arg);
    put_integer.SetIsGlobal(true);
    put_integer.SetDeclarationType(T_PROCEDURE);

    llvm::FunctionType *function_type = llvm::FunctionType::get(
            llvmBuilder->getInt1Ty(),
            {llvmBuilder->getInt32Ty()},
            false);
    llvm::Function *procedure = llvm::Function::Create(
            function_type,
            llvm::Function::ExternalLinkage,
            put_integer.GetId(),
            llvmModule);
    put_integer.SetLLVMFunction(procedure);

    AddSymbol(put_integer);
}

int SymbolTable::GetScopeCount()
{
    return scopeCount;
}
