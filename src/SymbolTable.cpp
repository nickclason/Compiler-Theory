//
// Created by Nick Clason on 2/2/21.
//

#include "../include/definitions.h"
#include "../include/SymbolTable.h"


SymbolTable::SymbolTable() {

}

SymbolTable::~SymbolTable() {

}

void SymbolTable::AddScope() {
    localScopes.push_back(std::map<std::string, Symbol>());
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
}
