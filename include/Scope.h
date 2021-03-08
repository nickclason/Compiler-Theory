//
// Created by Nick Clason on 2/11/21.
//

#ifndef COMPILER_THEORY_SCOPE_H
#define COMPILER_THEORY_SCOPE_H

#include "Symbol.h"

#include <map>

class Scope
{
public:
    Scope();
    ~Scope();

    Scope* prevScope;

    // For debug
    void SetName(std::string id);
    void PrintScope();

    bool AddSymbol(Symbol symbol);
    bool DoesSymbolExist(std::string id, bool isGlobal);

    Symbol GetSymbol(std::string id);

    std::map<std::string, Symbol> GetLocalScope();

private:
    std::map<std::string, Symbol> globalScope;
    std::map<std::string, Symbol> localScope;

    std::string scopeID; // mostly for debugging
};

#endif //COMPILER_THEORY_SCOPE_H
