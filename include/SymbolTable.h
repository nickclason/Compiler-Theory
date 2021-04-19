//
// Created by Nick Clason on 2/2/21.
//

#ifndef COMPILER_THEORY_SYMBOLTABLE_H
#define COMPILER_THEORY_SYMBOLTABLE_H

#include "Symbol.h"

#include <map>
#include <string>
#include <vector>

#include <llvm/IR/IRBuilder.h>

class SymbolTable {

public:

    SymbolTable();
    ~SymbolTable();

    void AddScope();
    void RemoveScope();

    void AddSymbol(Symbol &symbol);

    void SetScopeProc(Symbol proc);
    Symbol GetScopeProc();

    bool DoesSymbolExist(std::string id);

    Symbol FindSymbol(std::string id);

    std::map<std::string, Symbol> GetLocalScope();

    void AddIOFunctions(llvm::Module *llvmModule, llvm::IRBuilder<> * llvmBuilder);

    int GetScopeCount();

private:
    std::map<std::string, Symbol> globalScope;
    std::vector<std::map<std::string, Symbol> > localScopes;

    Symbol GeneratePutSymbol(std::string id, int type, Symbol args, llvm::Module *llvmModule,
                             llvm::IRBuilder<> *llvmBuilder, llvm::ArrayRef<llvm::Type *> llvmArgType);

    Symbol GenerateGetSymbol(std::string id, int type, llvm::Module *llvmModule,
                             llvm::IRBuilder<> *llvmBuilder, llvm::Type *llvmTy);

    int scopeCount;
};

#endif //COMPILER_THEORY_SYMBOLTABLE_H
