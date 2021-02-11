//
// Created by Nick Clason on 2/2/21.
//

#include "../include/definitions.h"
#include "../include/SymbolTable.h"

SymbolTable::SymbolTable()
{
    scope = 0;
}

void SymbolTable::AddScope()
{
    if (scopes.size() > 0)
    {
        scope++;
    }

    std::map<std::string, Symbol> newScope;
    scopes.push_back(newScope);
}

void SymbolTable::RemoveScope()
{
    scopes.pop_back();
    scope--;
}

bool SymbolTable::AddSymbol(Symbol symbol)
{
    it = scopes[0].find(symbol.id); // Look for this symbol in the Global scope first
    if (it != scopes[0].end() && scopes[0][symbol.id].isGlobal)
    {
        // This symbol has already been defined in the global scope
        // therefore we cannot add it again in the same scope.
        //
        return false;
    }
    else
    {
        it = scopes.back().find(symbol.id);
        if (it != scopes.back().end())
        {
            // This symbol has already been defined in the current scope
            // and cannot be defined again within the same scope.
            //
            return false;
        }
        else
        {
            // If we get here, then the symbol can be defined for the current scope
            //
            scopes.back().insert(std::pair<std::string, Symbol>(symbol.id, symbol));
            return true;
        }
    }

}

bool SymbolTable::AddSymbolToScope(Symbol symbol, int scope)
{
    it = scopes[0].find(symbol.id); // Look for this symbol in the Global scope first
    if (it != scopes[0].end() && scopes[0][symbol.id].isGlobal)
    {
        // This symbol has already been defined in the global scope
        // therefore we cannot add it again in the same scope.
        //
        return false;
    }
    else
    {
        it = scopes[scope].find(symbol.id);
        if (it != scopes[scope].end())
        {
            // This symbol has already been defined in the scope
            // and cannot be defined again within the same scope.
            //
            return false;
        }
        else
        {
            // If we get here, then the symbol can be defined for the scope
            //
            scopes[scope].insert(std::pair<std::string, Symbol>(symbol.id, symbol));
            return true;
        }
    }
}

bool SymbolTable::DoesSymbolExist(std::string &id, Symbol &symbol)
{
    it = scopes[scope].find(id);
    if (it != scopes[scope].end())
    {
        // The symbol was found in the current scope
        symbol = it->second;
        return true;
    }

    it = scopes[0].find(id); // Look for this symbol in the Global scope
    if (it != scopes[0].end() && scopes[0][id].isGlobal)
    {
        // The symbol was found in the Global scope
        symbol = it->second;
        return true;
    }

    // Symbol not found
    return false;
}

int SymbolTable::GetScope()
{
    return scope;
}

void SymbolTable::SetScope(int &scope)
{
    this->scope = scope;
}

void SymbolTable::PrintScopes()
{
    int i = 0;
    for (auto &scope : scopes)
    {
        printf("Scope: %i\n", i);
        for (auto symbol : scope)
        {
            std::cout << "\t{ID: " << symbol.first << ", Type: " << symbol.second.type << ", DeclType: " << symbol.second.declarationType << "}" << std::endl;
        }
        std::cout << std::endl;
        i++;
    }
}

void SymbolTable::PrintScope(int idx)
{
    std::map<std::string, Symbol> scope;
    try
    {
        scope = scopes[idx];
    }
    catch (int e)
    {
        std::cout << "Exception occured: " << e << std::endl;
    }

    for (auto symbol : scope)
    {
        std::cout << "{ID: " << symbol.first << ", Type: " << symbol.second.type << ", DeclType: " << symbol.second.declarationType << "}" << std::endl;
    }
}


