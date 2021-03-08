//
// Created by Nick Clason on 2/11/21.
//
#include "../include/Scope.h"

#include <iostream>

Scope::Scope()
{
    scopeID = "";
}

Scope::~Scope()
{
    prevScope = nullptr;
}

void Scope::SetName(std::string id)
{
    scopeID = id;
}

bool Scope::AddSymbol(Symbol symbol)
{
    std::string id = symbol.id;
    std::map<std::string, Symbol>::iterator it;
    it = localScope.find(id);

    if (it != localScope.end())
    {
        return false;
    }
    else
    {
        if (symbol.isGlobal)
        {
            globalScope[id] = symbol;
        }
        localScope[id] = symbol;
    }

    return true;
}

bool Scope::DoesSymbolExist(std::string id, bool isGlobal)
{
    std::map<std::string, Symbol>::iterator it;
    if (isGlobal)
    {
        it = globalScope.find(id);
        if (it != globalScope.end())
        {
            return true;
        }

        return false;
    }
    else
    {
        it = localScope.find(id);
        if (it != localScope.end())
        {
            return true;
        }

        return false;
    }
}

Symbol Scope::GetSymbol(std::string id)
{
    std::map<std::string, Symbol>::iterator it;
    it = localScope.find(id);
    if (it != localScope.end())
    {
        return it->second;
    }
    else
    {
        return Symbol(); // Unknown symbol, initialized with default values
    }
}

void Scope::PrintScope()
{
    std::cout << "Scope ID: " << scopeID << std::endl;
    for (auto symbol : localScope)
    {
        std::cout << "\t{ID: " << symbol.first << ", Type: " << symbol.second.type << ", DeclType: " << symbol.second.declarationType << "}" << std::endl;
    }
}

std::map<std::string, Symbol> Scope::GetLocalScope()
{
    return this->localScope;
}