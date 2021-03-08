//
// Created by Nick Clason on 2/2/21.
//

#ifndef COMPILER_THEORY_SYMBOLTABLE_H
#define COMPILER_THEORY_SYMBOLTABLE_H

#include "Scope.h"
#include "Symbol.h"

#include <map>
#include <string>
#include <vector>

class SymbolTable {

    public:
        SymbolTable();
        ~SymbolTable();

        void AddScope();
        void ExitScope();

        bool AddSymbol(std::string id, Symbol symbol, bool isGlobal);
        bool AddSymbolToPrevScope(std::string id, Symbol symbol, bool isGlobal);
        bool DoesSymbolExist(std::string id, Symbol &symbol, bool &isGlobal);

        void ChangeScopeName(std::string id);
        std::map<std::string, Symbol> GetLocalScope();

    private:
        Scope* curr;
        Scope* prev;
        Scope* top;
};

#endif //COMPILER_THEORY_SYMBOLTABLE_H
